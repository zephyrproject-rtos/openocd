ARC Bookshelf: Plain text OpenOCD Getting Started manual
========================================================

Content:

1. How to build OpenOCD on Linux
2. How to build OpenOCD for Windows (cross compile on Linux)
3. How to use OpenOCD on Linux
4. How to use OpenOCD on Windows
5. Advanced debug commands
6. Some useful ARC development commands
7. Find your way in the source code
8. Digilent cable installation instructions
9. How to program a bit-file into FPGA with the Digilent HS cable
10. Running internal testsuite

Important notes:

1. This document uses term "Digilent HS" to refer to both HS1 and HS2 cables.
   Statements that are applicable only to selected cable mention it explicitly.
2. There is NO Flash support in ARC OpenOCD.


1. How to build OpenOCD on Linux
--------------------------------

Install prerequisites: libtool, autoconf, automake, GCC, Git, makeinfo, libusb
and its development package. Command to install those on Ubuntu 12.04/14.04 build
host:

    $ sudo apt-get install libtool git-core build-essential autoconf automake
    texinfo libusb-1.0-dev pkg-config

Command to install prerequisites on RHEL 6:

    $ sudo yum install libtool gcc autoconf automake texinfo libusbx-devel

Optional dependencies required to build documentation are: asciidoc and
texinfo-tex.

Download OpenOCD sources:

    $ git clone https://github.com/foss-for-synopsys-dwc-arc-processors/openocd
    $ cd openocd

Configure OpenOCD. Consult `configure --help` and generic OpenOCD documentation
for details. For ARC you would want to have an ftdi driver.

    $ ./bootstrap
    $ ./configure --enable-ftdi --disable-doxygen-html

Notes:
  * if documentation is required, remove the option `--disable-doxygen-html`
  * for development activities, add those options:
    `--enable-verbose --enable-verbose-jtag-io`
  * if build fails and there in error log there is something about "warnings
    treated as errors", then try to configure with option `--disable-werror`.

Now build and install:
    $ make
    $ sudo make install

It is possible to run OpenOCD from build directory, without installing, however
this is out of scope of this document.

Finally you need to configure udev rules in such way that OpenOCD would be able
to claim your JTAG debug cable. In common case for ARC this is an FTDI-based
device. If you already have libftdi package installed on your system, then
required rules are already provided to udev. Otherwise create file
`/etc/udev/rules.d/99-ftdi.rules` with the following contents:

    # allow users to claim the device
    # Digilent HS1 and similiar products
    SUBSYSTEM=="usb", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6010", MODE="0664", GROUP="plugdev"
    # Digilent HS2
    SUBSYSTEM=="usb", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6014", MODE="0664", GROUP="plugdev"

You also can use file `contrib/openocd.udev` supplied with OpenOCD sources,
however this file doesn't work with Digilent HS2, though on the other hand it
mentions many other FTDI-based devices.

Then either reboot your system or reload udev configuration and reconnect debug
cable to the host computer:

    $ sudo udevadm control --reload-rules
    # Disconnect JTAG cable from host, then connect again.


2. How to build OpenOCD for Windows (cross compile on Linux)
------------------------------------------------------------

There are two ways to use OpenOCD with FTDI-based debug cables: with an
obsolete ft2232 interface and proprietary FTD2xx drivers (further down called
ft2232/ftd2xx) or with new ftdi interface and libusb driver (further down
called ftdi/libusb).

Since ft2232 interface is considered obsolete by OpenOCD developers in general
it is suggested to use ftdi/libusb combination, however on Windows this
requires replacing the original FTDI drivers with open source ones. This will
render Digilent cable unusable by Digilent tools, like Adept.  Consequently you
have to choose depending on your requirements. It is also has to be mentioned
that in practice ftdi/libusb is much faster on Windows than ft2232/ftd2xx.

Install the same prerequisites like for Linux build (except for libusb-dev) and
MinGW on your system:

    $ sudo apt-get install libtool git-core build-essential autoconf automake
    texinfo

Install MinGW cross-compiler to your system:

    $ sudo apt-get install gcc-mingw-w64

If you are going to use libusb drivers, then download libusb or
[libusbsx](http://libusbx.org/) sources. Configure and build them with MinGW
compiler. It is recommended to build only static libusb, so that OpenOCD will
not need this library's dll file to be copied around:

    $ tar xaf libusbx-1.0.18-rc1.tar.bz2
    $ cd libusbx-1.0.18-rc1
    $ ./configure --host=i686-w64-mingw32 --build=x86_64-linux-gnu \
      --prefix=</libusbx/install/path> --disable-shared --disable-static
    $ make
    $ make install

If you are planing to use FTD2xx drivers, then download them from [FTDI
site](http://www.ftdichip.com/Drivers/D2XX.htm). Unzip them:

    $ mkdir ftd2xx
    $ cd ftd2xx
    $ unzip ../CDM\ 2.08.30\ WHQL\ Certified.zip
    $ unzip CDM\ v2.08.30\ WHQL\ Certified.zip

Download OpenOCD sources:

    $ git clone https://github.com/foss-for-synopsys-dwc-arc-processors/openocd
    $ cd openocd
    $ ./bootstrap

Configure OpenOCD. Consult `configure --help` and generic OpenOCD documentation
for details. This command line is recommended for ARC with libusb/ftdi:

    $ PKG_CONFIG_PATH=</libusbx/install/path>/lib/pkgconfig ./configure \
      --enable-ftdi --host=i686-w64-mingw32 --build=x86_64-linux-gnu \
      --disable-werror --prefix=<openocd/install/path>

Note that it is required to set PKG_CONFIG_PATH, otherwise configure script
will detect host libusb installation, instead of the one cross-compiled for
Windows.

To configure for FTD2xx drivers:

    $ ./configure --enable-ft2232_ftd2xx --host=i686-w64-mingw32 \
      --build=x86_64-linux-gnu --disable-werror --prefix=</openocd/install/path> \
      --with-ftd2xx-win32-zipdir=</directory/where/you've/unzipped/ftd2xx>

It is highly recommended to remove libusb-1.0-dev package from your host
system, otherwise configure script will use information from your host system
when configuring, but compiler will not be able to link Windows' openocd binary
with libusb for Linux. Alternative is to compile with both interfaces. In the
latter case add `--enable-ft2232_ftd2xx --with-ftd2xx-win32-zipdir=...` to the
command line for libusb/ftdi.

Build and install:

    $ make
    $ make install

If your application uses libusb and is being linked dynamically (this is by
default), copy </libusbx/install/path>/bin/libusb-1.0.dll to the OpenOCD bin
directory. Copy OpenOCD installation to Window host.


3. How to use OpenOCD on Linux
-----------------------------------------------------------

> Connection host is a host that is connected to the debug target via USB cable
> and runs OpenOCD. Debug host is a hsot that runs GDB, which connects to the
> OpenOCD with TCP connection. Ehily typically it is the same host, they
> actually can be different host and it is important to distinguish them.

Connect debug target to the connection host. AXS10x products and EM Starter Kit
have built-in debug cable, the don't require a separate Digilent HS cable. HS
is required only for other debug targets like ML-509 board, etc.

Run lsusb application to ensure that FTDI device is there:

    $ lsusb

In case of HS1, EM Starter Kit and AXS10x there should line like this:

    Bus 001 Device 002: ID 0403:6010 Future Technology Devices International, Ltd FT2232C Dual USB-UART/FIFO IC

In case if HS2 there should be line like this:

    Bus 001 Device 003: ID 0403:6014 Future Technology Devices International, Ltd FT232H Single HS USB-UART/FIFO IC

Note that exact lines could differ from host to host.

Run OpenOCD:

    $ openocd -f <openocd.cfg>

Valid openocd.cfg files are installed into
`/usr/local/share/openocd/scripts/board/`:
 * snps_em_sk.cfg - DW ARC EM Starter Kit
 * snps_axs101.cfg - DW ARC AXS101 Software Development platform

On the debug host (PC with GDB) start an ELF32 GDB debugger:

    $ arc-elf32-gdb ./<elf_app_to_debug>

Make the connection between arc-elf32-gdb and OpenOCD:

    (gdb) target remote <connection host ip address>:3333

Load image to be debugged (./<app_to_debug>.elf) into the target memory:

    (gdb) load

Set breakpoints at functions main and exit:

    (gdb) break main
    (gdb) break exit

Start the execution on target of the image to debug, to reach function main:

  (gdb) continue

Resume execution to reach function exit:

  (gdb) continue


4. How to use OpenOCD on Windows
--------------------------------

If you are using libusb/ftdi combination, then you have to download WinUSB
driver and replace FTDI drivers for Digilent cables or EM Starter Kit with it.
Refer to [this page](https://github.com/libusbx/libusbx/wiki/Windows-Backend)
for details. In a nutshell, download [Zadig](http://zadig.akeo.ie/), run it and
use it to install WinUSB driver for FTDI device. If your FTDI device is not
shown by Zadig, then tick "List all devices" in "Options". Note that antivirus
might complain about driver files created by Zadig. After installing the driver
everything is the same as on Linux: run OpenOCD, connect to it with GDB, etc.

To use ft2232/ftd2xx combination, download [Digilent Adept
utility](http://www.digilentinc.com/Products/Detail.cfm?Prod=ADEPT2) and use it
to install latest FTDI drivers. Then you need to change OpenOCD configuration
file, because by default it uses ftdi/libusb combination. Refer to "How to use
OpenOCD on Linux" for a list of available files. In your configuration script
replace line `source [find interface/ftdi/digilent-hs1.cfg]` with line `source
[find interface/digilent-hs1.cfg]`. If you are using Digilent HS2, then use
files named `digilent-hs2.cfg`.  Everything else is the same as for Linux or
for ftdi/libusb combination.


5. Advanced debug commands
--------------------------

With the GDB "monitor" command, you have "direct" access to the core without
any interference from GDB anymore! With other words, GDB has no notion of
changes in core state when using the so called  monitor commands (but it is
very powerful). In GDB, connect to the OpenOCD target and type following command to get a list of available monitor commands:

    (gdb) monitor help

To get a list of some ARC-specific commands, run:

    (gdb) monitor help arc


6. Some useful ARC development commands
---------------------------------------

Some command that might be useful during development:

    $ arc-elf32-readelf u-boot.elf
    $ arc-elf32-readelf -S u-boot.elf
    $ arc-elf32-readelf u-boot.elf
    $ arc-elf32-readelf -x --hex-dump=.text u-boot.elf | more
    $ arc-elf32-readelf -x --hex-dump=1 u-boot.elf | more
    $ arc-elf32-readelf -x 1 u-boot.elf | more


7. Find your way in the source code
-----------------------------------

Automake makefile entry starts from: src/target/Makefile.am

In src/target is the ARC specific code base:

	Top and bottom interfaces into OpenOCD:

	arc.c				main hook into OpenOCD framework (target function body)
	arc.h				main include (gets everywhere included)
	arc_jtag.c + .h		ARC jtag interface into OpenOCD

	Supporting functions/modules as used by above interface into OpenOCD

	arc32.c + .h		generic ARC architecture functions
	arc_core.c + .h		ARC core internal specifics
	arc_dbg.c + .h		ARC debugger functions
	arc_mem.c + .h		ARC memory functions
	arc_mntr.c + .h		GDB monitor functions
	arc_ocd.c + .h		ARC OCD initialization
	arc_regs.c + .h		ARC register access
	arc_trgt.c + .h		target/board system functions


8. Digilent driver installation instructions
--------------------------------------------

OpenOCD doesn't use Digilent drivers to communicate with Digilent debug cables,
instead it uses it's own implementation of FTDI MPSSE protocol, which us
compatible with _virtually_ any other FTDI x232 cable. However to use some
features of Digilent cables you might need to install their drivers and
utilities. Following are instructions on how to do that.

Download appropriate version of runtime and utilities from Digilent site:
http://www.digilentinc.com/Products/Detail.cfm?Prod=ADEPT2 .

Untar and install products:

    $ tar xzfv digilent.adept.runtime_2.10.2-i686.tar.gz
    $ cd digilent.adept.runtime_2.10.2-i686
    $ sudo ./install.sh
    $ cd ftdi.drivers_1.0.4-i686
    $ sudo ./install.sh
    $ cd ../..

    $ tar xvzf digilent.adept.utilities_2.1.1-i686.tar.gz
    $ cd digilent.adept.utilities_2.1.1-i686
    $ sudo ./install.sh
    $ cd ..


9. How to program a bit-file into FPGA with the Digilent HS cable
-----------------------------------------------------------------

Below is an example given of how to program a Xilinx ML509 FPGA developers
board. NOTE: make sure the Digilent HS cable is connected to the right
JTAG connector on the board (programming the FPGA and not the memory).
So, it should be connected to PC4 JTAG and not to J51 BDM.
Further more, Device 4: XC5VLX110T is the FPGA to program, device 4 in the
JTAG scan chain.

    > djtgcfg enum
    Found 1 device(s)

    Device: JtagHs2
        Product Name:   Digilent JTAG-HS2
        User Name:      JtagHs2
        Serial Number:  210249810909

    > djtgcfg -d JtagHs2 init
    Initializing scan chain...
    Found Device ID: a2ad6093
    Found Device ID: 0a001093
    Found Device ID: 59608093
    Found Device ID: f5059093
    Found Device ID: f5059093

    Found 5 device(s):
        Device 0: XCF32P
        Device 1: XCF32P
        Device 2: XC95144XL
        Device 3: XCCACE
        Device 4: XC5VLX110T

    > djtgcfg -d JtagHs2 prog -i 4 -f <fpga bit file to progam>.bit
    Programming device. Do not touch your board. This may take a few minutes...
    Programming succeeded.

10. Running internal testsuite
------------------------------

There is a set of internal test for ARC and OpenOCD (consisting of just a
single test at the moment of this writing, though...). This testsuite aim is to
catch some issues with OpenOCD, JTAG or hardware. To run test suite: source
`tcl/test/arc.cfg` then run arc_test_run_all procedure, or run tests
individually.

