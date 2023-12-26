# CD-ROM Filing System for the BBC Micro

Allows the BBC Micro to read ISO-9660 formatted CD-ROMs, and control certain other drive functions, with an IDE CD-ROM drive attached to a [16-bit IDE interface](http://mdfs.net/Info/Comp/BBC/IDE/16bit/) as described by J.G.Harston.

[cc65](https://cc65.github.io/) is required to build- type `make` and you should get `cdfs.rom`.

## Experimental/incomplete

* I've mostly implemented `OSWORD &24-&27` to interface with ATAPI devices, similarly to VFS's `&60-&63` and ADFS's `&70-&73`.
* The filing system layer is not working yet.
* There are some optional test commands assuming a controller at `&FC40` with a CD-ROM drive in the slave position, which I have considered as drive 1 on the BBC.
* I'm developing this with [BeebEm (with host IDE passthrough)](https://github.com/davidknoll/beebem-windows/tree/idepassthru) on VMware Fusion on an Intel Mac.
* It uses zero page locations that it shouldn't, so may not coexist with some applications or languages other than BASIC. (See also [bbctestrom](https://github.com/davidknoll/bbctestrom). Ideas on fixing this while using `cc65` may be welcome.)
* It needs to be loaded into writable sideways RAM, not an actual ROM.
