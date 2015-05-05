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

