CREATE TABLE IF NOT EXISTS performance_schema.pxc_cluster_view(
  HOST_NAME CHAR(64) collate utf8_general_ci not null,
  UUID CHAR(36) collate utf8_bin not null,
  STATUS CHAR(64) collate utf8_bin not null,
  LOCAL_INDEX INTEGER not null,
  SEGMENT INTEGER not null
) ENGINE=PERFORMANCE_SCHEMA;
