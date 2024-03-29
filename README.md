iQUIK - the new old PowerMac bootloader
=======================================

iQUIK is an effort to maintain the old QUIK bootloader,
and is the only way to boot a recent Linux kernel on an
OldWorld, at least known to me.

OldWorld PowerMacs are all PCI-based macs with OpenFirmware,
but without built-in on-board USB. The OpenFirmware versions
are < 3.0.

[ As a proud owner of a '97 PB 3400c. iQUIK has been my
  painful journey towards running first 2.4 with initrd,
  and then 2.6, 3.x and now 4.x kernels ]

iQUIK differs from the plain-jane unmaintained QUIK in
a number of important ways:

1) Up to date.
   * Boots all kernels from 2.2 up.
   * Initrd support from config file and CLI.
2) Easier to use, harder to break.
   * Doesn't use block maps or the first stage loader,
     which would break if you moved anything around on FS
   * Can boot from floppy (i.e. rescue disk).
   * Can boot from anything via partition zero
     (i.e. :0/%BOOT).
   * Can boot as \\:TBXI or ELF on NewWorlds.
   * Symlink support, listing files, etc.
   * Preboot script support for rescue disks, install media.
3) Better firmware support.
   * Works around OF 1.0.5 bugs
   * Works around OF 2.0(.X) setprop bugs
     (i.e. the initrd not found bug)
   * Works around 3400c hangs with media bay without CDROM.
   * SMP support.
   * NewWorld disk access support
4) More maintainable
   * Doesn't need libext2fs
   * Builds with a recent toolchain (gcc version 8.3.0 (Debian 8.3.0-2)
   * Cross compile support (on any endianness).

Support
=======

Meant for OldWorlds in general, it has only been tested (by me) on:
1) PowerBook 3400c (OF 2.0.1, 603e)
2) PowerBook Wallstreet II (PDQ) (OF 2.0.1, G3)
3) UMAX StormSurge (OF 1.0.5, 604e), including SMP.
4) Performa 6400 (OF 2.0, 603ev)
5) QEMU (OpenBIOS 1.1 r1272+).
6) PowerBook 12'' 1.5GHz (OF 3, G4)
7) iBook 12" (OF 3, G4)

It would be nice to test with 1.1.2 and 2.4. See distrib/ for docs on getting
to run on certain hardware (including qemu and NewWorlds, although iQUIK
is not meant for NewWorlds).

Tested with 4.7 kernels and older.

- Sarge (yep! 2.4 kernel only).
- Etch (yep!)
- Lenny (yep!)
- Squeeze (yep!)
- Wheezy (yep! but not on my UMAX, and probably a few other machines :-/)
- Jessie (yep! but kernel has no MACE eth, using custom kernel)

Documentation under distrib/ is only for machines tested on. Apple
NVRAMRC patches under distrib/Apple_nvramrc may be necessary for other
machines. The patches are a direct mirror from
http://www.opensource.apple.com/source/system_cmds/system_cmds-175/nvram.tproj/nvram/

Documentation and functionality patches welcome!


Prebuilts
=========

I've checked-in the current prebuilts for iQUIK under distrib/.
* 'iquik.b' is the iQUIK boot code
* 'iquik.x64' is the installer, built on x64 Debian 10.6
* 'iquik.conf' is pretty self-explanatory. It doesn't try to be
   exhaustive.
* 'floppy.img' can be written out to a 1.44MB and booted via 'boot fd:0'.

[ The 'floppy-cfg.img' is an example image with a config file. It can
  be booted with 'boot fd:0 fd:3/iquik.conf'. If you're wondering
  how to create partitions either use loopback or a USB floppy, which
  appears as a SCSI device ]


Building
========

After fetching the sources.

$ make && make install

If building on non-PowerPC hosts:

$ CROSS=powerpc-linux-gnu- make

You can also build with any of these options:
$ make CONFIG_TINY=1    ...will create a smaller executable, for systems
                        where booting via partition zero seems to fail.
$ make CONFIG_MEMTEST=1 ...will add a !memtest command for simple testing.

The boot flow
=============

OF will boot the device listed under the 'boot-device' environment variable.
In our case it has to be the "partition zero" of whatever device you installed
iQUIK onto.

iQUIK will then try to locate the iquik.conf file using the path in
'boot-file'. It will also, on failure, try /etc/iquik.conf, /boot/iquik.conf
and /iquik.conf, as well as the quik.conf variants.

For me:
boot-device     ata0/ata-disk@0:0
boot-file       ata0/ata-disk@0:4

...the later corresponds to /etc/quik.conf on /dev/hda4 on PB 3400c.

Another example:
boot-device     upper/pccard45,401@0:0
boot-file       upper/pccard45,401@0:3/installer/iquik.conf 

iQUIK will then parse quik.conf similar to Yaboot, the old QUIK, MILO, LILO, etc.

You get the typical 'boot:' prompt. Tab shows the list of images defined
in your quik.conf, if any.

You can also type in custom image locations, thus booting any arbitrary
kernel with any initrd and any parameters, e.g.:
       [device:partno]/vmlinux
       [device:partno]/vmlinux -- kernel arguments
       [device:partno]/vmlinux [device:partno]/initrd kernel arguments


Installing
==========

NOTE: These instructions do not apply to a NewWorld. See
      distrib/NewWorld.

Whatever medium you want to install iQUIK on, it has to:
1) Be mac partitioned
2) Contain an Apple_Bootstrap partition.

NOTE: iQUIK does not support any other way of booting on OldWorlds.

Example (for floppy):
root@3400c:~# mac-fdisk /dev/fd0
/dev/fd0
Command (? for help): p
/dev/fd0
        #                    type name                length   base    ( size )  system
/dev/fd01     Apple_partition_map Apple                   63 @ 1       ( 31.5k)  Partition map
/dev/fd02         Apple_Bootstrap bootstrap             1600 @ 64      (800.0k)  NewWorld bootblock
/dev/fd03              Apple_Free Extra                 1216 @ 1664    (608.0k)  Free space

Block size=512, Number of Blocks=2880
DeviceType=0x0, DeviceId=0x0

To do this I've used the 'i' command to initialize the partition table,
and the 'b' command to create the new 800K bootstrap partition.

NOTE: It's possible to run into OF problems booting large disks. It's
probably better to create a /boot partition to contain vmlinux, initrd,
and iquik.conf right after the bootstrap partition.

Assuming the physical disk you've prepared as above contains your
root partition (with /boot), running 

$ iquik

...should be sufficient. 'iquik -v' for verbosity. 'iquik -T -v'
if you just want to see what iquik would have done. The tool will
not do anything if you don't have an Apple_Bootstrap partition.

If you want to install on some other media, use the '-d' flag:

$ iquik -d /dev/fd0

If you want to use an alternate iquik.b (instead of the one in /boot),
use the '-b' flag:

$ iquik -b /my/hacked/iquik.b

If you want iquik to evaluate a forth script before looking for
the boot-file:

$ iquik -p /my/special/preboot/script.of

NOTE: the 'iquik' tool doesn't set the NVRAM variables itself. You
need to do that yourself.

Example:

[ I installed to /dev/hda ]

root@3400c:~# nvsetenv boot-device "ata0/ata-disk@0:0"

[ Configuration is on /etc/quik.conf on /dev/hda4 ]

root@3400c:~# nvsetenv boot-file "ata0/ata-disk@0:4"


Other stuff
===========

From OF prompt, you can boot it several ways.

0 > boot

[ assuming boot-device and boot-file was correctly set ]

0 > boot fd:0

[ assuming boot-file was correctly set ]

0 > boot fd:0 -- Linux26

[ assuming boot-file was correctly set, iQUIK will immediately
  boot the image specified by the label Linux26 ]

0 > boot fd:0 ata0/ata-disk@0:4 Linux24

[ tells iQUIK to load image Linux24 using the quik.conf file
  at ata0/ata-disk@0:4/etc/quik.conf ]

0 > boot upper/pccard45,401@0:0 upper/pccard45,401@0:4/installer/quik.conf

[ tells iQUIK to boot from a CF card in top PCMCIA slot with a custom
  quik.conf location on /dev/hdc4 ]

Once you're at the boot prompt:

boot: !dev device:part

[ change the default device/partition - the default device/partition
  are the ones used to load quik.conf ]

boot: !ls

[ list files on the default device/partition ]

boot: !ls /boot

[ list files under /boot on default device/partition ]

boot: !ls upper/pccard45,401@0:4/installer

[ list files under /install on partition 4 of CF card in top PCMCIA slot ]

boot: ata0/ata-disk@0:4/boot/vmlinux /boot/initrd.img root=/dev/sda4

[ boot kernel with initrd and boot options ]

boot: ata0/ata-disk@0:4/boot/vmlinux ata0/ata-disk@0:4/boot/initrd.img root=/dev/sda4

[ boot kernel with initrd and boot options ]

boot: ata0/ata-disk@0:4/boot/vmlinux -- root=/dev/sda4

[ boot kernel with boot options ]

boot: !cat /quik.conf

[ show contents of /quik.conf on default device ]

boot: !of 1 1 + .

[ interpret Forth via OF ]

boot: !halt

[ enters OF... type 'go' to get back ]

boot: !debug

[ will pause at important places during boot-up so you can track progress ]

boot: !prom_flags

[ will list current prom.c behavior flags, advanced debugging ]

boot: !prom_flags 0xf00f

[ will change the current prom.c behavior flags, advanced debugging ]

boot: !memtest base size

[ a rudimentary memory test, assuming iquik is built with support for it ]

boot: !old

[ switch to allow booting pre-2.4 kernels, akin to old-kernel conf file flag ]


quik.conf differences
=====================

* Symlinks are supported
* initrd-size argument is unnecessary and unused.
* initrd can contain OF device:part
* forth eval image names via $ not supported (it was broken anyway).


Future development
==================

The next items I'll probably look at, are (in no order):

1) FAT and ISO9660 support.
2) More uncruftification.
3) Allow specifying a whole disk file system (need partitions today).
4) Build an XCOFF variant for loading from iso9660, enet, etc.
5) Check model and apply nvramrc patches
6) BSD kernel support.

Contact Info
============

Andrei Warkentin (andreiw@mm.st)
