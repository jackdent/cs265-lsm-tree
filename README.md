# Harvard CS265 - Big Data Systems #
This repository contains the code of a workload generator for an LSM tree.
It follows the DSL specified for the systems project of CS265 (Spring 2017).


## Workload and Data Generator ##
### Dependencies ###
You need the GNU scientific library in order to use the generator (https://www.gnu.org/software/gsl/).

* Ubuntu Linux: ```sudo apt-get install libgsl-dev```
* Mac OS X: ```brew install gsl```
* Cygwin: install the *gsl, gsl-devel* packages

### Building ###
```
cd generator;
make clean; make;
```

or simply...
```
cc generator.c -o generator -lgsl
```

### Running ###
You can now run the following to see all available options:
```
./generator --help
```

![Screen Shot 2017-01-24 at 1.21.17 PM.png](https://bitbucket.org/repo/9de5E4/images/2315274117-Screen%20Shot%202017-01-24%20at%201.21.17%20PM.png)

### Examples ###
**Query 1:** Insert 100000 keys, perform 1000 gets and 10 range queries and 20 deletes. The amount of misses of gets should be approximately 30% (--gets-misses-ratio) and 20% of the queries should be repeated (--gets-skewness).

```
./generator --puts 100000 --gets 1000 --ranges 10 --deletes 20 --gets-misses-ratio 0.3 --gets-skewness 0.2 > workload.txt
```

**Query 2:** Same as above but store the data in external (.dat) binary files.
```
./generator --puts 100000 --gets 1000 --ranges 10 --deletes 20 --gets-misses-ratio 0.3 --gets-skewness 0.2 --external-puts > workload.txt
```

**Query 3:** Perform 100000 puts and issue 100 range queries (drawn from a gaussian distribution).
```
./generator --puts 100000 --ranges 100 --gaussian-ranges > workload.txt
```

**Query 4:** Perform 100000 puts and issue 100 range queries (drawn from a uniform distribution).
```
./generator --puts 100000 --ranges 100 --uniform-ranges > workload.txt
```

**Note: You can always set the random number generator seed using --seed XXXX**


## Evaluating Workload ##

You can execute a workload and see some basic statistics about it, using the python script:

```
python evaluate.py workload.txt
```

For extra options etc, please look inside the script.