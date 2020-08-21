include NickelHook/NickelHook.mk

override DBUS_IFACE_NAME := com.github.shermp.nickeldbus
override DBUS_IFACE_XML := src/interface/$(DBUS_IFACE_NAME).xml
override DBUS_IFACE_CFG := $(subst .,-,$(DBUS_IFACE_NAME)).conf
override IFACE_DIR := src/interface

override LIBRARY  := libndb.so
# NDB sources
override SOURCES  += src/ndb/nickeldbus.cc src/ndb/ndb.cc $(IFACE_DIR)/ndb_adapter.cpp 
# NM sources
override SOURCES  += NickelMenu/src/util.c NickelMenu/src/action.c NickelMenu/src/action_c.c NickelMenu/src/action_cc.cc NickelMenu/src/kfmon.c
override CFLAGS   += -Wall -Wextra -Werror
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers

override CFLAGS   += -DNDB_DBUS_IFACE_NAME='"$(DBUS_IFACE_NAME)"'
override CXXFLAGS += -DNDB_DBUS_IFACE_NAME='"$(DBUS_IFACE_NAME)"'

override PKGCONF  += Qt5DBus Qt5Widgets

override MOCS 	  += src/ndb/ndb.h $(IFACE_DIR)/ndb_adapter.h

override ADAPTER  := $(IFACE_DIR)/ndb_adapter.h
override PROXY    := $(IFACE_DIR)/ndb_proxy.h

override KOBOROOT += res/$(DBUS_IFACE_CFG):/etc/dbus-1/system.d/$(DBUS_IFACE_CFG)
override KOBOROOT += src/cli/qndb:/usr/bin/qndb

override GENERATED += $(ADAPTER) $(ADAPTER:h=cpp) $(PROXY) $(PROXY:h=cpp) $(DBUS_IFACE_XML)

override GITIGNORE += $(PROXY:h=moc) $(PROXY:h=o) $(PROXY:h=moc.o) doc/html

.PHONY: interface
interface: $(ADAPTER) $(PROXY)

.PHONY: dbuscfg
dbuscfg:
	script/make-dbus-conf.sh res/$(DBUS_IFACE_CFG) $(DBUS_IFACE_NAME)

.PHONY: cli clean-cli gitignore-cli doc

cli: interface
	cd src/cli && $(MAKE)

clean-cli:
	cd src/cli && $(MAKE) clean

gitignore-cli:
	cd src/cli && $(MAKE) gitignore

clean: clean-cli

gitignore: gitignore-cli

doc:
	cd doc/config && qdoc ndb.qdocconf

$(SOURCES): $(ADAPTER)

$(DBUS_IFACE_XML): src/ndb/ndb.h | $(IFACE_DIR)
	qdbuscpp2xml -S -M -o $@ $<

$(ADAPTER) &: $(DBUS_IFACE_XML)
	cd $(IFACE_DIR) && qdbusxml2cpp -c NDBAdapter -a ndb_adapter $(<F)

$(PROXY) &: $(DBUS_IFACE_XML)
	cd $(IFACE_DIR) && qdbusxml2cpp -c NDBProxy -p ndb_proxy $(<F)

$(IFACE_DIR):
	mkdir $@

include NickelHook/NickelHook.mk
