![Supervised Learning Logo](logoDigraph.png)

# Naïve Supervised Learning Network - Getting Started

See the [main README](README.md).

*All trademarks are the property of their respective owners.*

---

## Done and Working

### Clone the Repository

```console
$ git clone https://github.com/nic-ch/naive-supervised.git
$ cd naive-supervised
```

### Download Weekly Stocks Training Data

To use script ***FORMAT/downloadStocks.rb***, get your own KEY and create your own ***stock_list.csv*** file, please see **Obtain Stocks Training Data** in [README](README.md).

Download weekly stocks training data for any number of weeks:

```console
$ mkdir WEEK_<i>
$ cd WEEK_<i>
$ ../FORMAT/downloadStocks.rb KEY < stock_list.csv
```

### Parse, Normalize and Format the Weekly Training Data

Parse, normalize and format each downloaded weekly stocks training data using script ***FORMAT/parseStocks.rb***. To see its usage, invoke it without arguments:

```console
$ FORMAT/parseStocks.rb 
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

For a description of its the input and output files formats, please see [format README](FORMAT/README.md) in directory *FORMAT*.

### Example: Week 1

Let's take Week 1 as an example where stocks training data were downloaded for stocks A, B and C and where Monday happens on 2022-01-10, Thursday on 2022-01-13, and Friday on 2022-01-14.

We want at this point to produce ***training inputs*** on the trading that happened Monday to Thursday, 09:31 to 16:00. And then use as a ***prediction output*** what happened on Friday between 09:31 and 16:00, i.e. which stock gained the most on Friday.

### Parse Week 1

```console
$ cd WEEK_1
$ ../FORMAT/parseStocks.rb '2022-01-10' '09:31' \
                           '2022-01-13' '16:00' \
                           '2022-01-14 09:31' '2022-01-14 16:00'
                           *.csv
```

This generates an ***inputs event*** for Week 1 and produces the following four files:

* Log file **PARSE_*date*.log**, among other things, lists the stocks with the highest gains between '2022-01-14 09:31' and '2022-01-14 16:00'.

Plus three files that contain the whole parsing of **all** stocks (e.g. A(A.csv), B(B.csv), C(C.csv) ...) and where ***date*** is the date and time the script was invoked:

* **EVENT_*date*_brute.txt** lists all the stocks, their timestamps and their amounts with all the prices × 10 000.
* **EVENT_*date*_normalized.txt** lists this data normalized.
* **EVENT_*date*.bin** contains this normalized data in a binary format.

### The Following Weeks

It is crucial for the other weeks that we always train on exactly the **same** ***train periods*** so to always get the same number of training inputs. We must also always use the **same** ***gain periods*** for consistency. Therefore, for each and every week that we will download:

* `<begin train date>` will always be Monday (formatted 'YYYY-MM-DD').
* `<begin train time>` will always be '09:31.
* `<end train date>` will always be Thursday (formatted 'YYYY-MM-DD').
* `<end train time>` will always be '16:00.
* `<begin gain timestamp>` will always be Friday 09:31 (formatted 'YYYY-MM-DD HH:MM').
* `<end gain timestamp>` will always be Friday 16:00 (formatted 'YYYY-MM-DD HH:MM').

Again, please see [format README](FORMAT/README.md) in directory *FORMAT* for a description of all these files' formats.

---

## Yet To Do: Train and Predict

### Train

We now want to develop a Supervised Learning, train and predict program (documented in [train README](TRAIN/README.md)). We want this program to train for hours and hours according to the inputs event files produced above and each the corresponding stock that performed the best.

Assuming that when we parsed Week 1, stock A performed the best, and similarly stock B for Week 2 and stock C for Week 3:

```console
$ train WEEK_1/EVENT_<date>.bin A
        WEEK_2/EVENT_<date>.bin B
        WEEK_3/EVENT_<date>.bin C
```

This shall produce a ***weights.bin*** file that can ideally predict future gains.

We shall then download again on **Thursday night** following Week 3 and parse with a phony gain period (Thursday instead of Friday) as Friday did not happen yet:

```console
$ mkdir WEEK_4_THURSDAY_NIGHT
$ cd WEEK_4_THURSDAY_NIGHT
$ ../FORMAT/downloadStocks.rb KEY < stock_list.csv
$ ../FORMAT/parseStocks.rb '2022-01-10' '09:31' \
                           '2022-01-13' '16:00' \
                           '2022-01-13 09:31' '2022-01-13 16:00'
                           *.csv
```

### Predict

Still on Thursday night, we shall then try to predict what stock will gain most on Friday:

```console
$ predict WEEK_4_THURSDAY_NIGHT/EVENT_<date>.bin weights.bin
```

---