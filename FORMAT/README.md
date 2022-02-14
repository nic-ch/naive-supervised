# Normalize and Format the Training Data

The [main README](../README.md) sits in the parent directory.

---

## License

*All trademarks are the property of their respective owners.*

Copyright 2022 Nicolas Chaussé (nicolaschausse@protonmail.com)

    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License only.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

---

## Introduction

As illustrated in [GETTING_STARTED](GETTING_STARTED.md), Ruby script ***FORMAT/parseStocks.rb*** is used to parse, normalize and format each downloaded weekly stocks training data, as well as list the stocks with the highest gains. The following sections explain its main points.

---

![Ruby Logo](logoRuby.png)[^1]

[^1]: © 2006 Yukihiro Matsumoto.

Ruby can be seen as a happy blend of Perl 5 and Smalltalk, plus a dose of Scheme (as `(print 'Hello ', 42)` is legal). It is extremely flexible and highly meta-programmable. Ruby is not used in this project in a strict Object-Oriented way, but to a certain extent, as a functional language. There is no classes: Stock, Timestamp, Extrapolator, Normalizer, etc. Built-in Arrays and Hashes (Dictionaries) are rather used between methods.

### Static Analysis

Awesome static analyzer [RuboCop](https://rubocop.org/) is used continually in this project as it detects bad habits and even bugs. Almost all *cops* are enable except a few listed in **.rubocop.yml** such as *Metrics* and *Naming*, but also a few odd-looking ones such as *Style/MissingElse* and *Style/Next*.

---

## Script ***FORMAT/parseStocks.rb***

Its usage is as follows:

```
$ ./parseStocks.rb 
USAGE:
	FORMAT/parseStocks.rb  TEST    ‡ ALL other arguments will be IGNORED.
-- OR --
	FORMAT/parseStocks.rb
		<begin train date>  <begin train time>
		<end train date>  <end train time>
		<begin gain timestamp>  <end gain timestamp>
		<CSV input file name>+

	‡‡ timestamps are of form 'YYYY-MM-DD HH:MM'.
$
```

Only one class: ***StocksParser*** is defined and used as an encapsulation class or somehow as a name space. Its methods, following, are called in order.

### Method ***extractArguments***

First validates that the first six arguments express valid time periods, and generate all ***train timestamps*** according the first four arguments.

### Method ***parseInputFiles***

Parses each ***input file*** name provided after the sixth argument and ensures that the corresponding *input file* is of format:

```
timestamp,open,high,low,close,volume
<YYYY-MM-DD> <HH:MM:SS>,<open>,<high>,<low>,<close>,<volume>
<YYYY-MM-DD> <HH:MM:SS>,<open>,<high>,<low>,<close>,<volume>
<YYYY-MM-DD> <HH:MM:SS>,<open>,<high>,<low>,<close>,<volume>
...
```

It then gathers lines only if within the provided *train timestamps* or *gain timestamps*. Finally, every unique *timestamp* showing multiple lines will get its amounts averaged. Oddly, consecutive downloads of stock training data of the exact same time period will occasionally show slightly different values. In that case, the script can be called as follows:

```
$ parseStocks ... DOWNLOAD_1/A.csv DOWNLOAD_2/A.csv DOWNLOAD_3/A.csv
```

... and it will average for A each unique *timestamp* showing multiple amounts lines.

### Method ***validateParsedData***

Validates what was parsed.

### Method ***computeGains***

Just computes and outputs each stock's gain according to the *gain timestamps* provided.

### Method ***extrapolateMissingTrainTimestamps***

Is more involved as we absolutely want a constant number of timestamps and any gap must absolutely extrapolated from its surroundings. Two main things are done.

**First**: the first and last *train timestamps* are repatriated if they are not already the begin and last *train timestamps*, e.g., for begin *train timestamp* '2022-01-01 09:31' and end *train timestamps* '2022-01-01 09:40':

```
timestamp,open,high,low,close,volume
2022-01-01 09:32:00, ...
2022-01-01 09:33:00, ...
2022-01-01 09:34:00, ...
2022-01-01 09:35:00, ...
2022-01-01 09:36:00, ...
2022-01-01 09:37:00, ...
```

... becomes:

```
timestamp,open,high,low,close,volume
2022-01-01 09:31:00, ...
2022-01-01 09:33:00, ...
2022-01-01 09:34:00, ...
2022-01-01 09:35:00, ...
2022-01-01 09:36:00, ...
2022-01-01 09:40:00, ...
```

**Second**: the gaps are extrapolated from the surroundings so that: 

```
timestamp,open,high,low,close,volume
2020-01-01 09:31, 2, 4, 1, 3, 11
2020-01-01 09:34, 7, 9, 6, 6, 30
```

... becomes:

```
timestamp,open,high,low,close,volume
2020-01-01 09:31, 2, 4, 1, 3, 11
2020-01-01 09:32, 3, 5, 3, 5, 10
2020-01-01 09:33, 5, 7, 5, 7, 10
2020-01-01 09:34, 7, 9, 6, 6, 10
```

Please note that:

1. The last three *timestamps* average the last existing *timestamp*'s volume among themselves.
1. For the *timestamps* that were missing, *open* and *close* gradually climb from the first existing *timestamp*'s *close* to the last existing *timestamp*'s open. *high* and *low* are derived accordingly.

### Method ***normalizeTrainAmounts***



### Methods ***outputTrainDataToTextFile*** and ***outputTrainDataToBinaryFile***

---

## Testing

The script' own test suite partially meta-programs Ruby itself so to self-encapsulate the script, so to invoke it as a user would. When invoking the script in test mode, its test suite catches all `abort` calls made by the script (via `rescue ::SystemExit`). Moreover, it temporarily "hijacks" $stderr and $stdout and redirects them into its own strings (through `$stderr = ::StringIO.new`). This allows the test suite to invoke the script in a completely silent mode and harvest all the script's outputs and error messages. The test suite can then output its own error or success messages depending on the script's original results.

Testing the whole script is simply done as follows:

```
$ parseStocks.rb TEST
```

... and will wither output that all tests succeeded or that some failed. In that case, relevant output is provided to debug including the original $stderr and $strout.

New test cases are simply defined as follows and added to the `@@testSymbols` array. Method `testParseStocks`'s parameters are the test name and the wanted command line arguments. Method `testParseStocks` returns true if the script called `abort`, else false. **newTest** can be defined as follows:

```
  @@testSymbols.push(:newTest)
  def newTest
    # Script attempted to abort.
    if testParseStocks(__method__, ['Hello']) && @stderr.include?('FATAL ERROR! USAGE:')
      testPassed
    else
      testFailed
    end
  end
```

... and will output:

```
∙ newTest: Passed.
```

---