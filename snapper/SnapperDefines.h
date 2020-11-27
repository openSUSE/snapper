/*
 * Copyright (c) [2004-2014] Novell, Inc.
 * Copyright (c) 2020 SUSE LLC
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
#define CONFIG_TEMPLATE_DIR "/etc/snapper/config-templates"

#define FILTERS_DIR "/etc/snapper/filters"

#define DEV_DIR "/dev"
#define DEV_MAPPER_DIR "/dev/mapper"


// keys from the config files

#define KEY_SUBVOLUME "SUBVOLUME"
#define KEY_FSTYPE "FSTYPE"

#define KEY_ALLOW_USERS "ALLOW_USERS"
#define KEY_ALLOW_GROUPS "ALLOW_GROUPS"
#define KEY_SYNC_ACL "SYNC_ACL"


#endif
