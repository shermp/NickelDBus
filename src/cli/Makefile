CROSS_COMPILE = arm-nickel-linux-gnueabihf-
MOC           = moc
CC            = $(CROSS_COMPILE)gcc
CXX           = $(CROSS_COMPILE)g++
PKG_CONFIG    = $(CROSS_COMPILE)pkg-config
STRIP         = $(CROSS_COMPILE)strip
AS            = $(CROSS_COMPILE)as
# NOTE: Point to the GCC wrappers for sane LTO handling.
#       But don't use LTO, it's terrible on GCC 4.9
AR            = $(CROSS_COMPILE)gcc-ar
RANLIB        = $(CROSS_COMPILE)gcc-ranlib
NM            = $(CROSS_COMPILE)gcc-nm
LD            = $(CROSS_COMPILE)ld
READELF       = $(CROSS_COMPILE)readelf
OBJCOPY       = $(CROSS_COMPILE)objcopy
OBJDUMP       = $(CROSS_COMPILE)objdump
STRINGS       = $(CROSS_COMPILE)strings

override nh_comma := ,

# pkgconf function
override define pkgconf =
 $(if $(filter-out undefined,$(origin $(strip $(1))_CFLAGS) $(origin $(strip $(1))_LIBS)) \
 ,$(info -- Using provided CFLAGS and LIBS for $(strip $(2))) \
 ,$(if $(shell $(PKG_CONFIG) --exists $(strip $(2)) >/dev/null 2>/dev/null && echo y) \
  ,$(info -- Found $(strip $(2)) ($(shell $(PKG_CONFIG) --modversion $(strip $(2)))) with pkg-config) \
   $(eval $(strip $(1))_CFLAGS := $(shell $(PKG_CONFIG) --silence-errors --cflags $(strip $(2)))) \
   $(eval $(strip $(1))_LIBS   := $(shell $(PKG_CONFIG) --silence-errors --libs $(strip $(2)))) \
   $(if $(strip $(3)) \
   ,$(if $(shell $(PKG_CONFIG) $(strip $(3)) $(strip $(2)) >/dev/null 2>/dev/null && echo y) \
    ,$(info .. Satisfies constraint $(strip $(3))) \
    ,$(info .. Does not satisfy constraint $(strip $(3))) \
     $(error Dependencies do not satisfy constraints)) \
   ,) \
  ,$(info -- Could not automatically detect $(strip $(2)) with pkg-config. Please specify $(strip $(1))_CFLAGS and/or $(strip $(1))_LIBS manually) \
   $(error Missing dependencies)))
endef

CFLAGS   ?= -O2 -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard -mthumb
CXXFLAGS ?= -O2 -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=hard -mthumb
LDFLAGS  ?= -Wl,--as-needed

override PKGCONF := Qt5Core Qt5DBus Qt5Widgets

# transform `pkg` into `PKG,pkg` in `$(PKGCONF)`
override PKGCONF := \
 $(foreach dep,$(PKGCONF) \
 ,$(strip $(if $(findstring $(nh_comma),$(dep)) \
  ,$(dep) \
  ,$(shell echo -n "$(dep)" | tr '[:lower:]' '[:upper:]')$(nh_comma)$(dep))))

# call pkgconf for each item in `$(PKGCONF)`
$(foreach dep,$(PKGCONF) \
,$(call pkgconf \
 ,$(word 1,$(subst $(nh_comma), ,$(dep))) \
 ,$(word 2,$(subst $(nh_comma), ,$(dep))) \
 ,$(word 3,$(subst $(nh_comma), ,$(dep)))))

# add flags
override CFLAGS   += $(foreach dep,$(PKGCONF),$($(word 1,$(subst $(nh_comma), ,$(dep)))_CFLAGS))
override CXXFLAGS += $(foreach dep,$(PKGCONF),$($(word 1,$(subst $(nh_comma), ,$(dep)))_CFLAGS))
override LDFLAGS  += $(foreach dep,$(PKGCONF),$($(word 1,$(subst $(nh_comma), ,$(dep)))_LIBS))

override CFLAGS   += -std=gnu11 -pthread
override CXXFLAGS += -std=gnu++11 -pthread
override LDFLAGS  += -Wl,--no-undefined -Wl,-rpath,/usr/local/Kobo -Wl,-rpath,/usr/local/Qt-5.2.1-arm/lib -pthread -ldl
override CFLAGS   += -Wall -Wextra -Werror
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers

override IFACE_DIR := ../interface
override BINARY   := qndb
# cli sources
override SOURCES  := qndb.cc nickel_dbus_cli.cc $(IFACE_DIR)/nickel_dbus_proxy.cpp

override MOCS 	  := $(IFACE_DIR)/nickel_dbus_proxy.h nickel_dbus_cli.h

override OBJECTS_C    := $(filter %.o,$(SOURCES:%.c=%.o))
override OBJECTS_CXX  := $(filter %.o,$(SOURCES:%.cc=%.o))
override OBJECTS_CXX1 := $(filter %.o,$(SOURCES:%.cpp=%.o))
override MOCS_MOC     := $(filter %.moc,$(MOCS:%.h=%.moc))
override OBJECTS_MOC  := $(MOCS_MOC:%=%.o)
override GENERATED    := $(BINARY) $(OBJECTS_C) $(OBJECTS_CXX) $(OBJECTS_CXX1) $(MOCS_MOC) $(OBJECTS_MOC)

$(BINARY): override CFLAGS   += -fPIC
$(BINARY): override CXXFLAGS += -fPIC

override nh_cmd_bin  = $(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $(1) $(2) $(LDFLAGS)
override nh_cmd_c    = $(CC) $(CPPFLAGS) $(CFLAGS) -c $(2) -o $(1)
override nh_cmd_cc   = $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(2) -o $(1)
override nh_cmd_moco = $(CXX) -xc++ $(CPPFLAGS) $(CXXFLAGS) -c $(2) -o $(1)
override nh_cmd_moch = $(MOC) $(2) -o $(1)


$(BINARY): $(OBJECTS_CXX) $(OBJECTS_CXX1) $(OBJECTS_MOC)
	$(call nh_cmd_bin,$@,$^)
$(OBJECTS_C): %.o: %.c
	$(call nh_cmd_c,$@,$^)
$(OBJECTS_CXX): %.o: %.cc
	$(call nh_cmd_cc,$@,$^)
$(OBJECTS_CXX1): %.o: %.cpp
	$(call nh_cmd_cc,$@,$^)
$(OBJECTS_MOC): %.moc.o: %.moc
	$(call nh_cmd_moco,$@,$^)
$(MOCS_MOC): %.moc: %.h
	$(call nh_cmd_moch,$@,$^)

.PHONY: all clean

all: $(BINARY)

clean:
	rm -f $(GENERATED)