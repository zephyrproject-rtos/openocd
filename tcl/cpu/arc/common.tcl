# Things common to all ARCs

# It is assumed that target is already halted.
proc arc_common_reset { } {
    # 1. Interrupts are disabled (STATUS32.IE)
    # 2. The status register flags are cleared.
    # All fields, except the H bit, are set to 0 when the processor is Reset.
    arc jtag aux-reg 0xA 0x1

    # 3. The loop count, loop start, and loop end registers are cleared.
    arc jtag core-reg 60 0
    arc jtag aux-reg 0x2 0
    arc jtag aux-reg	0x3 0

    # 4. The scoreboard unit is cleared.
    # 5. The pending-load flag is cleared.

    # Program execution begins at the address referenced by the four byte reset
    # vector located at the interrupt vector base address, which is the first
    # entry (offset 0x00) in the vector table.
    set int_vector_base [arc jtag aux-reg 0x25]
    set start_pc ""
    mem2array start_pc 32 $int_vector_base 1
    arc jtag aux-reg 0x6 $start_pc(0)

    # It is OK to do uncached writes - register cache will be invalidated by
    # the reset_assert() function.
}

