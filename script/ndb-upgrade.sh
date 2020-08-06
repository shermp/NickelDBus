#!/bin/sh

# Returns 1 if NickelDBus exists and satisfies requested version, or 0 otherwise
ndb_is_upgrade_required()
{
    version=$1
    vers_ret=$(dbus-send --system --print-reply --dest=com.github.shermp.nickeldbus /nickeldbus com.github.shermp.nickeldbus.ndbSatisfiesVersion string:"$version")
    if [ $? -eq 0 ] && $(echo "$vers_ret" | grep -q 'boolean true') ; then
        return 1
    fi
    return 0
}

# Copies provided KoboRoot file to '/mnt/onboard/.kobo/KoboRoot.tgz'. Returns 0 on success
ndb_do_upgrade()
{
    koboroot=$1
    if [ -f "$koboroot" ] && [ -d /mnt/onboard/.kobo ] ; then
        cp "$koboroot" /mnt/onboard/.kobo/KoboRoot.tgz
        return 0
    fi
    return 1
}

while getopts 'c:u:' c
do
    case $c in 
        c) CHECK_VERS=$OPTARG ;;
        u) NDB_KR=$OPTARG ;;
    esac
done

if [ "$CHECK_VERS" ] ; then
    ndb_is_upgrade_required "$CHECK_VERS"
    exit $?
fi
if [ "$NDB_KR" ] ; then
    ndb_do_upgrade "$NDB_KR"
    exit $?
fi
exit 2