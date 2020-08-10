include NickelHook/NickelHook.mk

override DBUS_IFACE_NAME := com.github.shermp.nickeldbus
override DBUS_IFACE_XML := src/interface/$(DBUS_IFACE_NAME).xml
override DBUS_IFACE_CFG := $(subst .,-,$(DBUS_IFACE_NAME)).conf
override IFACE_DIR := src/interface

override LIBRARY  := libndb.so
# NDB sources
override SOURCES  += src/nickeldbus.cc src/nickel_dbus.cc $(IFACE_DIR)/nickel_dbus_adapter.cpp 
# NM sources
override SOURCES  += NickelMenu/src/util.c NickelMenu/src/action.c NickelMenu/src/action_c.c NickelMenu/src/action_cc.cc NickelMenu/src/kfmon.c
override CFLAGS   += -Wall -Wextra -Werror
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers

override CFLAGS   += -DNDB_DBUS_IFACE_NAME='"$(DBUS_IFACE_NAME)"'
override CXXFLAGS += -DNDB_DBUS_IFACE_NAME='"$(DBUS_IFACE_NAME)"'

override PKGCONF  += Qt5DBus Qt5Widgets

override MOCS 	  += src/nickel_dbus.h $(IFACE_DIR)/nickel_dbus_adapter.h

override ADAPTER  := $(IFACE_DIR)/nickel_dbus_adapter.h
override PROXY    := $(IFACE_DIR)/nickel_dbus_proxy.h

override KOBOROOT += res/$(DBUS_IFACE_CFG):/etc/dbus-1/system.d/$(DBUS_IFACE_CFG)
override KOBOROOT += src/cli/qndb:/mnt/onboard/.adds/ndb/bin/qndb

override GENERATED += $(ADAPTER) $(ADAPTER:h=cpp) $(PROXY) $(PROXY:h=cpp) $(DBUS_IFACE_XML)

.PHONY: interface
interface: $(ADAPTER) $(PROXY)

.PHONY: dbuscfg
dbuscfg:
	script/make-dbus-conf.sh res/$(DBUS_IFACE_CFG) $(DBUS_IFACE_NAME)

.PHONY: cli
cli: interface
	cd src/cli && $(MAKE)

,PHONY: clean-cli
clean-cli:
	cd src/cli && $(MAKE) clean

clean: clean-cli

$(SOURCES): $(ADAPTER)

$(DBUS_IFACE_XML): src/nickel_dbus.h | $(IFACE_DIR)
	qdbuscpp2xml -S -M -o $@ $<

$(ADAPTER) &: $(DBUS_IFACE_XML)
	cd $(IFACE_DIR) && qdbusxml2cpp -c NickelDBusAdapter -a nickel_dbus_adapter $(<F)

$(PROXY) &: $(DBUS_IFACE_XML)
	cd $(IFACE_DIR) && qdbusxml2cpp -c NickelDBusProxy -p nickel_dbus_proxy $(<F)

$(IFACE_DIR):
	mkdir $@

include NickelHook/NickelHook.mk
