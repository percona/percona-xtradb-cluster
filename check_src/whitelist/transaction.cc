<
< #if 0
<   /* TODO: (G-4) Krunal
<   streaming replication transaction flow will regularly append
<   transactions fragments and persist them in streaming table
<   (wsrep_streaming_log). Once the main transaction is committed then
<   the relevant entries from the streaming table needs to be removed.
<   this removal action is done as part of main transaction commit action.
<   main transaction commit ha_commit_trans involves 2 commits:
<   - main commit: to register commit of main transaction.
<   - sub-commit: to register commit of streaming table entries removal.
<   This could be avoided by using a new THD for sub-commit purpose. */
< #endif

