# FDK-AAC

AAD_URL := git://github.com/mstorsjo/fdk-aac.git

#PKGS += fdk-aac
ifeq ($(call need_pkg,"fdk-aac"),)
PKGS_FOUND += fdk-aac
endif

fdk-aac:
	$(call download_git,$(AAC_URL))

AACCONF := $(HOSTCONF) --disable-shared --enable-static CPPFLAGS=-fPIC

.fdk-aac: fdk-aac
	cd $< && $(HOSTVARS) ./configure $(AACCONF)
	cd $< && $(MAKE) install
	touch $@
