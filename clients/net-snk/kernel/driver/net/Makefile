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

OBJS    = netmodule.o 

all:		net_driver.o

net_driver.o: $(OBJS)
		$(LD) $(LDFLAGS) $^ -o $@ -r

clean:		
		$(RM) -f *.o *.a *.i

include $(TOP)/make.depend

#depend :
#	@echo "create .depend file"
#	@touch .depend ; \
#	makedepend -v -f.depend -- $(CFLAGS) -- $(OBJS:.o=.c) 2> /dev/null 

#mrproper : clean
#	rm -rf .depend
