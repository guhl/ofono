/*
 *
 *  RIL library with GLib integration
 *
 *  Copyright (C) 2008-2011  Intel Corporation. All rights reserved.
 *  Copyright (C) 2012-2013  Canonical Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <glib.h>

#include <ofono/log.h>
#include <ofono/modem.h>
#include <ofono/gprs-context.h>

#include "grilunsol.h"

/* Minimum size is two int32s version/number of calls */
#define MIN_DATA_CALL_LIST_SIZE 8
/* Default call interface */
#define DEFAULT_CALL_INTERFACE "rmnet0"

static gint data_call_compare(gconstpointer a, gconstpointer b)
{
	const struct data_call *ca = a;
	const struct data_call *cb = b;

	if (ca->cid < cb->cid)
		return -1;

	if (ca->cid > cb->cid)
		return 1;

	return 0;
}

static void free_data_call(gpointer data, gpointer user_data)
{
	struct data_call *call = data;

	if (call) {
		if (call->type)
			g_free(call->type);
		if (call->ifname)
			g_free(call->ifname);
		if (call->addresses)
			g_free(call->addresses);
		if (call->dnses)
			g_free(call->dnses);
		if (call->gateways)
			g_free(call->gateways);
		g_free(call);
	}
}

void g_ril_unsol_free_data_call_list(struct unsol_data_call_list *unsol)
{
	if (unsol) {
		g_slist_foreach(unsol->call_list, (GFunc) free_data_call, NULL);
		g_slist_free(unsol->call_list);
		g_free(unsol);
	}
}

gboolean g_ril_unsol_cmp_dcl(struct unsol_data_call_list *current,
				struct unsol_data_call_list *old,
				gint cid)
{
	GSList *nl,*ol;
	struct data_call *new_call, *old_call;

	new_call = old_call = NULL;
	gboolean no_cid = TRUE;


	if (!current || !old)
		return FALSE;

	if (current->num != old->num)
		return FALSE;

	for (nl = current->call_list; nl; nl = nl->next) {
		new_call = (struct data_call *) nl->data;

		if (new_call->cid != cid)
			continue;

		for (ol = old->call_list; ol; ol = ol->next) {
			old_call = (struct data_call *) ol->data; 
			if(new_call->cid == old_call->cid) {
				no_cid = FALSE;
				break;
			}
		}
		if (no_cid)
			return FALSE;

		if (new_call->active != old_call->active)
			return FALSE;
		if (g_strcmp0(new_call->type,old_call->type))
			return FALSE;
		if (g_strcmp0(new_call->ifname,old_call->ifname))
			return FALSE;
		if (g_strcmp0(new_call->addresses,old_call->addresses))
			return FALSE;
		if (g_strcmp0(new_call->dnses,old_call->dnses))
			return FALSE;
		if (g_strcmp0(new_call->gateways,old_call->gateways))
			return FALSE;
	}
	if (no_cid)
		return FALSE;

	return TRUE;
}

struct unsol_data_call_list *g_ril_unsol_parse_data_call_list(GRil *gril,
					struct ril_msg *message,
					struct ofono_error *error)
{
	struct data_call *call;
	struct parcel rilp;
	struct unsol_data_call_list *reply =
		g_new0(struct unsol_data_call_list, 1);
	int i;
	char *apn;

	DBG("");

	OFONO_NO_ERROR(error);

	if (message->buf_len < MIN_DATA_CALL_LIST_SIZE) {
		ofono_error("%s: message too small: %d",
				__func__,
				(int) message->buf_len);
		OFONO_EINVAL(error);
		goto error;
	}

	g_ril_init_parcel(message, &rilp);

	/*
	 * ril.h documents the reply to a RIL_REQUEST_DATA_CALL_LIST
	 * as being an array of  RIL_Data_Call_Response_v6 structs,
	 * however in reality, the response also includes a version
	 * to start.
	 */
	reply->version = parcel_r_int32(&rilp);
	reply->num = parcel_r_int32(&rilp);

	g_ril_append_print_buf(gril,
				"(version=%d,num=%d",
				reply->version,
				reply->num);
	
	if (reply->version < 5) {
		/* in version 4 there will not be more then one valid call response */
		call = g_new0(struct data_call, 1);
                call->cid = parcel_r_int32(&rilp);
                call->active = parcel_r_int32(&rilp);
                call->type = parcel_r_string(&rilp);
		if (reply->version < 4)
			apn = parcel_r_string(&rilp); // apn not used
                call->addresses = parcel_r_string(&rilp);
		/* we need an interface but don't get one in V4 so use the default */
		call->ifname = g_strdup(DEFAULT_CALL_INTERFACE);

                g_ril_append_print_buf(gril,
                                        "%s [status=%d,retry=%d,cid=%d,"
                                        "active=%d,type=%s,ifname=%s,"
                                        "address=%s,dns=%s,gateways=%s]",
                                        print_buf,
                                        call->status,
                                        call->retry,
                                        call->cid,
                                        call->active,
                                        call->type,
                                        call->ifname,
                                        call->addresses,
                                        call->dnses,
                                        call->gateways);

                reply->call_list =
                        g_slist_insert_sorted(reply->call_list,
                                                call,
                                                data_call_compare);
	} else {	
		for (i = 0; i < reply->num; i++) {
			call = g_new0(struct data_call, 1);
	
			call->status = parcel_r_int32(&rilp);
			call->retry = parcel_r_int32(&rilp);
			call->cid = parcel_r_int32(&rilp);
			call->active = parcel_r_int32(&rilp);
	
			call->type = parcel_r_string(&rilp);
			call->ifname = parcel_r_string(&rilp);
			call->addresses = parcel_r_string(&rilp);
			call->dnses = parcel_r_string(&rilp);
			call->gateways = parcel_r_string(&rilp);
	
			g_ril_append_print_buf(gril,
						"%s [status=%d,retry=%d,cid=%d,"
						"active=%d,type=%s,ifname=%s,"
						"address=%s,dns=%s,gateways=%s]",
						print_buf,
						call->status,
						call->retry,
						call->cid,
						call->active,
						call->type,
						call->ifname,
						call->addresses,
						call->dnses,
						call->gateways);

			reply->call_list =
				g_slist_insert_sorted(reply->call_list,
							call,
							data_call_compare);
		}
	}

	g_ril_append_print_buf(gril, "%s}", print_buf);
	g_ril_print_unsol(gril, message);

error:
	return reply;
}
