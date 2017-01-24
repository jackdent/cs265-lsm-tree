# Generator #

You need the GNU scientific library in order to use the generator (https://www.gnu.org/software/gsl/).

* Ubuntu Linux: ```sudo apt-get install libgsl-dev```
* Mac OS X: ```brew install gsl```


**To build:**
```
#!bash

cd generator;
make clean; make;
./generator --help

```