APPLICATION = test-sixlowpan
include ../Makefile.tests_common

USEMODULE += embunit
USEMODULE += ng_sixlowpan
USEMODULE += od

DISABLE_MODULE = auto_init

CFLAGS += -DTEST_SUITES

include $(RIOTBASE)/Makefile.include

test:
		./tests/01-run.py^
