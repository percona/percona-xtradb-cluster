<     // clang-format off
<     /* We already started TOI. Below we will acquire ACL lock again.
<        If we do this leaving critical section, someone else can acquire
<        ACL lock before us, and then attempt to start TOI which will block.
<        In such a case when we try to acquire ACL lock below, we will end up
<        in a deadlock */
<     // clang-format off
<     /* We already started TOI. Below we will acquire ACL lock again.
<        If we do this leaving critical section, someone else can acquire
<        ACL lock before us, and then attempt to start TOI which will block.
<        In such a case when we try to acquire ACL lock below, we will end up
<        in a deadlock */
