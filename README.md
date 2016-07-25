# hdlc-tools
The hdlc-tools implement the HDLC protocol to easily talk to devices connected via serial communications

This package offers the system deamon "hdlcd" that implements the "high-Level data link" protocol. The purpose of
this deamon is to easily use serial devices that make use of the HDLC protocol for communication.

The hdlcd itself offers simple TCP-based connectivity using a simple command- and framing structure.

Current state:
- Since 25.07.2016 the master branch is considered stable to be used for the s-net(r) devices of the Fraunhofer-Institute for Integrated Circuits IIS
- The hdlc-vanilla branch containing an early development stage regarding an implementation of HDLC that conforms to the standard is not ready yet

See online doxygen documentation at http://strunzdesign.github.io/hdlc-tools/
