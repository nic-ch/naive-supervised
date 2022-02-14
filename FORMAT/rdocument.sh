#!/bin/bash
# Document the ruby script file provided with RDoc.

if [ -z "$1" ]; then
  echo -e '\e[1m\e[31mERROR!\e[0m No source file provided.'
  exit 1
fi
if [ ! -f "$1" ]; then
  echo -e "\e[1m\e[31mERROR!\e[0m File '$1' does not exist."
  exit 2
fi

echo -e "\e[1m\e[97m\e[41m RDoc-documenting '$1' \e[0m  \e[1m\e[30m\e[47m (Packages rdoc and graphviz needed.) \e[0m"
RDOC_DIRECTORY="RDoc_$1"
rm -fr "$RDOC_DIRECTORY" 2> /dev/null
rdoc -a -d -N -o "$RDOC_DIRECTORY" "$1" && (firefox "$RDOC_DIRECTORY/index.html" &)
