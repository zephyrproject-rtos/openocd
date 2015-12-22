#  Copyright (C) 2015 Synopsys, Inc.
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

source [find cpu/arc/common.tcl]

proc arc_arcompact_examine_target { {target ""} } {
	if { $target != "" } {
		targets $target
	}

	set arcver [arc reg-field identity arcver]
	if { $arcver < 0x20 || $arcver > 0x3F } {
		puts "ERROR: This is not an ARCompact core. Wrong CPU configuration file used?"
		exit 1
	}

	# Those registers always exist. DEBUG is formally optional, however it
	# comes with JTAG interface, and so far there is no way OpenOCD can
	# communicate with target without JTAG interface.
	arc set-reg-exists identity pc status32 debug lp_start lp_end

	# 32 core registers
	arc set-reg-exists \
		r0  r1  r2  r3  r4  r5  r6  r7  r8  r9  r10 r11 r12 \
		r13 r14 r15 r16 r17 r18 r19 r20 r21 r22 r23 r24 r25 \
		gp fp sp ilink1 ilink2 blink lp_count pcl
}

proc arc_arcompact_init_regs { } {
	# XML features
	set core_feature "org.gnu.gdb.arc.core.arcompact"
	set aux_min_feature "org.gnu.gdb.arc.aux-minimal"
	set aux_other_feature "org.gnu.gdb.arc.aux-other"

	# AUX registers types
	arc add-reg-type-struct -name debug_t \
		-bitfield ss  0  0 \
		-bitfield fh  1  1 \
		-bitfield is 11 11 \
		-bitfield ra 22 22 \
		-bitfield zz 23 23 \
		-bitfield ed 24 24 \
		-bitfield ep 25 25 \
		-bitfield ub 28 28 \
		-bitfield bh 29 29 \
		-bitfield sh 30 30 \
		-bitfield ld 31 31

	arc add-reg-type-struct -name identity_t \
		-bitfield arcver 0 7 -bitfield arcnum 8 15 -bitfield chipid 16 31
	arc add-reg-type-flags -name status32_t \
		-flag H  0 -flag E1 1 -flag E2  2 -flag A1 3 \
		-flag A2 4 -flag AE 5 -flag DE  6 -flag U  7 \
		-flag V  8 -flag C  9 -flag N  10 -flag Z 11 \
		-flag L 12 -flag R 13 -flag SE 14

	# BCR types
	arc add-reg-type-struct -name bcr_ver_t -bitfield version 0 7

	# Core registers
	set core_regs {
		r0       0  uint32
		r1       1  uint32
		r2       2  uint32
		r3       3  uint32
		r4       4  uint32
		r5       5  uint32
		r6       6  uint32
		r7       7  uint32
		r8       8  uint32
		r9       9  uint32
		r10      10 uint32
		r11      11 uint32
		r12      12 uint32
		r13      13 uint32
		r14      14 uint32
		r15      15 uint32
		r16      16 uint32
		r17      17 uint32
		r18      18 uint32
		r19      19 uint32
		r20      20 uint32
		r21      21 uint32
		r22      23 uint32
		r23      24 uint32
		r24      24 uint32
		r25      25 uint32
		gp       26 data_ptr
		fp       27 data_ptr
		sp       28 data_ptr
		ilink1   29 code_ptr
		ilink2   30 code_ptr
		blink    31 code_ptr
		r32      32 uint32
		r33      33 uint32
		r34      34 uint32
		r35      35 uint32
		r36      36 uint32
		r37      37 uint32
		r38      38 uint32
		r39      39 uint32
		r40      40 uint32
		r41      41 uint32
		r42      42 uint32
		r43      43 uint32
		r44      44 uint32
		r45      45 uint32
		r46      46 uint32
		r47      47 uint32
		r48      48 uint32
		r49      49 uint32
		r50      50 uint32
		r51      51 uint32
		r52      52 uint32
		r53      53 uint32
		r54      54 uint32
		r55      55 uint32
		r56      56 uint32
		r57      57 uint32
		r58      58 uint32
		r59      59 uint32
		lp_count 60 uint32
		reserved 61 uint32
		limm     62 uint32
		pcl      63 code_ptr
	}
	foreach {reg count type} $core_regs {
		arc add-reg -name $reg -num $count -core -type $type -g \
			-feature $core_feature
	}

	# AUX min
	set aux_min {
		0x6 pc       code_ptr
		0x2 lp_start code_ptr
		0x3 lp_end   code_ptr
		0xA status32 status32_t
	}
	foreach {num name type} $aux_min {
		arc add-reg -name $name -num $num -type $type -feature $aux_min_feature -g
	}

	# AUX other
	set aux_other {
		0x004 identity		identity_t
		0x005 debug			debug_t
	}
	foreach {num name type} $aux_other {
		arc add-reg -name $name -num $num -type $type -feature $aux_other_feature
	}

	[target current] configure \
		-event examine-end "arc_arcompact_examine_target [target current]"
}

# Scripts in "target" folder should call this function instead of direct
# invocation of arc_common_reset.
proc arc_arcompact_reset { {target ""} } {
	arc_common_reset $target
}
