
SUBDIRS      = src support-files po
DIST_SUBDIRS = $(SUBDIRS) workspaces wslib1 wslib2 rpm debian

dist_doc_DATA = README			\
		README-1-prerequisites	\
		README-2-configure	\
		README-3-keyboard	\
		README-4-compliance	\
		README-5-WINDOWS	\
                README-6-porting	\
		README-7-more-info	\
		apl.1			\
		apl.html

nobase_dist_sysconf_DATA = gnu-apl.d/preferences

info_TEXINFOS = apl.texi

man1_MANS = apl.1

# apl.info in HTML format
#
apl.html:   apl.texi
	    makeinfo --html apl.texi -o $@ --no-split


# show make targets
#
.PHONY:	help
help:
	@echo ""
	@echo ""
	@echo "in addition to standard targets (all, install, clean, ...) "
	@echo "the following make targets may be supported (at this level):"
	@echo ""
	@echo "    make help               - print this text"
	@echo "    make DOXY               - create Dougen documentation"
	@echo "    make RPM                - create (source and binary) RPMs"
	@echo "    make DEB                - create debian packages"
	@echo ""
	@echo "NOTE: The RPM and DEB targets may fail because they require" 
	@echo "additional tools that may not be present on your machine."
	@echo ""

# doxygen documentation
#
.PHONY:	DOXY
DOXY:
	doxygen

# source and binary RPMs
#
.PHONY:	RPM
RPM:	dist
	rm -f rpm/rpm-tmp.*
	make -C rpm $@
	mkdir -p ftp-upload
	cp rpm/SRPMS/$(PACKAGE)-$(PACKAGE_VERSION)-0.src.rpm  ftp-upload/
	cp rpm/RPMS/*/$(PACKAGE)-$(PACKAGE_VERSION)-0.*.rpm   ftp-upload/

# source and binary Debian packages
#
.PHONY:	DEB
DEB:	dist
	rm -Rf debian_tmp
	mkdir debian_tmp
	cp $(distdir).tar.gz \
           debian_tmp/$(PACKAGE)_$(PACKAGE_VERSION).orig.tar.gz
	cd debian_tmp ; tar xf $(PACKAGE)_$(PACKAGE_VERSION).orig.tar.gz
	cp apl.1 debian_tmp/$(distdir)/debian/manpage.1
	cd debian_tmp/$(distdir) ; dpkg-buildpackage
	mkdir -p ftp-upload
	cp debian_tmp/$(PACKAGE)_$(PACKAGE_VERSION)-1_*.deb          ftp-upload/
	cp debian_tmp/$(PACKAGE)_$(PACKAGE_VERSION)-1.debian.tar.gz  ftp-upload/


# a shortcut to configure for troubleshooting
#
.PHONY:	develop
develop:
	./configure \
		VALUE_CHECK_WANTED=yes \
		VALUE_HISTORY_WANTED=yes \
		DYNAMIC_LOG_WANTED=yes \
		ASSERT_LEVEL_WANTED=2  \
		PRINT_SEMA_WANTED=yes

# a shortcut to export changes to ../savannah-repo-apl (which is supposed to be
# a checkout of the GNU APL repository ar Savannah
#
.PHONY:	SYNC
SYNC:	dist
	tar xvzf apl-$(VERSION).tar.gz
	cp -a apl-$(VERSION)/* ../savannah-repo-apl/trunk/
	rm -Rf apl-$(VERSION)
	cd ../savannah-repo-apl ; svn add . --force -q

# build all packages and SYNC
#
.PHONY:	EXPO
EXPO:
	tools/check_src
	svn -m "auto-commit by make EXPO" commit
	svn update
	cd src; source buildtag
	svn -m "auto-commit by make EXPO" commit
	./configure
	make
	make dist
	make DEB
	make RPM
	make SYNC

am--refresh:
	echo "making am--refresh"


ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = config.rpath m4/ChangeLog

