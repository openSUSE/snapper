#!/bin/bash
#
# Copyright (c) 2013 SUSE
#
# Written by Matthias G. Eckermann <mge@suse.com>
#
# Run this shell script to delete a USER and all data in $HOME
#
# GLOBAL Settings
#

CMD_BTRFS="/sbin/btrfs"
CMD_SNAPPER="/usr/bin/snapper"
CMD_EGREP="grep -E"
CMD_PAM_CONFIG="/usr/sbin/pam-config"
CMD_USERDEL="userdel --remove"
CMD_CHOWN="chown"
#
SNAPPERCFGDIR="/etc/snapper/configs"
HOMEHOME=/home
DRYRUN=1
MYUSER=$1

# Usage
if [ "0$MYUSER" == "0" ]; then
	echo "USAGE: $0 <username>"
	exit 1
fi

# Sanity-Check: is $HOMEHOME a btrfs filesystem
${CMD_BTRFS} filesystem df ${HOMEHOME} 2>&1 > /dev/null
RETVAL=$?
if [ ${RETVAL} != 0 ]; then
	echo "ERROR $0: ${HOMEHOME} is not on btrfs. Exiting ..."
	exit 2
fi

if [ ${DRYRUN} == 0 ] ; then
	# Delete the snapper configuration
	# This deletes $SNAPPERCFGDIR/home_${MYUSER}
	# removes "home_${MYUSER}" from /etc/sysconfig/snapper
	# and deletes all snapshots
	${CMD_SNAPPER} -c home_${MYUSER} delete-config
	# Delete the USER's home subvolume
	${CMD_BTRFS} subvol delete /${HOMEHOME}/${MYUSER}
	# Delete the USER
	${CMD_USERDEL} ${MYUSER}
else
	echo -e "#"
	echo "DRYRUN - no changes submitted"
fi
