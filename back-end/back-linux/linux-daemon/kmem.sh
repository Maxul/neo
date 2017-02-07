#!/bin/sh

module="kmem"
device="dev_mem"
mode="664"

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
/sbin/insmod ./$module.ko $* || exit 1

# remove stale nodes
rm -f /dev/${device}

major=$(awk '$2 == "'$device'" {print $1}' /proc/devices)
mknod /dev/${device} c $major 0

# give appropriate group/permissions, and change the group.
# Not all distributions have staff, some have "wheel" instead.
#group="staff"
#grep -q '^staff:' /etc/group || group="wheel"
#chgrp $group /dev/${device}[0-3]
#chmod $mode /dev/${device}[0-3]
