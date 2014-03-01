export PROJECT =test_pnet
include ../Makefile.tests_common

ifeq ($(BOARD),stm32f4discovery)
	include Makefile.$(BOARD)
endif

BOARD_BLACKLIST := msb-430 mbed_lpc1768 redbee-econotag
# msb-430:			see https://github.com/RIOT-OS/RIOT/issues/658
# mbed_lpc1768: 	see https://github.com/RIOT-OS/RIOT/issues/675
# redbee-econotag: 	not enough RAM

USEMODULE += auto_init
USEMODULE += posix
USEMODULE += pnet
USEMODULE += vtimer

ifeq ($(strip $(BOARD)),native)
	USEMODULE += nativenet
else ifeq ($(strip $(BOARD)),msba2)
	USEMODULE += cc110x_ng
endif

export INCLUDES += -I$(RIOTBASE)/sys/net/include \
				   -I$(RIOTBASE)/sys/posix/pnet/include \
				   -I$(RIOTBASE)/sys/posix/include

include $(RIOTBASE)/Makefile.include
