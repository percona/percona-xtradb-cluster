#ifndef WSREP_MD5
#define WSREP_MD5

/* For certification we need to identify each row uniquely.
Generally this is done using PK but if table is created w/o PK
then a md5-hash (16 bytes) string is generated using the complete record.
Following functions act as helper function in generation of this md5-hash. */

void *wsrep_md5_init();
void wsrep_md5_update(void *ctx, char *buf, int len);
void wsrep_compute_md5_hash(unsigned char *digest, void *ctx);
void wsrep_enable_fips_mode();

#endif /* WSREP_MD5 */