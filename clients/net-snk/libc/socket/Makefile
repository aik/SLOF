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


CFLAGS = -g -I$(TOP)/include -O2 -fno-builtin -ffreestanding -msoft-float -Wall
LDFLAGS= -nostdlib


OBJS=send.c


all: socket.o


socket.o: $(OBJS:.c=.o)
	$(LD) $(LDFLAGS) -r $^ -o $@	

%.o : %.c
	$(CC) $(CFLAGS) -c $^ -o $@


clean:
	$(RM) -f *.o *.i *.s

distclean mrproper: clean
