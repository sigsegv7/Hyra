#!/bin/perl
#
# Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of Hyra nor the names of its contributors may be used
#    to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

use strict;
use warnings;

#
# Fetch a single line from a file, returns eof when there
# are no more lines to fetch.
#
# Arg0: File handle
#
sub get_line {
    my $fhand = shift;
    return <$fhand> || die "Failed to read file...";
}

my $argc = 0+@ARGV;

if ($argc < 1) {
    die "Usage: $0 <filename>";
}

# Grab the file
my $clean = 1;
my $lineno = 1;
my $RS = undef;
my $file = shift;
open my $fhand, '<', $file || die "Failed to open $file: $^E";
my $tmp = get_line($fhand);

# Is there a newline at the start of the file?
if ($tmp =~ /^\n/) {
    print "Found redundant newline at start of file\n";
    $clean = 0;
}

while (!eof($fhand)) {
    my $cur = get_line($fhand);

    if ($cur =~ /^\n$/ && $tmp =~ /^\n$/) {
        print "Found redundant newline @ $lineno\n";
        $clean = 0;
    }

    ++$lineno;
    $tmp = $cur;
}

# What about at the end?
if ($tmp =~ /^\n/) {
    print "Found redundant newline at end of file\n";
    $clean = 0;
}

if ($clean) {
    print "File is clean\n"
}
