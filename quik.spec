Summary: Disk bootstrap and installer for Linux/PPC
Name: quik
Version: 2.0
Release: 0e
Copyright: GPL
Group: Base
URL: http://www.ppc.kernel.org/
Packager: Cort Dougan <cort@ppc.kernel.org>
Source: ftp://ftp.ppc.kernel.org/pub/linuxppc/quik/quik-2.0.tar.gz
BuildRoot: /tmp/quik-root
ExclusiveArch: ppc

%description
The quik package provides the functionality necessary for booting a
Linux/PPC PowerMac or CHRP system from disk.  It includes first and second
stage disk bootstrap and a program for installing the first stage bootstrap
on the root disk.

%prep
%setup

%build
make

%install
make install DESTDIR="$RPM_BUILD_ROOT"

%post
leave()
{
	echo '*****************************************************************'
	echo '* The Quik installer script was not able to find a partition    *'
	echo '* to install into.  This may be due to an error or if you are   *'
	echo '* installing onto a drive other than /dev/sda quik errs on the  *'
	echo '* side of caution when installing and doesn''t not write itself  *'
	echo '* out to disk.  If you do want to finish the install you can do *'
	echo '* it by hand with: dd if=/boot/second of=/dev/XXX where XXX is  *'
	echo '* the partition you want to install the bootloader into.        *'
	echo '*****************************************************************'
	exit
}

if [ ! -z "`grep CHRP /proc/cpuinfo`" ] ; then
	echo CHRP system detected.
#	PART=`fdisk -l | fgrep "PPC PReP"`
	PART=`cfdisk -P s | fgrep "PPC PReP"`
	if [ -z "$PART" ] ; then
	    echo '*********************************************************'
	    echo '* You must create a PPC PReP Boot partition (type 0x41) *'
	    echo '* for the CHRP bootloader to be installed.              *'
	    echo '*********************************************************'
	    leave
	fi
	if [ `echo "$PART" | wc -l` != 1 ] ; then
	    echo '**************************************************************'
	    echo '* There are more than 1 PPC PReP Boot partitions (type 0x41) *'
	    echo '* on this system.  Aborting install rather than picking one. *'
	    echo '**************************************************************'
	    leave
	fi
	if [ -z "`echo "$PART" | grep "Boot (80)"`" ] ; then
	    echo '***************************************************************'
	    echo '* The PPC PReP Boot partition (type 0x41) is not marked as    *'
	    echo '* bootable.  You will need to mark this partition as bootable *'
	    echo '* before Quik can be installed onto it.                       *'
	    echo '**************************************************************'
	    leave
	fi
	P=`echo "$PART" | awk '{print $1}'`
	# assume /dev/sda!!!
	if [ "${P}" != "1" ] ; then
	    echo '***************************************************************'
	    echo '* You do not have sda1 setup as the boot partition.  This     *'
	    echo '* will work but Quik will not know where the configuration    *'
	    echo '* file is.	                                                *'
	    echo '***************************************************************'
	fi
	P=/dev/sda${P}
	echo Installing onto $P
	dd if=/boot/second of=$P
fi

%clean
rm -rf $RPM_BUILD_ROOT

%files
/sbin/iquik
/boot/preboot.b
/boot/iquik.b
%config /etc/quik.conf
/usr/man/man5/quik.conf.5
/usr/man/man8/quik.8
/usr/man/man8/bootstrap.8
