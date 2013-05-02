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
CMD_PAM_CONFIG="pamconfig"
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
	if [ -x /usr/bin/pam_snapper.sh ]; then
		${CMD_EGREP} pam_snapper.so /etc/pam.d/common-session 2>&1 > /dev/null
		RETVAL=$?
		if [ ${RETVAL} != 0 ]; then
			MODIFYPAMCONFIG=1
		else
			MODIFYPAMCONFIG=0
		fi
	else
		# echo "Please make sure to have /usr/bin/pam_snapper.sh installed with executable rights"
		MODIFYPAMCONFIG=0
	fi
}

function modifypamconfig () {
	if [ ${MODIFYPAMCONFIG} != 0 ]; then
		echo "Modifying PAM configuration for pam_snapper"
		echo -e -n "session optional pam_snapper.so" >> /etc/pam.d/common-session
		# echo -e -n "session optional\tpam_exec.so\t/usr/bin/pam_snapper.sh\n" >> /etc/pam.d/common-session
		# requires a patch to pam-config
		${CMD_PAM_CONFIG} -a --exec-option="/usr/bin/pam_snapper.sh"
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

