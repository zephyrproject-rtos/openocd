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

source [find cpu/arc/v2.tcl]

proc arc_hs_examine_target { target } {
	# Will set current target for us.
	arc_v2_examine_target $target
}

proc arc_hs_init_regs { } {
	arc_v2_init_regs

	[target current] configure \
		-event examine-end "arc_hs_examine_target [target current]"
}

# Scripts in "target" folder should call this function instead of direct
# invocation of arc_common_reset.
proc arc_hs_reset { {target ""} } {
	arc_v2_reset $target

	# Invalidate L2 cache if there is one.
	set l2_config [$target arc jtag aux-reg 0x901]
	# Will return 0, if cache is not present and register doesn't exist.
	set l2_ctrl [$target arc jtag aux-reg 0x903]
	if { ($l2_config != 0) && (($l2_ctrl & 1) == 0) } {
		puts "L2 cache is present and not disabled"

		# Wait until BUSY bit is 0.
		puts "Invalidating L2 cache..."
		$target arc jtag aux-reg 0x905 1
		# Dummy read of SLC_AUX_CACHE_CTRL bit, as described in:
		# https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git/commit/arch/arc?id=c70c473396cbdec1168a6eff60e13029c0916854
		set l2_ctrl [$target arc jtag aux-reg 0x903]
		set l2_ctrl [$target arc jtag aux-reg 0x903]
		while { ($l2_ctrl & 0x100) != 0 } {
			set l2_ctrl [$target arc jtag aux-reg 0x903]
		}

		# Flush cache if needed. If SLC_AUX_CACHE_CTRL.IM is 1, then invalidate
		# operation already flushed everything.
		if { ($l2_ctrl & 0x40) == 0 } {
			puts "Flushing L2 cache..."
			$target arc jtag aux-reg 0x904 1
			set l2_ctrl [$target arc jtag aux-reg 0x903]
			set l2_ctrl [$target arc jtag aux-reg 0x903]
			while { [expr $l2_ctrl & 0x100] != 0 } {
				set l2_ctrl [$target arc jtag aux-reg 0x903]
			}
		}

		puts "L2 cache has been flushed and invalidated."
	}
}
