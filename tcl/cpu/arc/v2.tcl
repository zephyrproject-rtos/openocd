#  Copyright (C) 2015 Synopsys, Inc.
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

# Currently 'examine_target' can only read JTAG registers and set properties -
# but it shouldn't write any of registers - writes will be cached, but cache
# will be invalidated before flushing after examine_target, and changes will be
# lost.  Perhaps that would be fixed later - perhaps writes shouldn't be cached
# after all.  But if write to register is really needed from TCL - then it
# should be done via "arc jtag" for now.
proc arc_v2_examine_target { {target ""} } {
	# Set current target, because OpenOCD event handlers don't do this for us.
	if { $target != "" } {
		targets $target
	}

	# Those registers always exist. DEBUG and DEBUGI are formally optional,
	# however they come with JTAG interface, and so far there is no way
	# OpenOCD can communicate with target without JTAG interface.
	arc set-reg-exists identity pc status32 bta	debug lp_start lp_end

	# 32 core registers
	arc set-reg-exists \
		r0  r1  r2  r3  r4  r5  r6  r7  r8  r9  r10 r11 r12 \
		r13 r14 r15 r16 r17 r18 r19 r20 r21 r22 r23 r24 r25 \
		gp fp sp ilink r30 blink lp_count pcl

	# Actionpoints
	if { [arc reg-field ap_build version] == 5 } {
		set ap_build_type [arc reg-field ap_build type]
		# AP_BUILD.TYPE > 0b0110 is reserved in current ISA.
		# Current ISA supports up to 8 actionpoints.
		if { $ap_build_type < 8 } {
			# Two LSB bits of AP_BUILD.TYPE define amount of actionpoints:
			# 0b00 - 2 actionpoints
			# 0b01 - 4 actionpoints
			# 0b10 - 8 actionpoints
			# 0b11 - reserved.
			set ap_num [expr 0x2 << ($ap_build_type & 3)]
			# Expression on top may produce 16 action points - which is a
			# reserved value for now.
			if { $ap_num < 16 } {
				# Enable actionpoint registers
				for {set i 0} {$i < $ap_num} {incr i} {
					arc set-reg-exists ap_amv$i ap_amm$i ap_ac$i
				}

				# Set amount of actionpoints
				arc num-actionpoints $ap_num
			}
		}
	}

	# DCCM
	if { [arc reg-field dccm_build version] == 3 } {
		arc set-reg-exists aux_dccm
	}

	# ICCM
	if { [arc reg-field iccm_build version] == 4 } {
		arc set-reg-exists aux_iccm
	}
}

proc arc_v2_init_regs { } {
	# XML features
	set core_feature "org.gnu.gdb.arc.core.v2"
	set aux_min_feature "org.gnu.gdb.arc.aux-minimal"
	set aux_other_feature "org.gnu.gdb.arc.aux-other"

	# Describe types
	# Types are sorted alphabetically according to their name.
	arc add-reg-type-struct -name ap_build_t -bitfield version 0 7 \
		-bitfield type 8 11
	arc add-reg-type-struct -name ap_control_t -bitfield at 0 3 -bitfield tt 4 5 \
		-bitfield m 6 6 -bitfield p 7 7 -bitfield aa 8 8 -bitfield q 9 9
	arc add-reg-type-struct -name dccm_build_t -bitfield version 0 7 \
		-bitfield size0 8 11 -bitfield size1 12 15
	arc add-reg-type-struct -name debug_t \
		-bitfield fh 1 1   -bitfield ah 2 2   -bitfield asr 3 10 \
		-bitfield is 11 11 -bitfield ep 19 19 -bitfield ed 20 20 \
		-bitfield eh 21 21 -bitfield ra 22 22 -bitfield zz 23 23 \
		-bitfield sm 24 26 -bitfield ub 28 28 -bitfield bh 29 29 \
		-bitfield sh 30 30 -bitfield ld 31 31
	arc add-reg-type-struct -name iccm_build_t -bitfield version 0 7 \
		-bitfield iccm0_size0  8 11 -bitfield iccm1_size0 12 15 \
		-bitfield iccm0_size1 16 19 -bitfield iccm1_size1 20 23
	arc add-reg-type-struct -name identity_t \
		-bitfield arcver 0 7 -bitfield arcnum 8 15 -bitfield chipid 16 31
	arc add-reg-type-struct -name isa_config_t -bitfield version 0 7 \
		-bitfield pc_size 8 11 -bitfield lpc_size 12 15 -bitfield addr_size 16 19 \
		-bitfield b 20 20 -bitfield a 21 21 -bitfield n 22 22 -bitfield l 23 23 \
		-bitfield c 24 27 -bitfield d 28 31
	arc add-reg-type-flags -name status32_t \
		-flag   H  0 -flag E0   1 -flag E1   2 -flag E2  3 \
		-flag  E3  4 -flag AE   5 -flag DE   6 -flag  U  7 \
		-flag   V  8 -flag  C   9 -flag  N  10 -flag  Z 11 \
		-flag   L 12 -flag DZ  13 -flag SC  14 -flag ES 15 \
		-flag RB0 16 -flag RB1 17 -flag RB2 18 \
		-flag  AD 19 -flag US  20 -flag IE  31

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
		ilink    29 code_ptr
		r30      30 uint32
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
		accl     58 uint32
		acch     59 uint32
		lp_count 60 uint32
		limm     61 uint32
		reserved 62 uint32
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
		0x004 identity	identity_t
		0x005 debug		debug_t
		0x018 aux_dccm	int
		0x208 aux_iccm	int

		0x220 ap_amv0	uint32
		0x221 ap_amm0	uint32
		0x222 ap_ac0	ap_control_t
		0x223 ap_amv1	uint32
		0x224 ap_amm1	uint32
		0x225 ap_ac1	ap_control_t
		0x226 ap_amv2	uint32
		0x227 ap_amm2	uint32
		0x228 ap_ac2	ap_control_t
		0x229 ap_amv3	uint32
		0x22A ap_amm3	uint32
		0x22B ap_ac3	ap_control_t
		0x22C ap_amv4	uint32
		0x22D ap_amm4	uint32
		0x22E ap_ac4	ap_control_t
		0x22F ap_amv5	uint32
		0x230 ap_amm5	uint32
		0x231 ap_ac5	ap_control_t
		0x232 ap_amv6	uint32
		0x233 ap_amm6	uint32
		0x234 ap_ac6	ap_control_t
		0x235 ap_amv7	uint32
		0x236 ap_amm7	uint32
		0x237 ap_ac7	ap_control_t

		0x412 bta		code_ptr
	}
	foreach {num name type} $aux_other {
		arc add-reg -name $name -num $num -type $type -feature $aux_other_feature
	}

	# AUX BCR
	set bcr {
		0x74 dccm_build
		0x76 ap_build
		0x78 iccm_build
		0xC1 isa_config
	}
	foreach {num reg} $bcr {
		arc add-reg -name $reg -num $num -type ${reg}_t -bcr -feature $aux_other_feature
	}

    [target current] configure \
		-event examine-end "arc_v2_examine_target [target current]"
}

proc arc_v2_reset { {target ""} } {
	arc_common_reset $target

	# Disable all actionpoints.  Cannot write via regcache yet, because it will
	# not be flushed and all changes to registers will get lost.  Therefore has
	# to write directly via JTAG layer...
	set num_ap [arc num-actionpoints]
	for {set i 0} {$i < $num_ap} {incr i} {
		arc jtag aux-reg [expr 0x222 + $i * 3] 0
	}
}
