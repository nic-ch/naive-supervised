![Supervised Learning Logo](logoDigraph.png)

# Naïve Supervised Learning Network - Theory

A step-by-step tutorial is available in [Getting Started](GETTING_STARTED.md).

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

This project has three main goals that happen to be mutually compatible:

1. To implement my own understanding of rudimentary **supervised learning**.
1. To implement the actual training in **C++17** so to get to master the language's many features, characteristics and quirks.
1. To make the training as fast and optimized as possible.

The first step to achieve this is to select simple, straightforward and consistent ***training data*** that feature unambiguous ***outputs*** (desired outcomes). Weekly stock data from the [Nasdaq Stock Market](https://www.nasdaq.com/) shall thus be used as the initial training data for this project. Except if there is a holiday, these stocks are traded minimally every week, Monday to Friday, 9:30 to 16:00. 

***Data normalization*** will be done in [Ruby](https://www.ruby-lang.org) as development time and flexibility are vastly more important than normalization running time.

[GNU g++](https://gcc.gnu.org) and [LLVM clang++](https://clang.llvm.org) will be used to analyze, compile and optimize C++17.

### General Approach

The approach to eat that elephant, perhaps naïvely, will be to:

1. Obtain stocks training data.
1. Normalize and format the training data.
1. Train on the formatted training data.
1. Predict what stocks will gain the most on a weekly basis.

---

## Obtain Stocks Training Data

Thankfully, Nasdaq stocks data can be obtained from [Alpha Vantage](https://www.alphavantage.co). According to their Web site: "*Alpha Vantage provides enterprise-grade financial market data through a set of powerful and developer-friendly APIs.*" After obtaining an [API key](https://www.alphavantage.co/support) from them, script ***FORMAT/downloadStocks.rb*** can be invoked with said key as first argument and a redirected (`<`) list of desired stocks, e.g.:

```
$ FORMAT/downloadStocks.rb KEYKEYKEY < stock_list.csv
```

Where ***stock_list.csv*** could list for example the [Nasdaq-100](https://www.nasdaq.com/market-activity/quotes/nasdaq-ndx-index)'s stocks, and shall look like:

```
Symbol, Name,
A, A Corp.,
B, B Inc.,
C, C Ltd.,
```

This is quite a simple script that downloads one .csv file per stock provided (e.g. A.csv, B.csv, C.csv) according to hard-coded parameters. See the [Alpha Vantage API Documentation](https://www.alphavantage.co/documentation/) to customize it. Obviously, any other way of obtaining stocks training data formatted as below or any other kind of training data can be used.

### Training Data Format

Each *STOCK.csv* downloaded thus will look like:

```
timestamp,open,high,low,close,volume
2022-01-13 09:35:00,8.6636,8.7282,8.6300,8.6650,2429
2022-01-13 09:34:00,8.7600,8.7700,8.6450,8.6650,5191
2022-01-13 09:33:00,8.6992,8.8300,8.6992,8.7700,6304
2022-01-13 09:32:00,8.7100,8.7850,8.6650,8.6900,3455
2022-01-13 09:31:00,8.7300,8.7870,8.6565,8.7000,8211
```

**For the rest of this project, stocks training data need to be downloaded on each Saturday, so to have data at least for the corresponding week's Monday to Friday, 9:31 to 16:30.**

---

## Normalize and Format

Please see the [format README](FORMAT/README.md) in directory *FORMAT*.

---

## Train

Please see [train README](TRAIN/README.md) in directory *TRAIN* for the general methods and algorithms used for training.

Please see [C++ README](TRAIN/README_CPP.md) in directory *TRAIN* for everything related to C++17.

---

## Predict

An ultimate goal of this project is to download stocks data for several week periods, train on them according to their performance on a pre-determined gain time period, and then predict the best stock for the coming gain time period.

---