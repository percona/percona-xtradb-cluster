#ident "$Id$"
/*======
This file is part of TokuDB


Copyright (c) 2006, 2015, Percona and/or its affiliates. All rights reserved.

    TokuDBis is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2,
    as published by the Free Software Foundation.

    TokuDB is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with TokuDB.  If not, see <http://www.gnu.org/licenses/>.

======= */

#ident \
    "Copyright (c) 2006, 2015, Percona and/or its affiliates. All rights reserved."

// This program finds all of the tokudb status dictionary files and deletes the
// FRM from them.
//
// Requirements:
// The directory containing the tokudb environment is passed as a parameter.
// Needs the log*.tokulog* crash recovery log files.
// Needs a clean shutdown in the recovery log.
// Needs the tokudb.* metadata files.
//
// Effects:
// Deletes the FRM row from all of the tokudb status dictionaries.
// Creates a new crash recovery log.

#include <assert.h>
#include <db.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int delete_frm_from_status(DB_ENV *env, DB_TXN *txn, const char *dname) {
  int r;
  DB *db = NULL;
  r = db_create(&db, env, 0);
  assert(r == 0);

  r = db->open(db, txn, dname, NULL, DB_BTREE, 0, 0);
  assert(r == 0);

  uint64_t k = 5;
  DBT delkey = {.data = &k, .size = sizeof k};
  r = db->del(db, txn, &delkey, 0);
  assert(r == 0);

  r = db->close(db, 0);
  assert(r == 0);

  return 0;
}

static int find_status_and_delete_frm(DB_ENV *env, DB_TXN *txn) {
  int r;
  DBC *c = NULL;
  r = env->get_cursor_for_directory(env, txn, &c);
  assert(r == 0);

  DBT key = {};
  key.flags = DB_DBT_REALLOC;
  DBT val = {};
  val.flags = DB_DBT_REALLOC;
  while (1) {
    r = c->c_get(c, &key, &val, DB_NEXT);
    if (r == DB_NOTFOUND) break;
    const char *dname = (const char *)key.data;
    const char *iname = (const char *)val.data;
    fprintf(stderr, "dname=%s iname=%s\n", dname, iname);
    assert(r == 0);

    if (strstr(iname, "_status_")) {
      fprintf(stderr, "delete frm from %s\n", iname);
      if (1) {
        r = delete_frm_from_status(env, txn, dname);
        assert(r == 0);
      }
    }
  }
  free(key.data);
  free(val.data);

  r = c->c_close(c);
  assert(r == 0);
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "datadir name missing\n");
    return 1;
  }
  char *datadir = argv[1];

  // open the env
  int r;
  DB_ENV *env = NULL;
  r = db_env_create(&env, 0);
  assert(r == 0);

  env->set_errfile(env, stderr);
  r = env->open(env, datadir,
                DB_INIT_LOCK + DB_INIT_MPOOL + DB_INIT_TXN + DB_INIT_LOG +
                    DB_PRIVATE + DB_CREATE,
                S_IRWXU + S_IRWXG + S_IRWXO);
  // open will fail if the recovery log was not cleanly shutdown
  assert(r == 0);

  // use a single txn to cover all of the status file changes
  DB_TXN *txn = NULL;
  r = env->txn_begin(env, NULL, &txn, 0);
  assert(r == 0);

  r = find_status_and_delete_frm(env, txn);
  assert(r == 0);

  r = txn->commit(txn, 0);
  assert(r == 0);

  // close the env
  r = env->close(env, 0);
  assert(r == 0);

  return 0;
}
