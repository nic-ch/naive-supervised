#!/bin/bash
# Document C++ files according to 'Doxyfile'.

echo -e '\e[1m\e[97m\e[41m Doxygen-documenting \e[0m  \e[1m\e[30m\e[47m (Packages doxygen and graphviz needed.) \e[0m'
rm -fr Doxygen_* 2> /dev/null
doxygen Doxyfile
firefox Doxygen_html/index.html &
