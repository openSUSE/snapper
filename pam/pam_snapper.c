/**
 * @file
 * @author  Matthias G. Eckermann <mge@suse.com>
 *
 * @section License
 *
 * Copyright (c) 2013 SUSE
 *
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, contact SUSE
 *
 * To contact SUSE about this file by physical or electronic mail, you
 * may find current contact information at www.suse.com
 *
 * @section Usage
 *
 * 1. Create a btrfs subvolume for the new user,
 *    and a snapper configuration, e.g. using
 *      "pam_snapper_useradd.sh".
 *
 * 2. Add the following line to /etc/pam.d/common-session:
 *      "session optional pam_snapper.so homeprefix=home_"
 *
 * See "man snapper" for more information.
 *
 * @section Related Projects
 *
 * pam-snapper was written by Matthias G. Eckermann <mge@suse.com>
 * as part of SUSE Hackweek#9 in April 2013.
 *
 * This module would not have been possible without the work of
 * Arvin Schnell on the snapper project. pam-snapper inherits
 * DBUS handling from "snapper_dbus_cli.c" by David Disseldorp.
 *
 * The module builds on the Linux PAM stack and its
 * documentation, written by Thorsten Kukuk.
 *
 * @section Coding Style
 *
 * indent -linux -i8 -ts8 -l140 -cbi8 -cli8 -prs -nbap -nbbb -nbad -sob *.c
 *
*/

/* #define PAM_SNAPPER_DEBUG */

#include "../config.h"

/*
 * PAM Preamble
*/

#define MODULE_NAME "pam_snapper"
#define PAM_SM_SESSION

#include <sys/types.h>
#include <pwd.h>
#include <security/pam_modules.h>
#include <security/pam_modutil.h>
#include <security/pam_ext.h>

/*
 * DBUS Preamble
*/

#include <dbus/dbus.h>

/*
 * Includes
*/

#define __USE_GNU
#include <unistd.h>
#undef __USE_GNU
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

/*
 * DBUS handling
*/

#define CDBUS_SIG_CREATE_SNAP_RSP "u"
#define CDBUS_SIG_STRING_DICT "{ss}"

/**
 * A simple dictionary, ...
 *
*/
struct dict {
	const char *key;
	const char *val;
};

/**
 * Are we executed during the PAM open_session
 * or close_session context?
 *
*/
typedef enum openclose_e {
	open_session,
	close_session
} openclose_t;

/**
 * Modes which snapper can use with the create
 * command. "post" implies that the number of the
 * respective "pre" snapshot is available.
 *
*/
typedef enum createmode_e {
	createmode_single,
	createmode_pre,
	createmode_post
} createmode_t;

/**
 * The type pam_options_t declares a struct
 * to keep all configuration options of the
 * PAM module "pam_snapper".
 *
*/
typedef struct {
	const char *homeprefix;
	const char *ignoreservices;
	const char *ignoreusers;
	bool rootasroot;
	bool ignoreroot;
	bool do_open;
	bool do_close;
	const char *cleanup;
	bool debug;
} pam_options_t;

/*
 * Functions for DBUS handling
*/

/**
 * init the connection to the DBUS
 *
 * @param pamh the PAM handle
 *
 * @return DBusConnection *
 *
*/
static DBusConnection *cdbus_conn( pam_handle_t * pamh )
{
	DBusError err;
	DBusConnection *conn;
	dbus_error_init( &err );
	/* bus connection */
	conn = dbus_bus_get( DBUS_BUS_SYSTEM, &err );
	if ( dbus_error_is_set( &err ) ) {
		pam_syslog( pamh, LOG_ERR, "connection error: %s", strerror( errno ) );
		dbus_error_free( &err );
	}
	if ( NULL == conn ) {
		return NULL;
	}
	return conn;
}

/**
 * send a message to the DBUS
 *
 * @param pamh PAM handle
 * @param conn current DBus connection
 * @param msg message to send
 * @param pend_out handle for a reply
 *
 * @return error condition
 *
*/
static int cdbus_msg_send( pam_handle_t * pamh, DBusConnection * conn, DBusMessage * msg, DBusPendingCall ** pend_out )
{
	DBusPendingCall *pending;
	/* send message and get a handle for a reply */
	if ( !dbus_connection_send_with_reply( conn, msg, &pending, -1 ) ) {
		return -ENOMEM;
	}
	if ( NULL == pending ) {
		pam_syslog( pamh, LOG_ERR, "Pending Call Null" );
		return -EINVAL;
	}
	dbus_connection_flush( conn );
	*pend_out = pending;
	return 0;
}

/**
 * receive a message from the DBUS
 *
 * @param pamh PAM handle
 * @param conn current DBus connection
 * @param pending pointer for the pending call
 * @param msg_out handle for the result data set
 *
 * @return error condition
 *
*/
static int cdbus_msg_recv( pam_handle_t * pamh, DBusConnection * conn, DBusPendingCall * pending, DBusMessage ** msg_out )
{
	DBusMessage *msg;
	/* block until we receive a reply */
	dbus_pending_call_block( pending );
	/* get the reply message */
	msg = dbus_pending_call_steal_reply( pending );
	if ( msg == NULL ) {
		pam_syslog( pamh, LOG_ERR, "Reply Null" );
		return -ENOMEM;
	}
	/* free the pending message handle */
	dbus_pending_call_unref( pending );
	*msg_out = msg;
	return 0;
}

/**
 * ??
 *
 * @param pamh PAM handle
 * @param
 * @param
 *
 * @return error condition or OK
 *
*/
static int cdbus_type_check( pam_handle_t * pamh, DBusMessageIter * iter, int expected_type )
{
	int type = dbus_message_iter_get_arg_type( iter );
	if ( type != expected_type ) {
		pam_syslog( pamh, LOG_ERR, "got type %d, expecting %d", type, expected_type );
		return -EINVAL;
	}
	return 0;
}

/**
 * ??
 *
 * @param pamh PAM handle
 * @param
 * @param
 *
 * @return error condition or OK
 *
*/
static int cdbus_type_check_get( pam_handle_t * pamh, DBusMessageIter * iter, int expected_type, void *val )
{
	int ret;
	ret = cdbus_type_check( pamh, iter, expected_type );
	if ( ret < 0 ) {
		return ret;
	}
	dbus_message_iter_get_basic( iter, val );
	return 0;
}

/**
 * ??
 *
 * @param pamh PAM handle
 * @param
 * @param
 * @param
 * @param
 * @param
 *
 * @return -ENOMEM or OK
 *
*/
static int cdbus_create_snap_pack( pam_handle_t * pamh, const char *snapper_conf, createmode_t create_mode, const char *cleanup,
				   uint32_t num_user_data, struct dict *user_data, DBusMessage ** req_msg_out )
{
	DBusMessage *msg;
	DBusMessageIter args;
	DBusMessageIter array_iter;
	DBusMessageIter struct_iter;
	const char *desc = MODULE_NAME;
	uint32_t i;
	uint32_t *snap_id = NULL;
	bool ret;
	const char *modestrings[3] = { "CreateSingleSnapshot", "CreatePreSnapshot", "CreatePostSnapshot" };
	msg = dbus_message_new_method_call( "org.opensuse.Snapper",	/* target for the method call */
					    "/org/opensuse/Snapper",	/* object to call on */
					    "org.opensuse.Snapper",	/* interface to call on */
					    modestrings[create_mode] );	/* method name */
	if ( msg == NULL ) {
		pam_syslog( pamh, LOG_ERR, "failed to create req msg" );
		return -ENOMEM;
	}
	/* append arguments */
	dbus_message_iter_init_append( msg, &args );
	if ( !dbus_message_iter_append_basic( &args, DBUS_TYPE_STRING, &snapper_conf ) ) {
		pam_syslog( pamh, LOG_ERR, "Out Of Memory!" );
		return -ENOMEM;
	}
	if ( create_mode == createmode_post ) {
		pam_get_data( pamh, "pam_snapper_snapshot_num", ( const void ** )&snap_id );
		if ( !dbus_message_iter_append_basic( &args, DBUS_TYPE_UINT32, snap_id ) ) {
			pam_syslog( pamh, LOG_ERR, "Out Of Memory!" );
			return -ENOMEM;
		}
	}
	if ( !dbus_message_iter_append_basic( &args, DBUS_TYPE_STRING, &desc ) ) {
		pam_syslog( pamh, LOG_ERR, "Out Of Memory!" );
		return -ENOMEM;
	}
	/* cleanup */
	if ( !dbus_message_iter_append_basic( &args, DBUS_TYPE_STRING, &cleanup ) ) {
		pam_syslog( pamh, LOG_ERR, "Out Of Memory!" );
		return -ENOMEM;
	}
	ret = dbus_message_iter_open_container( &args, DBUS_TYPE_ARRAY, CDBUS_SIG_STRING_DICT, &array_iter );
	if ( !ret ) {
		pam_syslog( pamh, LOG_ERR, "failed to open array container" );
		return -ENOMEM;
	}
	for ( i = 0; i < num_user_data; i++ ) {
		ret = dbus_message_iter_open_container( &array_iter, DBUS_TYPE_DICT_ENTRY, NULL, &struct_iter );
		if ( !ret ) {
			pam_syslog( pamh, LOG_ERR, "failed to open struct container" );
			return -ENOMEM;
		}
		if ( !dbus_message_iter_append_basic( &struct_iter, DBUS_TYPE_STRING, &user_data[i].key ) ) {
			pam_syslog( pamh, LOG_ERR, "Out Of Memory!" );
			return -ENOMEM;
		}
		if ( !dbus_message_iter_append_basic( &struct_iter, DBUS_TYPE_STRING, &user_data[i].val ) ) {
			pam_syslog( pamh, LOG_ERR, "Out Of Memory!" );
			return -ENOMEM;
		}
		ret = dbus_message_iter_close_container( &array_iter, &struct_iter );
		if ( !ret ) {
			pam_syslog( pamh, LOG_ERR, "failed to close struct container" );
			return -ENOMEM;
		}
	}
	dbus_message_iter_close_container( &args, &array_iter );
	*req_msg_out = msg;
	return 0;
}

/**
 * ??
 *
 * @param pamh PAM handle
 * @param
 * @param
 *
 * @return -EINVAL or OK
 *
*/
static int cdbus_create_snap_unpack( pam_handle_t * pamh, DBusConnection * conn, DBusMessage * rsp_msg, uint32_t * snap_id_out )
{
	DBusMessageIter iter;
	int msg_type;
	const char *sig;
	msg_type = dbus_message_get_type( rsp_msg );
	if ( msg_type == DBUS_MESSAGE_TYPE_ERROR ) {
		pam_syslog( pamh, LOG_ERR, "create snap error response: %s", dbus_message_get_error_name( rsp_msg ) );
		return -EINVAL;
	}
	if ( msg_type != DBUS_MESSAGE_TYPE_METHOD_RETURN ) {
		pam_syslog( pamh, LOG_ERR, "unexpected create snap ret type: %d", msg_type );
		return -EINVAL;
	}
	sig = dbus_message_get_signature( rsp_msg );
	if ( ( sig == NULL )
	     || ( strcmp( sig, CDBUS_SIG_CREATE_SNAP_RSP ) != 0 ) ) {
		pam_syslog( pamh, LOG_ERR, "bad create snap response sig: %s, "
			    "expected: %s", ( sig ? sig : "NULL" ), CDBUS_SIG_CREATE_SNAP_RSP );
		return -EINVAL;
	}
	/* read the parameters */
	if ( !dbus_message_iter_init( rsp_msg, &iter ) ) {
		pam_syslog( pamh, LOG_ERR, "Message has no arguments!" );
		return -EINVAL;
	}
	if ( cdbus_type_check_get( pamh, &iter, DBUS_TYPE_UINT32, snap_id_out ) ) {
		return -EINVAL;
	}
	return 0;
}

/**
 * ??
 *
 * @param pamh PAM handle
 * @param
 * @param
 * @param
 *
 * @return -EINVAL or OK
 *
*/
static void cdbus_fill_user_data( pam_handle_t * pamh, struct dict ( *user_data )[], int *user_data_num, int user_data_max )
{
	int fields[4] = { PAM_RUSER, PAM_RHOST, PAM_TTY, PAM_SERVICE };
	const char *names[4] = { "ruser", "rhost", "tty", "service" };
	int i;
	for ( i = 0; i < 4; ++i ) {
		const char *readval = NULL;
		int ret = pam_get_item( pamh, fields[i], ( const void ** )&readval );
		if ( !ret && readval ) {
			( *user_data )[*user_data_num].key = names[i];
			( *user_data )[*user_data_num].val = readval;
			if ( ( *user_data_num ) < user_data_max ) {
				( *user_data_num )++;
			}
		}
	}
}

static void cleanup_snapshot_num( pam_handle_t * pamh, void *data, int error_status )
{
	free( data );
}

/**
 * ??
 *
 * @param pamh PAM handle
 * @param
 * @param
 * @param
 *
 * @return -EINVAL or OK
 *
*/
static int cdbus_create_snap_call( pam_handle_t * pamh, const char *snapper_conf, createmode_t create_mode, const char *cleanup,
				   DBusConnection * conn )
{
	int ret;
	DBusMessage *req_msg;
	DBusMessage *rsp_msg;
	DBusPendingCall *pending;
	uint32_t *snap_id = malloc( sizeof( uint32_t ) );
#ifdef PAM_SNAPPER_DEBUG
	uint32_t *snap_id_check = NULL;
#endif
	int user_data_num = 0;
	enum { user_data_max = 6 };
	struct dict user_data[user_data_max];
	cdbus_fill_user_data( pamh, &user_data, &user_data_num, user_data_max );
	ret = cdbus_create_snap_pack( pamh, snapper_conf, create_mode, cleanup, user_data_num, user_data, &req_msg );
	if ( ret < 0 ) {
		pam_syslog( pamh, LOG_ERR, "failed to pack create snap request" );
		return ret;
	}
	ret = cdbus_msg_send( pamh, conn, req_msg, &pending );
	if ( ret < 0 ) {
		dbus_message_unref( req_msg );
		return ret;
	}
	ret = cdbus_msg_recv( pamh, conn, pending, &rsp_msg );
	if ( ret < 0 ) {
		dbus_message_unref( req_msg );
		dbus_pending_call_unref( pending );
		return ret;
	}
	ret = cdbus_create_snap_unpack( pamh, conn, rsp_msg, snap_id );
	if ( ret < 0 ) {
		pam_syslog( pamh, LOG_ERR, "failed to unpack create snap response: %s", strerror( -ret ) );
		dbus_message_unref( req_msg );
		dbus_message_unref( rsp_msg );
		return ret;
	}
	ret = pam_set_data( pamh, "pam_snapper_snapshot_num", snap_id, cleanup_snapshot_num );
	if ( ret != PAM_SUCCESS ) {
		pam_syslog( pamh, LOG_ERR, "pam_set_data failed." );
	}
#ifdef PAM_SNAPPER_DEBUG
	pam_syslog( pamh, LOG_DEBUG, "created new snapshot %u", *snap_id );
	pam_get_data( pamh, "pam_snapper_snapshot_num", ( const void ** )&snap_id_check );
	pam_syslog( pamh, LOG_DEBUG, "Check new snapshot: '%u'", *snap_id_check );
#endif
	dbus_message_unref( req_msg );
	dbus_message_unref( rsp_msg );
	return 0;
}

/**
 * ??
 *
 * @param
 *
 * @return PAM_SUCCESS
 *
*/
static void cdbus_pam_options_setdefault( pam_options_t * options )
{
	options->homeprefix = "home_";
	options->ignoreservices = "crond";
	options->ignoreusers = NULL;
	options->rootasroot = false;
	options->ignoreroot = false;
	options->do_open = true;
	options->do_close = true;
	options->cleanup = "";
	options->debug = false;
}

/**
 * ??
 *
 * @param
 *
 * @return PAM status
 *
*/
static int csv_contains( pam_handle_t * pamh, const char *haystack, const char *needle, bool debug )
{
	if ( debug ) {
		pam_syslog( pamh, LOG_DEBUG, "csv_contains haystack: '%s' needle: '%s'", haystack, needle );
	}

	const size_t l = strlen( needle );

	const char *s = haystack;
	const char *e;

	while ( ( e = strchr( s, ',' ) ) ) {
		if ( e - s == l && strncmp( s, needle, l ) == 0 )
			return 1;

		s = e + 1;
	}

	return strcmp( s, needle ) == 0;
}

/**
 * ??
 *
 * @param pamh PAM handle
 * @param
 * @param
 * @param
 *
 * @return -EINVAL or OK
 *
*/
static int cdbus_pam_options_parser( pam_handle_t * pamh, pam_options_t * options, int argc, const char **argv )
{
	const char *pamuser = NULL, *pamservice = NULL;
	pam_get_item( pamh, PAM_USER, ( const void ** )&pamuser );
	pam_get_item( pamh, PAM_SERVICE, ( const void ** )&pamservice );
	for ( ; argc-- > 0; ++argv ) {
		if ( !strncmp( *argv, "homeprefix=", 11 ) ) {
			options->homeprefix = 11 + *argv;
			if ( *( options->homeprefix ) != '\0' ) {
				if ( options->debug ) {
					pam_syslog( pamh, LOG_DEBUG, "set homeprefix: %s", options->homeprefix );
				}
			} else {
				options->homeprefix = NULL;
				if ( options->debug ) {
					pam_syslog( pamh, LOG_ERR, "homeprefix= specification missing argument - ignored" );
				}
			}
		} else if ( !strncmp( *argv, "ignoreservices=", 15 ) ) {
			options->ignoreservices = 15 + *argv;
			if ( *( options->ignoreservices ) != '\0' ) {
				if ( csv_contains( pamh, options->ignoreservices, pamservice, options->debug ) ) {
					return PAM_IGNORE;
				}
			} else {
				if ( options->debug ) {
					pam_syslog( pamh, LOG_ERR, "ignoreservices - specification missing argument - ignored" );
				}
			}
		} else if ( !strncmp( *argv, "ignoreusers=", 12 ) ) {
			options->ignoreusers = 12 + *argv;
			if ( *( options->ignoreusers ) != '\0' ) {
				if ( csv_contains( pamh, options->ignoreusers, pamuser, options->debug ) ) {
					return PAM_IGNORE;
				}
			} else {
				if ( options->debug ) {
					pam_syslog( pamh, LOG_ERR, "ignoreusers - specification missing argument - ignored" );
				}
			}
		} else if ( !strncmp( *argv, "cleanup=", 8 ) ) {
			options->cleanup = 8 + *argv;
		} else if ( !strcmp( *argv, "debug" ) ) {
			options->debug = true;
		} else if ( !strcmp( *argv, "rootasroot" ) ) {
			options->rootasroot = true;
		} else if ( !strcmp( *argv, "ignoreroot" ) ) {
			options->ignoreroot = true;
		} else if ( !strcmp( *argv, "openonly" ) ) {
			options->do_close = false;
			options->do_open = true;
		} else if ( !strcmp( *argv, "closeonly" ) ) {
			options->do_open = false;
			options->do_close = true;
		} else {
			pam_syslog( pamh, LOG_ERR, "unknown option: %s", *argv );
			pam_syslog( pamh, LOG_ERR,
				    "valid options: debug homeprefix=<> ignoreservices=<> ignoreusers=<> rootasroot ignoreroot openonly closeonly" );
		}
	}
	if ( options->ignoreservices ) {
		if ( csv_contains( pamh, options->ignoreservices, pamservice, options->debug ) ) {
			return PAM_IGNORE;
		}
	}
	if ( options->ignoreusers ) {
		if ( csv_contains( pamh, options->ignoreusers, pamuser, options->debug ) ) {
			return PAM_IGNORE;
		}
	}
	if ( options->ignoreroot ) {
		if ( strcmp( pamuser, "root" ) == 0 ) {
			return PAM_IGNORE;
		}
	}
	if ( options->debug ) {
		pam_syslog( pamh, LOG_ERR,
			    "current settings: homeprefix=%s ignoreservices=%s ignoreusers=%s",
			    options->homeprefix, options->ignoreservices, options->ignoreusers );
	}
	if ( options->rootasroot && options->ignoreroot ) {
		options->rootasroot = false;
		pam_syslog( pamh, LOG_WARNING, "'ignoreroot' options shadows 'rootasroot'. 'rootasroot' will be ignored." );
	}
	return PAM_SUCCESS;
}

/**
 *
 *
 *
*/
static int cdbus_pam_switch_to_user( pam_handle_t * pamh, struct passwd **user_entry, const char *real_user )
{
	int ret = -EINVAL;
	/* save current user */
	if ( ( ( *user_entry ) = getpwnam( real_user ) ) == NULL ) {
		ret = errno;
		pam_syslog( pamh, LOG_ERR, "getpwnam( %s ) failed: %s", real_user, strerror( ret ) );
		return PAM_IGNORE;
	}
	if ( setegid( ( unsigned long )( *user_entry )->pw_gid ) == -1 ) {
		ret = errno;
		pam_syslog( pamh, LOG_ERR, "setgid(%lu) failed: %s", ( unsigned long )( *user_entry )->pw_gid, strerror( ret ) );
		return PAM_IGNORE;
	}
	if ( seteuid( ( unsigned long )( *user_entry )->pw_uid ) == -1 ) {
		ret = errno;
		pam_syslog( pamh, LOG_ERR, "setuid(%lu) failed: %s", ( unsigned long )( *user_entry )->pw_uid, strerror( ret ) );
		return PAM_IGNORE;
	}
	return PAM_SUCCESS;
}

/**
 *
 *
 *
*/
static int cdbus_pam_drop_privileges( pam_handle_t * pamh, struct passwd **user_entry )
{
	int ret = -EINVAL;
	PAM_MODUTIL_DEF_PRIVS( privs );
	if ( pam_modutil_drop_priv( pamh, &privs, ( *user_entry ) ) ) {
		ret = errno;
		pam_syslog( pamh, LOG_ERR, "pam_modutil_drop_priv (%lu) failed: %s", ( unsigned long )( *user_entry )->pw_uid,
			    strerror( ret ) );
		return PAM_IGNORE;
	}
	return PAM_SUCCESS;
}

/**
 *
 *
 *
*/
static int cdbus_pam_switch_from_user( pam_handle_t * pamh )
{
	int ret = -EINVAL;
	uid_t ruid, euid, suid;
	gid_t rgid, egid, sgid;
	getresuid( &ruid, &euid, &suid );
	getresgid( &rgid, &egid, &sgid );
	if ( setegid( sgid ) == -1 ) {
		ret = errno;
		pam_syslog( pamh, LOG_ERR, "setgid(%lu) failed: %s", ( unsigned long )sgid, strerror( ret ) );
	}
	if ( seteuid( suid ) == -1 ) {
		ret = errno;
		pam_syslog( pamh, LOG_ERR, "setuid(%lu) failed: %s", ( unsigned long )suid, strerror( ret ) );
	}
	return PAM_SUCCESS;
}

/**
 * ??
 *
 * @param pamh PAM handle
 * @param
 * @param
 * @param
 * @param
 *
 * @return -EINVAL or OK
 *
*/
static int cdbus_pam_session( pam_handle_t * pamh, openclose_t openclose, const char *real_user, int flags, int argc, const char **argv )
{
	DBusConnection *conn;
	int ret = -EINVAL;
	char *real_user_config = NULL;
	pam_options_t options;
	cdbus_pam_options_setdefault( &options );
	ret = cdbus_pam_options_parser( pamh, &options, argc, argv );
	if ( ret == PAM_IGNORE ) {
		goto pam_snapper_ignore;
	}
	/* open the connection to the D-Bus */
	conn = cdbus_conn( pamh );
	if ( conn == NULL ) {
		pam_syslog( pamh, LOG_ERR, "connect to D-Bus failed" );
		goto pam_snapper_ignore;
	}
	if ( !strcmp( real_user, "root" ) && options.rootasroot ) {
		real_user_config = strdup( "root" );
	} else {
		real_user_config = malloc( strlen( options.homeprefix ) + strlen( real_user ) + 1 );
		if ( !real_user_config ) {
			goto pam_sm_open_session_err;
		}
		strcpy( real_user_config, options.homeprefix );
		strcat( real_user_config, real_user );
	}
	if ( options.debug ) {
		pam_syslog( pamh, LOG_DEBUG, MODULE_NAME " version " VERSION );
		pam_syslog( pamh, LOG_DEBUG, "real_user_config='%s'", real_user_config );
	}
	if ( ( openclose == open_session ) && options.do_open ) {
		ret =
		    cdbus_create_snap_call( pamh, real_user_config, options.do_close ? createmode_pre : createmode_single, options.cleanup,
					    conn );
	} else if ( ( openclose == close_session ) && options.do_close ) {
		ret =
		    cdbus_create_snap_call( pamh, real_user_config, options.do_open ? createmode_post : createmode_single, options.cleanup,
					    conn );
	}
	if ( ret < 0 ) {
		pam_syslog( pamh, LOG_ERR, "snapshot create call failed: %s", strerror( -ret ) );
	}
 pam_sm_open_session_err:
	if ( real_user_config ) {
		free( real_user_config );
	}
	dbus_connection_unref( conn );
 pam_snapper_ignore:
	return PAM_SUCCESS;
}

/*
 * HERE the PAM stuff starts
*/

/**
 * ??
 *
 * @param pamh PAM handle
 * @param
 * @param
 * @param
 *
 * @return PAM_SUCCESS (always)
 *
*/
PAM_EXTERN int pam_sm_open_session( pam_handle_t * pamh, int flags, int argc, const char **argv )
{
	int ret = -EINVAL;
	const char *real_user = NULL;
	struct passwd *user_entry;
	ret = pam_get_item( pamh, PAM_USER, ( const void ** )&real_user );
	if ( ret != PAM_SUCCESS ) {
		pam_syslog( pamh, LOG_ERR, "cannot get PAM_USER: %s", strerror( -ret ) );
		goto pam_snapper_skip;
	}
	if ( !real_user ) {
		goto pam_snapper_skip;
	}
	/* no need to proceed as root */
	if ( cdbus_pam_switch_to_user( pamh, &user_entry, real_user ) == PAM_IGNORE ) {
		goto pam_snapper_skip;
	}
	if ( cdbus_pam_drop_privileges( pamh, &user_entry ) == PAM_IGNORE ) {
		goto pam_snapper_skip;
	}
	/* do the real stuff */
	cdbus_pam_session( pamh, open_session, real_user, flags, argc, argv );
	/* go back to original user */
	cdbus_pam_switch_from_user( pamh );
 pam_snapper_skip:
	return PAM_SUCCESS;
}

/**
 * ??
 *
 * @param pamh PAM handle
 * @param
 * @param
 * @param
 *
 * @return PAM_SUCCESS (always)
 *
*/
PAM_EXTERN int pam_sm_close_session( pam_handle_t * pamh, int flags, int argc, const char **argv )
{
	int ret = -EINVAL;
	const char *real_user = NULL;
	struct passwd *user_entry;
	ret = pam_get_item( pamh, PAM_USER, ( const void ** )&real_user );
	if ( ret != PAM_SUCCESS ) {
		pam_syslog( pamh, LOG_ERR, "cannot get PAM_USER: %s", strerror( -ret ) );
		goto pam_snapper_skip;
	}
	if ( !real_user ) {
		goto pam_snapper_skip;
	}
	/* no need to proceed as root */
	if ( cdbus_pam_switch_to_user( pamh, &user_entry, real_user ) == PAM_IGNORE ) {
		goto pam_snapper_skip;
	}
	if ( cdbus_pam_drop_privileges( pamh, &user_entry ) == PAM_IGNORE ) {
		goto pam_snapper_skip;
	}
	/* do the real stuff */
	cdbus_pam_session( pamh, close_session, real_user, flags, argc, argv );
	/* go back to original user */
	cdbus_pam_switch_from_user( pamh );
 pam_snapper_skip:
	return PAM_SUCCESS;
}

#ifdef PAM_STATIC

struct pam_module _pam_panic_modstruct = {
	MODULE_NAME,
	NULL,
	NULL,
	NULL,
	pam_sm_open_session,
	pam_sm_close_session,
	NULL
};

#endif
