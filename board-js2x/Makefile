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

JS2X_TARGETS = tools_build romfs_build stage1

SUBDIRS = slof rtas 
COMMON_LIBS = libc libipmi libbootmsg libbases libelf

all:		$(JS2X_TARGETS) subdirs clients_build bcm57xx boot_rom_js2x.bin boot_rom_bimini.bin

.PHONY : subdirs $(SUBDIRS) clean distclean

include config
include Makefile.dirs
include $(TOPCMNDIR)/make.rules
include $(TOPCMNDIR)/Makefile.gen

subdirs: $(SUBDIRS)

$(SUBDIRS): libraries
		 @echo " ====== Building $@ ======"
		 $(MAKE) -C $@ $(MAKEARG)


libraries:
		 $(MAKE) -C $(LIBCMNDIR) $(COMMON_LIBS)

stage1:	
		make -C llfw RELEASE=-DRELEASE=\"\\\"$(RELEASE)\\\"\"

boot_rom_js2x.bin: 	$(JS2X_TARGETS) subdirs clients_build bcm57xx ../build_info.img
		@echo " ====== Building $@ ======"
		cat $(ROMFSBRDDIR)/boot_rom.ffs > ../.boot_rom.ffs
		cat $(SLOFBRDDIR)/OF.ffs >> ../.boot_rom.ffs
		@echo build_info.img  build_info.img 0 0 >> ../.boot_rom.ffs
		cd .. && ./romfs/tools/build_romfs .boot_rom.ffs boot_rom.bin
		cd .. && if [ -f boot_rom.bin.gz ]; then rm -f boot_rom.bin.gz; gzip -9 boot_rom.bin; fi
		rm -f ../.boot_rom.ffs

boot_rom_bimini.bin: 	$(JS2X_TARGETS) subdirs clients_build bcm57xx ../build_info.img
		@echo " ====== Building $@ ======"
		cat $(ROMFSBRDDIR)/boot_rom_bimini.ffs > ../.boot_rom.ffs
		cat $(SLOFBRDDIR)/OF.ffs >> ../.boot_rom.ffs
		@echo build_info.img  build_info.img 0 0 >> ../.boot_rom.ffs
		cd .. && ./romfs/tools/build_romfs .boot_rom.ffs boot_rom_bimini.bin
		cd .. && if [ -f boot_rom_bimini.bin.gz ]; then rm -f boot_romi_bimini.bin.gz; gzip -9 boot_rom_bimini.bin; fi
		rm -f ../.boot_rom.ffs

bcm57xx:
		make -C ../other-licence/bcm


clean_here:
		rm -f package-firmware
		rm -f ../slof/OF.ffs
		rm -f ../boot_rom.bin ../js2*.img ../ram.bin
		rm -f ../bimini.img ../bimini.ram.bin
		rm -rf ../boot_rom_bimini.bin

clean:		clean_here clean_gen
		make -C ../other-licence/bcm clean
		@for dir in $(SUBDIRS); do \
			$(MAKE) -C $$dir clean || exit 1; \
		done
		rm -f ../boot_rom.bin ../js2*.img 
		@make -C llfw clean
		@make -C $(TOPCMNDIR)/clients/takeover clean

distclean:	clean_here distclean_gen
		make -C ../other-licence/bcm clean
		@for dir in $(SUBDIRS); do \
			$(MAKE) -C $$dir distclean || exit 1; \
		done
		rm -f ../boot_rom.bin ../js2*.img 
		make -C llfw clean
		make -C $(TOPCMNDIR)/clients/takeover distclean

takeover: all
		$(MAKE) -C $(TOPCMNDIR)/clients/takeover

.driver_dirs:
		@rm -rf ../driver-$(RELEASE)
		@mkdir -p ../driver-$(RELEASE)

.tar_gz:	.driver_dirs takeover
		@mv ../boot_rom.bin \
			../driver-$(RELEASE)/$(RELEASE)-js2x.bin
		@mv ../boot_rom_bimini.bin \
			../driver-$(RELEASE)/$(RELEASE)-bimini.bin
		@mv $(TOPCMNDIR)/clients/takeover/takeover.elf \
			../driver-$(RELEASE)/$(RELEASE)-takeover.bin
		@cp ../VERSION ../driver-$(RELEASE)
		@cp changes.txt ../driver-$(RELEASE)
		@cd ../driver-$(RELEASE) && md5sum * > md5sum.txt
		@chmod 644 ../driver-$(RELEASE)/*
		@mv ../driver-$(RELEASE) ../driver-$(RELEASE)-`date +%Y-%h%d`
		@tar czf ../driver-$(RELEASE)-`date +%Y-%h%d`.tar.gz \
			../driver-$(RELEASE)-`date +%Y-%h%d` > /dev/null  2>&1
		@rm -rf ../driver-$(RELEASE)-`date +%Y-%h%d`

driver:		driver_prep clean .tar_gz
