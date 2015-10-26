APPLICATION = ccn-lite-relay

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

USEMODULE += shell_commands
USEMODULE += posix

USEPKG += ccn-lite

include $(RIOTBASE)/Makefile.include
