# name of your application
APPLICATION = ccn-lite-client

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# The CCN-lite API is currently expecting unsigned int to be at least 32 bits wide.
BOARD_BLACKLIST := chronos msb-430h telosb wsn430-v1_3b wsn430-v1_4 z1 redbee-econotag

# Modules to include:

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += uart0
USEMODULE += ps
USEMODULE += ng_netif_default
USEMODULE += auto_init_ng_netif
USEMODULE += random
USEMODULE += vtimer
USEMODULE += ccn_lite

include $(RIOTBASE)/Makefile.include
