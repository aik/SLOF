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

include ../../make.rules

CPPFLAGS	= -I$(INCLBRDDIR) -I$(INCLCMNDIR) -I$(INCLCMNDIR)/$(CPUARCH) \
		  -I$(LIBCMNDIR)/libc/include
CFLAGS		+= -fno-builtin $(CPPFLAGS) -O2  -msoft-float $(MAMBO) $(PLATFORM)
CFLAGS		+= $(BOOT) $(IOCONF) -Wa,-mregnames $(RELEASE) $(CPUARCHDEF) -Wall
ASFLAGS         = $(PLATFORM) $(BOOT) $(IOCONF) $(RELEASE)$(CPUARCHDEF)  -Wa,-mregnames
LDFLAGS1	 = -nostdlib -e__start -Tstage2.lds -N -Ttext=0x100


STG1OBJ		 = startup.o boot_abort.o romfs.o hw.o io_generic.o board_io.o 
#STG1OBJ	 +=  monitor.o ipmi_kcs.o mmu.o
STG1OBJ		 += stage2_head.o stage2.o comlib.o  \
		    romfs_wrap.o nvramlog.o

all	: stage1.js2x stage1.bimini stageS.bin

stage1.js2x:	$(STG1OBJ) Cboot.o u4maui.o $(LIBCMNDIR)/libc.a
		$(LD) $(LDFLAGS1) -o stage1.elf  $(STG1OBJ) u4maui.o  $(LIBCMNDIR)/libc.a
		$(OBJCOPY) -O binary stage1.elf stage1.js2x

stage1.bimini:	$(STG1OBJ) Cboot.o u4bimini.o $(LIBCMNDIR)/libc.a
		$(LD) $(LDFLAGS1) -o stage1.elf  $(STG1OBJ) u4bimini.o  $(LIBCMNDIR)/libc.a
		$(OBJCOPY) -O binary stage1.elf stage1.bimini

stageS.bin:	stage_s.o
		$(LD) -nostdlib -N -Tstage_s.lds -o stage_s.elf stage_s.o
		$(OBJCOPY) -O binary stage_s.elf stageS.bin

u4bimini.o:	u4mem.c
		$(CC) $(CFLAGS) -DBOARD_BIMINI  -o u4bimini.o -c u4mem.c

u4maui.o:	u4mem.c
		$(CC) $(CFLAGS) -DBOARD_MAUI -o u4maui.o -c u4mem.c

romfs.o:	../../llfw/romfs.S
		$(CC) $(CFLAGS) -c ../../llfw/romfs.S

ipmi_kcs.o:	../rtas/ipmi_kcs.c
		$(CC) $(CFLAGS) -c ../rtas/ipmi_kcs.c

boot_abort.o:	../../llfw/boot_abort.S
		$(CC) $(CFLAGS) -c ../../llfw/boot_abort.S

nvramlog.o:	../../llfw/nvramlog.S
		$(CC) $(CFLAGS) -c ../../llfw/nvramlog.S

include $(LLFWCMNDIR)/clib/Makefile.inc

include $(LLFWCMNDIR)/io_generic/Makefile.inc

romfs_wrap.o:	../../llfw/romfs_wrap.c
		$(CC) $(CFLAGS) -c ../../llfw/romfs_wrap.c
Cboot.o: Cboot.S
		$(CC) $(CFLAGS) -c $^
		$(OBJCOPY) -O binary Cboot.o Cboot.bin

$(LIBCMNDIR)/libc.a:
	$(MAKE) -C $(LIBCMNDIR) libc

%.o: %.S
		$(CC) $(CFLAGS) -c $^

clean:
		rm -f *.o *.bin *.elf *.js2x *.bimini
