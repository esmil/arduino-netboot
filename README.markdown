arduino-netboot
===============

About
-----

This is a TFTP bootloader for the [Arduino Ethernet][1],
or Duemilanove or Uno with an [Ethernet Shield][2].

It makes it possible to program your Arduino over ethernet.

When the Arduino is powered on this bootloader will use [BOOTP][3] to get its
own IP address, the address of a [TFTP][4] server and the program to download.
It then proceeds to download the file and program the internal
flash memory of the chip before executing the program.

However, the bootloader only actually programs those flash pages which are
different from the downloaded file. This means you can leave your program on
the TFTP server and not worry that the internal flash is rewritten every time
the Arduino is powered on. The bootloader will just download the file and
verify that this is what's already stored on the chip.

Updating your program on the Arduino is very simple. Just update the
file on the TFTP server and press the reset button on your Arduino.
It is even possible to program the Arduino to reset itself, so the
bootloader will be run again to reprogram the Arduino.

[1]: http://arduino.cc/en/Main/ArduinoBoardEthernet
[2]: http://arduino.cc/en/Main/ArduinoEthernetShield
[3]: http://en.wikipedia.org/wiki/Bootstrap_Protocol
[4]: http://en.wikipedia.org/wiki/Trivial_File_Transfer_Protocol

Usage
-----

To use this bootloader you'll need 3 things:

1. A BOOTP server configured to assign your Arduino an IP address and forward
   it to a TFTP server.

2. A TFTP server.

3. A way to program your Arduino with a new bootloader and configure its fuses
   for a 2k bootloader.

Most, if not all, DHCP servers also work as BOOTP servers since DHCP is in
fact just an extension to the older BOOTP protocol.
[dnsmasq][5] is a nifty program with many functions including a
BOOTP/DHCP server and a TFTP server.
By default the MAC address of the Arduino will be set to 00:08:DC:00:00:4F.

For the 3rd point you'll need an ISP programmer. In a pinch you can program
one Arduino work as an ISP programmer to program another Arduino.
A prebuilt Intex HEX file with the bootloader is available from the download
section [here][6]. If you're not afraid the command line or Makefiles please
read the next section for an easy way to flash your Arduino.

[5]: http://thekelleys.org.uk/dnsmasq/doc.html
[6]: https://github.com/downloads/esmil/arduino-netboot/netboot.hex

Hacking
-------

Right now the files in this repo assume you're using Linux, OSX or some
other \*NIX variant, and have AVR GCC (and the rest of the AVR toolchain)
in your path.

Use the following commands to check out the files from github

    $ git clone git://github.com/esmil/arduino-netboot.git
    $ cd arduino netboot
    $ git submodule update --init

Now you should have a Makefile with a few targets described below.

#### $ make
This builds a debug version as a normal program. It'll print all it
does to the serial port, but won't actually program any memory.
It just says "Flashing page 0xXXXX" instead.

#### $ make bootloader
This builds the actual bootloader without debugging and places it
at the right offset into the flash. You'll need to do `make clean`
between building the debug version and this version.

#### $ make upload
Use avrdude to flash the Arduino with the last build version of the
bootloader. By default it'll use the normal Arduino bootloader method,
but you can do `make PROG=avrispmkII upload` to make it use AVR's ISP
programmer.

#### $ make fuses
Set the fuses for a 2k bootloader. Again, you probably want to use
`make PROG=avrispmkII fuses` (or fix the Makefile for whatever
programmer you use).

#### $ make hello\_world.bin
This builds a small hello world program and produces a file suitable
for the bootloader. Basically it just gives "-O binary" instead of
"-O ihex" to avr-objcopy.

#### $ sudo ./dnsmasq
Lastly this command runs a small script which just starts a dnsmasq
instance with a lot of debugging output on eth0 and points the
bootloader to download the hello\_world.bin program.
Don't worry. The bootloader is also tested on a a network with a FreeBSD
server running isc-dhcpd and the standard tftp daemon, so it shouldn't be
specific to dnsmasq.

License
-------

arduino-netboot is free software. It is distributed under the terms of
the [GNU General Public License][7]

[7]: http://www.fsf.org/licensing/licenses/gpl.html

Contact
-------

Please send bug reports, patches, feature requests, praise and general
gossip to me, Emil Renner Berthing <esmil@mailme.dk>.
