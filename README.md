# simple-http-server 
A simple HTTP server written in C++ using boost.beast and c++20 corroutines TS

## How to build
```
$ git clone https://github.com/rjahanbakhshi/simple-http-server.git
$ cd simple-http-server
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## icecream build issue
For some reason, Hunter.pm doesn't work correctly with icecream. A local 
toolchain should be forced or Hunter.pm needs to be disabled.

to build with hunter.pm disabled

```
$ git clone https://github.com/rjahanbakhshi/simple-http-server.git
$ cd simple-http-server
$ mkdir build
$ cd build
$ cmake -DHUNTER_ENABLED=Off ..
$ make
```

to build using local toolchain

```
$ git clone https://github.com/rjahanbakhshi/simple-http-server.git
$ cd simple-http-server
$ mkdir build
$ cd build
$ CC=/usr/bin/gcc CXX=/usr/bin/g++ cmake ..
$ make
```

## Ubuntu 20.04 build
Ubuntu 20.04 is using GCC 9.4.0 which doesn't support coroutines TS which is 
required to build this project. clang compiler can be used instead which has
experimental support for coroutines TS. Here's an example of how to configure 
the project to use clang and libc++

```
$ sudo apt -y install clang
$ sudo apt -y install libc++-dev
$ sudo apt -y install libc++abi-dev
$ git clone https://github.com/rjahanbakhshi/simple-http-server.git
$ cd simple-http-server
$ mkdir build
$ cd build
$ CC=/usr/bin/clang CXX=/usr/bin/clang++ CXXFLAGS="-stdlib=libc++" cmake ..
$ make
```


## Installation
Run the following inside the build directory to install the binary into the prefix
directory

```
$ sudo make install
```

## Running
This will run the HTTP server at localhost:55555 using the working directory as
document root.

```
$ ./simple-http-server localhost 55555 ./
```

