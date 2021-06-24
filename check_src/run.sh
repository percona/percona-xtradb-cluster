# Compares every source file between PS/PXC for differences without the if WITH_WSREP
# blocks.
# Usage:  bash check_src/run.sh <PS_DIRECTORY>
PS_56_LOCATION=$1
find . -type f -name '*.h' -exec bash check_src/check_file.sh '{}' $PS_56_LOCATION \;
find . -type f -name '*.cc' -exec bash check_src/check_file.sh '{}' $PS_56_LOCATION \;
find . -type f -name '*.c' -exec bash check_src/check_file.sh '{}' $PS_56_LOCATION \;
find . -type f -name '*.ic' -exec bash check_src/check_file.sh '{}' $PS_56_LOCATION \;

