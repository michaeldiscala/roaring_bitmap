#!/usr/bin/env ruby
require "mkmf"

# Set the compiler to use C99, enable all warnings, and treat warnings
# as errors
$CFLAGS = "-std=c99 -Wall -Werror"

# Have the compiler also compile the roaring bitmap library itself and
# configure it so that the header is available to be included
$INCFLAGS << " -I$(srcdir)/libcroaring"
$VPATH ||= []
$VPATH << "$(srcdir)/libcroaring"
$srcs = Dir.glob("#{$srcdir}/{,libcroaring/}*.c").map {|n| File.basename(n) }

create_header
create_makefile "roaring_bitmap_ext"
