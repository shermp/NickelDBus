include NickelHook/NickelHook.mk

override DBUS_IFACE_NAME := com.github.shermp.nickeldbus
override DBUS_IFACE_XML := src/adapter/$(DBUS_IFACE_NAME).xml
override DBUS_IFACE_CFG := $(subst .,-,$(DBUS_IFACE_NAME)).conf
override ADAPTER_DIR := src/adapter

override LIBRARY  := libndb.so
# NDB sources
override SOURCES  += src/nickeldbus.cc src/nickel_dbus.cc $(ADAPTER_DIR)/nickel_dbus_adapter.cpp
# NM sources
override SOURCES  += NickelMenu/src/util.c NickelMenu/src/action.c NickelMenu/src/action_c.c NickelMenu/src/action_cc.cc NickelMenu/src/kfmon.c
override CFLAGS   += -Wall -Wextra -Werror
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers

override CFLAGS   += -DNDB_DBUS_IFACE_NAME='"$(DBUS_IFACE_NAME)"'
override CXXFLAGS += -DNDB_DBUS_IFACE_NAME='"$(DBUS_IFACE_NAME)"'

override PKGCONF  += Qt5DBus Qt5Widgets

override MOCS 	  += src/nickel_dbus.h $(ADAPTER_DIR)/nickel_dbus_adapter.h

override ADAPTER := $(ADAPTER_DIR)/nickel_dbus_adapter.h

override KOBOROOT += res/$(DBUS_IFACE_CFG):/etc/dbus-1/system.d/$(DBUS_IFACE_CFG)
ifndef NDB_EXCLUDE_CLI
override KOBOROOT += ndb-cli/ndb-cli:/mnt/onboard/.adds/ndb/bin/ndb-cli
endif

override GENERATED += $(ADAPTER) $(ADAPTER:h=cpp) $(DBUS_IFACE_XML)
override GITIGNORE += ndb-cli/ndb-cli

.PHONY: adapter
adapter: $(ADAPTER)

.PHONY: dbuscfg
dbuscfg:
	script/make-dbus-conf.sh res/$(DBUS_IFACE_CFG) $(DBUS_IFACE_NAME)

$(SOURCES): $(ADAPTER)

$(DBUS_IFACE_XML): src/nickel_dbus.h | $(ADAPTER_DIR)
	qdbuscpp2xml -S -M -o $@ $<

$(ADAPTER) &: $(DBUS_IFACE_XML)
	cd src/adapter && qdbusxml2cpp -c NickelDBusAdapter -a nickel_dbus_adapter $(<F)

$(ADAPTER_DIR):
	mkdir $@

include NickelHook/NickelHook.mk
