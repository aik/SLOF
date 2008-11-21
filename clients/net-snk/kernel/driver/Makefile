# *****************************************************************************
# * Copyright (c) 2004, 2007 IBM Corporation
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

OBJS = net_support.o  
OBJDIRS = net/net_driver.o 

all: net.o

net.o: subdirs $(OBJS)
		$(LD) $(LDFLAGS) $(OBJDIRS) $(OBJS) -o $@ -r

subdirs:
	for dir in $(dir $(OBJDIRS)); do \
		$(MAKE) -C $$dir DIRECTORY=$(DIRECTORY)$$dir || exit 1; \
	done

clean:		
		$(RM) -f *.o *.a *.i
		@for dir in $(dir $(OBJDIRS)); do \
			$(CLEAN); \
			$(MAKE) -C $$dir DIRECTORY=$(DIRECTORY)$$dir clean; \
		done

-include $(TOP)/make.depend
