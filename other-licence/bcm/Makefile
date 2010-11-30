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

CFLAGS = -O2 -I. -I../common -I$(TOP)/clients/net-snk/include -I$(TOP)/lib/libc/include -fno-builtin -ffreestanding -msoft-float -Wall -nostdinc

SRCS   = bcm57xx.c

COMMONOBJS = ../common/module_entry.o

OBJS   += $(COMMONOBJS) $(SRCS:.c=.o)


all:		Makefile.dep net_bcm57xx.bin

bcm57xx_net.o:  $(OBJS) 
		$(LD) $(LDFLAGS) $^ -o $@ -T ../common/module.lds -N

net_bcm57xx.bin: bcm57xx_net.o
		$(OBJCOPY) -O binary $^ $@ 

# A rule for making the object files in the common directory:
../common/%.o: ../common/%.c
	$(MAKE) -C ../common all


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
