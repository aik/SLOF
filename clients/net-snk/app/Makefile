# *****************************************************************************
# * Copyright (c) 2004, 2008 IBM Corporation
# * All rights reserved.
# * This program and the accompanying materials
# * are made available under the terms of the BSD License
# * which accompanies this distribution, and is available at
# * http://www.opensource.org/licenses/bsd-license.php
# *
# * Contributors:
# *     IBM Corporation - initial implementation
# ****************************************************************************/

ifndef TOP
TOP = $(shell while ! test -e make.rules; do cd ..  ; done; pwd)
export TOP
endif
include $(TOP)/make.rules

CFLAGS +=$(ADDCFLAGS)

OBJS = main.o
ifeq ($(SNK_LJTAG_PROCESS), 1)
OBJDIRS = ljtag/ljtag.o
CFLAGS += -DSNK_LJTAG_PROCESS
else
OBJDIRS = netlib/netlib.o netapps/netboot.o 
OBJDIRS += netapps/netflash.o
OBJDIRS += netapps/ping.o
OBJDIRS += netapps/args.o
ifeq ($(SNK_BIOSEMU_APPS), 1)
OBJDIRS += biosemu/biosemu_app.o
CFLAGS += -DSNK_BIOSEMU_APPS
endif
ifeq ($(SNK_GENMODULE_APPS), 1)
OBJDIRS += forth/forth.o snkshell/snkshell.o
CFLAGS += -DSNK_GENMODULE_APPS
endif
endif

SUBDIRS = $(dir $(OBJDIRS))

all: app.o

subdirs:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir DIRECTORY=$(DIRECTORY)$$dir || exit 1; \
	done

app.o: subdirs $(OBJS)
	$(LD) $(LDFLAGS) $(OBJDIRS) $(OBJS) -o $@ -r 

clean :	
	$(RM) -f *.o *.a *.i 
	for dir in $(SUBDIRS); do \
		$(CLEAN) ; \
		$(MAKE) -C $$dir DIRECTORY=$(DIRECTORY)$$dir clean; \
	done

include $(TOP)/make.depend
