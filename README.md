# The HDLC Daemon (HDLCd)
The HDLC Daemon (HDLCd) implement the HDLC protocol to easily talk to devices connected via serial communications.

This package offers the "HDLC Daemon" (HDLCd) that implements the "High-level Data Link Control" protocol (HDLC).
The purpose of this deamon is to easily use serial devices that make use of the HDLC protocol for communication.
Currently it is tailored to the stripped-down flavor of HDLC offered by the s-net(r) sensor tags by the Fraunhofer-Institute
for Integrated Circuits (IIS). The HDLCd itself offers TCP-based connectivity using framing for the control- and user-plane.

This software is intended to be portable and makes use of the boost libraries. It was tested on GNU/Linux (GCC toolchain)
and Microsoft Windows (nuwen MinGW).

## Multiple client-like tools to access the HDLCd for multiple use-cases are available in seperate repositories:
- https://github.com/Strunzdesign/hdlcd-tools
- https://github.com/Strunzdesign/snet-tools
- https://github.com/Strunzdesign/snet-gateway

## Latest stable release of the HDLCd:
- v1.3 from 06.10.2016
  - Works well with s-net(r) BASE release 3.6

## Required libraries and tools:
- GCC, the only tested compiler collection thus far (tested: GCC 4.9.3, GCC 6.1)
- Boost, a platform-independent toolkit for development of C++ applications
- CMake, the build system
- Doxygen, for development
- nuwen MinGW, to compile the software on Microsoft Windows (tested: 13.4, 14.0)

## Documentation
- See online doxygen documentation at http://strunzdesign.github.io/hdlcd/
- Check the change log at https://github.com/Strunzdesign/hdlcd/blob/master/CHANGELOG.md
- Read the specification of the HDLCd access protocol at https://github.com/Strunzdesign/hdlcd/blob/master/doc/protocol.txt
