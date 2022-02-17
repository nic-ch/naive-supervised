FileNameExixts() {
  if [ -z "$1" ]; then
    echo -e '\e[1m\e[31mERROR!\e[0m No source file provided.'
    exit 1
  fi
  if [ ! -f "$1" ]; then
    echo -e "\e[1m\e[31mERROR!\e[0m File '$1' does not exist."
    exit 2
  fi
}
