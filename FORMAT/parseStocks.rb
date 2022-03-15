#!/usr/bin/ruby
# frozen_string_literal: true

#
# parseStocks.rb
#
# Copyright 2022 Nicolas Chausse (nicolaschausse@protonmail.com)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License only.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# version 0.1
#

require 'stringio'
require 'time'
require 'tmpdir'

#################################################
## ALTERATIONS TO EXISTING CLASSES AND MODULES ##
#################################################

# Alterations to Comparable.
module Comparable
  public

  # Return the maximum of two comparables.
  def max(value)
    self < value ? value : self
  end

  # Return the minimum of two comparables.
  def min(value)
    self < value ? self : value
  end
end

# Alterations to Kernel.
module Kernel
  # private so not to be called with an explicit receiver.

  private

  alias originalPrint print
  # Also print on $Log all that is printed, & means if not null.
  def print(*printables)
    originalPrint(*printables)
    $Log&.print(*printables)
  end

  alias originalAbort abort
  # Also print on $Log what is printed by abort, & means if not null.
  def abort(*printables)
    message = "\n██ FATAL ERROR! " + printables.join + "\n"
    $Log&.print(message)
    originalAbort(message)
  end

  # Print error message.
  def error(*printables)
    print('█ ERROR! ', *printables)
  end

  # Print error message, followed by exception content.
  def exception(exception, *printables)
    error(*printables, "Exception '", exception.class, "': '", exception.message, "'.\n")
  end
end

#############
## CLASSES ##
#############

# Parse the input files, compute gains, normalize amounts and output train values to files, according to arguments.
# Create with a file name infix as argument.
class StocksParser
  public

  ##############
  # INITIALIZE #
  ##############

  # All files outputted will get #fileNameInfix in their file name.
  def initialize(fileNameInfix)
    @fileNameInfix = fileNameInfix
  end

  ###############
  # DEFINITIONS #
  ###############

  # Including the terminating null.
  def stockNameMaximumLength
    16
  end

  def inputFilesHeader
    'timestamp,open,high,low,close,volume'
  end

  # timestamp will be stripped.
  def columnsCount
    5
  end

  # Prices are expressed in dollars as 123.4567 in the csv files. Shoud NOT be 0.
  def priceMultiplier
    10_000
  end

  ###########################
  # PUBLIC INSTANCE METHODS #
  ###########################

  # Start the script, either in test mode or in normal mode.
  def start
    if ::ARGV.first.eql?('TEST')
      testAll
    else
      begin
        logFileName = 'PARSE_' + @fileNameInfix + '.log'
        $Log = ::File.open(logFileName, 'a')
      rescue ::StandardError => error
        exception(error, "Can not open log file '", logFileName, "' for writing.\n")
      end

      @arguments = ::ARGV
      parseStocks

      $Log&.close
    end
  end

  ############################
  # PRIVATE INSTANCE METHODS #
  ############################

  private

  # Do everything.
  def parseStocks
    print('▒▒ BEGINNING at ', ::Time.now.strftime('%Y-%m-%d %H:%M:%S'), ".\n")

    extractArguments
    parseInputFiles
    validateParsedData
    computeGains
    extrapolateMissingTrainTimestamps
    outputTrainDataToTextFile('brute')
    normalizeTrainAmounts
    outputTrainDataToTextFile('normalized')
    outputTrainDataToBinaryFile

    print("\n▒▒ ALL DONE at ", ::Time.now.strftime('%Y-%m-%d %H:%M:%S'), ".\n")
  end

  # Ensure at least one stock was parsed.
  def ensureThereAreStocks
    abort("No stock were parsed.\n") if @stockName_timestamp_amounts&.empty?
  end

  # Extract the arguments. Abort on fatal errors.
  def extractArguments
    if @arguments.size < 8
      abort(
        "USAGE:\n\t",
        __FILE__,
        '  TEST    ‡ ALL other arguments will be IGNORED.' \
        "\n-- OR --\n\t", \
        __FILE__,
        "\n\t\t<begin train date>  <begin train time>" \
        "\n\t\t<end train date>  <end train time>" \
        "\n\t\t<begin gain timestamp>  <end gain timestamp>" \
        "\n\t\t<maximum train timestamps gap size>" \
        "\n\t\t<CSV input file name>+" \
        "\n\n\t‡‡ timestamps are of form 'YYYY-MM-DD HH:MM'.\n"
      )
    end

    # Output back the call line.
    print("\n", __FILE__, "  '", @arguments.join("'  '"), "'\n")

    print("\n∙ EXTRACTING ARGUMENTS...\n")

    # Extract all the arguments into global variables.
    beginTrainDateString = @arguments.shift
    beginTrainTimeString = @arguments.shift
    endTrainDateString = @arguments.shift
    endTrainTimeString = @arguments.shift
    @beginGainTimestamp = @arguments.shift
    @endGainTimestamp = @arguments.shift
    maximumTrainTimestampsGapSizeString = @arguments.shift
    begin
      @maximumTrainTimestampsGapSize = Integer(maximumTrainTimestampsGapSizeString, 10)
    rescue ::StandardError
      abort("'", maximumTrainTimestampsGapSizeString, "' can not be converted to a number gap size.\n")
    end
    @inputFileNames = @arguments

    # Print back the extracted arguments.
    @beginTrainTimestamp = beginTrainDateString + ' ' + beginTrainTimeString
    @endTrainTimestamp = endTrainDateString + ' ' + endTrainTimeString
    print(
      "◦ Begin train timestamp is '",
      @beginTrainTimestamp,
      "', end train timestamp is '",
      @endTrainTimestamp,
      "'.\n"
    )
    print("◦ Begin gain timestamp is '", @beginGainTimestamp, "', end gain timestamp '", @endGainTimestamp, "'.\n")
    print('◦ Maximum train timestamps gap size is ', @maximumTrainTimestampsGapSize, ".\n")

    # Verify the timestamps.
    abort("Begin train timestamp is AFTER end train timestamp.\n") if @beginTrainTimestamp > @endTrainTimestamp
    abort("Begin gain timestamp is AFTER end gain timestamp.\n") if @beginGainTimestamp > @endGainTimestamp

    # Generate the train timestamps, which is a perfect rectangle with four corners:
    #
    # '<begin train date> <begin train time>'  ==  '<end train date> <begin train time>'
    # ||                                                                              ||
    # '<begin train date> <end train time>'    ==    '<end train date> <end train time>'

    # Set { timestamp => true }.
    @trainTimestamp_bool = {}
    oneDay = 60 * 60 * 24
    oneMinute = 60
    dateFormat = '%Y-%m-%d'
    dateTimeFormat = '%Y-%m-%d %H:%M'
    endTrainDate = ::Time.strptime(endTrainDateString, dateFormat)
    trainDate = ::Time.strptime(beginTrainDateString, dateFormat)
    while trainDate <= endTrainDate
      trainDateString = trainDate.strftime(dateFormat)
      endTrainDateTime = ::Time.strptime(trainDateString + ' ' + endTrainTimeString, dateTimeFormat)
      trainDateTime = ::Time.strptime(trainDateString + ' ' + beginTrainTimeString, dateTimeFormat)
      while trainDateTime <= endTrainDateTime
        @trainTimestamp_bool[trainDateTime.strftime(dateTimeFormat)] = true
        trainDateTime += oneMinute
      end
      trainDate += oneDay
    end
    @sortedTrainTimestamps = @trainTimestamp_bool.keys.sort
    print("\n∙∙∙  ", @sortedTrainTimestamps.size, " TRAIN TIMESTAMP(S) WERE GENERATED.\n\n")

    # Print back the extracted file names.
    print('∙ The ', @inputFileNames.size, " input file names are: '", @inputFileNames.join("', '"), "'.\n")
  end

  # Parse the input files and generate the main @stockName_timestamp_amounts hash. Abort on fatal errors.
  def parseInputFiles
    print("\n∙ PARSING INPUT FILES...\n")

    # Mapping { stockName => { timestamp => amounts is [ [open, high, low, close, volume]+ ] } }.
    # amounts begins as a matrix (array of arrays).
    @stockName_timestamp_amounts = \
      ::Hash.new { |hash, stockName| hash[stockName] = ::Hash.new { |innerHash, timestamp| innerHash[timestamp] = [] } }

    # Parse the input files.
    @inputFileNames.each do |inputFileName|
      stockName = ::File.basename(inputFileName, '.*')
      print("◦ Reading file '", inputFileName, "' for stock '", stockName, "'... ")

      # Try to read all lines from input file.
      begin
        inputLines = ::File.readlines(inputFileName, chomp: true)
      rescue ::StandardError => error
        exception(error, "Can not read file.\n")
      else
        # If first line matches the header.
        if (header = inputLines.shift).eql?(inputFilesHeader)
          timestamp_amounts = @stockName_timestamp_amounts[stockName]
          trainTimestampsCount = 0
          gainTimestampsCount = 0
          ignoredInputLinesCount = 0
          # Extract each line's timestamp and amounts.
          inputLines.each do |inputLine|
            if (amounts = inputLine.split(',')).size.eql?(6)
              # Strip timestamp from amounts, and don't get the seconds (in-file format is 'YYYY-MM-DD HH:MM:SS').
              timestamp = amounts.shift[0..15]

              # Use the amounts only if timestamp is a train timestamp or within gain timestamps.
              trainTimestampsCount += 1 if (isTimestampTrain = @trainTimestamp_bool.key?(timestamp))
              gainTimestampsCount += 1 if (isTimestampGain = timestamp.between?(@beginGainTimestamp, @endGainTimestamp))
              if isTimestampTrain || isTimestampGain
                # amounts is now [open, high, low, close, volume].
                # Convert decimals to rationals, then to integers after being multiplied.
                amounts[0] = Integer(Rational(amounts[0]) * priceMultiplier)
                amounts[1] = Integer(Rational(amounts[1]) * priceMultiplier)
                amounts[2] = Integer(Rational(amounts[2]) * priceMultiplier)
                amounts[3] = Integer(Rational(amounts[3]) * priceMultiplier)
                amounts[4] = Integer(amounts[4])

                # Add a row to amounts which is a matrix for now.
                timestamp_amounts[timestamp].append(amounts)
              end
            else
              ignoredInputLinesCount += 1
            end
          end

          print('Gathered: ', trainTimestampsCount, ' train and ', gainTimestampsCount, " gain timestamps.\n")

          error(ignoredInputLinesCount, " line(s) ignored.\n") if ignoredInputLinesCount.positive?

          # Remove completely for stockName is there are either no train or gain timestamps.
          unless trainTimestampsCount.positive? && gainTimestampsCount.positive?
            error("Stock ignored.\n")
            @stockName_timestamp_amounts.delete(stockName)
          end
        else
          error("File ignored as first line '", header, "' does not match header '", inputFilesHeader, "'.\n")
        end
      end
    end
    ensureThereAreStocks

    # Average each amounts matrix column, into an array.
    print("∙ AVERAGING multiple amounts...\n")

    @stockName_timestamp_amounts.each_value do |timestamp_amounts|
      timestamp_amounts.each_pair do |timestamp, amountsMatrix|
        # amountsMatrix has multiple rows.
        if (amountsMatrixRowsCount = amountsMatrix.size) > 1
          timestamp_amounts[timestamp] = \
            amountsMatrix.transpose.map { |amountsVector| amountsVector.sum.div(amountsMatrixRowsCount) }
        # amountsMatrix is a one-row matrix.
        else
          timestamp_amounts[timestamp] = amountsMatrix.first
        end
      end

      # # transform_values! is broken under ruby 2.7.0p0 (2019-12-25 revision 647ee6f091) [x86_64-linux-gnu].
      # # The else condition works fine, but the if condition either CRASHES with a core dump, or does nothing.
      # timestamp_amounts.transform_values! do |amountsMatrix|
      #   # amountsMatrix has multiple rows.
      #   if (amountsMatrixRowsCount = amountsMatrix.size) > 1
      #     # Average each matrix column.
      #     amountsMatrix.transpose.map { |amountsVector| amountsVector.sum.div(amountsMatrixRowsCount) }
      #   # amountsMatrix is a one-row matrix.
      #   else
      #     amountsMatrix.first
      #   end
      # end
    end
  end

  # Validate the stock names and timestamps and output progress. Abort on fatal errors.
  def validateParsedData
    print("\n∙ VALIDATING PARSE DATA...\n")

    ensureThereAreStocks

    # Print about stock names gathered.
    sortedStockNames = @stockName_timestamp_amounts.keys.sort
    print('◦ Gathered ', sortedStockNames.size, " stock names.\n")
    # Validate stock name lengths.
    lengthyStockNames = sortedStockNames.select { |stockName| stockName.length >= stockNameMaximumLength }
    unless lengthyStockNames.empty?
      abort(
        'The following stock name(s) contain ',
        stockNameMaximumLength,
        " characters or more: '",
        lengthyStockNames.join("', '"),
        "'.\n"
      )
    end

    # Generate sorted timestamps for each stock name.
    @stockName_sortedTimestamps = {}
    @stockName_reverseSortedTimestamps = {}
    @stockName_timestamp_amounts.each_pair do |stockName, timestamp_amounts|
      @stockName_sortedTimestamps[stockName] = sortedTimestamps = timestamp_amounts.keys.sort
      @stockName_reverseSortedTimestamps[stockName] = sortedTimestamps.reverse
    end

    # Print about timestamps gathered.
    minmaxTrainTimestampCounts = \
      @stockName_timestamp_amounts.minmax_by { |_, timestamp_amounts| timestamp_amounts.size }
    print(
      '◦ Gathered ',
      # hash.minmax_by returns [min, max] where min and max are [key, value].
      minmaxTrainTimestampCounts[0][1].size,
      ' to ',
      minmaxTrainTimestampCounts[1][1].size,
      " timestamps.\n"
    )
  end

  # Compute all the gains. Abort on fatal errors.
  def computeGains
    print("\n∙ COMPUTING GAINS...\n")

    ensureThereAreStocks

    # Compute the gains.
    gainMultiplier = 100
    gain_stockNames = ::Hash.new { |hash, gain| hash[gain] = [] }
    @stockName_timestamp_amounts.each_pair do |stockName, timestamp_amounts|
      # Seek first timestamp >= @beginGainTimestamp.
      firstGainTimestamp = \
        @stockName_sortedTimestamps[stockName].bsearch { |timestamp| timestamp >= @beginGainTimestamp }
      if firstGainTimestamp
        # If firstGainTimestamp is > @beginGainTimestamp then say so.
        unless firstGainTimestamp.eql?(@beginGainTimestamp)
          print("◦ First gain timestamp is '", firstGainTimestamp, "' for stock '", stockName, "'.\n")
        end
      else
        # No timestamp is >= @beginGainTimestamp.
        abort("Can not get a begin gain timestamp for stock '", stockName, "'.\n")
      end

      # Seek last timestamp <= @endGainTimestamp.
      lastGainTimestamp = \
        @stockName_reverseSortedTimestamps[stockName].bsearch { |timestamp| timestamp <= @endGainTimestamp }
      if lastGainTimestamp
        # If lastGainTimestamp is < @endGainTimestamp then say so.
        unless lastGainTimestamp.eql?(@endGainTimestamp)
          print("◦ Last gain timestamp is '", lastGainTimestamp, "' for stock '", stockName, "'.\n")
        end
      else
        # No timestamp is <= @endGainTimestamp.
        abort("Can not get an end gain timestamp for stock '", stockName, "'.\n")
      end

      abort("First gain timestamp is AFTER last gain timestamp.\n") if firstGainTimestamp > lastGainTimestamp

      openPrice = timestamp_amounts[firstGainTimestamp][0]
      closePrice = timestamp_amounts[lastGainTimestamp][3]
      unless openPrice.eql?(0)
        gain_stockNames[((Float(closePrice - openPrice) * gainMultiplier) / openPrice).round(3)].append(stockName)
      end
    end

    # Print all gains.
    print("∙ Gains are:\n")
    gain_stockNames.keys.sort.reverse.each do |gain|
      print('◦ ', gain, "% for stock '", gain_stockNames[gain].join("', '"), "'.\n")
    end
  end

  # Extrapolate the missing timestamps and output progress. Abort on fatal errors.
  def extrapolateMissingTrainTimestamps
    print("\n∙ EXTRAPOLATING MISSING TRAIN TIMESTAMPS...\n")

    ensureThereAreStocks

    # Mapping { missingTrainTimestampsCount => stockNames = [ stockName1, stockName2, ...] }.
    missingTrainTimestampsCount_stockNames = \
      ::Hash.new { |hash, missingTrainTimestampsCount| hash[missingTrainTimestampsCount] = [] }
    # Mapping { stockName => largestGap }.
    stockName_largestGapSize = {}
    # Mapping { stockName => gapsCount }.
    stockName_gapsCount = {}
    stocksWithTooManyTrainTimestampsGaps = []

    @stockName_timestamp_amounts.each_pair do |stockName, timestamp_amounts|
      # Seek smallest timestamp >= @beginTrainTimestamp.
      firstTrainTimestamp = \
        @stockName_sortedTimestamps[stockName].bsearch { |timestamp| timestamp >= @beginTrainTimestamp }
      if firstTrainTimestamp
        # If firstTrainTimestamp is > @beginTrainTimestamp then say so.
        unless firstTrainTimestamp.eql?(@beginTrainTimestamp)
          print("◦ First train timestamp is '", firstTrainTimestamp, "' for stock '", stockName, "'.\n")
        end
      else
        # No timestamp is >= @beginTrainTimestamp.
        abort("Can not get a begin train timestamp for stock '", stockName, "'.\n")
      end

      # Seek largest timestamp <= @endTrainTimestamp.
      lastTrainTimestamp = \
        @stockName_reverseSortedTimestamps[stockName].bsearch { |timestamp| timestamp <= @endTrainTimestamp }
      if lastTrainTimestamp
        # If lastTrainTimestamp is < @endTrainTimestamp then say so.
        unless lastTrainTimestamp.eql?(@endTrainTimestamp)
          print("◦ Last train timestamp is '", lastTrainTimestamp, "' for stock '", stockName, "'.\n")
        end
      else
        # No timestamp is <= @endTrainTimestamp.
        abort("Can not get an end train timestamp for stock '", stockName, "'.")
      end

      abort("First train timestamp is AFTER last train timestamp.\n") if firstTrainTimestamp > lastTrainTimestamp
      abort("First and last train timestamps are the same.\n") if firstTrainTimestamp.eql?(lastTrainTimestamp)

      unless firstTrainTimestamp.eql?(@beginTrainTimestamp)
        timestamp_amounts[@beginTrainTimestamp] = timestamp_amounts.delete(firstTrainTimestamp)
      end
      unless lastTrainTimestamp.eql?(@endTrainTimestamp)
        timestamp_amounts[@endTrainTimestamp] = timestamp_amounts.delete(lastTrainTimestamp)
      end

      # Row-extrapolate amounts for the missing timestamps.
      missingTrainTimestampsCount = 0
      gap = []
      gapsCount = 0
      largestGapSize = 0

      latestTrainTimestamp = @beginTrainTimestamp
      # Use div and ignore rounding.
      @sortedTrainTimestamps.each do |trainTimestamp|
        # The stock possesses this timestamp.
        if timestamp_amounts.key?(trainTimestamp)
          # If gap is not empty then then we just came out of a gap.
          unless gap.empty?
            gapsCount += 1
            missingTrainTimestampsCount += (gapSize = gap.size)
            largestGapSize = gapSize if gapSize > largestGapSize

            # amounts is [open, high, low, close, volume], so for a size 2 gap:
            # 2022-01-01 09:31, 2, 4, 1, 3, 11  # latestTrainTimestamp;
            # 2022-01-01 09:32,  ,  ,  ,  ,     # missing;
            # 2022-01-01 09:33,  ,  ,  ,  ,     # missing;
            # 2022-01-01 09:34, 7, 9, 6, 6, 30  # trainTimestamp.

            # First open in gap is latest train timestamp's close = 3.
            open = timestamp_amounts[latestTrainTimestamp][3]
            # openIncrement is (current train timestamp's open - first open) / gap size = (7 - 3) / 2 = 2.
            openIncrement = (timestamp_amounts[trainTimestamp][0] - open).div(gapSize)
            # So to get:
            # 2022-01-01 09:31, 2, 4, 1, 3, 11  # latestTrainTimestamp;
            # 2022-01-01 09:32, 3  (+2)  5,     # missing;
            # 2022-01-01 09:33, 5, (+2)  7,     # missing;
            # 2022-01-01 09:34, 7, 9, 6, 6, 30  # trainTimestamp.

            # volume just following a gap, sums up all the previous missing train timestamps:
            volume = timestamp_amounts[trainTimestamp][4].div(gapSize + 1)
            timestamp_amounts[trainTimestamp][4] = volume
            # So to get:
            # 2022-01-01 09:31, 2, 4, 1, 3, 11  # latestTrainTimestamp;
            # 2022-01-01 09:32,  ,  ,  ,  , 10  # missing;
            # 2022-01-01 09:33,  ,  ,  ,  , 10  # missing;
            # 2022-01-01 09:34, 7, 9, 6, 6, 10  # trainTimestamp.

            # Generate all missing train timestamps, so to get:
            # 2022-01-01 09:31, 2, 4, 1, 3, 11  # latestTrainTimestamp;
            # 2022-01-01 09:32, 3, 5, 3, 5, 10  # missing;
            # 2022-01-01 09:33, 5, 7, 5, 7, 10  # missing;
            # 2022-01-01 09:34, 7, 9, 6, 6, 10  # trainTimestamp.

            # (gap is already sorted.)
            gap.each do |missingTrainTimestamp|
              close = open + openIncrement
              # high and low are approximated to be max and min of open and close.
              timestamp_amounts[missingTrainTimestamp] = [open, open.max(close), open.min(close), close, volume]
              open = close
            end
            gap = []
          end

          latestTrainTimestamp = trainTimestamp
        # If the stock does not possess this timestamps, then the gap is widening.
        else
          gap.append(trainTimestamp)
        end
      end

      # Store the total missing train timestamps counts.
      if missingTrainTimestampsCount.positive?
        missingTrainTimestampsCount_stockNames[missingTrainTimestampsCount].append(stockName)
        stockName_largestGapSize[stockName] = largestGapSize
        stockName_gapsCount[stockName] = gapsCount

        stocksWithTooManyTrainTimestampsGaps.append(stockName) if largestGapSize > @maximumTrainTimestampsGapSize
      end
    end

    # Print all missing train timestamp counts.
    missingTrainTimestampsCount_stockNames.keys.sort.reverse.each do |missingTrainTimestampsCount|
      stockNames = missingTrainTimestampsCount_stockNames[missingTrainTimestampsCount]
      stockNames.map! do |stockName|
        "'" + stockName + "' (in " + stockName_gapsCount[stockName].to_s + ' gaps of widest size ' + \
          stockName_largestGapSize[stockName].to_s + ')'
      end
      print('◦ Missed ', missingTrainTimestampsCount, ' train timestamps: ', stockNames.join(', '), ".\n")
    end

    unless stocksWithTooManyTrainTimestampsGaps.empty?
      print(
        "\n∙∙∙ The following ", \
        stocksWithTooManyTrainTimestampsGaps.size, \
        ' stocks with gap(s) wider than ', \
        @maximumTrainTimestampsGapSize, \
        ' will be ignored: ', \
        stocksWithTooManyTrainTimestampsGaps.join("', '"), \
        "'.\n"
      )
      stocksWithTooManyTrainTimestampsGaps.each { |stockName| @stockName_timestamp_amounts.delete(stockName) }
    end
  end

  # Output train timestamps and amounts to a text file. Output progress and errors.
  def outputTrainDataToTextFile(outputName)
    # Output to STOCKS_EVENT_<@fileNameInfix>_<outputName>.txt.
    outputFileName = 'EVENT_' + (@fileNameInfix ? (@fileNameInfix + '_') : '') + outputName + '.txt'
    sortedStockNames = @stockName_timestamp_amounts.keys.sort
    print("\n∙ OUTPUTTING train data for ", sortedStockNames.size, " stocks to text file '", outputFileName, "'... ")

    ensureThereAreStocks

    begin
      ::File.open(outputFileName, 'w') do |outputFile|
        # Output by sorted stock name.
        sortedStockNames.each do |stockName|
          timestamp_amounts = @stockName_timestamp_amounts[stockName]
          # Output '# stockName'.
          outputFile.print('# ', stockName, "\n# ", inputFilesHeader, "\n")
          # Output for all train timestamps.
          @sortedTrainTimestamps.each do |trainTimestamp|
            # amounts is [open, high, low, close, volume].
            amounts = timestamp_amounts[trainTimestamp]
            outputFile.print(
              trainTimestamp,
              ',',
              amounts[0],
              ',',
              amounts[1],
              ',',
              amounts[2],
              ',',
              amounts[3],
              ',',
              amounts[4],
              "\n"
            )
          end
        end
      end
    rescue ::StandardError => error
      exception(error, "Can not write to file.\n")
    else
      print("Done.\n")
    end
  end

  # Globally rescale all prices and reproportion to 16 bits all amounts. Output progress and abort on fatal errors.
  def normalizeTrainAmounts
    # Normalize to 16 bits.
    maximumAmount = (2**16) - 1
    print("\n∙ NORMALIZING ALL AMOUNTS to maximum ", maximumAmount, "...\n")

    ensureThereAreStocks

    # Rescale each stock price to the maximum stock price range, as they typically do not vary widely.
    # Do not rescale volumes as they typically vary widely.
    maximumPriceRangeFraction = 0r
    # Opportunistically gather the maximum price for each stock.
    stockName_maximumPrice = {}
    @stockName_timestamp_amounts.each_pair do |stockName, timestamp_amounts|
      # Calculate each stockName's maximum and minimum prices, e.i.: open, high, low and close.
      # amounts is [open, high, low, close, volume].

      minimumPricesTrainTimestamp = \
        @sortedTrainTimestamps.min_by { |trainTimeStamp| timestamp_amounts[trainTimeStamp][0..3].min }
      minimumPrice = timestamp_amounts[minimumPricesTrainTimestamp][0..3].min

      maximumPricesTrainTimestamp = \
        @sortedTrainTimestamps.max_by { |trainTimestamp| timestamp_amounts[trainTimestamp][0..3].max }
      stockName_maximumPrice[stockName] = maximumPrice = timestamp_amounts[maximumPricesTrainTimestamp][0..3].max

      # Compute the maximum price range fraction of all stocks.
      priceRangeFraction = Rational(maximumPrice - minimumPrice) / maximumPrice
      maximumPriceRangeFraction = priceRangeFraction if priceRangeFraction > maximumPriceRangeFraction
    end

    print("\n∙∙∙  MAXIMUM PRICE RANGE FRACTION IS ", Float(maximumPriceRangeFraction).round(6), ".\n")

    # Rescale, and reproportion prices to new interval 1..(2^16 - 1).
    maximumPriceAmount = maximumAmount - 1
    @stockName_timestamp_amounts.each_pair do |stockName, timestamp_amounts|
      # Maximum stock price rescaled according to the maximum price range fraction.
      maximumPrice = stockName_maximumPrice[stockName]
      rescaledMaximumPrice = maximumPriceRangeFraction * maximumPrice
      # Compute the rescale delta.
      rescaleDelta = maximumPrice - rescaledMaximumPrice

      # Normalize volumes to 0..(2^16 -1).
      # amounts is [open, high, low, close, volume].
      maximumVolumeTrainTimestamp = \
        @sortedTrainTimestamps.max_by { |trainTimestamp| timestamp_amounts[trainTimestamp][4] }
      maximumVolume = timestamp_amounts[maximumVolumeTrainTimestamp][4]

      # Rescale and normalize all amounts.
      # amounts is [open, high, low, close, volume].
      @sortedTrainTimestamps.each do |trainTimestamp|
        amounts = timestamp_amounts[trainTimestamp]

        # Rescaled price = price - rescale delta.
        # div returns an integer.
        # Reproportioned price = (rescaled price * maximum price amount / rescaled maximum price) + 1.
        amounts[0] = ((amounts[0] - rescaleDelta) * maximumPriceAmount).div(rescaledMaximumPrice) + 1
        amounts[1] = ((amounts[1] - rescaleDelta) * maximumPriceAmount).div(rescaledMaximumPrice) + 1
        amounts[2] = ((amounts[2] - rescaleDelta) * maximumPriceAmount).div(rescaledMaximumPrice) + 1
        amounts[3] = ((amounts[3] - rescaleDelta) * maximumPriceAmount).div(rescaledMaximumPrice) + 1

        # Reproportion (without rescaling) volume.
        amounts[4] = (amounts[4] * maximumAmount).div(maximumVolume)

        # Sanity check that no Normalized amount is too big.
        if amounts.max > maximumAmount
          abort('At least one normalized amount is greater than ', maximumAmount.to_s, ".\n")
        end
      end
    end
  end

  # Output current amounts to binary file. Output progress and error.
  def outputTrainDataToBinaryFile
    # Output to STOCKS_EVENT_<@fileNameInfix>.bin.
    outputFileName = 'EVENT' + (@fileNameInfix ? ('_' + @fileNameInfix) : '') + '.bin'
    sortedStockNames = @stockName_timestamp_amounts.keys.sort
    print(
      "\n∙ OUTPUTTING 16 bit unsigned train data for ", \
      sortedStockNames.size, \
      " stocks to binary file '", \
      outputFileName, \
      "'... "
    )

    ensureThereAreStocks

    begin
      ::File.open(outputFileName, 'wb') do |outputFile|
        # Header: number of stock matrixes, matrix rows count, matrix columns count, stock name size.
        outputFile.print(
          [
            sortedStockNames.size,
            @sortedTrainTimestamps.size,
            columnsCount,
            stockNameMaximumLength
          ].pack('LLLL')
          # L for pack is 32-bit unsigned, native endian (uint32_t).
        )

        # Output by sorted stock names.
        sortedStockNames.each do |stockName|
          timestamp_amounts = @stockName_timestamp_amounts[stockName]

          packedStockName = stockName.dup
          ((packedStockName.size)..(stockNameMaximumLength - 1)).each { |index| packedStockName[index] = "\0" }
          outputFile.print(packedStockName)

          # Output only for the train timestamps ordered by timestamp.
          @sortedTrainTimestamps.each do |trainTimestamp|
            # amounts is [open, high, low, close, volume], S for pack is 16-bit unsigned, native endian (uint16_t).
            outputFile.print(timestamp_amounts[trainTimestamp].pack('SSSSS'))
          end
        end
      end
    rescue ::StandardError => error
      exception(error, "Can not write to file.\n")
    else
      print("Done.\n")
    end
  end

  #########
  # TESTS #
  #########

  @@testSymbols = []
  # Run all test cases registered in @@testSymbols.
  def testAll
    print("█ BEGIN TESTING...\n")
    @@testSymbols.each { |testSymbol| testInTemporaryDirectory(testSymbol) }
    print("\n█ DONE TESTING.\n")
  rescue ::StandardError => error
    exception(error, "Can not test.\n")
    abort
  end

  @@testSymbols.push(:testNoArguments)
  # Call the script with no argument.
  def testNoArguments
    if testParseStocks(__method__, []) && @stderr.include?('FATAL ERROR! USAGE:')
      testPassed
    else
      testFailed
    end
  end

  @@testSymbols.push(:testOneTooFewArgument)
  # Call the script with too few arguments.
  def testOneTooFewArgument
    if testParseStocks(__method__, [1, 2, 3, 4, 5, 6]) && @stderr.include?('FATAL ERROR! USAGE:')
      testPassed
    else
      testFailed
    end
  end

  @@testSymbols.push(:testTimestampsOutOfOrder)
  # Call the script with timestamps reversed.
  def testTimestampsOutOfOrder
    arguments = ['2022-01-11', '11:07', '2022-01-11', '11:06', '2022-02-12 09:31', '2022-02-12 16:00', 'a']
    if testParseStocks(__method__, arguments.dup) && \
       @stderr.include?('Begin train timestamp is AFTER end train timestamp.')

      testPassed('Train Timestamps')
    else
      testFailed('Train Timestamps')
    end

    arguments[1], arguments[3], arguments[4], arguments[5] = arguments[3], arguments[1], arguments[5], arguments[4]
    if testParseStocks(__method__, arguments) && @stderr.include?('Begin gain timestamp is AFTER end gain timestamp.')

      testPassed('Gain Timestamps')
    else
      testFailed('Gain Timestamps')
    end
  end

  @@testSymbols.push(:testTimestampsCountGenerated_CanNotReadFile)
  # Call the script with timestamps out of order and a non-existing file name.
  def testTimestampsCountGenerated_CanNotReadFile
    arguments = ['2022-01-11', '09:31', '2022-01-15', '16:00', '2022-02-12 09:31', '2022-02-12 16:00', '/a']
    # Should abort as no file will be read. 9:31 to 16:00 is exactly 6.5 hours * 60 minutes * 5 days = 1950 timestamps.
    if testParseStocks(__method__, arguments) && @stderr.include?('No stock were parsed.') \
       && @stdout.include?('1950 TRAIN TIMESTAMP(S) WERE GENERATED.') && @stdout.include?('Can not read file.')

      testPassed
    else
      testFailed
    end
  end

  @@testSymbols.push(:testFileWrongHeader)
  # Call the script with a file with wrong header.
  def testFileWrongHeader
    fileName = 'A.csv'
    ::File.open(fileName, 'w') { |outputFile| outputFile.print("Hello\n") }

    arguments = ['2022-01-11', '11:01', '2022-01-11', '11:10', '2022-02-12 09:31', '2022-02-12 16:00', fileName]
    if testParseStocks(__method__, arguments) && @stderr.include?('No stock were parsed.') \
       && @stdout.include?('does not match header')

      testPassed
    else
      testFailed
    end
  end

  @@testSymbols.push(:testLinesIgnored)
  # Call the script with a file with a malformed line.
  def testLinesIgnored
    fileName = 'B.csv'
    ::File.open(fileName, 'w') do |outputFile|
      outputFile.print(inputFilesHeader + "\n")
      outputFile.print("Hello\n")
    end

    arguments = ['2022-01-11', '11:01', '2022-01-11', '11:10', '2022-02-12 09:31', '2022-02-12 16:00', fileName]
    if testParseStocks(__method__, arguments) && @stderr.include?('No stock were parsed.') \
       && !@stdout.include?('does not match header') && @stdout.include?('1 line(s) ignored.')

      testPassed
    else
      testFailed
    end
  end

  @@testSymbols.push(:testStockIgnored)
  # Call the script with a stock having all its timestamps out of both time periods.
  def testStockIgnored
    fileName = 'C.csv'
    # 'timestamp,open,high,low,close,volume'
    ::File.open(fileName, 'w') do |outputFile|
      outputFile.print(inputFilesHeader + "\n")
      outputFile.print("2022-01-11 13:01:00, 2.0000, 3.0000, 2.0000, 3.0000, 234\n")
      outputFile.print("2022-01-11 13:00:00, 1.0000, 2.0000, 1.0000, 2.0000, 123\n")
    end

    # Disjoint timestamps.
    arguments = ['2022-01-11', '11:01', '2022-01-11', '11:10', '2022-02-12 09:31', '2022-02-12 16:00', fileName]
    if testParseStocks(__method__, arguments) && @stderr.include?('No stock were parsed.') \
       && @stdout.include?('Gathered: 0 train and 0 gain timestamps.') && @stdout.include?('Stock ignored.')

      testPassed
    else
      testFailed
    end
  end

  @@testSymbols.push(:testStockNameTooLong)
  # Call the script with a stock name that is too long.
  def testStockNameTooLong
    fn1 = '123456789012345.csv'
    fn2 = '1234567890123456.csv'
    # 'timestamp,open,high,low,close,volume'
    ::File.open(fn1, 'w') do |outputFile|
      outputFile.print(inputFilesHeader + "\n")
      outputFile.print("2022-01-11 13:01:00, 2.0000, 3.0000, 2.0000, 3.0000, 234\n")
      outputFile.print("2022-01-11 13:00:00, 1.0000, 2.0000, 1.0000, 2.0000, 123\n")
    end
    ::File.open(fn2, 'w') do |outputFile|
      outputFile.print(inputFilesHeader + "\n")
      outputFile.print("2022-01-11 13:01:00, 2.0000, 3.0000, 2.0000, 3.0000, 234\n")
      outputFile.print("2022-01-11 13:00:00, 1.0000, 2.0000, 1.0000, 2.0000, 123\n")
    end

    arguments = ['2022-01-11', '13:00', '2022-01-11', '13:01', '2022-01-11 13:00', '2022-01-11 13:01', fn1, fn2]
    if testParseStocks(__method__, arguments)
      if @stdout.include?("The following stock name(s) contain 16 characters or more: '123456789012345'.") \
         || !@stderr.include?("The following stock name(s) contain 16 characters or more: '1234567890123456'.")

        testFailed
      else
        testPassed
      end
    else
      testFailed
    end
  end

  @@testSymbols.push(:testGatheredTimestamps)
  # Call the script with and check if it gathered the right number of timestamps.
  def testGatheredTimestamps
    fn1 = '12345678901234.csv'
    fn2 = '123456789012345.csv'
    ::File.open(fn1, 'w') do |outputFile|
      outputFile.print(inputFilesHeader + "\n")
      outputFile.print("2022-01-11 13:00:00, 1.0000, 2.0000, 1.0000, 2.0000, 123\n")
    end
    ::File.open(fn2, 'w') do |outputFile|
      outputFile.print(inputFilesHeader + "\n")
      outputFile.print("2022-01-11 13:01:00, 2.0000, 3.0000, 2.0000, 3.0000, 234\n")
      outputFile.print("2022-01-11 13:00:00, 1.0000, 2.0000, 1.0000, 2.0000, 123\n")
    end

    arguments = ['2022-01-11', '13:00', '2022-01-11', '13:01', '2022-01-11 13:00', '2022-01-11 13:01', fn1, fn2]
    if testParseStocks(__method__, arguments)
      if @stdout.include?('Gathered 1 to 2 timestamps.') \
         && @stderr.include?('First and last train timestamps are the same.')

        testPassed
      else
        testFailed
      end
    else
      testFailed
    end
  end

  @@testSymbols.push(:testGathered_Averaging_OutputToTextFile)
  # Call the script with files to test other gathered infos,
  # as well as ensure multiple files for a same stock name are averaged correctly.
  def testGathered_Averaging_OutputToTextFile
    # 'timestamp,open,high,low,close,volume'
    d1 = [\
      inputFilesHeader \
      , '2022-01-15 10:03:00,9.0009,1.0010,2.0011,3.0012,34' \
      , '2022-01-15 10:02:00,4.0013,5.0014,6.0015,7.0016,45' \
      , '2022-01-15 10:01:00,8.0017,9.0018,1.0019,2.0020,56'
    ]
    d2 = [\
      inputFilesHeader \
      , '2022-01-15 10:03:00,1.0001,2.0011,3.0012,4.0013,4' \
      , '2022-01-15 10:02:00,5.0014,6.0015,7.0016,8.0017,5' \
      , '2022-01-15 10:01:00,9.0018,1.0019,2.0020,3.0021,6'
    ]
    e = [\
      inputFilesHeader \
      , '2022-01-15 10:03:00,9.0000,1.0000,2.0000,3.0000,345' \
      , '2022-01-15 10:02:00,4.0000,5.0000,6.0000,7.0000,456' \
      , '2022-01-15 10:01:00,8.0000,9.0000,1.0000,2.0000,567'
    ]
    # D's prices had their decimal dots removed, then averaged by hand, then the timestamps were sorted!
    manualStocks = [\
      '# D' \
      , ('# ' + inputFilesHeader) \
      , '2022-01-15 10:01,85017,50018,15019,25020,31' \
      , '2022-01-15 10:02,45013,55014,65015,75016,25' \
      , '2022-01-15 10:03,50005,15010,25011,35012,19' \
      , '# E' \
      , ('# ' + inputFilesHeader) \
      , '2022-01-15 10:01,80000,90000,10000,20000,567' \
      , '2022-01-15 10:02,40000,50000,60000,70000,456' \
      , '2022-01-15 10:03,90000,10000,20000,30000,345'
    ]

    fn1 = 'D.1'
    fn2 = 'D.2'
    fn3 = 'E.1'
    ::File.open(fn1, 'w') { |outputFile| d1.each { |line| outputFile.print(line, "\n") } }
    ::File.open(fn2, 'w') { |outputFile| d2.each { |line| outputFile.print(line, "\n") } }
    ::File.open(fn3, 'w') { |outputFile| e.each { |line| outputFile.print(line, "\n") } }

    arguments = ['2022-01-15', '10:01', '2022-01-15', '10:03', '2022-01-15 10:01', '2022-01-15 10:03', fn1, fn2, fn3]
    if testParseStocks(__method__, arguments)
      testFailed
    else
      if @stdout.include?("Reading file 'D.1' for stock 'D'... Gathered: 3 train and 3 gain timestamps.") \
         && @stdout.include?("Reading file 'D.2' for stock 'D'... Gathered: 3 train and 3 gain timestamps.") \
         && @stdout.include?("Reading file 'E.1' for stock 'E'... Gathered: 3 train and 3 gain timestamps.") \
         && @stdout.include?('Gathered 2 stock names.')

        testPassed('Gathered')
      else
        testFailed('Gathered')
      end

      if (bruteFile = ::Dir.glob('*brute*')&.first)
        averagedStocks = ::File.readlines(bruteFile, chomp: true)
        if averagedStocks.eql?(manualStocks)
          testPassed('Averaging')
        else
          testFailed('Stocks not averaged properly')
          print("\naveragedStocks is:\n", averagedStocks, "\nmanualStocks is:\n", manualStocks, "\n")
        end
      else
        testFailed("No '*brute*' file found")
      end
    end
  end

  @@testSymbols.push(:testExtrapolateMissingTrainTimestamps)
  # Call the script with stocks having gaps in their timestamps and ensure they are extrapolated properly.
  def testExtrapolateMissingTrainTimestamps
    # 'timestamp,open,high,low,close,volume'
    original = [\
      inputFilesHeader \
      , '2022-01-01 09:32,2.0000,4.0000,1.0000,3.0000,11' \
      , '2022-01-01 09:33,7.0000,9.0000,6.0000,6.0000,30'
    ]
    manual = [\
      '# G' \
      , ('# ' + inputFilesHeader) \
      , '2022-01-01 09:31,20000,40000,10000,30000,11' \
      , '2022-01-01 09:32,30000,50000,30000,50000,10' \
      , '2022-01-01 09:33,50000,70000,50000,70000,10' \
      , '2022-01-01 09:34,70000,90000,60000,60000,10'
    ]
    fileName = 'G'
    ::File.open(fileName, 'w') { |outputFile| original.each { |line| outputFile.print(line, "\n") } }

    arguments = ['2022-01-01', '09:31', '2022-01-01', '09:34', '2022-01-01 09:31', '2022-01-01 09:34', fileName]
    if !testParseStocks(__method__, arguments) && (bruteFile = ::Dir.glob('*brute*')&.first)
      brute = ::File.readlines(bruteFile, chomp: true)
      if brute.eql?(manual)
        testPassed
      else
        testFailed
        print("\nmanual is:\n", manual, "\nbrute is:\n", brute)
      end
    else
      testFailed
    end
  end

  @@testSymbols.push(:testNormalizeTrainAmounts)
  # Call the script to ensure price amounts are rescaled and reproportionned properly,
  # and volumes are reproportionned only.
  def testNormalizeTrainAmounts
    # 'timestamp,open,high,low,close,volume'
    a = [\
      inputFilesHeader \
      , '2022-01-15 10:03:00,95.0000,100.0000,95.0000,100.0000,34' \
      , '2022-01-15 10:02:00,90.0000,95.0000,90.0000,95.0000,45' \
      , '2022-01-15 10:01:00,85.0000,90.0000,85.0000,90.0000,56'
    ]
    b = [\
      inputFilesHeader \
      , '2022-01-15 10:03:00,0.0800,0.0800,0.0700,0.0700,10' \
      , '2022-01-15 10:02:00,0.0900,0.0900,0.0800,0.0800,9' \
      , '2022-01-15 10:01:00,0.1000,0.1000,0.0900,0.0900,8'
    ]

    # A's volumes are: 56 * 65535 / 56 = 65535, 45 * 65535 / 56 = 52662.05, 34 * 65535 / 56 = 39789.11.
    # B's volumes are: B: 8 * 65535 / 10 = 52428, 9 * 65535 / 10 = 58981.5, 10 * 65535 / 10 = 65535.
    # A's price range fraction is: 100-90/100 = 10%.
    # price range fraction for B: 0.1-0.07/0.1 = 30%.
    # maximum price range fraction is 30%.
    # A's rescaled prices are thus 30, 25, 20, 15, B's rescaled prices are thus 0.0, 0.1, 0.2, 0.3.
    # A's reproportioned prices are thus: (15 * 65534 / 30) + 1 = 32768, (20 * 65534 / 30) + 1 = 43690.3,
    # (25 * 65534 / 30) + 1 = 54612.7, (30 * 65534 / 30) + 1 = 65535.
    # B's reproportioned prices are thus: (0.3 * 65534 / 0.3) + 1 = 65535, (0.2 * 65534 / 0.3) + 1 = 43690.3,
    # (0.1 * 65534 / 0.3) + 1 = 21845.7, (0.0 * 65534 / 0.3) + 1 = 1.
    manual = [\
      '# A' \
      , ('# ' + inputFilesHeader) \
      , '2022-01-15 10:01,32768,43690,32768,43690,65535' \
      , '2022-01-15 10:02,43690,54612,43690,54612,52662' \
      , '2022-01-15 10:03,54612,65535,54612,65535,39789' \
      , '# B' \
      , ('# ' + inputFilesHeader) \
      , '2022-01-15 10:01,65535,65535,43690,43690,52428' \
      , '2022-01-15 10:02,43690,43690,21845,21845,58981' \
      , '2022-01-15 10:03,21845,21845,1,1,65535'
    ]

    fn1 = 'A'
    fn2 = 'B'
    ::File.open(fn1, 'w') { |outputFile| a.each { |line| outputFile.print(line, "\n") } }
    ::File.open(fn2, 'w') { |outputFile| b.each { |line| outputFile.print(line, "\n") } }

    arguments = ['2022-01-15', '10:01', '2022-01-15', '10:03', '2022-01-15 10:01', '2022-01-15 10:03', fn1, fn2]
    if !testParseStocks(__method__, arguments) && (bruteFile = ::Dir.glob('*brute*')&.first) \
       && (normalizedFile = ::Dir.glob('*normalized*')&.first) \

      brute = ::File.readlines(bruteFile, chomp: true)
      normalized = ::File.readlines(normalizedFile, chomp: true)
      if normalized.eql?(manual)
        testPassed
      else
        print("\nbrute is:\n", brute, "\nnormalized is:\n", normalized, "\nmanual is:\n", manual, "\n")
        testFailed
      end
    else
      testFailed
    end
  end

  ##########################
  # TESTING INFRASTRUCTURE #
  ##########################

  # Run a test method according to the symbol provided, in a temporary directory deleted when the test finishes.
  def testInTemporaryDirectory(testSymbol)
    ::Dir.mktmpdir do |directory|
      ::Dir.chdir(directory) do
        __send__(testSymbol)
        print("\n")
      end
    end
  end

  # Test the script (in testName's context) according to the provided arguments. Return true if script tried to abort.
  def testParseStocks(testName, arguments)
    print("\n∙ ", testName, ':')
    hijackStandardOutputs
    begin
      @arguments = arguments
      parseStocks
    # Return true if script tried to abort.
    rescue ::SystemExit
      reinstateStandardOutputs
      true
    # Propagate any other exception.
    rescue ::Exception
      reinstateStandardOutputs
      raise
    # Return false if script did not try to abort.
    else
      reinstateStandardOutputs
      false
    end
  end

  # The following two methods must absolutely be called in pairs, right before and after running a test.

  # Hijack the standard outputs and forward them in a string IOs instead.
  def hijackStandardOutputs
    @originalStderr = $stderr
    @stderr = ($stderr = ::StringIO.new)

    @originalStdout = $stdout
    @stdout = ($stdout = ::StringIO.new)
  end

  # Reinstate the standard outputs and make the hijacked outputs available as strings.
  def reinstateStandardOutputs
    $stderr = @originalStderr
    @stderr = @stderr.string

    $stdout = @originalStdout
    @stdout = @stdout.string
  end

  # Print Passed and *message if any.
  def testPassed(*message)
    if message.empty?
      print(' Passed.')
    else
      print(' Passed (', *message, ').')
    end
  end

  # Print FAILED and *message if any.
  def testFailed(*message)
    if message.empty?
      print(' FAILED!')
    else
      print(' FAILED (', *message, ')!')
    end
    printLastTestOutputs
  end

  # Print the whole standard outputs that were lastly captured.
  def printLastTestOutputs
    if @stderr&.empty?
      print("\n\nNOTHING in $stderr.\n")
    else
      print("\n\n$stderr is:\n--->  --->  --->  --->  --->\n", @stderr, "<---  <---  <---  <---  <---\n")
    end

    if @stdout&.empty?
      print("\nNOTHING in $stdout.\n")
    else
      print("\n$stdout is:\n--->  --->  --->  --->  --->\n", @stdout, "<---  <---  <---  <---  <---\n")
    end
  end
end

##########
## MAIN ##
##########

::StocksParser.new(::Time.now.strftime('%Y-%m-%d_%H-%M-%S')).start
