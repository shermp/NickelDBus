include NickelHook/NickelHook.mk

override DBUS_IFACE_NAME := com.github.shermp.nickeldbus
override DBUS_IFACE_XML := src/interface/$(DBUS_IFACE_NAME).xml
override DBUS_IFACE_CFG := $(subst .,-,$(DBUS_IFACE_NAME)).conf
override IFACE_DIR := src/interface

override LIBRARY  := libndb.so
# NDB sources
override SOURCES  += src/ndb/nickeldbus.cc src/ndb/NDBDbus.cc src/ndb/NDBCfmDlg.cc src/ndb/NDBWidgets.cc src/ndb/NDBVolContent.cc src/ndb/util.cc $(IFACE_DIR)/ndb_adapter.cpp  
# NM sources
override SOURCES  += NickelMenu/src/util.c NickelMenu/src/action.c NickelMenu/src/action_c.c NickelMenu/src/action_cc.cc NickelMenu/src/kfmon.c
override CFLAGS   += -Wall -Wextra -Werror
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers

override CFLAGS   += -DNDB_DBUS_IFACE_NAME='"$(DBUS_IFACE_NAME)"'
override CXXFLAGS += -DNDB_DBUS_IFACE_NAME='"$(DBUS_IFACE_NAME)"'

override PKGCONF  += Qt5DBus Qt5Widgets

override MOCS 	  += src/ndb/NDBDbus.h src/ndb/NDBCfmDlg.h src/ndb/NDBWidgets.h src/ndb/NDBVolContent.h $(IFACE_DIR)/ndb_adapter.h

override ADAPTER  := $(IFACE_DIR)/ndb_adapter.h
override PROXY    := $(IFACE_DIR)/ndb_proxy.h

override KOBOROOT += res/$(DBUS_IFACE_CFG):/etc/dbus-1/system.d/$(DBUS_IFACE_CFG)
override KOBOROOT += src/cli/qndb:/usr/bin/qndb
override KOBOROOT += $(UNINSTALL_FILE):/mnt/onboard/.adds/nickeldbus

override UNINSTALL_FILE := res/ndb_version

override GENERATED += $(ADAPTER) $(ADAPTER:h=cpp) $(PROXY) $(PROXY:h=cpp) $(DBUS_IFACE_XML) $(UNINSTALL_FILE)

override GITIGNORE += $(PROXY:h=moc) $(PROXY:h=o) $(PROXY:h=moc.o) qdoc/html/

.PHONY: debug cli clean-cli gitignore-cli doc internal-doc dbuscfg interface uninstall-file

interface: $(ADAPTER) $(PROXY)

dbuscfg:
	script/make-dbus-conf.sh res/$(DBUS_IFACE_CFG) $(DBUS_IFACE_NAME)

cli: interface
	cd src/cli && $(MAKE)

clean-cli:
	cd src/cli && $(MAKE) clean

gitignore-cli:
	cd src/cli && $(MAKE) gitignore

clean: clean-cli

gitignore: gitignore-cli

internal-doc:
	cd qdoc/config && qdoc NickelDBus.qdocconf
	sed -i 's/This function was introduced in  Qt/This function was introduced in NickelDBus/g' qdoc/html/ndb-ndbdbus.html
doc: internal-doc
	cd qdoc/html && \
	rm ndb.html && \
	rm ndb-ndbcfmdlg* && \
	rm ndb-ndbprogressbar*

uninstall-file:
	echo "$(VERSION)" > $(UNINSTALL_FILE)

all: uninstall-file

debug: CFLAGS += -DDEBUG
debug: CXXFLAGS += -DDEBUG
debug: all

$(SOURCES): $(ADAPTER)

$(DBUS_IFACE_XML): src/ndb/NDBDbus.h | $(IFACE_DIR)
	qdbuscpp2xml -S -M -o $@ $<

$(ADAPTER) : $(DBUS_IFACE_XML)
	cd $(IFACE_DIR) && qdbusxml2cpp -c NDBAdapter -a ndb_adapter $(<F)

$(PROXY) : $(DBUS_IFACE_XML)
	cd $(IFACE_DIR) && qdbusxml2cpp -c NDBProxy -p ndb_proxy $(<F)

$(IFACE_DIR):
	mkdir $@

include NickelHook/NickelHook.mk
