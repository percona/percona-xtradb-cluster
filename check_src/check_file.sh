# Compares a single PS/PXC source file
# The script will print out an error and a diff if there is any difference
# after removing the PXC specific code, not counting whitespace and empty lines
# The script also allows whitelisting expected differences, such as different
# product names (Percona Server vs Percona Xtradb Cluster)

# Usage: bash check_src/check_file.sh <file_name> <PS 5.6 source directory>
# The script should be run from the root directory of the PXC source
# The PS source should be the currently merged PS tag

# Extending/changing the whitelist:
# The whitelist directory contains files in a single directory (without
# subdirectories), each file whitelist a single real source file, with the
# same name.
# The whitelist file contains the lines starting with > or < of the expected
# diff: only the change without the actual line/offset numbers, to prevent
# unrelated changes from breaking the whitelisted change.
if [ ! -s $2/$1 ] ; then
  exit 0
fi
F=`mktemp`
gawk -f check_src/rem.awk $1 > $F

D=`mktemp`
D2=`mktemp`
diff -wB $F $2/$1 > $D

if [ -s $D ] ; then
  WHITELIST_NAME=check_src/whitelist/${1##*/}
  if [ -f $WHITELIST_NAME ] ; then
    D3=`mktemp`
    cat $D | grep "^[<>].*" > $D3
    diff -wB $WHITELIST_NAME $D3 > $D2
    rm $D3
    if [ ! -s $D2 ] ; then
      echo -n "" > $D
    fi
  fi
fi

if [ -s $D ] ; then
  echo "============"
  echo "Error: $1"
  echo "============"
  cat $D
fi

rm $F
rm $D
rm $D2
