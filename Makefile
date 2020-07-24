include NickelHook/NickelHook.mk

_ADAPTDIR := $(shell mkdir -p -m777 src/adapter)

override DBUS_IFACE_NAME := local.shermp.nickeldbus
override DBUS_IFACE_XML := src/adapter/$(DBUS_IFACE_NAME).xml

override LIBRARY  := libndb.so
# NDB sources
override SOURCES  += src/nickeldbus.cc src/nickel_dbus.cc src/adapter/nickel_dbus_adapter.cpp
# NM sources
override SOURCES  += NickelMenu/src/action.c NickelMenu/src/action_c.c NickelMenu/src/action_cc.cc NickelMenu/src/kfmon.c
override CFLAGS   += -Wall -Wextra -Werror
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers

override PKGCONF  += Qt5DBus Qt5Widgets

override MOCS 	  += src/nickel_dbus.h src/adapter/nickel_dbus_adapter.h
override ADAPTERS := src/adapter/nickel_dbus_adapter.h src/adapter/nickel_dbus_adapter.cpp

override GENERATED += $(ADAPTERS) $(DBUS_IFACE_XML)
override GITIGNORE += ndb-cli/ndb-cli

.PHONY: adapter
adapter: $(ADAPTERS)

$(DBUS_IFACE_XML): src/nickel_dbus.h
	qdbuscpp2xml -S -M -o $@ $<

$(ADAPTERS) &: $(DBUS_IFACE_XML)
	cd src/adapter && qdbusxml2cpp -c NickelDBusAdapter -a nickel_dbus_adapter $(<F)

include NickelHook/NickelHook.mk
