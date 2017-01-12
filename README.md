ISO-9660 (CDFS) for the BBC Micro
=================================

Allows the BBC Micro to read CD-ROMs, and control certain other drive functions, with an IDE CD-ROM drive attached to a [16-bit IDE interface](http://mdfs.net/Info/Comp/BBC/IDE/16bit/) as described by J.G.Harston.

The [ASxxxx cross assemblers](http://shop-pdp.net/ashtml/asxxxx.htm) and [SRecord](http://srecord.sourceforge.net/) are required to build. Just type `make` and you should get `iso9660.rom` which can be burned into a 27128 or similar EPROM.
