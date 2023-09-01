# `lmno` an Embedded Programming Language for C++

`lmno` (pronounced "elemenoh") is a programming language presented as a generic
C++ library. The core language itself is symbolic, following in the path of many
array programming languages such as APL, J, and BQN. The `lmno` language borrows
many symbols and concepts from APL and BQN, but has several important
differences.

# IMPORTANT!

This repository is not a supported project of the author! This started as a
personal experiment that exploded into something viable as a real tool set.

This GitHub repository does not accept issues or PRs, but you are encouraged to
look through, inspect, download, build, and experiment with the work yourself.
If you want to fork the project and continue it, you have my blessing and
eternal gratitude!

For more thorough (yet still very incomplete) documentation, the `docs`
directory contains a Sphinx project and rST files.


# Building

Before building anything, you will need to have
[Poetry](https://python-poetry.org) installed!

Once you have Poetry, you can run the Make targets in the root repo:

```shell
## Build the project and run the tests
$ make build
## Build the documentation
$ make docs
## Run a documentation server
$ make docs-server
```

## Note on compiler support

Currently, the only toolchains tested with this repository are GCC 11 and Clang
16 . This repository features very heavy use of template metaprogramming and
C++20 concepts. It is very likely that any compiler outside of those tested will
reject the code, either due to bugs in *your* compilers, or bugs in *my*
compilers that allowed malformed code to pass through.
