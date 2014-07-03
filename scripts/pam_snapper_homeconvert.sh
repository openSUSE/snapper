#!/bin/bash
#
# Copyright (c) 2013 SUSE
#
# Written by Matthias G. Eckermann <mge@suse.com>
#
# Run this shell script for every user
#
# GLOBAL Settings
#

CMD_BTRFS="/sbin/btrfs"
CMD_SNAPPER="/usr/bin/snapper"
CMD_EGREP="grep -E"
CMD_PAM_CONFIG="/usr/sbin/pam-config"
CMD_SED="sed"
CMD_USERADD="useradd -m"
CMD_USERDEL="userdel -r"
CMD_CHOWN="chown"
CMD_CHMOD="chmod"
#
SNAPPERCFGDIR="/etc/snapper/configs"
HOMEHOME=/home
DEBUG=1
DRYRUN=1
MYUSER=$1
MYGROUP=$2
if [ ".${MYGROUP}" == "." ] ; then MYGROUP="users"; fi
AUTOCONVERT=1
CREATEUSER=0
CONVERTUSERDIR=0
CREATEUSERSUBVOLUME=1
DELETEUSERSNAPVOLUME=0
CREATESNAPPERCONFIG=1

# Usage

if [ "0$MYUSER" == "0" ]; then
	echo "USAGE: $0 <username> [group]"
	exit 1
fi

function is_btrfs_homehome () {
	# Sanity-Check: ist $HOMEHOME a btrfs filesystem
	${CMD_BTRFS} filesystem df ${HOMEHOME} 2>&1 > /dev/null
	RETVAL=$?
	if [ ${RETVAL} != 0 ]; then
		echo "ERROR $0: ${HOMEHOME} is not on btrfs. Exiting ..."
		exit 2
	fi
}


function user_exists () {
	# Does ${MYUSER} exist?
	${CMD_EGREP} "^${MYUSER}:" /etc/passwd 2>&1 > /dev/null
	RETVAL=$?
	if [ ${RETVAL} != 0 ]; then
		CREATEUSER=1
	fi
}

function check_userdir () {
	#
	if [ -f ${SNAPPERCFGDIR}/home_${MYUSER} ]; then
		CREATESNAPPERCONFIG=0
	fi
	# Does Directory exist?
	if [ -e ${HOMEHOME}/${MYUSER} ] ; then
		if [ -d ${HOMEHOME}/${MYUSER} ] ; then
			# echo "directory exists"
			if [ $( stat -c "%i" ${HOMEHOME}/${MYUSER} ) == 256 ]; then
				# echo "${HOMEHOME}/${MYUSER} is a subvolume"
				CREATEUSERSUBVOLUME=0
			else
				# echo "${HOMEHOME}/${MYUSER} is not a subvolume"
				CONVERTUSERDIR=1
			fi
		else
			# echo "${HOMEHOME}/${MYUSER} is a file"
			CONVERTUSERDIR=2
		fi
	fi
	if [ -d ${HOMEHOME}/${MYUSER}/.snapshots ] ; then
		if [ $( stat -c "%i" ${HOMEHOME}/${MYUSER}/.snapshots ) == 256 ]; then
			# echo "Subvolume $HOMEHOME/${MYUSER}/.snapshots already exits."
			if [ -f ${SNAPPERCFGDIR}/home_${MYUSER} ]; then
				# nothing to do
				CREATESNAPPERCONFIG=0
			else
				# that's confusing: why do we have the
				# .snapshots subvolume, but not a config?
				DELETEUSERSNAPVOLUME=1
				CREATESNAPPERCONFIG=1
			fi
		else
			# that's confusing: why do we have a .snapshots directory?
			# Probably a wrongly configured "rsync"?
			DELETEUSERSNAPVOLUME=2
		fi
	fi
}

function createusersubvolume () {
	if [ ${CREATEUSERSUBVOLUME} != 0 ]; then
		echo "Create Subvolume ${HOMEHOME}/${MYUSER} for user ${MYUSER}"
		${CMD_BTRFS} subvol create ${HOMEHOME}/${MYUSER}
	fi
}

function convertuserdir () {
	if [ ${CONVERTUSERDIR} != 0 ] && [ ${AUTOCONVERT} != 0 ] ; then
		echo "Convert ${HOMEHOME}/${MYUSER} to btrfs subvolume"
		mv ${HOMEHOME}/${MYUSER} ${HOMEHOME}/${MYUSER}.tmp
		createusersubvolume
		if [ ${CONVERTUSERDIR} == 1 ] ; then
			mv ${HOMEHOME}/${MYUSER}.tmp/* ${HOMEHOME}/${MYUSER}.tmp/.??* ${HOMEHOME}/${MYUSER}/
			rmdir ${HOMEHOME}/${MYUSER}.tmp
		else 
			if [ ${CONVERTUSERDIR} == 2 ] ; then
				mv ${HOMEHOME}/${MYUSER}.tmp ${HOMEHOME}/${MYUSER}/${MYUSER}.file
			fi
		fi
	fi
}

function createsnapperconfig () {
	if [ ${CREATESNAPPERCONFIG} != 0 ]; then
		if [ ${DELETEUSERSNAPVOLUME} == 1 ]; then
			echo "Delete orphaned subvolume ${HOMEHOME}/${MYUSER}/.snapshots"
			${CMD_BTRFS} subvol delete ${HOMEHOME}/${MYUSER}/.snapshots
		else 
			if [ ${DELETEUSERSNAPVOLUME} == 2 ]; then
				echo "Delete orphaned directory ${HOMEHOME}/${MYUSER}/.snapshots"
				rm -rf ${HOMEHOME}/${MYUSER}/.snapshots
			fi
		fi
		echo "Create snapper configuration for user ${MYUSER}"
		${CMD_SNAPPER} -c home_${MYUSER} create-config ${HOMEHOME}/${MYUSER}
		${CMD_SED} -i -e "s/ALLOW_USERS=\"\"/ALLOW_USERS=\"${MYUSER}\"/g" ${SNAPPERCFGDIR}/home_${MYUSER}
	fi
	${CMD_CHMOD} 755 ${HOMEHOME}/${MYUSER}/.snapshots
}

function createuser () {
	if [ ${CREATEUSER} != 0 ]; then
		echo "useradd -m ${MYUSER}"
		# yast users add username=${MYUSER} home=/home/${MYUSER} password=""
		${CMD_USERADD} ${MYUSER}
	fi
	${CMD_CHOWN} ${MYUSER}.${MYGROUP} ${HOMEHOME}/${MYUSER}
}

is_btrfs_homehome
user_exists
check_userdir

if [ ${DEBUG} != 0 ]; then
	echo -e "DEBUG                = "${DEBUG}
	echo -e "DRYRUN               = "${DRYRUN}
	echo -e "AUTOCONVERT          = "${AUTOCONVERT}
	echo -e "#"
	echo -e "HOMEHOME             = "${HOMEHOME}
	echo -e "MYUSER               = "${MYUSER}
	echo -e "MYGROUP              = "${MYGROUP}
	echo -e "CREATEUSER           = "${CREATEUSER}
	echo -e "CONVERTUSERDIR       = "${CONVERTUSERDIR}
	echo -e "DELETEUSERSNAPVOLUME = "${DELETEUSERSNAPVOLUME}
	echo -e "CREATEUSERSUBVOLUME  = "${CREATEUSERSUBVOLUME}
	echo -e "CREATESNAPPERCONFIG  = "${CREATESNAPPERCONFIG}
fi

if [ ${DRYRUN} == 0 ] ; then
	convertuserdir
	createusersubvolume
	createsnapperconfig
	createuser
else
	echo -e "#"
	echo "DRYRUN - no changes submitted"
fi
