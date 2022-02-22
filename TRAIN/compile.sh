#!/bin/bash
# Compile the .cpp source file provided with a number of GNU and LLVM compilers.
# Add or remove version numbers in lines 'for VERSION in ... do' below at will.

source include.sh
FileNameExixts "$1"

echo -e "\e[1m\e[97m\e[44m▒▒ Compiling '$1' \e[0m"
source flags.sh

# Commands for parallel.
COMMANDS=''
# Arguments: compiler, flags, executable.
AddCommand() {
  COMMANDS+="echo -e \"\n\e[1m\e[97m\e[41m Compiling with $1 to '$3' \e[0m\"; $1 $2 -o $3 && strip -s -x $3  @  "
}

# GNU g++, space-separated list of clang-tidy versions to use.
for VERSION in 10 11; do
  AddCommand "g++-$VERSION" "$LINK_FLAGS_GNU $BUILD_FLAGS_GNU $1" "${1%.*}_GNU-$VERSION"
done

# LLVM clang++, space-separated list of clang-tidy versions to use.
for VERSION in 13 14 15; do
  AddCommand "clang++-$VERSION" "$LINK_FLAGS_LLVM $BUILD_FLAGS_LLVM $1" "${1%.*}_LLVM-$VERSION"
done

# Run the commands with parallel.
#echo "$COMMANDS"
parallel -d @ -j0 -k ::: "$COMMANDS"
