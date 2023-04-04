< /**
< Enqueue a lock wait for normal transaction. If it is a high priority transaction
< then jump the record lock wait queue and if the transaction at the head of the
< queue is itself waiting roll it back, also do a deadlock check and resolve.
< @param[in, out] wait_for	The lock that the joining transaction is
<                                 waiting for
< @param[in] prdt			Predicate [optional]
< @return DB_SUCCESS, DB_LOCK_WAIT, DB_DEADLOCK, or
<         DB_SUCCESS_LOCKED_REC; DB_SUCCESS_LOCKED_REC means that
<         there was a deadlock, but another transaction was chosen
<         as a victim, and we got the lock immediately: no need to
<         wait then */
<  @param[in] c_lock         conflicting lock
