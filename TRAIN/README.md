[Back](../README.md)

# Train the Formatted Inputs

Please see the [C++ README](README_CPP.md) for everything specifically related to C++17, building and compiling.

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

## Introduction

As stated in [Getting Started](GETTING_STARTED.md), we now want to develop a Supervised Learning train and predict program that shall be called as follows to train:

```
$ train WEEK_1/EVENT_<date>.bin A
        WEEK_2/EVENT_<date>.bin B
        WEEK_3/EVENT_<date>.bin C
```

And as follows to predict:

```
$ predict WEEK_4_THURSDAY_NIGHT/EVENT_<date>.bin weights.bin
```

To build, compile, analyze and run any of the programs following, please see the [C++ README](README_CPP.md) in directory *TRAIN*.

## Utilities

A number of utility classes and static functions have been defined in include file ***Utilities.hpp***.

### Utility Procedures

* ***OpenInputBinaryFileNamed(fileName)*** opens a file in binary mode and returns a tuple containing the corresponding `std::ifstream` object, an error message on error and the file size.
* ***String(value ...)*** returns a `std::string` made of any number of values whose types are recognized by `std::ostringstream`.
* ***TypeNameOf(object)*** returns *object*'s class name in a `std::string`.

### Utility Classes

* ***Array*** is composed of a `std::vector` but which size can only be set once. Used to avoid checking sizes and overflows all the time.
* ***GoferThreadsPool*** is instantiated with a fixed number of threads (e.g. number of actual cores) that execute enqueued errands in order. Used to limit CPU usage if flooded with errands, and to control the proliferation of threads that may hurt CPU caching.
* ***Logger*** logs simultaneously to stdout and to a file.
* ***NoConstructAllocator*** is used to instantiate huge collections that absolutely do not need all their values to be zeroed. Used to save time and CPU cycles.
* ***RandomBoolean*** uses every bit of an expensive random integer to provide random booleans.
* ***Timer*** times to the microsecond and prints on any `std::basic_ostream`.

## Testing

* ***testUtilities.cpp*** tests all utility procedures and classes, and uses C++ testing framework [doctest](https://github.com/doctest/doctest). To run it:

```
$ ./run.sh testUtilities.cpp
```

* ***testCollectionsSpeeds.cpp*** tests various collections of various sizes and of types: `C-array`, `std::vector`, `Array`; as well as inside smart pointers. This is to validate that compilers will optimize away and that everything performs the same. To run it:

```
$ ./run.sh testCollectionsSpeeds.cpp
```

* ***testRandomsSpeeds.cpp*** tests three kings of random integer as well as validates that RandomBoolean is indeed substantially faster than just the random integer. To run it:

```
$ ./run.sh testRandomsSpeeds.cpp
```

## Train

File ***trainInputMatrices.cpp*** is the project's main C++17 file to be compiled. It *#includes* files ***SupervisedNetworksBases.hpp*** and ***NaiveSupervisedNetworks.hpp*** and contains only a `main()` function which first instantiates a `Logger`and then a `SupervisedNetworkTrainer` that is populated using the command line arguments. The `SupervisedNetworkTrainer` is then run.

**All** exceptions are caught and logged into the logger. Signals `SIGABRT`, `SIGINT` (Ctrl-C by the user) and `SIGTERM` are set up to asynchronously **stop** the `SupervisedNetworkTrainer`.

### Compile and Analyze

***trainInputMatrices.cpp*** can be compiled and analyzed simply by invoking build script `build.sh`[^1]:

[^1]: [C++ README](README_CPP.md).

```
$ ./build.sh trainInputMatrices.cpp
```

### Run it

```
$ ./trainInputMatrices

Usage: ./trainInputMatrices
       <maximum number of training cycles>
       <number of training threads, 0 for hardware threads ÷ 2>
       [ <desired matrix name>  <event file name>  ]+
       [ <weights file name> ]
```

## Patterns Used

### Strategy versus Template Method (NVI)

In the class hierarchy above, the [Strategy pattern](https://en.wikipedia.org/wiki/Str    ategy_pattern) shall be utilized as opposed to the [Template Method pattern](https://en.wikipedi    a.org/wiki/Template_method_pattern) or [Non-Virtual Interface pattern (NVI)](https://en.wikipedi    a.org/wiki/Non-virtual_interface_pattern), as one can assume that there shall be little invarian    t commonalities between a single layer networks and deeper learning networks.

## Supervised Networks Bases

File ***SupervisedNetworksBases.hpp*** contains the following classes:

* ***WeightsCrafter*** is the abstract base class for all the weights crafting classes. It holds all the ***weights*** and the crucially important *random integer* and *random boolean*. It also declares pure virtual functions `weightsImproved()` and `weightsDidNotImprove()` to be implemented by the concrete weights crafter subclasses.
* ***MatrixDigraph*** is the abstract base class for all the matrix digraph classes.
* ***SupervisedNetworkEvent*** builds a vector of *MatrixDigraphs* according to a provided file 'EVENT....bin' produced by script *FORMAT/parseStocks.rb*.
* ***SupervisedNetworkTrainer*** is the verbose class and logs every action and every progress. It first parses and validates the provided command line arguments, and then builds a vector of *SupervisedNetworkEvents*, a *WeightsCrafter* as well as a *GoferThreadsPool* accordingly. It then continuously applies the *WeightsCrafter*'s weights to all the *MatrixDigraphs* through the *SupervisedNetworkEvent*.

## Naïve Supervised Networks

File ***NaiveSupervisedNetworks.hpp*** contains:

* Concrete class ***GeometricWeightsCrafter*** that crudely randomizes weights in a geometric way, so to oscillate as much as possible between randomizing all to only one weight. Of course, it implements functions `weightsImproved()` and `weightsDidNotImprove()`.
* Concrete class ***LogarithmicMatrixDigraph*** that logarithmically deceases the number of inputs to a single sink output value. This is only a first approximation and is **far** from deep learning.