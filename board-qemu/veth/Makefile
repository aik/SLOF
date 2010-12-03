# *****************************************************************************
# * Copyright (c) 2004, 2011 IBM Corporation
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

CFLAGS = -O2 -I./ -I$(TOP)/clients/net-snk/include/ -I$(TOP)/lib/libc/include/
CFLAGS += -I$(TOP)/lib/libhvcall
CFLAGS += -fno-builtin -ffreestanding -fno-stack-protector -msoft-float -nostdinc -Wall

SRCS   = module_entry.c veth.c

OBJS   = $(SRCS:.c=.o) $(TOP)/lib/libhvcall/hvcall.o

all: Makefile.dep net_veth.bin

net_veth.o: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@ -T veth.lds -N

net_veth.bin: net_veth.o
	$(OBJCOPY) -O binary $^ $@ 
clean:		
	$(RM) -f *.o *.a *.i *.bin

distclean : clean
	rm -f Makefile.dep


# Rules for creating the dependency file:
depend:
		$(CC) -MM $(CFLAGS) $(SRCS) > Makefile.dep
Makefile.dep:
		$(MAKE) depend

# Include dependency file if available:
ifneq (,$(wildcard Makefile.dep))
include Makefile.dep
endif
