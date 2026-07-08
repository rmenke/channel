include $(top_srcdir)/aminclude_static.am

TESTS = 01-channel.test 02-thick.test

TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
	$(top_srcdir)/build-aux/tap-driver.sh

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CXXFLAGS = -Wall -Wpedantic
AM_DEFAULT_SOURCE_EXT = .cpp

LIBS = @LIBS@

if CODE_COVERAGE_ENABLED

AM_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
AM_CXXFLAGS += $(CODE_COVERAGE_CXXFLAGS)

LIBS += $(CODE_COVERAGE_LIBS)

else !CODE_COVERAGE_ENABLED

code-coverage-clean: ;
code-coverage-dist-clean: ;

endif !CODE_COVERAGE_ENABLED

EXTRA_DIST = tap.hpp raster.hpp

AM_TEST_LOG_DRIVER_FLAGS = $(am__test_driver_flags_@AM_V@)
am__test_driver_flags_ = $(am__test_driver_flags_@AM_DEFAULT_V@)
am__test_driver_flags_0 =
am__test_driver_flags_1 = --comments

check_PROGRAMS = $(TESTS)

clean-local: code-coverage-clean
	$(RM) *.ppm

distclean-local: code-coverage-dist-clean
