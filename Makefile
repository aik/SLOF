# =============================================================================
#  * Copyright (c) 2004, 2005 IBM Corporation
#  * All rights reserved. 
#  * This program and the accompanying materials 
#  * are made available under the terms of the BSD License 
#  * which accompanies this distribution, and is available at
#  * http://www.opensource.org/licenses/bsd-license.php
#  * 
#  * Contributors:
#  *     IBM Corporation - initial implementation
# =============================================================================


CROSS ?= powerpc64-linux-

ifeq ($(origin CC), default)
	CC = $(CROSS)gcc
endif
ifeq ($(origin LD), default)
	LD = $(CROSS)ld
endif
OBJCOPY ?= $(CROSS)objcopy

TARG = ppc64

CFLAGS = -g -DTARG=$(TARG) -static -Wall -W -std=gnu99 -O2 -fno-gcse \
         -fno-crossjumping -fomit-frame-pointer -mcpu=970 -msoft-float 
LDFLAGS = -g -static -nostdlib 

DICT = prim.in engine.in $(TARG).in

all: slof.img

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ && $(OBJCOPY) -R .note.GNU-stack $@

dict.xt: $(DICT) ref.pl
	cat $(DICT) | perl ref.pl > dict.xt

$(TARG).fs: fs/*.fs
	perl incs.pl main.fs > $@ || rm $@

$(TARG).o: $(TARG).fs
	$(LD) -o $@ -r -bbinary $<

payload-obj.o: payload
	$(LD) -o $@ -r -bbinary $<	

paflof.o: paflof.c prim.h prim.code $(TARG).h $(TARG).code dict.xt

paflof: paflof.lds paflof.o entry.o oco.o payload-obj.o $(TARG).o
	$(LD) $(LDFLAGS) -T $^ -o $@

slof: slof.lds startup.o loader.o paflof
	$(LD) -T $^ -o $@

slof.bin: slof
	$(OBJCOPY) $< $@ -O binary 	

slof.img: slof.bin
	./package-firmware -l $^ -o $@

clean:
	-rm -f paflof.o entry.o loader.o payload-obj.o $(TARG).fs $(TARG).o \
	dict.xt paflof slof slof.bin slof.img
