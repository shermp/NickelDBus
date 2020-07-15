#!/bin/sh

conf_file=$1
iface_name=$2

tee "$conf_file" <<EOF
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
    <policy user="root">
        <allow own="${iface_name}"/>
        <allow send_interface="${iface_name}"/>
        <allow send_destination="${iface_name}"/>
    </policy>
    <policy context="default">
        <allow send_interface="${iface_name}"/>
        <allow send_destination="${iface_name}"/>
    </policy>
</busconfig>

EOF