/**
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
 * Add the following line to /etc/pam.d/common-session:
 *      "session optional pam_snapper.so"
 *
 * See "man pam_snapper" and "man snapper" for more information.
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
 * indent -linux -i8 -ts8 -l100 -cbi8 -cli8 -prs -nbap -nbbb -nbad -sob *.c
 *
*/

#include "../config.h"

/*
 * Includes
*/

#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pwd.h>
#include <grp.h>

/*
 * PAM Preamble
*/

#define MODULE_NAME "pam_snapper"
#define PAM_SM_SESSION

#include <security/pam_modules.h>
#include <security/pam_ext.h>

/*
 * DBUS Preamble
*/

#include <dbus/dbus.h>

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

static int cdbus_msg_send( DBusConnection * conn, DBusMessage * msg, DBusPendingCall ** pend_out )
{
	DBusPendingCall *pending;
	/* send message and get a handle for a reply */
	if ( !dbus_connection_send_with_reply( conn, msg, &pending, 0x7fffffff ) ) {
		return -ENOMEM;
	}
	if ( NULL == pending ) {
		return -EINVAL;
	}
	dbus_connection_flush( conn );
	*pend_out = pending;
	return 0;
}

static int cdbus_msg_recv( DBusConnection * conn, DBusPendingCall * pending,
			   DBusMessage ** msg_out )
{
	DBusMessage *msg;
	/* block until we receive a reply */
	dbus_pending_call_block( pending );
	/* get the reply message */
	msg = dbus_pending_call_steal_reply( pending );
	if ( msg == NULL ) {
		return -ENOMEM;
	}
	/* free the pending message handle */
	dbus_pending_call_unref( pending );
	*msg_out = msg;
	return 0;
}

static int cdbus_type_check( DBusMessageIter * iter, int expected_type )
{
	int type = dbus_message_iter_get_arg_type( iter );
	if ( type != expected_type ) {
		return -EINVAL;
	}
	return 0;
}

static int cdbus_type_check_get( DBusMessageIter * iter, int expected_type, void *val )
{
	int ret;
	ret = cdbus_type_check( iter, expected_type );
	if ( ret < 0 ) {
		return ret;
	}
	dbus_message_iter_get_basic( iter, val );
	return 0;
}

static int cdbus_create_snap_pack( const char *snapper_conf, createmode_t createmode,
				   const char *cleanup, uint32_t num_user_data,
				   const struct dict *user_data, const uint32_t * snapshot_num_in,
				   DBusMessage ** req_msg_out )
{
	DBusMessage *msg;
	DBusMessageIter args;
	DBusMessageIter array_iter;
	DBusMessageIter struct_iter;
	const char *modestrings[3] =
	    { "CreateSingleSnapshot", "CreatePreSnapshot", "CreatePostSnapshot" };
	msg = dbus_message_new_method_call( "org.opensuse.Snapper",	/* target for the method call */
					    "/org/opensuse/Snapper",	/* object to call on */
					    "org.opensuse.Snapper",	/* interface to call on */
					    modestrings[createmode] );	/* method name */
	if ( msg == NULL ) {
		return -ENOMEM;
	}

	dbus_message_iter_init_append( msg, &args );
	if ( !dbus_message_iter_append_basic( &args, DBUS_TYPE_STRING, &snapper_conf ) ) {
		return -ENOMEM;
	}
	if ( createmode == createmode_post ) {
		if ( !dbus_message_iter_append_basic( &args, DBUS_TYPE_UINT32, snapshot_num_in ) ) {
			return -ENOMEM;
		}
	}

	const char *desc = MODULE_NAME;
	if ( !dbus_message_iter_append_basic( &args, DBUS_TYPE_STRING, &desc ) ) {
		return -ENOMEM;
	}

	if ( !dbus_message_iter_append_basic( &args, DBUS_TYPE_STRING, &cleanup ) ) {
		return -ENOMEM;
	}

	dbus_bool_t ret =
	    dbus_message_iter_open_container( &args, DBUS_TYPE_ARRAY, CDBUS_SIG_STRING_DICT,
					      &array_iter );
	if ( !ret ) {
		return -ENOMEM;
	}

	for ( uint32_t i = 0; i < num_user_data; ++i ) {
		ret =
		    dbus_message_iter_open_container( &array_iter, DBUS_TYPE_DICT_ENTRY, NULL,
						      &struct_iter );
		if ( !ret ) {
			return -ENOMEM;
		}
		if ( !dbus_message_iter_append_basic
		     ( &struct_iter, DBUS_TYPE_STRING, &user_data[i].key ) ) {
			return -ENOMEM;
		}
		if ( !dbus_message_iter_append_basic
		     ( &struct_iter, DBUS_TYPE_STRING, &user_data[i].val ) ) {
			return -ENOMEM;
		}
		ret = dbus_message_iter_close_container( &array_iter, &struct_iter );
		if ( !ret ) {
			return -ENOMEM;
		}
	}
	dbus_message_iter_close_container( &args, &array_iter );
	*req_msg_out = msg;
	return 0;
}

static int cdbus_create_snap_unpack( DBusConnection * conn, DBusMessage * rsp_msg,
				     uint32_t * snapshot_num )
{
	DBusMessageIter iter;
	int msg_type;
	const char *sig;
	msg_type = dbus_message_get_type( rsp_msg );
	if ( msg_type == DBUS_MESSAGE_TYPE_ERROR ) {
		return -EINVAL;
	}
	if ( msg_type != DBUS_MESSAGE_TYPE_METHOD_RETURN ) {
		return -EINVAL;
	}
	sig = dbus_message_get_signature( rsp_msg );
	if ( ( sig == NULL ) || ( strcmp( sig, CDBUS_SIG_CREATE_SNAP_RSP ) != 0 ) ) {
		return -EINVAL;
	}
	/* read the parameters */
	if ( !dbus_message_iter_init( rsp_msg, &iter ) ) {
		return -EINVAL;
	}
	if ( cdbus_type_check_get( &iter, DBUS_TYPE_UINT32, snapshot_num ) ) {
		return -EINVAL;
	}
	return 0;
}

static int cdbus_create_snapshot( const char *snapper_conf, createmode_t createmode,
				  const char *cleanup, uint32_t num_user_data,
				  const struct dict *user_data, const uint32_t * snapshot_num_in,
				  uint32_t * snapshot_num_out )
{
	DBusError err;
	dbus_error_init( &err );

	/* without a private connection setting uid/gui can be in vain, e.g. with gdm */

	DBusConnection *conn = dbus_bus_get_private( DBUS_BUS_SYSTEM, &err );
	if ( dbus_error_is_set( &err ) ) {
		dbus_error_free( &err );
	}

	DBusMessage *req_msg;
	int ret = cdbus_create_snap_pack( snapper_conf, createmode, cleanup, num_user_data,
					  user_data, snapshot_num_in, &req_msg );
	if ( ret < 0 ) {
		dbus_connection_close( conn );
		dbus_connection_unref( conn );
		return ret;
	}

	DBusPendingCall *pending;
	ret = cdbus_msg_send( conn, req_msg, &pending );
	if ( ret < 0 ) {
		dbus_message_unref( req_msg );
		dbus_connection_close( conn );
		dbus_connection_unref( conn );
		return ret;
	}

	DBusMessage *rsp_msg;
	ret = cdbus_msg_recv( conn, pending, &rsp_msg );
	if ( ret < 0 ) {
		dbus_message_unref( req_msg );
		dbus_pending_call_unref( pending );
		dbus_connection_close( conn );
		dbus_connection_unref( conn );
		return ret;
	}

	ret = cdbus_create_snap_unpack( conn, rsp_msg, snapshot_num_out );
	if ( ret < 0 ) {
		dbus_message_unref( req_msg );
		dbus_message_unref( rsp_msg );
		dbus_connection_close( conn );
		dbus_connection_unref( conn );
		return ret;
	}

	dbus_message_unref( req_msg );
	dbus_message_unref( rsp_msg );
	dbus_connection_close( conn );
	dbus_connection_unref( conn );
	return 0;
}

/**
 * Special functions for pam_snapper
 */

static int forker( pam_handle_t * pamh, const char *pam_user, uid_t uid, gid_t gid,
		   const char *snapper_conf, createmode_t createmode, const char *cleanup,
		   uint32_t num_user_data, const struct dict *user_data,
		   const uint32_t * snapshot_num_in, uint32_t * snapshot_num_out )
{
	void *p = mmap( NULL, sizeof( *snapshot_num_out ), PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0 );
	if ( p == MAP_FAILED ) {
		pam_syslog( pamh, LOG_ERR, "mmap failed" );
		return -1;
	}

	int child = fork(  );
	if ( child == 0 ) {

		/* setting uid/gui affects other threads so it has to be done in a separate process */

		if ( setgid( gid ) != 0 || initgroups( pam_user, gid ) != 0 || setuid( uid ) != 0 ) {
			munmap( p, sizeof( *snapshot_num_out ) );
			_exit( EXIT_FAILURE );
		}

		if ( cdbus_create_snapshot( snapper_conf, createmode, cleanup, num_user_data,
					    user_data, snapshot_num_in, snapshot_num_out ) != 0 ) {
			munmap( p, sizeof( *snapshot_num_out ) );
			_exit( EXIT_FAILURE );
		}

		memcpy( p, snapshot_num_out, sizeof( *snapshot_num_out ) );

		munmap( p, sizeof( *snapshot_num_out ) );

		_exit( EXIT_SUCCESS );

	} else if ( child > 0 ) {

		int status;
		int ret = waitpid( child, &status, 0 );

		if ( ret == -1 ) {
			pam_syslog( pamh, LOG_ERR, "waitpid failed" );
			munmap( p, sizeof( *snapshot_num_out ) );
			return -1;
		}

		if ( !WIFEXITED( status ) ) {
			pam_syslog( pamh, LOG_ERR, "child exited abnormal" );
			munmap( p, sizeof( *snapshot_num_out ) );
			return -1;
		}

		if ( WEXITSTATUS( status ) != EXIT_SUCCESS ) {
			pam_syslog( pamh, LOG_ERR, "child exited normal but with failure" );
			munmap( p, sizeof( *snapshot_num_out ) );
			return -1;
		}

		memcpy( snapshot_num_out, p, sizeof( *snapshot_num_out ) );

		munmap( p, sizeof( *snapshot_num_out ) );

		return 0;

	} else {

		pam_syslog( pamh, LOG_ERR, "fork failed" );
		return -1;

	}
}

static void fill_user_data( pam_handle_t * pamh, struct dict ( *user_data )[],
			    uint32_t * num_user_data, uint32_t max_user_data )
{
	int fields[4] = { PAM_RUSER, PAM_RHOST, PAM_TTY, PAM_SERVICE };
	const char *names[4] = { "ruser", "rhost", "tty", "service" };
	for ( int i = 0; i < 4; ++i ) {
		const char *readval = NULL;
		int ret = pam_get_item( pamh, fields[i], ( const void ** )&readval );
		if ( ret == PAM_SUCCESS && readval ) {
			( *user_data )[*num_user_data].key = names[i];
			( *user_data )[*num_user_data].val = readval;
			if ( ( *num_user_data ) < max_user_data ) {
				( *num_user_data )++;
			}
		}
	}
}

static void cleanup_snapshot_num( pam_handle_t * pamh, void *data, int error_status )
{
	free( data );
}

static int get_ugid( pam_handle_t * pamh, const char *pam_user, uid_t * uid, gid_t * gid )
{
	struct passwd pwd;
	struct passwd *result;

	long bufsize = sysconf( _SC_GETPW_R_SIZE_MAX );
	char buf[bufsize];

	if ( getpwnam_r( pam_user, &pwd, buf, bufsize, &result ) != 0 || result != &pwd ) {
		pam_syslog( pamh, LOG_ERR, "getpwnam failed" );
		return -1;
	}

	memset( pwd.pw_passwd, 0, strlen( pwd.pw_passwd ) );

	*uid = pwd.pw_uid;
	*gid = pwd.pw_gid;

	return 0;
}

static int worker( pam_handle_t * pamh, const char *pam_user, const char *snapper_conf,
		   createmode_t createmode, const char *cleanup )
{
	const uint32_t max_user_data = 5;
	struct dict user_data[max_user_data];
	uint32_t num_user_data = 0;
	fill_user_data( pamh, &user_data, &num_user_data, max_user_data );

	uid_t uid;
	gid_t gid;
	if ( get_ugid( pamh, pam_user, &uid, &gid ) != 0 )
		return -1;

	uint32_t *snapshot_num_in = NULL;
	uint32_t *snapshot_num_out = malloc( sizeof( *snapshot_num_out ) );
	if ( !snapshot_num_out ) {
		pam_syslog( pamh, LOG_ERR, "out of memory" );
		return -1;
	}

	if ( createmode == createmode_post ) {
		if ( pam_get_data
		     ( pamh, "pam_snapper_snapshot_num",
		       ( const void ** )&snapshot_num_in ) != PAM_SUCCESS ) {
			pam_syslog( pamh, LOG_ERR, "getting previous snapshot_num failed" );
			free( snapshot_num_out );
			return -1;
		}
	}

	if ( forker( pamh, pam_user, uid, gid, snapper_conf, createmode, cleanup, num_user_data,
		     user_data, snapshot_num_in, snapshot_num_out ) != 0 ) {
		free( snapshot_num_out );
		return -1;
	}

	if ( pam_set_data
	     ( pamh, "pam_snapper_snapshot_num", snapshot_num_out,
	       cleanup_snapshot_num ) != PAM_SUCCESS ) {
		free( snapshot_num_out );
		pam_syslog( pamh, LOG_ERR, "pam_set_data failed" );
	}

	return 0;
}

static void set_default_options( pam_options_t * options )
{
	options->homeprefix = "home_";
	options->ignoreservices = "crond";
	options->ignoreusers = "";
	options->rootasroot = false;
	options->ignoreroot = false;
	options->do_open = true;
	options->do_close = true;
	options->cleanup = "";
	options->debug = false;
}

static int csv_contains( pam_handle_t * pamh, const char *haystack, const char *needle, bool debug )
{
	if ( debug ) {
		pam_syslog( pamh, LOG_DEBUG, "csv_contains haystack: '%s' needle: '%s'", haystack,
			    needle );
	}

	const size_t l = strlen( needle );

	const char *s = haystack;
	const char *e;

	while ( ( e = strchr( s, ',' ) ) ) {
		if ( e == s + l && strncmp( s, needle, l ) == 0 )
			return 1;

		s = e + 1;
	}

	return strcmp( s, needle ) == 0;
}

static void options_parser( pam_handle_t * pamh, pam_options_t * options, int argc,
			    const char **argv )
{
	for ( ; argc-- > 0; ++argv ) {
		if ( !strncmp( *argv, "homeprefix=", 11 ) ) {
			options->homeprefix = 11 + *argv;
		} else if ( !strncmp( *argv, "ignoreservices=", 15 ) ) {
			options->ignoreservices = 15 + *argv;
		} else if ( !strncmp( *argv, "ignoreusers=", 12 ) ) {
			options->ignoreusers = 12 + *argv;
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
			pam_syslog( pamh, LOG_ERR, "valid options: debug homeprefix=<> "
				    "ignoreservices=<> ignoreusers=<> rootasroot ignoreroot "
				    "openonly closeonly cleanup=<>" );
		}
	}

	if ( options->rootasroot && options->ignoreroot ) {
		options->rootasroot = false;
		pam_syslog( pamh, LOG_WARNING, "'ignoreroot' options shadows 'rootasroot'. "
			    "'rootasroot' will be ignored." );
	}

	if ( options->debug ) {
		pam_syslog( pamh, LOG_ERR, "current settings: homeprefix='%s' ignoreservices='%s' "
			    "ignoreusers='%s' cleanup='%s'", options->homeprefix,
			    options->ignoreservices, options->ignoreusers, options->cleanup );
	}
}

static int check_ignore( pam_handle_t * pamh, const pam_options_t * options )
{
	if ( options->ignoreservices ) {

		const char *pam_service = NULL;
		int ret = pam_get_item( pamh, PAM_SERVICE, ( const void ** )&pam_service );
		if ( ret != PAM_SUCCESS ) {
			pam_syslog( pamh, LOG_ERR, "cannot get PAM_SERVICE" );
			return PAM_IGNORE;
		}
		if ( !pam_service ) {
			pam_syslog( pamh, LOG_ERR, "PAM_SERVICE is null" );
			return PAM_IGNORE;
		}

		if ( options->debug ) {
			pam_syslog( pamh, LOG_DEBUG, "PAM_SERVICE is '%s'", pam_service );
		}

		if ( options->ignoreservices ) {
			if ( csv_contains
			     ( pamh, options->ignoreservices, pam_service, options->debug ) ) {
				return PAM_IGNORE;
			}
		}
	}

	if ( options->ignoreusers || options->ignoreroot ) {

		const char *pam_user = NULL;
		int ret = pam_get_item( pamh, PAM_USER, ( const void ** )&pam_user );
		if ( ret != PAM_SUCCESS ) {
			pam_syslog( pamh, LOG_ERR, "cannot get PAM_USER" );
			return PAM_IGNORE;
		}
		if ( !pam_user ) {
			pam_syslog( pamh, LOG_ERR, "PAM_USER is null" );
			return PAM_IGNORE;
		}

		if ( options->debug ) {
			pam_syslog( pamh, LOG_DEBUG, "PAM_USER is '%s'", pam_user );
		}

		if ( options->ignoreusers ) {
			if ( csv_contains( pamh, options->ignoreusers, pam_user, options->debug ) ) {
				return PAM_IGNORE;
			}
		}
		if ( options->ignoreroot ) {
			if ( strcmp( pam_user, "root" ) == 0 ) {
				return PAM_IGNORE;
			}
		}
	}

	return PAM_SUCCESS;
}

static char *get_snapper_conf( const char *pam_user, const pam_options_t * options )
{
	char *snapper_conf = NULL;
	if ( options->rootasroot && strcmp( pam_user, "root" ) == 0 ) {
		snapper_conf = strdup( "root" );
	} else {
		snapper_conf = malloc( strlen( options->homeprefix ) + strlen( pam_user ) + 1 );
		if ( snapper_conf ) {
			strcpy( snapper_conf, options->homeprefix );
			strcat( snapper_conf, pam_user );
		}
	}
	return snapper_conf;
}

static void pam_session( pam_handle_t * pamh, openclose_t openclose, int argc, const char **argv )
{
	pam_options_t options;
	set_default_options( &options );
	options_parser( pamh, &options, argc, argv );

	if ( check_ignore( pamh, &options ) == PAM_IGNORE ) {
		return;
	}

	const char *pam_user = NULL;
	int ret = pam_get_item( pamh, PAM_USER, ( const void ** )&pam_user );
	if ( ret != PAM_SUCCESS ) {
		pam_syslog( pamh, LOG_ERR, "cannot get PAM_USER" );
		return;
	}
	if ( !pam_user ) {
		pam_syslog( pamh, LOG_ERR, "PAM_USER is null" );
		return;
	}

	char *snapper_conf = get_snapper_conf( pam_user, &options );
	if ( !snapper_conf ) {
		pam_syslog( pamh, LOG_ERR, "out of memory" );
		return;
	}

	if ( options.debug ) {
		pam_syslog( pamh, LOG_DEBUG, MODULE_NAME " version " VERSION );
		pam_syslog( pamh, LOG_DEBUG, "pam_user='%s', snapper_conf='%s'", pam_user,
			    snapper_conf );
	}

	if ( ( openclose == open_session ) && options.do_open ) {
		ret = worker( pamh, pam_user, snapper_conf, options.do_close ? createmode_pre :
			      createmode_single, options.cleanup );
	} else if ( ( openclose == close_session ) && options.do_close ) {
		ret = worker( pamh, pam_user, snapper_conf, options.do_open ? createmode_post :
			      createmode_single, options.cleanup );
	}

	free( snapper_conf );
}

/*
 * PAM stuff starts here
*/

PAM_EXTERN int pam_sm_open_session( pam_handle_t * pamh, int flags, int argc, const char **argv )
{
	pam_session( pamh, open_session, argc, argv );

	return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_close_session( pam_handle_t * pamh, int flags, int argc, const char **argv )
{
	pam_session( pamh, close_session, argc, argv );

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
