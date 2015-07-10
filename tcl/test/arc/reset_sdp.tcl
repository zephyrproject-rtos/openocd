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

#
# Hardware reset for Synopsys DesignWare ARC AXS10x Software Development
# Platforms. Should work with v2 and v3 mainboards.
#

# SDP has built-in FT2232 chip, which is similiar to Digilent HS-1, except that
# it uses channel B for JTAG, instead of channel A.
interface ftdi
ftdi_vid_pid 0x0403 0x6010
ftdi_layout_init 0x0088 0x408b
# JTAG is at channel B, but reset signal is at A.
ftdi_channel 0

# Reset signal specification
ftdi_layout_signal nSRST -data 0x4000
reset_config srst_only
reset_config srst_push_pull

# Useless (no JTAG is available), but OpenOCD requires those to be specified.
adapter_khz 10000
transport select jtag

# OpenOCD will die with failed assertion as soon as SRST is used:
#
# openocd: ../../../openocd/src/jtag/core.c:340: jtag_checks: Assertion
# `jtag_trst == 0' failed.
#
# Nevertheless the job will be done - board will be reset, and that is what
# needed.
init

