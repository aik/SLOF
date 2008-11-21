#!/usr/bin/perl
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

sub one {
	open my $f, "<fs/@_" or die "Can't open @_: $!";
	while (<$f>) {
		if (/^INCLUDE (.*)/) {
			one($1);
		} else {
			print $_;
		}
	}
	close $f;
}

one $ARGV[0];
