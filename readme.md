# miniCC

mini C Compile

## Environment

- GNU Linux Release: Ubuntu 20.04, kernel version 5.15.0-76-generic
- GCC version 9.4.0
- GNU Flex version 2.6.4
- GNU Bison version 3.5.1
- SPIM version 8.0

## Usage

### Generate miniCC

```shell
cd src/
make
cd ../
```
### Run

```shell
./miniCC test/test1.c [out1.s]
```
### Test

**Command Line**

Before testing, make sure you've installed SPIM Simulator : `sudo apt-get install spim`

```shell
spim -file out1.s
```
**GUI**
Install QtSPIM, and run out1.s

### Clean

```shell
rm miniCC
rm *.s
cd src/
make clean
```

