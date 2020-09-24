/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
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

// test tokudb cardinality in status dictionary
#include <assert.h>
#include <db.h>
#include <errno.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
typedef unsigned long long ulonglong;
#include <tokudb_buffer.h>
#include <tokudb_status.h>

#include "fake_mysql.h"

#if __APPLE__
typedef unsigned long ulong;
#endif
#include <tokudb_card.h>

// verify that we can create and close a status dictionary
static void test_create(DB_ENV *env) {
  int error;

  DB_TXN *txn = NULL;
  error = env->txn_begin(env, NULL, &txn, 0);
  assert(error == 0);

  DB *status_db = NULL;
  error = tokudb::create_status(env, &status_db, "status.db", txn);
  assert(error == 0);

  error = txn->commit(txn, 0);
  assert(error == 0);

  error = tokudb::close_status(&status_db);
  assert(error == 0);
}

// verify that no card row in status works
static void test_no_card(DB_ENV *env) {
  int error;

  DB_TXN *txn = NULL;
  error = env->txn_begin(env, NULL, &txn, 0);
  assert(error == 0);

  DB *status_db = NULL;
  error = tokudb::open_status(env, &status_db, "status.db", txn);
  assert(error == 0);

  error = tokudb::get_card_from_status(status_db, txn, 0, NULL);
  assert(error == DB_NOTFOUND);

  error = txn->commit(txn, 0);
  assert(error == 0);

  error = tokudb::close_status(&status_db);
  assert(error == 0);
}

// verify that a card row with 0 array elements works
static void test_0(DB_ENV *env) {
  int error;

  DB_TXN *txn = NULL;
  error = env->txn_begin(env, NULL, &txn, 0);
  assert(error == 0);

  DB *status_db = NULL;
  error = tokudb::open_status(env, &status_db, "status.db", txn);
  assert(error == 0);

  tokudb::set_card_in_status(status_db, txn, 0, NULL);

  error = tokudb::get_card_from_status(status_db, txn, 0, NULL);
  assert(error == 0);

  error = txn->commit(txn, 0);
  assert(error == 0);

  error = tokudb::close_status(&status_db);
  assert(error == 0);
}

// verify that writing and reading card info works for several sized card arrays
static void test_10(DB_ENV *env) {
  int error;

  for (uint64_t i = 0; i < 20; i++) {
    uint64_t rec_per_key[i];
    for (uint64_t j = 0; j < i; j++)
      rec_per_key[j] = j == 0 ? 10 + i : 10 * rec_per_key[j - 1];

    DB_TXN *txn = NULL;
    error = env->txn_begin(env, NULL, &txn, 0);
    assert(error == 0);

    DB *status_db = NULL;
    error = tokudb::open_status(env, &status_db, "status.db", txn);
    assert(error == 0);

    tokudb::set_card_in_status(status_db, txn, i, rec_per_key);

    uint64_t stored_rec_per_key[i];
    error = tokudb::get_card_from_status(status_db, txn, i, stored_rec_per_key);
    assert(error == 0);

    for (uint64_t j = 0; j < i; j++)
      assert(rec_per_key[j] == stored_rec_per_key[j]);

    error = txn->commit(txn, 0);
    assert(error == 0);

    error = tokudb::close_status(&status_db);
    assert(error == 0);

    error = env->txn_begin(env, NULL, &txn, 0);
    assert(error == 0);

    error = tokudb::open_status(env, &status_db, "status.db", txn);
    assert(error == 0);

    tokudb::set_card_in_status(status_db, txn, i, rec_per_key);

    error = tokudb::get_card_from_status(status_db, txn, i, stored_rec_per_key);
    assert(error == 0);

    for (uint64_t j = 0; j < i; j++)
      assert(rec_per_key[j] == stored_rec_per_key[j]);

    error = txn->commit(txn, 0);
    assert(error == 0);

    // test delete card
    error = env->txn_begin(env, NULL, &txn, 0);
    assert(error == 0);

    error = tokudb::delete_card_from_status(status_db, txn);
    assert(error == 0);

    error = tokudb::get_card_from_status(status_db, txn, 0, NULL);
    assert(error == DB_NOTFOUND);

    error = txn->commit(txn, 0);
    assert(error == 0);

    error = tokudb::close_status(&status_db);
    assert(error == 0);
  }
}

int main() {
  int error;

  error = system("rm -rf " __FILE__ ".testdir");
  assert(error == 0);

  error = mkdir(__FILE__ ".testdir", S_IRWXU + S_IRWXG + S_IRWXO);
  assert(error == 0);

  DB_ENV *env = NULL;
  error = db_env_create(&env, 0);
  assert(error == 0);

  error = env->open(env, __FILE__ ".testdir",
                    DB_INIT_MPOOL + DB_INIT_LOG + DB_INIT_LOCK + DB_INIT_TXN +
                        DB_PRIVATE + DB_CREATE,
                    S_IRWXU + S_IRWXG + S_IRWXO);
  assert(error == 0);

  test_create(env);
  test_no_card(env);
  test_0(env);
  test_10(env);

  error = env->close(env, 0);
  assert(error == 0);

  return 0;
}
