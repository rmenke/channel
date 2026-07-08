# Channel
Implementation of the Bresenham-Murphy thick line drawing algorithm
in C++20.

This is a header-only library.  The configuration script is used to set up
the test harness and confirm that the necessary compiler features are
available.

There are likely more efficient implementations out there, but as the
Murphy paper is frustratingly opaque, this implementation serves as a
pedagogical example.

## Using the library

    #include "channel.hpp"

    std::pair p0{123,45};
    std::pair p1{-67,89};

    // Thin line example
    for (auto &&[x, y] : channel::views::line(p0, p1)) {
        // do something with the coordinates
    }

    // Thick line example
    for (auto &&stroke : channel::views::line(p0, p1, thickness)) {
        for (auto &&[x, y] : stroke) {
            // do something with the coordinates
        }
    }

The `channel::ranges::thick_line_view` and
`channel::ranges::thin_line_view` are views and may be treated as
such.  The object `channel::views::line` selects
the view class based on the arguments passed to the functor.

## To build from a Git checkout:

    touch aclocal.m4 configure Makefile.am test/Makefile.am
    touch Makefile.in test/Makefile.in
    ./configure CXX='clang++' CXXFLAGS='-stdlib=libc++'
    make && make check
