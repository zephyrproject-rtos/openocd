#  Copyright (C) 2014-2015 Synopsys, Inc.
#  Anton Kolesov <anton.kolesov@synopsys.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the
#  Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

add_help_text arc_test_consecutive_memory_reads "Test for STAR 9000830091. Args: address=0x10000000 count=2"

proc arc_test_consecutive_memory_reads { {address 0x10000000} {count 2} } {
    set mem_value 0xdeadbeef

    for {set i 0} {$i < $count} {incr i} {
	mww [expr $address+$i*4] $mem_value
    }

    set mem_content ""
    mem2array mem_content 32 $address $count
    for {set i 0} {$i < $count} {incr i} {
	if {$mem_value != $mem_content($i)} {
	    echo "Test for consecutive memory reads: FAIL at word $i"
	    exit 1
	}
    }
    echo "Test for consecutive memory reads: PASS"
}

add_help_text arc_test_run_all "Run all ARC tests"
proc arc_test_run_all { } {
    set tests {consecutive_memory_reads}
    foreach i $tests {
	arc_test_$i
    }
}

# vi:ft=tcl
