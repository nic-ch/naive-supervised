#!/bin/bash
# Compile the .cpp source file provided then run it.

source include.sh
FileNameExixts "$1"

source flags.sh

# GNU
COMPILER='g++-11'
TARGET="${1%.*}_GNU-11"
LINK_FLAGS=$LINK_FLAGS_GNU
BUILD_FLAGS=$BUILD_FLAGS_GNU

# LLVM
#COMPILER='clang++-14'
#TARGET="${1%.*}_LLVM-14"
#LINK_FLAGS=$LINK_FLAGS_LLVM
#BUILD_FLAGS=$BUILD_FLAGS_LLVM

echo -e "\e[1m\e[97m\e[41m Compiling '$1' with $COMPILER to '$TARGET' \e[0m"
$COMPILER $LINK_FLAGS $BUILD_FLAGS $1 -o $TARGET && strip -s -x $TARGET
if [ "$?" -eq 0 ]; then
  echo -e "\n\e[1m\e[97m\e[42m Running '$TARGET' \e[0m"
  shift
  ./$TARGET "$@"
fi
