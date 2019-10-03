proc init_ocd { } {
    global _TARGETNAME
    source [find interface/ftdi/digilent-hs1.cfg]
    adapter_khz 10000

    # ARCs support only JTAG.
    transport select jtag

    # Configure FPGA. This script supports both LX45 and LX150.
    source [find cpu/arc/common.tcl]

    set _CHIPNAME arc-em
    set _TARGETNAME $_CHIPNAME.cpu

    # EM SK IDENTITY is 0x200444b1
    # EM SK v2 IDENTITY is 0x200044b1
    jtag newtap $_CHIPNAME cpu -irlen 4 -ircapture 0x1 -expected-id 0x200444b1 \
    -expected-id 0x200044b1

    set _coreid 0
    set _dbgbase [expr 0x00000000 | ($_coreid << 13)]

    target create $_TARGETNAME arc32 -chain-position $_TARGETNAME \
    -coreid 0 -dbgbase $_dbgbase -endian little

    # There is no SRST, so do a software reset
    $_TARGETNAME configure -event reset-assert arc_common_reset
}

proc fail_test { {msg ""} } {
    puts "FAIL: [info level 1]: $msg"
    exit 1
}

proc pass_test { {msg ""} } {
    puts "PASS: [info level 1]: $msg"
}

proc run_test { block ex } {
    if {![catch $block m]} {
        puts "FAIL: [info level 1]: $ex"
        exit 1
    } elseif {$m != $ex} {
        puts "FAIL: [info level 1]: $ex"
        exit 1
    } else {
        puts "PASS: [info level 1]: $ex"
    }
}

# Check that OpenOCD complains when specified type canot be found
proc arc_test_regs_unknown_type { } {
    init_ocd

    # Describe registers
    set feat "org.gnu.gdb.arc.core.v2"
    # Core registers
    arc add-reg -name r0 -num 0 -gdbnum 0 -core -feature $feat
    arc add-reg -name r1 -num 1 -gdbnum 1 -core -type int -feature $feat
    if {![catch {arc add-reg -name r2 -num 2 -gdbnum 2 -core -type ptr -feature $feat }]} {
        fail_test
    } else {
        pass_test
    }
    arc add-reg -name pc    -num 0x6 -feature $feat
    arc add-reg -name debug -num 0x5 -feature $feat
}


# Check that OpenOCD complains about invalid parameters
proc arc_test_regs_params { } {
    init_ocd
    set f "org.gnu.gdb.arc.core.v2"
    # No -name
    run_test "arc add-reg -num 0 -gdbnum 0 -core -feature $f" "-name option is required"
    # -name without argument
    run_test "arc add-reg -name -num 0 -gdbnum 0 -core -feature $f" \
        "Unknown param: 0, try one of: -name, -gdbnum, -num, -core, -feature, or -type"
    run_test "arc add-reg -num 0 -gdbnum 0 -core -feature $f -name" \
        "wrong # args: should be \"-name ?name? ...\""
    # No num
    run_test "arc add-reg -name r0 -gdbnum 0 -core -feature $f" "-num option is required"
    # Num without argument
    run_test "arc add-reg -name r0 -num -gdbnum 0 -core -feature $f" \
        "expected integer but got \"-gdbnum\""
    run_test "arc add-reg -name r0 -gdbnum 0 -core -feature $f -num" \
        "wrong # args: should be \"-num ?int? ...\""
    # gdbnum without argument
    run_test "arc add-reg -name r0 -gdbnum -core -feature $f" \
        "expected integer but got \"-core\""
    run_test "arc add-reg -name r0 -core -feature $f -gdbnum" \
        "wrong # args: should be \"-gdbnum ?int? ...\""
    # Check that core doesn't take arguments
    run_test "arc add-reg -name r0 -core is_core -feature $f" \
        "Unknown param: is_core, try one of: -name, -gdbnum, -num, -core, -feature, or -type"
    # No -feature
    run_test "arc add-reg -name r0 -num 0" "-feature option is required"
    # -feature without argument
    run_test "arc add-reg -name r0 -feature -num 0" \
        "Unknown param: 0, try one of: -name, -gdbnum, -num, -core, -feature, or -type"
    run_test "arc add-reg -name r0 -num 0 -feature" \
        "wrong # args: should be \"-feature ?name? ...\""
    # -type without argument
    run_test "arc add-reg -name r0 -type -feature F -num 0" \
        "Unknown param: F, try one of: -name, -gdbnum, -num, -core, -feature, or -type"
    run_test "arc add-reg -name r0 -num 0 -feature F -type" \
        "wrong # args: should be \"-type ?type? ...\""
    arc add-reg -name pc    -num 0x6 -feature $f
    arc add-reg -name debug -num 0x5 -feature $f
}

proc arc_test_regs_full { } {
    global _TARGETNAME
    init_ocd

    # Describe types
    arc add-reg-type-flags -name status32_type \
        -flag   H  0 -flag E0   1 -flag E1   2 -flag E2  3 \
        -flag  E3  4 -flag AE   5 -flag DE   6 -flag  U  7 \
        -flag   V  8 -flag  C   9 -flag  N  10 -flag  Z 11 \
        -flag   L 12 -flag DZ  13 -flag SC  14 -flag ES 15 \
        -flag RB0 16 -flag RB1 17 -flag RB2 18 \
        -flag  AD 19 -flag US  20 -flag IE  31
    # Describe registers
    set core_feat "org.gnu.gdb.arc.core.v2"
    set aux_min_feat "org.gnu.gdb.arc.aux-minimal"
    set aux_other_feat "org.gnu.gdb.arc.aux-other"
    # Core registers
    set core_regs {
        r0  0  uint32
        r1  1  uint32
        r2  2  uint32
        r3  3  uint32
        r4  4  uint32
        r5  5  uint32
        r6  6  uint32
        r7  7  uint32
        r8  8  uint32
        r9  9  uint32
        r10 10 uint32
        r11 11 uint32
        r12 12 uint32
        r13 13 uint32
        r14 14 uint32
        r15 15 uint32
        r16 16 uint32
        r17 17 uint32
        r18 18 uint32
        r19 19 uint32
        r20 20 uint32
        r21 21 uint32
        r22 23 uint32
        r23 24 uint32
        r24 24 uint32
        r25 25 uint32
        gp    26 data_ptr
        fp    27 data_ptr
        sp    28 data_ptr
        ilink 29 code_ptr
        r30 30 uint32
        blink 31 code_ptr
        r32 32 uint32
        r33 33 uint32
        r34 34 uint32
        r35 35 uint32
        r36 36 uint32
        r37 37 uint32
        r38 38 uint32
        r39 39 uint32
        r40 40 uint32
        r41 41 uint32
        r42 42 uint32
        r43 43 uint32
        r44 44 uint32
        r45 45 uint32
        r46 46 uint32
        r47 47 uint32
        r48 48 uint32
        r49 49 uint32
        r50 50 uint32
        r51 51 uint32
        r52 52 uint32
        r53 53 uint32
        r54 54 uint32
        r55 55 uint32
        r56 56 uint32
        r57 57 uint32
        r58 58 uint32
        r59 59 uint32
        lp_count 60 uint32
        limm 61 uint32
        reserved 62 uint32
        pcl   63 code_ptr
    }
    foreach {reg count type} $core_regs {
        arc add-reg -name $reg -num $count -gdbnum $count -core -type $type \
            -feature $core_feat
    }

    # AUX
    set aux_min {
        pc       6   64 code_ptr
        lp_start 2   65 code_ptr
        lp_end   3   66 code_ptr
        status32 0xA 66 status32_type
    }
    foreach {reg num gdbnum type} $aux_min {
        arc add-reg -name $reg -num $num -gdbnum $gdbnum -type $type -feature $aux_min_feat
    }
    arc add-reg -name debug    -num 0x5 -feature $aux_other_feat

    $_TARGETNAME configure -event examine-end  {
        arc set-reg-exists r0 r1 r2 r3 r4 r5 r6 r7 r8 r9 r10 r11 r12 r13 r14 \
          r15 r16 r17 r18 r19 r20 r21 r22 r23 r24 r25 gp fp sp ilink r30 blink \
          lp_count pcl pc lp_start lp_end status32 debug
    }

    # Initialize
    init
    gdb_save_tdesc
}

# vi:ft=tcl expandtab
