#
#                              Copyright (c) 2012
#                            Lantiq Deutschland GmbH
#
#  For licensing information, see the file 'LICENSE' in the root folder of
#  this software module.
#


AUTOMAKE_OPTIONS = foreign 1.6 nostdinc
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

# set distcheck options - see configure.in for list of modifications
DISTCHECK_CONFIGURE_FLAGS=@CONFIGURE_OPTIONS@

tapidemo_docdir = ${prefix}/doc/tapidemo

tapidemo_doc_DATA = \
    README \
    AUTHORS \
    TODO \
    LICENSE \
    JUMPER


if SYS_EASY336
EXTRA_DIST = \
    LICENSE \
    JUMPER \
    CHANGELOG\
    doc/doxyconfig\
    doc/footer.html\
    doc/header.html\
    doc/logo.jpg\
    doc/stylesheet.css \
    script/prepare-vxworks-6-4.pl \
    script/prepare-vxworks-6-6.pl
else
EXTRA_DIST = \
    $(tapidemo_doc_DATA) \
    CHANGELOG \
    autogen.sh \
    bin/vin-cpe_pDRAMfw.c \
    bin/vin-cpe_pPRAMfw.c \
    bin/vin-cpe_bbd_fxs.c \
    bin/vin-vip_pDRAMfw.c \
    bin/vin-vip_pPRAMfw.c \
    bin/vin-vip_bbd_fxs.c \
    bin/danube_bbd_fxo.c \
    doc/doxyconfig\
    doc/footer.html\
    doc/header.html\
    doc/logo.jpg\
    doc/stylesheet.css\
    doc/html/logo.jpg \
    script/prepare-vxworks-6-4.pl \
    script/prepare-vxworks-6-6.pl
endif

VXWORKS_6_4_PRJFILES = \
	vxworks_6.4/.project \
	vxworks_6.4/.wrproject \
	vxworks_6.4/.wrmakefile
VXWORKS_6_6_PRJFILES = \
	vxworks_6.6/.cproject \
	vxworks_6.6/.project \
	vxworks_6.6/.wrproject \
	vxworks_6.6/.wrproject_xt \
	vxworks_6.6/.wrmakefile

EXTRA_DIST += $(VXWORKS_6_4_PRJFILES) $(VXWORKS_6_6_PRJFILES)

if VXWORKS_6_4_DIST
VXWORKS_PRJFILES = $(VXWORKS_6_4_PRJFILES)
else
VXWORKS_PRJFILES = $(VXWORKS_6_6_PRJFILES)
endif

# Copy all the spec files. Of course, only one is actually used.
dist-hook: $(VXWORKS_PRJFILES)
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done
	@for f in $(VXWORKS_PRJFILES) ; do \
		echo "Copying file $$f to folder $(distdir)" ; \
		cp -p @srcdir@/$$f $(distdir) ; \
	done

# Create self extracting linux distribution
distcheck-hook:
	@echo -n "Checking for the line ending style... "; \
	failure_list="`find $(distdir) -path '$(distdir)/doc' -prune -o -type f -exec grep -Pqsc $$'\r\n' {} \; -printf '\tDOS line ending style: %p\n'`"; \
	if test -z "$$failure_list"; then \
		echo "Ok"; \
	else \
		echo "Fail"; \
		echo "$$failure_list"; \
		exit -1; \
	fi
	makeself.sh --gzip --notemp $(distdir) tapidemo-${PACKAGE_VERSION}.sh \
	"${PACKAGE_NAME}"

distvob:
	cleartool mkact -nc distvob_tapidemo_${PACKAGE_VERSION}
	cleartool co -nc /var/vob/comacsd/comacsd_dist/ifx-voice-tapidemo
	cleartool mv `\ls /var/vob/comacsd/comacsd_dist/ifx-voice-tapidemo/*.tar.gz` /var/vob/comacsd/comacsd_dist/ifx-voice-tapidemo/tapidemo-${PACKAGE_VERSION}.tar.gz
	cleartool ci -nc /var/vob/comacsd/comacsd_dist/ifx-voice-tapidemo
	cleartool co -nc /var/vob/comacsd/comacsd_dist/ifx-voice-tapidemo/tapidemo-${PACKAGE_VERSION}.tar.gz
	cp tapidemo-${PACKAGE_VERSION}.tar.gz /var/vob/comacsd/comacsd_dist/ifx-voice-tapidemo
	cleartool ci -nc /var/vob/comacsd/comacsd_dist/ifx-voice-tapidemo/tapidemo-${PACKAGE_VERSION}.tar.gz

lint:
	@target=lint; \
    list='$(SUBDIRS)'; \
    for subdir in $$list; do \
        echo "Making $$target in $$subdir"; \
        if test "$$subdir" = "."; then \
            dot_seen=yes; \
            local_target="$$target-am"; \
        else \
            local_target="$$target"; \
        fi; \
        (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
        || eval $$failcom; \
    done;

rsm:
	@target=rsm; \
    list='$(SUBDIRS)'; \
    for subdir in $$list; do \
        echo "Making $$target in $$subdir"; \
        if test "$$subdir" = "."; then \
            dot_seen=yes; \
            local_target="$$target-am"; \
        else \
            local_target="$$target"; \
        fi; \
        (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
        || eval $$failcom; \
    done;

style:
	find @top_srcdir@/src -type f -name '*.[hc]' -exec @top_srcdir@/script/checkStyle.pl {} \;
