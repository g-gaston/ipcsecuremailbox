#!/bin/sh
#
#	setup 4.1 - install a MINIX distribution	
#
# Changes:
#    Aug     2005   robustness checks and beautifications  (Jorrit N. Herder)
#    Jul     2005   extended with autopart and networking  (Ben Gras)
#    Dec 20, 1994   created  (Kees J. Bot)
#						

LOCALRC=/usr/etc/rc.local
MYLOCALRC=/mnt/etc/rc.local
ROOTMB=16
ROOTSECTS="`expr $ROOTMB '*' 1024 '*' 2`"
USRKB="`cat /.usrkb`"
TOTALMB="`expr 3 + $USRKB / 1024 + $ROOTMB`"
ROOTFILES="`cat /.rootfiles`"
USRFILES="`cat /.usrfiles`"
EXTRASRCFILES="`cat /.extrasrcfiles`"
EXTRASRCKB="`cat /.extrasrckb`"

# Install size without extra sources (rounded up)
NOSRCMB="`expr $TOTALMB - $EXTRASRCKB / 1024`"
NOSRCUSRFILES="`expr $USRFILES - $EXTRASRCFILES`"

if [ "$EXTRASRCKB" -lt 1 ]
then	 
	echo "Are you really running from CD?"
	echo "Something wrong with the extra-source-kb on CD."
	exit 1
fi

if [ "$EXTRASRCFILES" -lt 1 ]
then	 
	echo "Are you really running from CD?"
	echo "Something wrong with the extra-source-files estimate on CD."
	exit 1
fi

if [ "$TOTALMB" -lt 1 ]
then	 
	echo "Are you really running from CD?"
	echo "Something wrong with size estimate on CD."
	exit 1
fi

if [ "$ROOTFILES" -lt 1 ]
then	 
	echo "Are you really running from CD?"
	echo "Something wrong with root files count on CD."
	exit 1
fi

if [ "$USRFILES" -lt 1 ]
then	 
	echo "Are you really running from CD?"
	echo "Something wrong with usr files count on CD."
	exit 1
fi

PATH=/bin:/usr/bin
export PATH


usage()
{
    cat >&2 <<'EOF'
Usage:	setup		# Install a skeleton system on the hard disk.
	setup /usr	# Install the rest of the system (binaries or sources).

	# To install from other things then floppies:

	urlget http://... | setup /usr		# Read from a web site.
	urlget ftp://... | setup /usr		# Read from an FTP site.
	mtools copy c0d0p0:... - | setup /usr	# Read from the C: drive.
	dosread c0d0p0 ... | setup /usr		# Likewise if no mtools.
EOF
    exit 1
}

warn() 
{
  echo -e "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b ! $1"
}

# No options.
while getopts '' opt; do usage; done
shift `expr $OPTIND - 1`

if [ ! -f /CD ]
then	echo "Please run setup from the CD, not from a live system."
	exit 1
fi

if [ "$USER" != root ]
then	echo "Please run setup as root."
	exit 1
fi

# Find out what we are running from.
exec 9<&0 </etc/mtab			# Mounted file table.
read thisroot rest			# Current root (/dev/ram or /dev/fd?)
read fdusr rest				# USR (/dev/fd? or /dev/fd?p2)
exec 0<&9 9<&-

# What do we know about ROOT?
case $thisroot:$fdusr in
/dev/ram:/dev/fd0p2)	fdroot=/dev/fd0		# Combined ROOT+USR in drive 0
			;;
/dev/ram:/dev/fd1p2)	fdroot=/dev/fd1		# Combined ROOT+USR in drive 1
			;;
/dev/ram:/dev/fd*)	fdroot=unknown		# ROOT is some other floppy
			;;
/dev/fd*:/dev/fd*)	fdroot=$thisroot	# ROOT is mounted directly
			;;
*)			fdroot=$thisroot	# ?
esac

echo -n "
Welcome to the MINIX 3 setup script.  This script will guide you in setting up
MINIX on your machine.  Please consult the manual for detailed instructions.

Note 1: If the screen blanks, hit CTRL+F3 to select \"software scrolling\".
Note 2: If things go wrong then hit CTRL+C to abort and start over.
Note 3: Default answers, like [y], can simply be chosen by hitting ENTER.
Note 4: If you see a colon (:) then you should hit ENTER to continue.
:"
read ret

# begin Step 1
echo ""
echo " --- Step 1: Select keyboard type --------------------------------------"
echo ""

    echo "What type of keyboard do you have?  You can choose one of:"
    echo ""
    ls -C /usr/lib/keymaps | sed -e 's/\.map//g' -e 's/^/    /'
    echo ""
step1=""
while [ "$step1" != ok ]
do
    echo -n "Keyboard type? [us-std] "; read keymap
    test -n "$keymap" || keymap=us-std
    if loadkeys "/usr/lib/keymaps/$keymap.map" 2>/dev/null 
    then step1=ok 
    else warn "invalid keyboard"
    fi
done
# end Step 1

# begin Step 2
echo ""
echo " --- Step 2: Select your Ethernet chip ---------------------------------"
echo ""

# Ask user about networking
echo "MINIX 3 currently supports the following Ethernet cards. Please choose: "
    echo ""
    echo "0. No Ethernet card (no networking)"
    echo "1. Intel Pro/100"
    echo "2. 3Com 501 or 3Com 509 based card"
    echo "3. Realtek 8139 based card"
    echo "4. Realtek 8029 based card (also emulated by Qemu)"
    echo "5. NE2000, 3com 503 or WD based card (also emulated by Bochs)"
    echo "6. AMD LANCE (also emulated by VMWare)"
    echo "7. Different Ethernet card (no networking)"
    echo ""
    echo "You can always change your mind after the setup."
    echo ""
step2=""
while [ "$step2" != ok ]
do
    eth=""
    echo -n "Ethernet card? [0] "; read eth
    test -z $eth && eth=0
    driver=""
    driverargs=""
    case "$eth" in
        0) step2="ok"; ;;    
	1) step2="ok";	driver=fxp;      ;;
	2) step2="ok";	driver=dpeth;    driverargs="#dpeth_arg='DPETH0=port:irq:memory'";
	   echo ""
           echo "Note: After installing, edit $LOCALRC to the right configuration."
		;;
	3) step2="ok";	driver=rtl8139;  ;;
	4) step2="ok";	driver=dp8390;   driverargs="dp8390_arg='DPETH0=pci'";	;;
	5) step2="ok";	driver=dp8390;   driverargs="dp8390_arg='DPETH0=240:9'"; 
	   echo ""
           echo "Note: After installing, edit $LOCALRC to the right configuration."
           echo " chose option 4, the defaults for emulation by Bochs have been set."
		;;
        6) driver="lance"; driverargs="LANCE0=on"; step2="ok"; ;;    
        7) step2="ok"; ;;    
        *) warn "choose a number"
    esac
done
# end Step 2

# begin Step 3
step3=""
while [ "$step3" != ok ]
do
	echo ""
	echo " --- Step 3: Select binary or source distribution ----------------------"
	echo ""
	echo "You can install MINIX as (B)inary or (S)ource. (B)inary"
	echo "includes only the binary system and basic system sources."
	echo "(S)ource also includes commands sources."
	echo ""
	echo "Please select:"
	echo "  (B)inary install (only basic sources) ($NOSRCMB MB required)"
	echo "  (S)ource install (full install) ($TOTALMB MB required)"
	echo " "
	echo -n "Basic (B)inary or full (S)ource install? [S] "
	read conf
	case "$conf" in
	"") 	step3="ok"; nobigsource="" ;;
	[Ss]*)	step3="ok"; nobigsource="" ;;
	[Bb]*)	step3="ok"; nobigsource="1"; TOTALMB=$NOSRCMB; USRFILES=$NOSRCUSRFILES ;;
	esac
done
# end Step 3

# begin Step 4
step4=""
while [ "$step4" != ok ]
do
	echo ""
	echo " --- Step 4: Create or Select a partition for MINIX 3 -------------------"
	echo ""

    echo "Now you need to create a MINIX 3 partition on your hard disk."
    echo "It has to have $TOTALMB MB at the very least."
    echo "You can also select one that's already there."
    echo " "
    echo "If you have an existing installation, reinstalling will let you"
    echo "keep your current partitioning and subpartitioning, and overwrite"
    echo "everything except your s1 subpartition (/home). If you want to"
    echo "reinstall, select your existing minix partition."
    echo " "
    echo "Unless you are an expert, you are advised to use the automated"
    echo "step-by-step help in setting up."
    echo ""
    ok=""
    while [ "$ok" = "" ]
    do
	    echo -n "Press ENTER for automatic mode, or type 'expert': "
	    read mode
	    if [ -z "$mode" ]; then auto="1"; ok="yes"; fi 
	    if [ "$mode" = expert ]; then auto=""; ok="yes"; fi
	    if [ "$ok" != yes ]; then warn "try again"; fi 
    done

	primary=

	if [ -z "$auto" ]
	then
		# Expert mode
		echo -n "
MINIX needs one primary partition of $TOTALMB MB for a full install,
plus what you want for /home.
The maximum file system currently supported is 4 GB.

If there is no free space on your disk then you have to choose an option:
   (1) Delete one or more partitions
   (2) Allocate an existing partition to MINIX 3
   (3) Exit setup and shrink a partition using a different OS

To make this partition you will be put in the editor \"part\".  Follow the
advice under the '!' key to make a new partition of type MINIX.  Do not
touch an existing partition unless you know precisely what you are doing!
Please note the name of the partition (e.g. c0d0p1, c0d1p3, c1d1p0) you
make.  (See the devices section in usage(8) on MINIX device names.)
:"
		read ret

		while [ -z "$primary" ]
		do
		    part || exit

		    echo -n "
Please finish the name of the primary partition you have created:
(Just type ENTER if you want to rerun \"part\")                   /dev/"
		    read primary
		done
		echo ""
		echo "This is the point of no return.  You have selected to install MINIX"
		echo "on partition /dev/$primary.  Please confirm that you want to use this"
		echo "selection to install MINIX."
		echo ""
		confirmation=""
		while [ -z "$confirmation" -o "$confirmation" != yes -a "$confirmation" != no ]
		do
			echo -n "Are you sure you want to continue? Please enter 'yes' or 'no': "
			read confirmation
			if [ "$confirmation" = yes ]; then step4=ok; fi
		done
		biosdrivename="Actual BIOS device name unknown, due to expert mode."
	else
		if [ "$auto" = "1" ]
		then
			# Automatic mode
			PF="/tmp/pf"
			if autopart -m$TOTALMB -f$PF
			then	if [ -s "$PF" ]
				then
					set `cat $PF`
					bd="$1"
					bdn="$2"
					biosdrivename="Probably, the right command is \"boot $bdn\"."
					if [ -b "/dev/$bd" ]
					then	primary="$bd"
					else	echo "Funny device $bd from autopart."
					fi
				else
					echo "Didn't find output from autopart."
				fi 
			else	echo "Autopart tool failed. Trying again."
			fi

			# Reset at retries and timeouts in case autopart left
			# them messy.
			atnormalize

			if [ -n "$primary" ]; then step4=ok; fi
		fi
	fi

	if [ ! -b "/dev/$primary" ]
	then	echo "/dev/$primary is not a block device."
		step4=""
	else
		devsize="`devsize /dev/$primary`"

		if [ "$devsize" -lt 1 ]
		then	echo "/dev/$primary is a 0-sized device."
			step4=""
		fi
	fi
done	# while step4 != ok
# end Step 4

root=${primary}s0
home=${primary}s1
usr=${primary}s2
umount /dev/$root 2>/dev/null && echo "Unmounted $root for you."
umount /dev/$home 2>/dev/null && echo "Unmounted $home for you."
umount /dev/$usr 2>/dev/null && echo "Unmounted $usr for you."

devsizemb="`expr $devsize / 1024 / 2`"
maxhome="`expr $devsizemb - $TOTALMB - 1`"

if [ "$devsizemb" -lt "$TOTALMB" ]
then	echo "The selected partition ($devsizemb MB) is too small."
	echo "You'll need $TOTALMB MB at least."
	exit 1
fi

if [ "$maxhome" -lt 1 ]
then	echo "Note: you can't have /home with that size partition."
	maxhome=0
fi

TMPMP=/m
mkdir $TMPMP >/dev/null 2>&1

confirm=""

while [ "$confirm" = "" ]
do
	auto=""
	echo ""
echo " --- Step 5: Reinstall choice ------------------------------------------"
	if mount -r /dev/$home $TMPMP >/dev/null 2>&1
	then	umount /dev/$home >/dev/null 2>&1
		echo ""
		echo "You have selected an existing MINIX 3 partition."
		echo "Type F for full installation (to overwrite entire partition)"
		echo "Type R for a reinstallation (existing /home will not be affected)"
		echo ""
		echo -n "(F)ull or (R)einstall? [R] "
		read conf
		case "$conf" in
		"") 	confirm="ok"; auto="r"; ;;
		[Rr]*)	confirm="ok"; auto="r"; ;;
		[Ff]*)	confirm="ok"; auto="" ;;
		esac

	else	echo ""
		echo "No old /home found. Doing full install."
		echo ""
		confirm="ok";
	fi

done

nohome="0"

if [ ! "$auto" = r ]
then	homesize=""
echo ""
echo " --- Step 6: /home configuration ---------------------------------------"
	while [ -z "$homesize" ]
	do

		# 20% of what is left over after /home and /usr
		# are taken.
		defmb="`expr $maxhome / 5`"
		if [ "$defmb" -gt "$maxhome" ]
		then
			defmb=$maxhome
		fi

		echo ""
		echo "MINIX will take up $TOTALMB MB, without /home."
		echo -n "How big do you want your /home to be in MB (0-$maxhome) ? [$defmb] "
		read homesize
		if [ "$homesize" = "" ] ; then homesize=$defmb; fi
		if [ "$homesize" -lt 1 ]
		then	nohome=1
			homesize=0
		else
			if [ "`expr $TOTALMB + $homesize`" -gt "$devsizemb" ]
			then	echo "That won't fit!"
			else
				echo -n "$homesize MB Ok? [Y] "
				read ok
				[ "$ok" = Y -o "$ok" = y -o "$ok" = "" ] || homesize=""
			fi
		fi
		echo ""
	done
	# Homesize in sectors
	homemb="$homesize MB"
	homesize="`expr $homesize '*' 1024 '*' 2`"
else
	# Homesize unchanged (reinstall)
	homesize=exist
	homemb="current size"
fi

blockdefault=4

if [ ! "$auto" = "r" ]
then
	echo ""
echo " --- Step 7: Select a block size ---------------------------------------"
	echo ""
	
	echo "The maximum (and default) file system block size is $blockdefault KB."
	echo "For a small disk or small RAM you may want 1 or 2 KB blocks."
	echo ""
	
	while [ -z "$blocksize" ]
	do	
		echo -n "Block size in kilobytes? [$blockdefault] "; read blocksize
		test -z "$blocksize" && blocksize=$blockdefault
		if [ "$blocksize" -ne 1 -a "$blocksize" -ne 2 -a "$blocksize" -ne $blockdefault ]
		then	
			warn "1, 2 or 4 please"
			blocksize=""
		fi
	done
else
	blocksize=$blockdefault
fi

blocksizebytes="`expr $blocksize '*' 1024`"

echo "
You have selected to (re)install MINIX 3 in the partition /dev/$primary.
The following subpartitions are now being created on /dev/$primary:

    Root subpartition:	/dev/$root	$ROOTMB MB
    /home subpartition:	/dev/$home	$homemb
    /usr subpartition:	/dev/$usr	rest of $primary
"
					# Secondary master bootstrap.
installboot -m /dev/$primary /usr/mdec/masterboot >/dev/null || exit
					# Partition the primary.
partition /dev/$primary 1 81:${ROOTSECTS}* 81:$homesize 81:0+ > /dev/null || exit

echo "Creating /dev/$root for / .."
mkfs -B $blocksizebytes /dev/$root || exit

if [ "$nohome" = 0 ]
then
	if [ ! "$auto" = r ]
	then	echo "Creating /dev/$home for /home .."
		mkfs -B $blocksizebytes /dev/$home || exit
	fi
else	echo "Skipping /home"
fi

echo "Creating /dev/$usr for /usr .."
mkfs -B $blocksizebytes /dev/$usr || exit

echo ""
echo " --- Step 8: Wait for bad block detection ------------------------------"
echo ""
echo "Scanning disk for bad blocks.  Hit CTRL+C to stop the scan if you are"
echo "sure that there can not be any bad blocks.  Otherwise just wait."

trap ': nothing;echo' 2

echo ""
echo "Scanning /dev/$root for bad blocks:"
readall -b /dev/$root | sh

if [ "$nohome" = 0 ]
then
	echo ""
	echo "Scanning /dev/$home for bad blocks:"
	readall -b /dev/$home | sh
	fshome="home=/dev/$home"
else	fshome=""
fi

echo ""
echo "Scanning /dev/$usr for bad blocks:"
readall -b /dev/$usr | sh

trap 2

echo ""
echo " --- Step 9: Wait for files to be copied -------------------------------"
echo ""
echo "This is the final step of the MINIX 3 setup.  All files will be now be"
echo "copied to your hard disk.  This may take a while."
echo ""

mount /dev/$usr /mnt >/dev/null || exit		# Mount the intended /usr.

(cd /usr || exit 1
 if [ "$nobigsource" = 1 ]
 then	list="`ls | fgrep -v src.`"
 else	list="`ls`"
 fi
 for d in $list
 do	
 	cpdir -v $d /mnt/$d
 done
) | progressbar "$USRFILES" || exit	# Copy the usr floppy.

if [ -d /mnt/src.commands ]
then	mv /mnt/src.commands /mnt/src/commands
fi

if [ -d /mnt/src.contrib ]
then	mv /mnt/src.contrib /mnt/src/contrib
fi
					# Set inet.conf to correct driver
if [ -n "$driver" ]
then	echo "$driverargs" >$MYLOCALRC
	disable=""
else	disable="disable=inet;"
fi
					# Set inet.conf to correct driver
if [ -n "$driver" ]
then	echo "$driverargs" >$MYLOCALRC
	disable=""
else	disable="disable=inet;"
fi

umount /dev/$usr >/dev/null || exit		# Unmount the intended /usr.
mount /dev/$root /mnt >/dev/null || exit

# Running from the installation CD.
cpdir -vx / /mnt | progressbar "$ROOTFILES" || exit	

if [ -n "$driver" ]
then	echo "eth0 $driver 0 { default; };" >/mnt/etc/inet.conf
fi

# CD remnants that aren't for the installed system
rm /mnt/etc/issue /mnt/CD /mnt/.* 2>/dev/null
					# Change /etc/fstab. (No swap.)
					# ${swap:+swap=/dev/$swap}
echo >/mnt/etc/fstab "\
# Poor man's File System Table.

root=/dev/$root
usr=/dev/$usr
$fshome"

					# National keyboard map.
test -n "$keymap" && cp -p "/usr/lib/keymaps/$keymap.map" /mnt/etc/keymap

umount /dev/$root >/dev/null || exit	# Unmount the new root.
mount /dev/$usr /mnt >/dev/null || exit

# Make bootable.
installboot -d /dev/$root /usr/mdec/bootblock /boot/boot >/dev/null || exit

edparams /dev/$root "rootdev=$root; ramimagedev=$root; $disable; minix(1,Start MINIX 3) { unset image; boot; }; smallminix(2,Start Small MINIX 3) { image=/boot/image_small; ramsize=0; boot; }; main() { echo By default, MINIX 3 will automatically load in 3 seconds.; echo Press ESC to enter the monitor for special configuration.; trap 3000 boot; menu; }; save" || exit
pfile="/mnt/src/tools/fdbootparams"
echo "rootdev=$root; ramimagedev=$root; $disable; save" >$pfile

sync

bios="`echo $primary | sed 's/d./dX/g'`"

if [ ! "$auto" = "r" ]
then	if mount /dev/$home /home 2>/dev/null
	then	for u in bin ast
		do	if mkdir ~$u
			then	echo " * Creating home directory for $u in ~$u"
				cpdir /usr/ast ~$u
				chown -R $u:operator ~$u
			else	echo " * Couldn't create ~$u"
			fi
		done
		umount /dev/$home
	fi
fi

echo "
Please type 'shutdown' to exit MINIX 3 and enter the boot monitor. At
the boot monitor prompt, type 'boot $bios', where X is the bios drive
number of the drive you installed on, to try your new MINIX system.
$biosdrivename

This ends the MINIX 3 setup script.  After booting your newly set up system,
you can run the test suites as indicated in the setup manual.  You also 
may want to take care of local configuration, such as securing your system
with a password.  Please consult the usage manual for more information. 

"

