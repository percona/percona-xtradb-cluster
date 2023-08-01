# galera_connect.inc
# ==================
#
# Description
# -----------
# Open a connection to the specified server number ($galera_server_number).
# The connection itself would be identified by $galera_connection_name.
#
# Parameters
# ----------
# $galera_connection_name
#   Name of the resulting connection.
#
# $galera_server_number
#   Sequence number of the node in the galera cluster.
#
# $galera_debug
#   Print debug information.
#
# $galera_use_socket_client_connection
#   Use socket connection instead of TCP/IP
#

if (!$galera_connection_name)
{
  --die ERROR IN TEST: $galera_connection_name must be set before sourcing include/galera_connect.inc
}

if (!$galera_server_number)
{
  --die ERROR IN TEST: $galera_server_number must be set before sourcing include/galera_connect.inc
}


--let $_galera_port=
--let $_host=
if (!$galera_use_socket_client_connection)
{
  --let $_galera_port= \$NODE_MYPORT_$galera_server_number
  --let $_host= 127.0.0.1
  if (!$_galera_port)
  {
    --echo Bug in test case: '\$NODE_MYPORT_$galera_server_number' not initialized. Check the test's .cfg file.
    --die Not all NODE_MYPORT_* environment variables are setup correctly.
  }
}

--let $_galera_sock=
if ($galera_use_socket_client_connection)
{
  --let $_galera_sock= \$NODE_MYSOCK_$galera_server_number
  --let $_host= localhost
  if (!$_galera_sock)
  {
    --echo Bug in test case: '\$NODE_MYSOCK_$galera_server_number' not initialized. Check the test's .cfg file.
    --die Not all NODE_MYSOCK_* environment variables are setup correctly.
  }
}

if ($galera_debug)
{
  --echo connect($galera_connection_name,$_host,root,,test,$_galera_port,$_galera_sock)
}

# Open a connection
--connect($galera_connection_name,$_host,root,,test,$_galera_port,$_galera_sock)

