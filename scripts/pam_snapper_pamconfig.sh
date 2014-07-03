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
#
DEBUG=1
DRYRUN=1
MODIFYPAMCONFIG=1

# Usage

function check_pamconfig () {
	if [ -x /lib/security/pam_snapper.so -o -x /lib64/security/pam_snapper.so ]; then
		${CMD_EGREP} pam_snapper.so /etc/pam.d/common-session 2>&1 > /dev/null
		RETVAL=$?
		if [ ${RETVAL} != 0 ]; then
			MODIFYPAMCONFIG=1
		else
			MODIFYPAMCONFIG=0
		fi
	else
		# echo "Please make sure to have pam_snapper.so installed with executable rights"
		MODIFYPAMCONFIG=0
	fi
}

function modifypamconfig () {
	if [ ${MODIFYPAMCONFIG} != 0 ]; then
		echo "Modifying PAM configuration for pam_snapper"
		echo -e "session optional\tpam_snapper.so" >> /etc/pam.d/common-session
	fi
}

check_pamconfig

if [ ${DEBUG} != 0 ]; then
	echo -e "DEBUG                = "${DEBUG}
	echo -e "DRYRUN               = "${DRYRUN}
	echo -e "#"
	echo -e "MODIFYPAMCONFIG      = "${MODIFYPAMCONFIG}
fi

if [ ${DRYRUN} == 0 ] ; then
	modifypamconfig
else
	echo -e "#"
	echo "DRYRUN - no changes submitted"
fi
