# Channel
Implementation of the Bresenham-Murphy thick line drawing algorithm
in C++23.

This is a header-only library.  The configuration script is used to set up
the test harness and confirm that the necessary compiler features are
available.

## To build from a Git checkout:

    touch aclocal.m4 configure Makefile.am test/Makefile.am
    touch Makefile.in test/Makefile.in
    ./configure CXX='clang++' CXXFLAGS='-stdlib=libc++'
    make && make check
