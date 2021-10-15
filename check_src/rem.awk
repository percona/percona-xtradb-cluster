# AWK script to remove PXC specific code from source files
# Based on common PXC specific ifdefs/macros
BEGIN {
	# We are currently checking a WITH_WSREP ifdef line
	with_wsrep = 0
	# We encountered an if(n)def WITH_WSREP block before,
	# and it's scope, or the scope of its else clause is still open
	had_wsrep = 0
	# How many ifdefs we have nested?
	# Used to ensure that we find the correct else/endif
	nest_level = 0
	# We encountered an ifndef WITH_WSREP, or the else clause of
	# the ifdef version
	ifndef_wsrep = 0
}

{
	skip_this = 0
	if ($0 ~ /WSREP_TO_ISOLATION_BEGIN/) {
		# Macro defined to nothing without WITH_WSREP
		skip_this = 1
	}
	if ($0 ~ /WSREP_TO_ISOLATION_END/) {
		# Macro defined to nothing without WITH_WSREP
		skip_this = 1
	}
	if ($0 ~ /WSREP_SYNC_WAIT/) {
		# Macro defined to nothing without WITH_WSREP
		skip_this = 1
	}
	if ($0 ~ /WAIT_ALLOW_WRITES/) {
		# Macro defined to nothing without WITH_WSREP
		skip_this = 1
	}
	if ($0 ~ /#if/) {
		if ($0 ~ /WITH_WSREP/ || $0 ~ /WITH_INNODB_DISALLOW_WRITES/) {
			if ($0 ~ /ifndef/) {
				had_wsrep = 1
				ifndef_wsrep = 1
				nest_level = 0
				skip_this = 1
			} else {
				with_wsrep = 1
				had_wsrep = 1
				nest_level = 0
			}
		} else if (had_wsrep == 1) {
			nest_level = nest_level + 1
		}
	}
	if ($0 ~ /#else/ && (with_wsrep == 1 || ifndef_wsrep == 1) && nest_level == 0) {
		if (ifndef_wsrep == 1) {
			with_wsrep = 1
			ifndef_wsrep = 0
		} else {
			with_wsrep = 0
		}
		skip_this = 1
	}
	if ($0 ~ /#endif/ && nest_level == 0) {
		if (had_wsrep == 1) {
			skip_this = 1
		}
		had_wsrep = 0
		with_wsrep = 0
		ifndef_wsrep = 0
	}
	if ($0 ~ /#endif/ && nest_level != 0) {
		nest_level = nest_level - 1
	}
	if (with_wsrep == 0 && skip_this == 0) {
		print $0
	}
}
