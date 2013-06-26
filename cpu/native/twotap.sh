#!/bin/sh

COMMAND=${1}

if [ -z "${USER}" ]; then
    echo 'need to export $USER'
    exit 1
fi
if [ -z "${COMMAND}" ]; then
    echo "usage: $(basename $0) <create|delete>"
    exit 1
fi

if [ "${COMMAND}" = 'create' ]; then
    sudo ip tuntap add dev tap0 mode tap user ${USER} || exit 1
    sudo ip tuntap add dev tap1 mode tap user ${USER} || exit 1
    echo 1 | sudo dd status=none of=/proc/sys/net/ipv6/conf/tap0/disable_ipv6 || exit 1
    echo 1 | sudo dd status=none of=/proc/sys/net/ipv6/conf/tap1/disable_ipv6 || exit 1
    sudo brctl addbr tapbr0 || exit 1
    echo 1 | sudo dd status=none of=/proc/sys/net/ipv6/conf/tapbr0/disable_ipv6 || exit 1
    sudo brctl addif tapbr0 tap1 || exit 1
    sudo brctl addif tapbr0 tap0 || exit 1
    sudo ip link set tapbr0 up || exit 1
    sudo ip link set tap0 up || exit 1
    sudo ip link set tap1 up || exit 1
elif [ "${COMMAND}" = 'delete' ]; then
    sudo ip link delete tap0
    sudo ip link delete tap1
    sudo ip link set tapbr0 down
    sudo brctl delbr tapbr0
else
    echo 'unknown command'
    exit 1
fi
exit 0
