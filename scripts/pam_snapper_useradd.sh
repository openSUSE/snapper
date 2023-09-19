#!/bin/bash
#
# Copyright (c) 2013 SUSE
#
# Written by Matthias G. Eckermann <mge@suse.com>
#
# Run this shell script to create a USER
#
# GLOBAL Settings
#

CMD_BTRFS="/sbin/btrfs"
CMD_SNAPPER="/usr/bin/snapper"
CMD_EGREP="grep -E"
CMD_PAM_CONFIG="/usr/sbin/pam-config"
CMD_USERADD="useradd --no-create-home"
CMD_CHOWN="chown"
CMD_CHMOD="chmod"
CMD_CPA="cp -a"
#
SNAPPERCFGDIR="/etc/snapper/configs"
HOMEHOME=/home
SKELLDIR=/etc/skel
DRYRUN=1
MYUSER=$1
MYGROUP=$2
if [ ".${MYGROUP}" == "." ] ; then MYGROUP="users"; fi

# Usage
if [ "0$MYUSER" == "0" ]; then
	echo "USAGE: $0 <username> [group]"
	exit 1
fi

# Sanity-Check: ist $HOMEHOME a btrfs filesystem
${CMD_BTRFS} filesystem df ${HOMEHOME} 2>&1 > /dev/null
RETVAL=$?
if [ ${RETVAL} != 0 ]; then
	echo "ERROR $0: ${HOMEHOME} is not on btrfs. Exiting ..."
	exit 2
fi

if [ ${DRYRUN} == 0 ] ; then
	# Create subvolume for USER
	${CMD_BTRFS} subvol create ${HOMEHOME}/${MYUSER}
	# Create snapper config for USER
	${CMD_SNAPPER} -c home_${MYUSER} create-config ${HOMEHOME}/${MYUSER}
	${CMD_SNAPPER} -c home_${MYUSER} set-config ALLOW_USERS=${MYUSER}
	# Create USER
	${CMD_USERADD} ${MYUSER}
	# Give USER skeleton files
	${CMD_CPA} ${SKELLDIR}/. ${HOMEHOME}/${MYUSER}
	# yast users add username=${MYUSER} home=/home/${MYUSER} password=""
	# !! IMPORTANT !!
	# chown USER's home directory
	${CMD_CHOWN} ${MYUSER}:${MYGROUP} ${HOMEHOME}/${MYUSER}
	${CMD_CHMOD} 755 ${HOMEHOME}/${MYUSER}/.snapshots
else
	echo -e "#"
	echo "DRYRUN - no changes submitted"
fi
