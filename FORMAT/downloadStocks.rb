#!/usr/bin/ruby
# frozen_string_literal: true

abort("ERROR! Need an Alpha Vantage key (alphavantage.co/support/#api-key) as argument.\n") unless (key = ::ARGV.first)

# Pipe in (<) a csv file.
sleep = false
count = 1
# Drop the header.
$stdin.read.split("\n").drop(1).each do |line|
  # Site puts limit of four downloads per minute.
  if sleep
    print('Sleeping... ')
    15.downto(1) do |index|
      print(index, ' ')
      sleep(1)
    end
    print("0\n")
  end
  sleep = true

  symbol = line.split(',').first
  command = 'wget -O ' + symbol + ".csv 'https://www.alphavantage.co/query?symbol=" + symbol + \
            '&function=TIME_SERIES_INTRADAY&interval=1min&outputsize=full&datatype=csv&apikey=' + key + "'"
  print("\n▒▒ BEGIN ", count, "\n", command, "\n\n")
  system command
  print('▒▒ END ', count, "\n\n")
  count += 1
end
