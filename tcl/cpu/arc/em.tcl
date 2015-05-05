source [find cpu/arc/v2.tcl]

proc arc_em_examine_target { {target ""} } {
	# Will set current target
	arc_v2_examine_target $target
}

proc arc_em_init_regs { } {
	arc_v2_init_regs

	[target current] configure \
		-event examine-end "arc_em_examine_target [target current]"
}

