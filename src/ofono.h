/*
 *
 *  oFono - Open Source Telephony
 *
 *  Copyright (C) 2008-2009  Intel Corporation. All rights reserved.
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

#include <glib.h>

#define OFONO_API_SUBJECT_TO_CHANGE

int __ofono_manager_init();
void __ofono_manager_cleanup();

const char **__ofono_modem_get_list();

#include <ofono/log.h>

int __ofono_log_init(gboolean detach, gboolean debug);
void __ofono_log_cleanup(void);

void __ofono_toggle_debug(void);

#include <ofono/dbus.h>

int __ofono_dbus_init(DBusConnection *conn);
void __ofono_dbus_cleanup(void);

DBusMessage *__ofono_error_invalid_args(DBusMessage *msg);
DBusMessage *__ofono_error_invalid_format(DBusMessage *msg);
DBusMessage *__ofono_error_not_implemented(DBusMessage *msg);
DBusMessage *__ofono_error_failed(DBusMessage *msg);
DBusMessage *__ofono_error_busy(DBusMessage *msg);
DBusMessage *__ofono_error_not_found(DBusMessage *msg);
DBusMessage *__ofono_error_not_active(DBusMessage *msg);
DBusMessage *__ofono_error_not_supported(DBusMessage *msg);
DBusMessage *__ofono_error_timed_out(DBusMessage *msg);

void __ofono_dbus_pending_reply(DBusMessage **msg, DBusMessage *reply);

#include <ofono/types.h>

#include <ofono/plugin.h>

int __ofono_plugin_init(const char *pattern, const char *exclude);
void __ofono_plugin_cleanup(void);

struct ofono_modem {
	int		id;
	char		*path;

	void 		*userdata;

	GSList *ss_control_list;
	GSList *ss_passwd_list;

	GSList		*atoms;

	struct ofono_modem_data *modem_info;
	struct network_registration_data *network_registration;
	struct voicecalls_data *voicecalls;
	struct ofono_call_forwarding *call_forwarding;
	struct ussd_data *ussd;
	struct ofono_call_settings *call_settings;
	struct call_waiting_data *call_waiting;
	struct ofono_call_barring *call_barring;
	struct cssn_data *cssn;
	struct sim_manager_data *sim_manager;
	struct sms_manager_data *sms_manager;
	struct phonebook_data *phonebook;
	struct message_waiting_data *message_waiting;

	GSList *history_contexts;
};

#include <ofono/modem.h>

unsigned int __ofono_modem_alloc_callid(struct ofono_modem *modem);
void __ofono_modem_release_callid(struct ofono_modem *modem, int id);

struct ofono_atom;

enum ofono_atom_type {
	OFONO_ATOM_TYPE_INFO = 0,
	OFONO_ATOM_TYPE_CALL_BARRING = 1,
	OFONO_ATOM_TYPE_CALL_FORWARDING = 2,
	OFONO_ATOM_TYPE_CALL_METER = 3,
	OFONO_ATOM_TYPE_CALL_SETTINGS = 4,
	OFONO_ATOM_TYPE_NETWORK_REGISTRATION = 5,
	OFONO_ATOM_TYPE_PHONEBOOK = 6,
	OFONO_ATOM_TYPE_SMS = 7,
	OFONO_ATOM_TYPE_SIM = 8,
	OFONO_ATOM_TYPE_USSD = 9,
	OFONO_ATOM_TYPE_VOICECALL = 10,
	OFONO_ATOM_TYPE_HISTORY = 11
};

struct ofono_atom *__ofono_modem_add_atom(struct ofono_modem *modem,
					enum ofono_atom_type type,
					void (*destruct)(struct ofono_atom *),
					void *data);

struct ofono_atom *__ofono_modem_find_atom(struct ofono_modem *modem,
						enum ofono_atom_type type);

void *__ofono_atom_get_data(struct ofono_atom *atom);

void __ofono_atom_register(struct ofono_atom *atom,
				void (*unregister)(struct ofono_atom *));
void __ofono_atom_unregister(struct ofono_atom *atom);

void __ofono_modem_remove_atom(struct ofono_modem *modem,
				struct ofono_atom *atom);

#include <ofono/call-barring.h>
#include <ofono/call-forwarding.h>
#include <ofono/call-meter.h>
#include <ofono/call-settings.h>

#include <ofono/history.h>

void __ofono_history_probe_drivers(struct ofono_modem *modem);
void __ofono_history_remove_drivers(struct ofono_modem *modem);

void __ofono_history_call_ended(struct ofono_modem *modem,
				const struct ofono_call *call,
				time_t start, time_t end);

void __ofono_history_call_missed(struct ofono_modem *modem,
				const struct ofono_call *call, time_t when);

struct sms;

int ofono_message_waiting_register(struct ofono_modem *modem);
void ofono_message_waiting_unregister(struct ofono_modem *modem);
void ofono_handle_sms_mwi(struct ofono_modem *modem,
				struct sms *sms, gboolean *out_discard);
