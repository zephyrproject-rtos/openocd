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

# In some cores when wait-until-write-finished is ON, write operations
# overwrite next register/memory word. This is a testcase for this bug (whether
# wait-until-write-finished is set or not is defined outside of this function).
proc arc_test_reg_write { } {
	# Set known values to registers.
	arc jtag core-reg 0 0
	arc jtag core-reg 1 0

	# Write R0.
	arc jtag core-reg 0 1

	# Check that R1 hasn't been modified.
	set r1 [arc jtag core-reg 1]
	if { $r1 != 0 } {
		echo "Test for register write: FAIL"
	} else {
		echo "Test for register write: PASS"
	}
}

# In some cores when wait-until-write-finished is ON, write operations
# overwrite next register/memory word. This is a testcase for this bug (whether
# wait-until-write-finished is set or not is defined outside of this function).
proc arc_test_mem_write { {address 0x80000000} } {
	# Set known values to memory.
	mww $address 0xdeadbeef
	mww [expr $address + 4] 0xdeadbeef

	# Write first memory word
	mww $address 1

	# Check that second memory word hasn't been modified.
	set mem_content ""
	mem2array mem_content 32 [expr $address + 4] 1
	if { $mem_content(0) != 0xdeadbeef } {
		echo "Test for memory write: FAIL"
	} else {
		echo "Test for memory write: PASS"
	}
}

add_help_text arc_test_run_all "Run all ARC tests"
proc arc_test_run_all { } {
	set tests {consecutive_memory_reads reg_write mem_write}
	foreach i $tests {
		arc_test_$i
	}
}

# vi:ft=tcl
