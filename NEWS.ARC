Changes in 2014.12 release:
    * Merge with latest OpenOCD master
    * Support EM Starter Kit v2
    * Support Break/Ctrl-C command from GDB
    * Fix errors when reading data from DDR memory in EM7D
    * Support custom reset-assert event handlers
    * Valid reset implementation for ARC cores on FPGA development systems
    * New commands:
	- arc jtag always-check-status-rd
	- arc jtag check-status-fl
	- arc jtag aux-reg (replaces arc (read|write)-aux-reg
	- arc jtag core-reg (replaces arc (read|write)-core-reg
    * Removed commands:
	- arc set-pc
	- arc set-core-into-halted
	- arc read-mem-word
	- arc write-mem-word
	- arc print-core-status
	- arc read-aux-reg
	- arc write-aux-reg
	- arc read-core-reg
	- arc write-core-reg
    * Numerous bug fixes

