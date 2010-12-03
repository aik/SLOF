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


CFLAGS = -g -I$(TOP)/include -I$(LIBCMNDIR)/libc/include -O2 \
         -fno-builtin -ffreestanding -msoft-float -Wall \
	 -fno-stack-protector
LDFLAGS= -nostdlib


OBJS = send.o


all: socket.o


socket.o: $(OBJS)
	$(LD) $(LDFLAGS) -r $< -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	$(RM) -f *.o *.i *.s

distclean mrproper: clean

include $(TOP)/make.depend
