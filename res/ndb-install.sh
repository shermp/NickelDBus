#!/bin/sh

# First thing we do is delete the udev rule that spawned us
rm /etc/udev/rules.d/89-ndb.rules

NDB_LIB_LOC=/usr/local/ndb/lib

cd $NDB_LIB_LOC

first_lib=true
for NDB_LIB in $(ls -vr libndb*); do
    # We're sorting in reverse 'version' order, so the first should be 
    # the latest version of NickelDBus
    if [ $first_lib = "true" ]; then
        # Create a symlink in the kobo 'imageformats' directory pointing to this library
        # The symlink is replaced if it already exists
        ln -sf $NDB_LIB_LOC/$NDB_LIB /usr/local/Kobo/imageformats/libndb.so
        first_lib=false
    else
        # Delete any older versions
        rm ./$NDB_LIB
    fi
done
