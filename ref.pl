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

#
# Copyright 2002,2003,2004  Segher Boessenkool  <segher@kernel.crashing.org>
#

use Data::Dumper;

$CELLSIZE = length(sprintf "%x", ~0) / 2;
$CELLSIZE = 8; # Hard code for cross-compiling to a different size ABI.

sub string
{
	my ($s, $extra) = @_;

	$s = sprintf "%s%c%s", $extra, length($s), $s;
	@s = ($s =~ /(.{1,$CELLSIZE})/gs);
	do { s/([\x00-\x1f\x22\x5c\x7f-\xff])/sprintf "\\%03o", ord $1/egs } for @s;
	my @reut = ("{ .c = \"" . (join "\" }, { .c = \"", @s) . "\" },", scalar @s);
	return @reut;
}

sub forth_to_c_name
{
	($_, my $numeric) = @_;
	s/([^a-zA-Z0-9])/sprintf("_X%02x_", ord($1))/ge;
	s/__/_/g;
	s/_$//;
	s/^(\d)/_$1/ if $numeric;
	return $_;
}

sub special_forth_to_c_name
{
	($_, my $numeric) = @_;

	my ($name, $arg) = (/^([^(]+)(.*)$/);
	if ($special{$name} == 1) {
		$_ = forth_to_c_name($name, $numeric) . $arg;
	} elsif ($special{$name} != 2) {
		$_ = forth_to_c_name($_, $numeric);
	}
	return $_;
}

$link = "0";
%special = ( _N => 2, _O => 2, _C => 2, _A => 2 );

while ($line = <>) {
	if ($line =~ /^([a-z]{3})\(([^ ]+)./) {
		$typ = $1;
		$name = $2;
		$name =~ s/\)$// if $line =~ /\)\s+_ADDING.*$/;
		$cname = forth_to_c_name($name, 1);
		$par = '';
		$add = '';
		$extra = "\0";
		if ($typ eq "imm") {
			$typ = "col";
			$extra = "\1";
		}
		($str, $strcells) = (string $name, $extra);
		if ($line =~ /^str\([^"]*"([^"]*)"/) {
			($s) = (string $1);
			$line =~ s/"[^"]*"/$s/;
		}
		if ($line =~ /_ADDING +(.*)$/) {
			$special{$name} = 1;
			@typ = (split /\s+/, $1);
			$count = 0;
			$par = "(" . (join ", ", map { $count++; "_x$count" } @typ) . ")";
			$count = 0;
			$add = join " ", map { $count++; "$_(_x$count)" } @typ;
			$line =~ s/\s+_ADDING.*$//;
		}
		($body) = ($line =~ /^...\((.*)\)$/);
		@body = split " ", $body;
		if ($typ ne "str" and $typ ne "con") {
			@body = map { special_forth_to_c_name($_, $typ eq "col") } @body;
		} else {
			$body[0] = special_forth_to_c_name($body[0]);
		}
		$body = join " ", @body;
		$body =~ s/ /, /;

		print "header($cname, { .a = $link }, $str) ";
		$link = "xt_$cname";
		print "$typ($body)\n";
		print "#define $cname$par ref($cname, $strcells+1) $add\n";
		(my $xxcname) = ($cname =~ /^_?(.*)/);
		$add and print "#define DO$xxcname ref($cname, $strcells+1)\n";
	} else {
		print $line;
	}
}
