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

CFLAGS += -I../

OBJS    = tftp.o netbase.o arp.o dns.o bootp.o dhcp.o icmp.o 

all: netlib.o

netlib.o: $(OBJS) 
		$(LD) $(LDFLAGS) $^ -o $@ -r

clean:	
		$(RM) -f *.o *.a *.i

include $(TOP)/make.depend
