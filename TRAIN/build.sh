#!/bin/bash
# Compile then analyze the .cpp source file provided.

source include.sh
FileNameExixts "$1"

./compile.sh "$1"
if [ "$?" -eq 0 ]; then
  echo
  ./analyze.sh "$1"
fi
