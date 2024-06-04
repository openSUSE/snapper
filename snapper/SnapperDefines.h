/*
 * Copyright (c) [2004-2014] Novell, Inc.
 * Copyright (c) [2020-2024] SUSE LLC
 *
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may
 * find current contact information at www.novell.com.
 */


#ifndef SNAPPER_SNAPPER_DEFINES_H
#define SNAPPER_SNAPPER_DEFINES_H


// path definitions

#define SYSCONFIG_FILE CONF_DIR "/snapper"

#define CONFIGS_DIR "/etc/snapper/configs"

#define ETC_CONFIG_TEMPLATE_DIR "/etc/snapper/config-templates"
#define USR_CONFIG_TEMPLATE_DIR "/usr/share/snapper/config-templates"

#define ETC_FILTERS_DIR "/etc/snapper/filters"
#define USR_FILTERS_DIR "/usr/share/snapper/filters"

#define PLUGINS_DIR "/usr/lib/snapper/plugins"

#define DEV_DIR "/dev"
#define DEV_MAPPER_DIR "/dev/mapper"


// parts of the path of snapshots
// "/.snapshots/42/snapshot/" = "/SNAPSHOTS_NAME/42/snapshot/"

#define SNAPSHOTS_NAME ".snapshots"
#define SNAPSHOT_NAME "snapshot"


// commands

#define SH_BIN "/bin/sh"

#define BTRFS_BIN "/usr/sbin/btrfs"

#define SYSTEMCTL_BIN "/usr/bin/systemctl"


// keys from the config files

#define KEY_SUBVOLUME "SUBVOLUME"
#define KEY_FSTYPE "FSTYPE"

#define KEY_ALLOW_USERS "ALLOW_USERS"
#define KEY_ALLOW_GROUPS "ALLOW_GROUPS"
#define KEY_SYNC_ACL "SYNC_ACL"
#define KEY_COMPRESSION "COMPRESSION"
#define KEY_TIMELINE_CREATE "TIMELINE_CREATE"


#endif
