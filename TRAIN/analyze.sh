#!/bin/bash
# Analyze the .cpp source file provided.
# Add or remove version numbers in line 'for VERSION in ... do' below at will.

source include.sh
FileNameExixts "$1"

echo -e "\e[1m\e[97m\e[44m▒▒ Analyzing '$1' \e[0m"

source flags.sh

# Commands for parallel.
COMMANDS=''
# Arguments: analyzer pretty name, analyzer, flags.
AddCommand() {
  COMMANDS+="echo -e '\n\e[1m\e[97m\e[45m Analyzing with $1 \e[0m'; $2 $3  @  "
}

# cppcheck.
#CPPCHECK1='/PACKAGES/cppcheck-2.4.1/cppcheck'
#AddCommand "$($CPPCHECK1 --version)" "$CPPCHECK1" "$CPPCHECK_FLAGS $1"
CPPCHECK2='/PACKAGES/cppcheck/cppcheck'
AddCommand "$($CPPCHECK2 --version)" "$CPPCHECK2" "$CPPCHECK_FLAGS $1"

# clang-tidy, space-separated list of clang-tidy versions to use.
for VERSION in 14 15; do
  AddCommand "clang-tidy-$VERSION" "clang-tidy-$VERSION" "$CLANG_TIDY_FLAGS $1 --"
done

# Run the commands with parallel.
#echo "$COMMANDS"
parallel -d @ -j0 -k ::: "$COMMANDS"
