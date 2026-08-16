include $(top_srcdir)/aminclude_static.am

ACLOCAL_AMFLAGS = -I m4 --install

## Force top-level build before subdirectory builds
SUBDIRS = . test

EXTRA_DIST = git-semver

AM_CPPFLAGS =
AM_CXXFLAGS =

LIBS = @LIBS@

include_HEADERS = include/channel.hpp

if CODE_COVERAGE_ENABLED

CODE_COVERAGE_IGNORE_PATTERN = "*/usr/include/*" "$(abs_srcdir)/test/*"

AM_CPPFLAGS += $(CODE_COVERAGE_CPPFLAGS)
AM_CXXFLAGS += $(CODE_COVERAGE_CXXFLAGS)

LIBS += $(CODE_COVERAGE_LIBS)

else !CODE_COVERAGE_ENABLED

code-coverage-clean:
code-coverage-dist-clean:

endif !CODE_COVERAGE_ENABLED

git-add:
	$(MAKE) distdir
	@(cd "$(distdir)"; find . -depth -type f -print0) | (cd "$(top_srcdir)"; xargs -0r git ls-files -z --other --modified --exclude-standard | xargs -0rt git add --verbose)
	@(cd "$(top_srcdir)"; find . -name .gitignore -print0 | xargs -0r git ls-files -z --other --modified --exclude-standard | xargs -0rt git add --verbose)
	$(RM) -r "$(distdir)"

doc: Doxyfile $(include_HEADERS)
	doxygen "$(abs_top_builddir)/Doxyfile"

git-doc: doc
	@set -x; TMPDIR="`mktemp -d`"; trap "$(RM) -r $${TMPDIR}" 0 1 2 15; cp -r "$(top_builddir)/doc" "$${TMPDIR}/doc"; cd "$(top_srcdir)"; git checkout gh-pages && $(RM) -r doc && cp -r "$${TMPDIR}/doc" doc && git add doc

git-tag:
	cd "$(top_srcdir)"; git tag -f -a "v$(PACKAGE_VERSION)" -m "v$(PACKAGE_VERSION)"

clean-local: code-coverage-clean
	@if $(AM_V_P); then XF='-0t'; else XF='-0'; fi; \
	echo "Removing backup files..."; \
	find "$(abs_top_srcdir)" "$(abs_top_builddir)" \( -name '*~' -o -name '*.bak' \) -print0 | xargs $$XF $(RM)

distclean-local: code-coverage-dist-clean
