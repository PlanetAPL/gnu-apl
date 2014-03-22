
TAR_FILENAME = $(PACKAGE)-$(VERSION).tar.gz
RPM_DIRS = BUILD BUILDROOT SOURCES RPMS SPECS SRPMS
RPM_ARGS = -ba SPECS/apl.spec --buildroot $(abs_top_builddir)/rpm/BUILDROOT

# work around a problem that sets -march=i386 on my box
if ON_i686
RPM_i686 = --target i486
else
RPM_i686 =
endif

.PHONY: RPM
RPM:
	rm -Rf $(RPM_DIRS)
	mkdir  $(RPM_DIRS)
	cp apl.spec SPECS/
	cp ../$(TAR_FILENAME) SOURCES/
	rpmbuild $(RPM_ARGS) $(RPM_i686)

clean-local:
	rm -Rf BUILD BUILDROOT SOURCES RPMS SPECS SRPMS

