# Kea
Programming language inspired by C and Rust

## How to run
```
$ git clone https://github.com/cibtr/kea.git --recurse-submodules && cd kea
$ make deps all
$ ./bin/keac [file]
```
## Makefile settings
For release, uncomment the `CFLAGS += -O3` line and comment out the `CFLAGS += -O0 -ggdb` line, then do `make clean all`.
For debug, do the opposite.
