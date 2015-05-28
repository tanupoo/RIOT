# name of your application
APPLICATION = ng_networking

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Specify the modules that are used
USEMODULE += ng_ipv6_default
USEMODULE += auto_init_ng_netif
USEMODULE += ng_nomac
USEMODULE += ng_netdev_eth
USEMODULE += ng_nativenet
USEMODULE += ng_icmpv6_echo
USEMODULE += ng_udp
USEMODULE += fib
USEMODULE += ng_pktdump
USEMODULE += uart0
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP -DTEST_SUITES

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
include $(RIOTBASE)/dist/testbed-support/Makefile.iot-lab
