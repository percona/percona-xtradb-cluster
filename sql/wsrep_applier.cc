/* Copyright (C) 2013-2015 Codership Oy <info@codership.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "wsrep_applier.h"

#include "wsrep_binlog.h"  // wsrep_dump_rbr_buf()
#include "wsrep_priv.h"
#include "wsrep_xid.h"
#include "mysql/components/services/log_builtins.h"

#include "debug_sync.h"
#include "log_event.h"  // class THD, EVENT_LEN_OFFSET, etc.
#include "sql/sql_lex.h"
#include "mysql/plugin.h"
#include "sql/binlog_reader.h"

#include "wsrep_thd.h"
#include "wsrep_trans_observer.h"
#include "service_wsrep.h"
#include "wsrep_priv.h"

/*
  read the first event from (*buf). The size of the (*buf) is (*buf_len).
  At the end (*buf) is shitfed to point to the following event or NULL and
  (*buf_len) will be changed to account just being read bytes of the 1st event.
*/

static Log_event *wsrep_read_log_event(
    char **arg_buf, size_t *arg_buf_len,
    const Format_description_log_event *description_event) {
  DBUG_ENTER("wsrep_read_log_event");
  char *head = (*arg_buf);

  uint data_len = uint4korr(head + EVENT_LEN_OFFSET);
  char *buf = (*arg_buf);

  Log_event *ev = NULL;
  Binlog_read_error binlog_read_error =
      binlog_event_deserialize(reinterpret_cast<unsigned char *>(buf), data_len,
                               description_event, false, &ev);

  if (binlog_read_error.has_error()) {
    WSREP_ERROR(
        "Error in reading event (wsrep_read_log_event): "
        "'%s', data_len: %d, event_type: %d",
        binlog_read_error.get_str(), data_len, head[EVENT_TYPE_OFFSET]);
  }
  (*arg_buf) += data_len;
  (*arg_buf_len) -= data_len;
  DBUG_RETURN(ev);
}

#include "rpl_rli.h"      // class Relay_log_info;
#include "sql_base.h"     // close_temporary_table()
#include "transaction.h"  // trans_commit(), trans_rollback()

inline void wsrep_set_apply_format(THD *thd, Format_description_log_event *ev) {
  if (thd->wsrep_apply_format) {
    delete (Format_description_log_event *)thd->wsrep_apply_format;
  }
  thd->wsrep_apply_format = ev;
}

inline Format_description_log_event *wsrep_get_apply_format(THD *thd) {
  if (thd->wsrep_apply_format)
    return (Format_description_log_event *)thd->wsrep_apply_format;
  return thd->wsrep_rli->get_rli_description_event();
}

void wsrep_apply_error::store(const THD *const thd) {
  Diagnostics_area::Sql_condition_iterator it =
      thd->get_stmt_da()->sql_conditions();
  const Sql_condition *cond;

  static size_t const max_len =
      2 * MAX_SLAVE_ERRMSG;  // 2x so that we have enough

  if (NULL == str_) {
    // this must be freeable by standard free()
    str_ = static_cast<char *>(malloc(max_len));
    if (NULL == str_) {
      WSREP_ERROR("Failed to allocate %zu bytes for error buffer.", max_len);
      len_ = 0;
      return;
    }
  } else {
    /* This is possible when we invoke rollback after failed applying.
     * In this situation DA should not be reset yet and should contain
     * all previous errors from applying and new ones from rollbacking,
     * so we just overwrite is from scratch */
  }

  char *slider = str_;
  const char *const buf_end = str_ + max_len - 1;  // -1: leave space for \0

  for (cond = it++; cond && slider < buf_end; cond = it++) {
    uint const err_code = cond->mysql_errno();
    const char *const err_str = cond->message_text();

    slider += snprintf(slider, buf_end - slider, " %s, Error_code: %d;",
                       err_str, err_code);
  }

  *slider = '\0';
  len_ = slider - str_ + 1;  // +1: add \0

  WSREP_DEBUG("Error buffer for thd %u seqno %lld, %zu bytes: %s",
              thd->thread_id(), (long long)wsrep_thd_trx_seqno(thd), len_,
              str_ ? str_ : "(null)");
}

/**
  Helper function to apply replicated event.
*/
int wsrep_apply_events(THD *thd, Relay_log_info *rli __attribute__((unused)),
                       const void *events_buf, size_t buf_len) {
  char *buf = (char *)events_buf;
  int rcode = WSREP_RET_SUCCESS;
  int event = 1;

  DBUG_ENTER("wsrep_apply_events");

  if (!buf_len)
    WSREP_DEBUG("Empty apply event found while processing write-set: %lld",
                (long long)wsrep_thd_trx_seqno(thd));

  while (buf_len) {
    int exec_res;
    Log_event *ev =
        wsrep_read_log_event(&buf, &buf_len, wsrep_get_apply_format(thd));

    if (!ev) {
      WSREP_ERROR("Applier could not read binlog event, seqno: %lld, len: %zu",
                  (long long)wsrep_thd_trx_seqno(thd), buf_len);
      rcode = WSREP_ERR_BAD_EVENT;
      goto error;
    }

    switch (ev->get_type_code()) {
      case binary_log::FORMAT_DESCRIPTION_EVENT:
        wsrep_set_apply_format(thd, (Format_description_log_event *)ev);
        continue;
      /* If gtid_mode=OFF then async master-slave will generate ANONYMOUS GTID.
       */
      case binary_log::GTID_LOG_EVENT:
      case binary_log::ANONYMOUS_GTID_LOG_EVENT: {
        Gtid_log_event *gev = (Gtid_log_event *)ev;
        if (gev->get_gno() == 0) {
          /* Skip GTID log event to make binlog to generate LTID on commit */
          delete ev;
          continue;
        }
        /*
           gtid_pre_statement_checks will fail on the subsequent statement
           if the bits below are set. So we don't mark the thd to run in
           transaction mode yet, and assume there will be such a "BEGIN"
           log event that will set those appropriately.
        */
        thd->variables.option_bits &= ~OPTION_BEGIN;
        thd->server_status &= ~SERVER_STATUS_IN_TRANS;
        assert(event == 1);
        break;
      }
      default:
        break;
    }

    thd->server_id = ev->server_id;  // use the original server id for logging
    thd->unmasked_server_id = ev->common_header->unmasked_server_id;
    thd->set_time();  // time the query

    if (wsrep_thd_is_toi(thd)) {
      wsrep_xid_init(thd->get_transaction()->xid_state()->get_xid(),
                     thd->wsrep_cs().toi_meta().gtid());
    } else {
      wsrep_xid_init(thd->get_transaction()->xid_state()->get_xid(),
                     thd->wsrep_trx().ws_meta().gtid());
    }

    thd->lex->set_current_select(NULL);
    if (!ev->common_header->when.tv_sec)
      my_micro_time_to_timeval(my_micro_time(), &ev->common_header->when);
    ev->thd = thd;  // because up to this point, ev->thd == 0

    set_timespec_nsec(&thd->wsrep_rli->ts_exec[0], 0);
    thd->wsrep_rli->stats_read_time +=
        diff_timespec(&thd->wsrep_rli->ts_exec[0], &thd->wsrep_rli->ts_exec[1]);

    exec_res = ev->apply_event(thd->wsrep_rli);
    DBUG_PRINT("info", ("exec_event result: %d", exec_res));

    if (exec_res) {
      WSREP_WARN("Event %d %s apply failed: %d, seqno %lld", event,
                 ev->get_type_str(), exec_res,
                 (long long)wsrep_thd_trx_seqno(thd));
      rcode = exec_res;
      /* stop processing for the first error */
      delete ev;
      goto error;
    }

    set_timespec_nsec(&thd->wsrep_rli->ts_exec[1], 0);
    thd->wsrep_rli->stats_exec_time +=
        diff_timespec(&thd->wsrep_rli->ts_exec[1], &thd->wsrep_rli->ts_exec[0]);

    DBUG_PRINT("info", ("wsrep apply_event error = %d", exec_res));
    event++;

    switch (ev->get_type_code()) {
    case binary_log::ROWS_QUERY_LOG_EVENT:
      /*
        Setting binlog_rows_query_log_events to ON will generate
        ROW_QUERY_LOG_EVENT. This event logs an extra information while logging
        row information and so event should be kept infact till ROW_LOG_EVENT
        is processed and should be freed once ROW_LOG_EVENT is done.

        Keeping Rows_log event, it will be needed still, and will be deleted later
        in rli->cleanup_context()
        Also FORMAT_DESCRIPTION_EVENT is needed further, but it skipped from this loop
        by 'continue' above, and thus  avoids the following 'delete ev'
      */
      continue;
    default:
      delete ev;
      break;
    }
  }

error:
  if (thd->killed == THD::KILL_CONNECTION)
    WSREP_INFO("applier aborted while processing write-set: %lld",
               (long long)wsrep_thd_trx_seqno(thd));

  wsrep_set_apply_format(thd, NULL);
  DBUG_RETURN(rcode);
}
