PKG_NAME=openwsn
PKG_URL=https://github.com/openwsn-berkeley/openwsn-fw.git
PKG_VERSION=8b1e96f48f29f0d9083be08c94797c890ca5ce08
PKG_DIR=$(CURDIR)/$(PKG_NAME)

ifneq ($(RIOTBOARD),)
include $(RIOTBOARD)/$(BOARD)/Makefile.include
endif

.PHONY: all clean patch reset

all: patch
	"$(MAKE)" -C $(PKG_DIR)

patch: $(PKG_DIR)/Makefile

$(PKG_DIR)/Makefile: $(PKG_DIR)/.git/config
	cd "$(PKG_DIR)" && git am --ignore-whitespace $(CURDIR)/*.patch

$(PKG_DIR)/.git/config:
	test -d "$(PKG_DIR)" || git clone "$(PKG_URL)" "$(PKG_DIR)"; \
		cd "$(PKG_DIR)" && git checkout -f "$(PKG_VERSION)"

clean::
	@echo "Cleaning up OpenWSN package..."
	@cd "$(PKG_DIR)" 2> /dev/null > /dev/null && \
		git clean -x -f && \
		git am --abort ; \
		git reset --hard "$(PKG_VERSION)" && \
		$(MAKE) patch || true

distclean::
	rm -rf "$(PKG_DIR)"

Makefile.include:
	@true
