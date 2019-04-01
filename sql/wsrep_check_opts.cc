/* Copyright 2011 Codership Oy <http://www.codership.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include "sql_class.h"

#include "wsrep_mysqld.h"
#include "mysql/components/services/log_builtins.h"
#include "sql/log.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* This file is about checking for correctness of mysql configuration options */

struct opt {
  const char *const name;
  const char *value;
};

/* A list of options to check.
 * At first we assume default values and then see if they are changed on CLI or
 * in my.cnf */
static struct opt opts[] = {{"wsrep_sst_method", "xtrabackup-v2"},  // mysqld.cc
                            {"wsrep_sst_receive_address", "AUTO"},  // mysqld.cc
                            {"binlog_format", "ROW"},               // mysqld.cc
                            {"wsrep_provider", "none"},             // mysqld.cc
                            {"locked_in_memory", "0"},              // mysqld.cc
                            {"autoinc_lock_mode", "2"},  // ha_innodb.cc
                            {"innodb_read_only", "0"},   // ha_innodb.cc
                            {0, 0}};

enum {
  WSREP_SST_METHOD,
  WSREP_SST_RECEIVE_ADDRESS,
  BINLOG_FORMAT,
  WSREP_PROVIDER,
  LOCKED_IN_MEMORY,
  AUTOINC_LOCK_MODE,
  INNODB_READ_ONLY_MODE
};

/* A class to make a copy of argv[] vector */
struct argv_copy {
  int const argc_;
  char **argv_;

  argv_copy(int const argc, const char *const argv[])
      : argc_(argc),
        argv_(reinterpret_cast<char **>(calloc(argc_, sizeof(char *)))) {
    if (argv_) {
      for (int i = 0; i < argc_; ++i) {
        argv_[i] = strdup(argv[i]);

        if (!argv_[i]) {
          argv_free();  // free whatever bee allocated
          return;
        }
      }
    }
  }

  ~argv_copy() { argv_free(); }

 private:
  argv_copy(const argv_copy &);
  argv_copy &operator=(const argv_copy &);

  void argv_free() {
    if (argv_) {
      for (int i = 0; (i < argc_) && argv_[i]; ++i) free(argv_[i]);
      free(argv_);
      argv_ = 0;
    }
  }
};

/* a short corresponding to '--' byte sequence */
static short const long_opt_prefix('-' + ('-' << 8));

/* Normalizes long options to have '_' instead of '-' */
static int normalize_opts(argv_copy &a) {
  if (a.argv_) {
    for (int i = 0; i < a.argc_; ++i) {
      char *ptr = a.argv_[i];
      if (long_opt_prefix == *(short *)ptr)  // long option
      {
        ptr += 2;
        const char *end = strchr(ptr, '=');

        if (!end) end = ptr + strlen(ptr);

        for (; ptr != end; ++ptr)
          if ('-' == *ptr) *ptr = '_';
      }
    }

    return 0;
  }

  return EINVAL;
}

/* Find required options in the argument list and change their values */
static int find_opts(argv_copy &a, struct opt *const opts) {
  for (int i = 0; i < a.argc_; ++i) {
    char *ptr = a.argv_[i] + 2;  // we're interested only in long options

    struct opt *opt = opts;
    for (; 0 != opt->name; ++opt) {
      if (!strstr(ptr, opt->name)) continue;  // try next option

      /* 1. try to find value after the '=' */
      opt->value = strchr(ptr, '=') + 1;

      /* 2. if no '=', try next element in the argument vector */
      if (reinterpret_cast<void *>(1) == opt->value) {
        /* also check that the next element is not an option itself */
        if (i + 1 < a.argc_ && *(a.argv_[i + 1]) != '-') {
          ++i;
          opt->value = a.argv_[i];
        } else
          opt->value = "";  // no value supplied (like boolean opt)
      }

      break;  // option found, break inner loop
    }
  }

  return 0;
}

/* Parses string for an integer. Returns 0 on success. */
int get_long_long(const struct opt &opt, long long *const val, int const base) {
  const char *const str = opt.value;

  if ('\0' != *str) {
    char *endptr;

    *val = strtoll(str, &endptr, base);

    if ('k' == *endptr || 'K' == *endptr) {
      *val *= 1024L;
      endptr++;
    } else if ('m' == *endptr || 'M' == *endptr) {
      *val *= 1024L * 1024L;
      endptr++;
    } else if ('g' == *endptr || 'G' == *endptr) {
      *val *= 1024L * 1024L * 1024L;
      endptr++;
    }

    if ('\0' == *endptr) return 0;  // the whole string was a valid integer
  }

  WSREP_ERROR("Bad value for *%s: '%s'. Should be integer.", opt.name,
              opt.value);

  return EINVAL;
}

/* This is flimzy coz hell knows how mysql interprets boolean strings...
 * and, no, I'm not going to become versed in how mysql handles options -
 * I'd rather sing.

 Aha, http://dev.mysql.com/doc/refman/5.1/en/dynamic-system-variables.html:
 Variables that have a type of “boolean” can be set to 0, 1, ON or OFF. (If you
 set them on the command line or in an option file, use the numeric values.)

 So it is '0' for FALSE, '1' or empty string for TRUE

 */
int get_bool(const struct opt &opt, bool *const val) {
  const char *str = opt.value;

  while (isspace(*str)) ++str;  // skip initial whitespaces

  ssize_t str_len = strlen(str);
  switch (str_len) {
    case 0:
      *val = true;
      return 0;
    case 1:
      if ('0' == *str || '1' == *str) {
        *val = ('1' == *str);
        return 0;
      }
  }

  WSREP_ERROR("Bad value for *%s: '%s'. Should be '0', '1' or empty string.",
              opt.name, opt.value);

  return EINVAL;
}

static int check_opts(int const argc, const char *const argv[],
                      struct opt opts[]) {
  /* First, make a copy of argv to be able to manipulate it */
  argv_copy a(argc, argv);

  if (!a.argv_) {
    WSREP_ERROR("Could not copy argv vector: not enough memory.");
    return ENOMEM;
  }

  int err = normalize_opts(a);
  if (err) {
    WSREP_ERROR("Failed to normalize options.");
    return err;
  }

  err = find_opts(a, opts);
  if (err) {
    WSREP_ERROR("Failed to parse options.");
    return err;
  }

  int rcode = 0;

  /* At this point we have updated default values in our option list to
     what has been specified on the command line / my.cnf */

  /* Galera doesn't support lock in memory as it uses memory for gcache,
  SST (externally forked process), etc... */
  bool locked_in_memory;
  err = get_bool(opts[LOCKED_IN_MEMORY], &locked_in_memory);
  if (err) {
    WSREP_ERROR("get_bool error: %s", strerror(err));
    return err;
  }
  if (locked_in_memory) {
    WSREP_ERROR("Memory locking is not supported (locked_in_memory=%s)",
                locked_in_memory ? "ON" : "OFF");
    rcode = EINVAL;
  }

  /* Galera doesn't recommend setting sst_receieve_address to localhost
  as it may affect incoming connection from DONOR node. */
  if (strcasecmp(opts[WSREP_SST_RECEIVE_ADDRESS].value, "AUTO")) {
    if (!strncasecmp(opts[WSREP_SST_RECEIVE_ADDRESS].value, "127.0.0.1",
                     strlen("127.0.0.1")) ||
        !strncasecmp(opts[WSREP_SST_RECEIVE_ADDRESS].value, "localhost",
                     strlen("localhost"))) {
      WSREP_WARN(
          "wsrep_sst_receive_address is set to '%s' which "
          "makes it impossible for another host to reach this "
          "one. Please set it to the address which this node "
          "can be connected at by other cluster members.",
          opts[WSREP_SST_RECEIVE_ADDRESS].value);
    }
  }

  /* Galera need logical replication (ROW based replication) to preserve
  replication order so this enforcement. */
  if (strcasecmp(opts[WSREP_PROVIDER].value, "none")) {
    if (strcasecmp(opts[BINLOG_FORMAT].value, "ROW")) {
      WSREP_ERROR(
          "Only binlog_format = 'ROW' is currently supported. "
          "Configured value: '%s'. Please adjust your "
          "configuration.",
          opts[BINLOG_FORMAT].value);

      rcode = EINVAL;
    }
  }

  /* Galera only support InnoDB so no point in operating with writes
  to innodb disabled. */
  bool read_only;
  err = get_bool(opts[INNODB_READ_ONLY_MODE], &read_only);
  if (err) {
    WSREP_ERROR("get_bool error: %s", strerror(err));
    return err;
  }
  if (read_only) {
    WSREP_ERROR("innodb_read_only is not supported (innodb_read_only=%s)",
                read_only ? "1" : "0");
    rcode = EINVAL;
  }

  return rcode;
}

int wsrep_check_opts(int const argc, char *const *const argv) {
  return check_opts(argc, argv, opts);
}

