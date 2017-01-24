# Generator #

## Requirements, building and running ##
You need the GNU scientific library in order to use the generator (https://www.gnu.org/software/gsl/).

* Ubuntu Linux: ```sudo apt-get install libgsl-dev```
* Mac OS X: ```brew install gsl```


**To build:**
```
cd generator;
make clean; make;
```

or more simply...
```
cc generator.c -o generator -lgsl
```

You can now run the following to see all available options:
```
./generator --help
```

## Examples ##
1. Insert 100000 keys, perform 1000 gets and 10 range queries and 20 deletes. The amount of misses of gets should be approximately 30% (--gets-misses-ratio) and 20% of the queries should be repeated (--gets-skewness).

```
./generator --puts 100000 --gets 1000 --ranges 10 --deletes 20 --gets-misses-ratio 0.3 --gets-skewness 0.2 
```

2. Same as above but store the data in external (.dat) binary files.
```
./generator --puts 100000 --gets 1000 --ranges 10 --deletes 20 --gets-misses-ratio 0.3 --gets-skewness 0.2 --external-puts 
```

3. Perform 100000 puts and issue 100 range queries (drawn from a gaussian distribution).
```
./generator --puts 100000 --ranges 100 --gaussian-ranges
```

4. Perform 100000 puts and issue 100 range queries (drawn from a uniform distribution).
```
./generator --puts 100000 --ranges 100 --uniform-ranges
```

*Note:* You can always set the random number generator seed using --seed XXXX