/*
 * Copyright (c) [2011-2012] Novell, Inc.
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
#include <dbus/dbus.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define CDBUS_SIG_LIST_SNAPS_RSP "a(uquxussa{ss})"
#define CDBUS_SIG_LIST_CONFS_RSP "a(ssa{ss})"
#define CDBUS_SIG_CREATE_SNAP_RSP "u"
#define CDBUS_SIG_DEL_SNAPS_RSP ""
#define CDBUS_SIG_STRING_DICT "{ss}"

struct dict {
	char *key;
	char *val;
};

struct snap {
	uint32_t id;
	uint16_t type;
	uint32_t pre_id;
	int64_t time;
	uint32_t creator_uid;
	char *desc;
	char *cleanup;
	uint32_t num_user_data;
	struct dict *user_data;
};

struct config {
	char *name;
	char *mnt;
	uint32_t num_attrs;
	struct dict *attrs;
};

static DBusConnection *cdbus_conn(void)
{
	DBusError err;
	DBusConnection *conn;

	dbus_error_init(&err);

	/* bus connection */
	conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err)) {
		fprintf(stderr, "connection error: %s\n", err.message);
		dbus_error_free(&err);
	}
	if (NULL == conn) {
		return NULL;
	}

	return conn;
}

static int cdbus_msg_send(DBusConnection *conn,
			  DBusMessage *msg,
			  DBusPendingCall **pend_out)
{
	DBusPendingCall *pending;

	/* send message and get a handle for a reply */
	if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
		return -ENOMEM;
	}
	if (NULL == pending) {
		fprintf(stderr, "Pending Call Null\n");
		return -EINVAL;
	}

	dbus_connection_flush(conn);
	*pend_out = pending;

	return 0;
}

static int cdbus_msg_recv(DBusConnection *conn,
			  DBusPendingCall *pending,
			  DBusMessage **msg_out)
{
	DBusMessage *msg;

	/* block until we receive a reply */
	dbus_pending_call_block(pending);

	/* get the reply message */
	msg = dbus_pending_call_steal_reply(pending);
	if (msg == NULL) {
		fprintf(stderr, "Reply Null\n");
		return -ENOMEM;
	}
	/* free the pending message handle */
	dbus_pending_call_unref(pending);
	*msg_out = msg;

	return 0;
}


static int cdbus_list_snaps_pack(char *snapper_conf,
				 DBusMessage **req_msg_out)
{
	DBusMessage *msg;
	DBusMessageIter args;

	msg = dbus_message_new_method_call("org.opensuse.Snapper", /* target for the method call */
					   "/org/opensuse/Snapper", /* object to call on */
					   "org.opensuse.Snapper", /* interface to call on */
					   "ListSnapshots"); /* method name */
	if (NULL == msg) {
		fprintf(stderr, "Message Null\n");
		return -ENOMEM;
	}

	/* append arguments */
	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING,
					    &snapper_conf)) {
		fprintf(stderr, "Out Of Memory!\n");
		return -ENOMEM;
	}

	*req_msg_out = msg;

	return 0;
}

static int cdbus_type_check(DBusMessageIter *iter, int expected_type)
{
	int type = dbus_message_iter_get_arg_type(iter);
	if (type != expected_type) {
		fprintf(stderr, "got type %d, expecting %d\n",
			type, expected_type);
		return -EINVAL;
	}

	return 0;
}

static int cdbus_type_check_get(DBusMessageIter *iter, int expected_type,
				void *val)
{
	int ret;
	ret = cdbus_type_check(iter, expected_type);
	if (ret < 0) {
		return ret;
	}

	dbus_message_iter_get_basic(iter, val);

	return 0;
}

static int dict_unpack(DBusMessageIter *iter,
		       struct dict *dict_out)

{
	int ret;
	DBusMessageIter dct_iter;

	if (cdbus_type_check(iter, DBUS_TYPE_DICT_ENTRY) < 0) {
		return -EINVAL;
	}
	dbus_message_iter_recurse(iter, &dct_iter);

	ret = cdbus_type_check_get(&dct_iter, DBUS_TYPE_STRING, &dict_out->key);
	if (ret < 0) {
		return ret;
	}

	dbus_message_iter_next(&dct_iter);
	ret = cdbus_type_check_get(&dct_iter, DBUS_TYPE_STRING, &dict_out->val);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void dict_array_print(uint32_t num_dicts,
			     struct dict *dicts)
{
	int i;

	for (i = 0; i < num_dicts; i++) {
		printf("dict (\n"
		       "\tkey: %s\n"
		       "\tval: %s\n"
		       ")\n",
		       dicts[i].key, dicts[i].val);
	}
}

static int dict_array_unpack(DBusMessageIter *iter,
			     uint32_t *num_dicts_out,
			     struct dict **dicts_out)
{
	int ret;
	DBusMessageIter array_iter;
	uint32_t num_dicts;
	struct dict *dicts = NULL;

	if (cdbus_type_check(iter, DBUS_TYPE_ARRAY) < 0) {
		return -EINVAL;
	}
	dbus_message_iter_recurse(iter, &array_iter);

	num_dicts = 0;
	while (dbus_message_iter_get_arg_type(&array_iter)
							!= DBUS_TYPE_INVALID) {
		num_dicts++;
		dicts = realloc(dicts, sizeof(struct dict) * num_dicts);
		if (dicts == NULL)
			abort();

		ret = dict_unpack(&array_iter, &dicts[num_dicts - 1]);
		if (ret < 0) {
			free(dicts);
			return ret;
		}
		dbus_message_iter_next(&array_iter);
	}

	*num_dicts_out = num_dicts;
	*dicts_out = dicts;

	return 0;
}

static int snap_struct_unpack(DBusMessageIter *iter,
			      struct snap *snap_out)
{
	int ret;
	DBusMessageIter st_iter;

	if (cdbus_type_check(iter, DBUS_TYPE_STRUCT) < 0) {
		return -EINVAL;
	}
	dbus_message_iter_recurse(iter, &st_iter);

	ret = cdbus_type_check_get(&st_iter, DBUS_TYPE_UINT32,
				&snap_out->id);
	if (ret < 0) {
		return ret;
	}

	dbus_message_iter_next(&st_iter);
	ret = cdbus_type_check_get(&st_iter, DBUS_TYPE_UINT16,
				   &snap_out->type);
	if (ret < 0) {
		return ret;
	}

	dbus_message_iter_next(&st_iter);
	ret = cdbus_type_check_get(&st_iter, DBUS_TYPE_UINT32,
				   &snap_out->pre_id);
	if (ret < 0) {
		return ret;
	}

	dbus_message_iter_next(&st_iter);
	ret = cdbus_type_check_get(&st_iter, DBUS_TYPE_INT64,
				   &snap_out->time);
	if (ret < 0) {
		return ret;
	}

	dbus_message_iter_next(&st_iter);
	ret = cdbus_type_check_get(&st_iter, DBUS_TYPE_UINT32,
				   &snap_out->creator_uid);
	if (ret < 0) {
		return ret;
	}

	dbus_message_iter_next(&st_iter);
	ret = cdbus_type_check_get(&st_iter, DBUS_TYPE_STRING,
				   &snap_out->desc);
	if (ret < 0) {
		return ret;
	}

	dbus_message_iter_next(&st_iter);
	ret = cdbus_type_check_get(&st_iter, DBUS_TYPE_STRING,
				   &snap_out->cleanup);
	if (ret < 0) {
		return ret;
	}

	dbus_message_iter_next(&st_iter);
	ret = dict_array_unpack(&st_iter, &snap_out->num_user_data,
				&snap_out->user_data);

	return ret;
}

static void snap_array_free(int32_t num_snaps,
			    struct snap *snaps)
{
	int i;

	for (i = 0; i < num_snaps; i++) {
		free(snaps[i].user_data);
	}
	free(snaps);
}

static void snap_array_print(int32_t num_snaps,
			     struct snap *snaps)
{
	int i;

	for (i = 0; i < num_snaps; i++) {
		printf("id: %u\n"
		       "type: %u\n"
		       "pre_id: %u\n"
		       "time: %" PRId64 "\n"
		       "creator_uid: %u\n"
		       "desc: %s\n"
		       "cleanup: %s\n",
		       snaps[i].id,
		       snaps[i].type,
		       snaps[i].pre_id,
		       snaps[i].time,
		       snaps[i].creator_uid,
		       snaps[i].desc,
		       snaps[i].cleanup);
		dict_array_print(snaps[i].num_user_data, snaps[i].user_data);
		printf("---\n");
	}
}

static int snap_array_unpack(DBusMessageIter *iter,
			     uint32_t *num_snaps_out,
			     struct snap **snaps_out)
{
	uint32_t num_snaps;
	int ret;
	struct snap *snaps = NULL;
	DBusMessageIter array_iter;


	if (cdbus_type_check(iter, DBUS_TYPE_ARRAY) < 0) {
		return -EINVAL;
	}
	dbus_message_iter_recurse(iter, &array_iter);

	num_snaps = 0;
	while (dbus_message_iter_get_arg_type(&array_iter)
							!= DBUS_TYPE_INVALID) {
		num_snaps++;
		snaps = realloc(snaps, sizeof(struct snap) * num_snaps);
		if (snaps == NULL)
			abort();

		ret = snap_struct_unpack(&array_iter, &snaps[num_snaps - 1]);
		if (ret < 0) {
			free(snaps);
			return ret;
		}
		dbus_message_iter_next(&array_iter);
	}

	*num_snaps_out = num_snaps;
	*snaps_out = snaps;

	return 0;
}

static int cdbus_list_snaps_unpack(DBusConnection *conn,
				   DBusMessage *rsp_msg,
				   uint32_t *num_snaps_out,
				   struct snap **snaps_out)
{
	int ret;
	DBusMessageIter iter;
	int msg_type;
	uint32_t num_snaps;
	struct snap *snaps;
	const char *sig;

	msg_type = dbus_message_get_type(rsp_msg);
	if (msg_type == DBUS_MESSAGE_TYPE_ERROR) {
		fprintf(stderr, "list_snaps error response: %s\n",
			dbus_message_get_error_name(rsp_msg));
		return -EINVAL;
	}

	if (msg_type != DBUS_MESSAGE_TYPE_METHOD_RETURN) {
		fprintf(stderr, "unexpected list_snaps ret type: %d\n",
			msg_type);
		return -EINVAL;
	}

	sig = dbus_message_get_signature(rsp_msg);
	if ((sig == NULL)
	 || (strcmp(sig, CDBUS_SIG_LIST_SNAPS_RSP) != 0)) {
		fprintf(stderr, "bad list snaps response sig: %s, "
				"expected: %s\n",
			(sig ? sig : "NULL"), CDBUS_SIG_LIST_SNAPS_RSP);
		return -EINVAL;
	}

	/* read the parameters */
	if (!dbus_message_iter_init(rsp_msg, &iter)) {
		fprintf(stderr, "Message has no arguments!\n");
		return -EINVAL;
	}

	ret = snap_array_unpack(&iter, &num_snaps, &snaps);
	if (ret < 0) {
		fprintf(stderr, "failed to unpack snap array\n");
		return -EINVAL;
	}

	*num_snaps_out = num_snaps;
	*snaps_out = snaps;

	return 0;
}

static int cdbus_list_snaps_call(DBusConnection *conn)
{
	int ret;
	DBusMessage *req_msg;
	DBusMessage *rsp_msg;
	DBusPendingCall *pending;
	uint32_t num_snaps = 0;
	struct snap *snaps = NULL;

	ret = cdbus_list_snaps_pack("root", &req_msg);
	if (ret < 0) {
		fprintf(stderr, "failed to pack list snaps request\n");
		return ret;
	}

	ret = cdbus_msg_send(conn, req_msg, &pending);
	if (ret < 0) {
		dbus_message_unref(req_msg);
		return ret;
	}

	ret = cdbus_msg_recv(conn, pending, &rsp_msg);
	if (ret < 0) {
		dbus_message_unref(req_msg);
		dbus_pending_call_unref(pending);
		return ret;
	}

	ret = cdbus_list_snaps_unpack(conn, rsp_msg,
				      &num_snaps, &snaps);
	if (ret < 0) {
		fprintf(stderr, "failed to unpack list snaps response\n");
		dbus_message_unref(req_msg);
		dbus_message_unref(rsp_msg);
		return ret;
	}

	snap_array_print(num_snaps, snaps);

	snap_array_free(num_snaps, snaps);

	dbus_message_unref(req_msg);
	dbus_message_unref(rsp_msg);

	return 0;
}

static int cdbus_list_confs_pack(DBusMessage **req_msg_out)
{
	DBusMessage *msg;

	msg = dbus_message_new_method_call("org.opensuse.Snapper",
					   "/org/opensuse/Snapper",
					   "org.opensuse.Snapper",
					   "ListConfigs");
	if (NULL == msg) {
		fprintf(stderr, "Message Null\n");
		return -ENOMEM;
	}

	/* no arguments to append */
	*req_msg_out = msg;

	return 0;
}

static int conf_struct_unpack(DBusMessageIter *iter,
			      struct config *conf_out)
{
	int ret;
	DBusMessageIter st_iter;

	if (cdbus_type_check(iter, DBUS_TYPE_STRUCT) < 0) {
		return -EINVAL;
	}
	dbus_message_iter_recurse(iter, &st_iter);

	ret = cdbus_type_check_get(&st_iter, DBUS_TYPE_STRING,
				   &conf_out->name);
	if (ret < 0) {
		return ret;
	}

	dbus_message_iter_next(&st_iter);
	ret = cdbus_type_check_get(&st_iter, DBUS_TYPE_STRING,
				   &conf_out->mnt);
	if (ret < 0) {
		return ret;
	}

	dbus_message_iter_next(&st_iter);
	ret = dict_array_unpack(&st_iter, &conf_out->num_attrs,
				&conf_out->attrs);

	return ret;
}

static void conf_array_free(int32_t num_confs,
			    struct config *confs)
{
	int i;

	for (i = 0; i < num_confs; i++) {
		free(confs[i].attrs);
	}
	free(confs);
}

static void conf_array_print(int32_t num_confs,
			     struct config *confs)
{
	int i;

	for (i = 0; i < num_confs; i++) {
		printf("name: %s\n"
		       "mnt: %s\n",
		       confs[i].name, confs[i].mnt);
		dict_array_print(confs[i].num_attrs, confs[i].attrs);
		printf("---\n");
	}
}

static int conf_array_unpack(DBusMessageIter *iter,
			     uint32_t *num_confs_out,
			     struct config **confs_out)
{
	uint32_t num_confs;
	int ret;
	struct config *confs = NULL;
	DBusMessageIter array_iter;


	if (cdbus_type_check(iter, DBUS_TYPE_ARRAY) < 0) {
		return -EINVAL;
	}
	dbus_message_iter_recurse(iter, &array_iter);

	num_confs = 0;
	while (dbus_message_iter_get_arg_type(&array_iter)
							!= DBUS_TYPE_INVALID) {
		num_confs++;
		confs = realloc(confs, sizeof(struct config) * num_confs);
		if (confs == NULL)
			abort();

		ret = conf_struct_unpack(&array_iter, &confs[num_confs - 1]);
		if (ret < 0) {
			free(confs);
			return ret;
		}
		dbus_message_iter_next(&array_iter);
	}

	*num_confs_out = num_confs;
	*confs_out = confs;

	return 0;
}

static int cdbus_list_confs_unpack(DBusConnection *conn,
				   DBusMessage *rsp_msg,
				   uint32_t *num_confs_out,
				   struct config **confs_out)
{
	int ret;
	DBusMessageIter iter;
	int msg_type;
	uint32_t num_confs;
	struct config *confs;
	const char *sig;

	msg_type = dbus_message_get_type(rsp_msg);
	if (msg_type == DBUS_MESSAGE_TYPE_ERROR) {
		fprintf(stderr, "list_confs error response: %s\n",
			dbus_message_get_error_name(rsp_msg));
		return -EINVAL;
	}

	if (msg_type != DBUS_MESSAGE_TYPE_METHOD_RETURN) {
		fprintf(stderr, "unexpected list_confs ret type: %d\n",
			msg_type);
		return -EINVAL;
	}

	sig = dbus_message_get_signature(rsp_msg);
	if ((sig == NULL)
	 || (strcmp(sig, CDBUS_SIG_LIST_CONFS_RSP) != 0)) {
		fprintf(stderr, "bad list confs response sig: %s, "
				"expected: %s\n",
			(sig ? sig : "NULL"), CDBUS_SIG_LIST_CONFS_RSP);
		return -EINVAL;
	}

	if (!dbus_message_iter_init(rsp_msg, &iter)) {
		/* FIXME return empty? */
		fprintf(stderr, "Message has no arguments!\n");
		return -EINVAL;
	}

	ret = conf_array_unpack(&iter, &num_confs, &confs);
	if (ret < 0) {
		fprintf(stderr, "failed to unpack conf array\n");
		return -EINVAL;
	}

	*num_confs_out = num_confs;
	*confs_out = confs;

	return 0;
}

static int cdbus_list_confs_call(DBusConnection *conn)
{
	int ret;
	DBusMessage *req_msg;
	DBusMessage *rsp_msg;
	DBusPendingCall *pending;
	uint32_t num_confs = 0;
	struct config *confs = NULL;
	const char *sig;

	ret = cdbus_list_confs_pack(&req_msg);
	if (ret < 0) {
		fprintf(stderr, "failed to pack list confs request\n");
		return ret;
	}

	ret = cdbus_msg_send(conn, req_msg, &pending);
	if (ret < 0) {
		dbus_message_unref(req_msg);
		return ret;
	}

	ret = cdbus_msg_recv(conn, pending, &rsp_msg);
	if (ret < 0) {
		dbus_message_unref(req_msg);
		dbus_pending_call_unref(pending);
		return ret;
	}

	sig = dbus_message_get_signature(rsp_msg);
	if ((sig == NULL)
	 || (strcmp(sig, CDBUS_SIG_LIST_CONFS_RSP) != 0)) {
		fprintf(stderr, "bad list confs response sig: %s, "
				"expected: %s\n",
			(sig ? sig : "NULL"), CDBUS_SIG_LIST_CONFS_RSP);
		dbus_message_unref(req_msg);
		dbus_message_unref(rsp_msg);
		return -EINVAL;
	}

	ret = cdbus_list_confs_unpack(conn, rsp_msg,
				      &num_confs, &confs);
	if (ret < 0) {
		fprintf(stderr, "failed to unpack list confs response\n");
		dbus_message_unref(req_msg);
		dbus_message_unref(rsp_msg);
		return ret;
	}

	conf_array_print(num_confs, confs);

	conf_array_free(num_confs, confs);

	dbus_message_unref(req_msg);
	dbus_message_unref(rsp_msg);

	return 0;
}

static int cdbus_create_snap_pack(char *snapper_conf,
				  char *desc,
				  uint32_t num_user_data,
				  struct dict *user_data,
				  DBusMessage **req_msg_out)
{
	DBusMessage *msg;
	DBusMessageIter args;
	DBusMessageIter array_iter;
	DBusMessageIter struct_iter;
	const char *empty = "";
	uint32_t i;
	bool ret;

	msg = dbus_message_new_method_call("org.opensuse.Snapper", /* target for the method call */
					   "/org/opensuse/Snapper", /* object to call on */
					   "org.opensuse.Snapper", /* interface to call on */
					   "CreateSingleSnapshot"); /* method name */
	if (msg == NULL) {
		fprintf(stderr, "failed to create req msg\n");
		return -ENOMEM;
	}

	/* append arguments */
	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING,
					    &snapper_conf)) {
		fprintf(stderr, "Out Of Memory!\n");
		return -ENOMEM;
	}

	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING,
					    &desc)) {
		fprintf(stderr, "Out Of Memory!\n");
		return -ENOMEM;
	}

	/* cleanup */
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING,
					    &empty)) {
		fprintf(stderr, "Out Of Memory!\n");
		return -ENOMEM;
	}

	ret = dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY,
					       CDBUS_SIG_STRING_DICT,
					       &array_iter);
	if (!ret) {
		fprintf(stderr, "failed to open array container\n");
		return -ENOMEM;
	}

	for (i = 0; i < num_user_data; i++) {
		ret = dbus_message_iter_open_container(&array_iter,
						       DBUS_TYPE_DICT_ENTRY,
						       NULL, &struct_iter);
		if (!ret) {
			fprintf(stderr, "failed to open struct container\n");
			return -ENOMEM;
		}

		if (!dbus_message_iter_append_basic(&struct_iter,
						    DBUS_TYPE_STRING,
						    &user_data[i].key)) {
			fprintf(stderr, "Out Of Memory!\n");
			return -ENOMEM;
		}
		if (!dbus_message_iter_append_basic(&struct_iter,
						    DBUS_TYPE_STRING,
						    &user_data[i].val)) {
			fprintf(stderr, "Out Of Memory!\n");
			return -ENOMEM;
		}

		ret = dbus_message_iter_close_container(&array_iter,
							&struct_iter);
		if (!ret) {
			fprintf(stderr, "failed to close struct container\n");
			return -ENOMEM;
		}
	}

	dbus_message_iter_close_container(&args, &array_iter);

	*req_msg_out = msg;

	return 0;
}

static int cdbus_create_snap_unpack(DBusConnection *conn,
				    DBusMessage *rsp_msg,
				    uint32_t *snap_id_out)
{
	DBusMessageIter iter;
	int msg_type;
	const char *sig;

	msg_type = dbus_message_get_type(rsp_msg);
	if (msg_type == DBUS_MESSAGE_TYPE_ERROR) {
		fprintf(stderr, "create snap error response: %s\n",
			dbus_message_get_error_name(rsp_msg));
		return -EINVAL;
	}

	if (msg_type != DBUS_MESSAGE_TYPE_METHOD_RETURN) {
		fprintf(stderr, "unexpected create snap ret type: %d\n",
			msg_type);
		return -EINVAL;
	}

	sig = dbus_message_get_signature(rsp_msg);
	if ((sig == NULL)
	 || (strcmp(sig, CDBUS_SIG_CREATE_SNAP_RSP) != 0)) {
		fprintf(stderr, "bad create snap response sig: %s, "
				"expected: %s\n",
			(sig ? sig : "NULL"), CDBUS_SIG_CREATE_SNAP_RSP);
		return -EINVAL;
	}

	/* read the parameters */
	if (!dbus_message_iter_init(rsp_msg, &iter)) {
		fprintf(stderr, "Message has no arguments!\n");
		return -EINVAL;
	}

	if (cdbus_type_check_get(&iter, DBUS_TYPE_UINT32, snap_id_out)) {
		return -EINVAL;
	}

	return 0;
}


static int cdbus_create_snap_call(DBusConnection *conn)
{
	int ret;
	DBusMessage *req_msg;
	DBusMessage *rsp_msg;
	DBusPendingCall *pending;
	uint32_t snap_id;
	struct dict user_data[1];

	user_data[0].key = "key data";
	user_data[0].val = "val data";

	ret = cdbus_create_snap_pack("root", "this is a desc", 1, user_data,
				     &req_msg);
	if (ret < 0) {
		fprintf(stderr, "failed to pack create snap request\n");
		return ret;
	}

	ret = cdbus_msg_send(conn, req_msg, &pending);
	if (ret < 0) {
		dbus_message_unref(req_msg);
		return ret;
	}

	ret = cdbus_msg_recv(conn, pending, &rsp_msg);
	if (ret < 0) {
		dbus_message_unref(req_msg);
		dbus_pending_call_unref(pending);
		return ret;
	}

	ret = cdbus_create_snap_unpack(conn, rsp_msg, &snap_id);
	if (ret < 0) {
		fprintf(stderr, "failed to unpack create snap response\n");
		dbus_message_unref(req_msg);
		dbus_message_unref(rsp_msg);
		return ret;
	}

	printf("created new snapshot %u\n", snap_id);

	dbus_message_unref(req_msg);
	dbus_message_unref(rsp_msg);

	return 0;
}

static int cdbus_del_snap_pack(char *snapper_conf,
			       uint32_t snap_id,
			       DBusMessage **req_msg_out)
{
	DBusMessage *msg;
	DBusMessageIter args;
	DBusMessageIter array_iter;
	bool ret;

	msg = dbus_message_new_method_call("org.opensuse.Snapper",
					   "/org/opensuse/Snapper",
					   "org.opensuse.Snapper",
					   "DeleteSnapshots");
	if (msg == NULL) {
		fprintf(stderr, "failed to create req msg\n");
		return -ENOMEM;
	}

	/* append arguments */
	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING,
					    &snapper_conf)) {
		fprintf(stderr, "Out Of Memory!\n");
		return -ENOMEM;
	}

	ret = dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY,
					       DBUS_TYPE_UINT32_AS_STRING,
					       &array_iter);
	if (!ret) {
		fprintf(stderr, "failed to open array container\n");
		return -ENOMEM;
	}

	if (!dbus_message_iter_append_basic(&array_iter,
					    DBUS_TYPE_UINT32,
					    &snap_id)) {
		fprintf(stderr, "Out Of Memory!\n");
		return -ENOMEM;
	}

	dbus_message_iter_close_container(&args, &array_iter);

	*req_msg_out = msg;

	return 0;
}

static int cdbus_del_snap_unpack(DBusConnection *conn,
				 DBusMessage *rsp_msg)
{
	int msg_type;
	const char *sig;

	msg_type = dbus_message_get_type(rsp_msg);
	if (msg_type == DBUS_MESSAGE_TYPE_ERROR) {
		fprintf(stderr, "del snap error response: %s\n",
			dbus_message_get_error_name(rsp_msg));
		return -EINVAL;
	}

	if (msg_type != DBUS_MESSAGE_TYPE_METHOD_RETURN) {
		fprintf(stderr, "unexpected del snap ret type: %d\n",
			msg_type);
		return -EINVAL;
	}

	sig = dbus_message_get_signature(rsp_msg);
	if ((sig == NULL)
	 || (strcmp(sig, CDBUS_SIG_DEL_SNAPS_RSP) != 0)) {
		fprintf(stderr, "bad del snap response sig: %s, "
				"expected: %s\n",
			(sig ? sig : "NULL"), CDBUS_SIG_DEL_SNAPS_RSP);
		return -EINVAL;
	}

	/* no parameters in response */

	return 0;
}
static int cdbus_del_snap_call(DBusConnection *conn,
			       uint32_t snap_id)
{
	int ret;
	DBusMessage *req_msg;
	DBusMessage *rsp_msg;
	DBusPendingCall *pending;

	ret = cdbus_del_snap_pack("root", snap_id, &req_msg);
	if (ret < 0) {
		fprintf(stderr, "failed to pack del snap request\n");
		return ret;
	}

	ret = cdbus_msg_send(conn, req_msg, &pending);
	if (ret < 0) {
		dbus_message_unref(req_msg);
		return ret;
	}

	ret = cdbus_msg_recv(conn, pending, &rsp_msg);
	if (ret < 0) {
		dbus_message_unref(req_msg);
		dbus_pending_call_unref(pending);
		return ret;
	}

	ret = cdbus_del_snap_unpack(conn, rsp_msg);
	if (ret < 0) {
		fprintf(stderr, "failed to unpack del snap response\n");
		dbus_message_unref(req_msg);
		dbus_message_unref(rsp_msg);
		return ret;
	}

	printf("deleted snapshot %u\n", snap_id);

	dbus_message_unref(req_msg);
	dbus_message_unref(rsp_msg);

	return 0;
}

int main(int argc, char **argv)
{
	DBusConnection *conn;
	int ret = -EINVAL;

	if (argc < 2) {
		fprintf(stderr, "Syntax: %s <list_confs | list_snaps | create_snap | del_snap> <arg>\n",
			argv[0]);
		return -EINVAL;
	}

	conn = cdbus_conn();
	if (conn == NULL) {
		fprintf(stderr, "connect failed\n");
		return -ENOMEM;
	}

	if (!strcmp(argv[1], "list_confs")) {
		ret = cdbus_list_confs_call(conn);
	} else if (!strcmp(argv[1], "list_snaps")) {
		ret = cdbus_list_snaps_call(conn);
	} else if (!strcmp(argv[1], "create_snap")) {
		ret = cdbus_create_snap_call(conn);
	} else if (!strcmp(argv[1], "del_snap")) {
		if (argc < 3) {
			fprintf(stderr, "del_snap requires a snapshot_id argument\n");
			return -EINVAL;
		}
		ret = cdbus_del_snap_call(conn, atoi(argv[2]));
	} else {
		fprintf(stderr, "Syntax: %s <list_confs | list_snaps | create_snap> | del_snap> <arg>\n",
			argv[0]);
		ret = -EINVAL;
		goto err_conn_close;
	}

	if (ret < 0) {
		fprintf(stderr, "%s call failed: %s\n",
			argv[1], strerror(-ret));
	}

err_conn_close:
	dbus_connection_unref(conn);

	return ret;
}
