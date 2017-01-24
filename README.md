# Generator #

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