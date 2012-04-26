/*
 ============================================================================
 Name        : hev-main.c
 Author      : Heiher <admin@heiher.info>
 Version     : 0.0.1
 Copyright   : Copyright (C) 2012 everyone.
 Description : 
 ============================================================================
 */

#include <glib-unix.h>
#include <gio/gio.h>

#include "hev-main.h"
#include "hev-dbus-object-idcard-reader-manager.h"
#include "hev-dbus-interface-idcard-reader-manager.h"

static gboolean unix_signal_handler(gpointer user_data)
{
	GMainLoop *main_loop = user_data;

	g_main_loop_quit(main_loop);

	return FALSE;
}

static void debug_log_handler(const gchar *log_domain,
			GLogLevelFlags log_level,
			const gchar *message,
			gpointer user_data)
{
}

static void bus_acquired_handler(GDBusConnection *connection,
			const gchar *name, gpointer user_data)
{
	GDBusObjectManagerServer *server = G_DBUS_OBJECT_MANAGER_SERVER(user_data);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_dbus_object_manager_server_set_connection(server, connection);
}

static void bus_name_lost_handler(GDBusConnection *connection,
			const gchar *name, gpointer user_data)
{
	GObject *server = G_OBJECT(user_data);
	GMainLoop *main_loop = g_object_get_data(server, "main-loop");

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_main_loop_quit(main_loop);
}

int main(int argc, char *argv[])
{
	GMainLoop *main_loop = NULL;
	static gboolean debug = FALSE;
	static GOptionEntry option_entries[] =
	{
		{ "debug", 'd', 0, G_OPTION_ARG_NONE,  &debug, "Debug mode", NULL},
		{ NULL }
	};
	GOptionContext *option_context = NULL;
	GError *error = NULL;
	GDBusObjectManagerServer *server = NULL;
	GObject *dbus_obj = NULL, *dbus_iface = NULL;

	g_type_init();

	option_context = g_option_context_new("");
	g_option_context_add_main_entries(option_context,
				option_entries, NULL);
	if(!g_option_context_parse(option_context, &argc, &argv, &error))
	{
		g_error("%s:%d[%s]=>(%s)", __FILE__, __LINE__, __FUNCTION__,
					error->message);
		g_error_free(error);
	}
	g_option_context_free(option_context);

	if(!debug)
	{
		g_log_set_handler(NULL, G_LOG_LEVEL_DEBUG,
					debug_log_handler, NULL);
		daemon(1, 1);
	}

	main_loop = g_main_loop_new(NULL, FALSE);

	server = g_dbus_object_manager_server_new("/hev/idcard/Reader");
	g_object_set_data(G_OBJECT(server), "main-loop", main_loop);

	/* Reader Manager */
	dbus_obj = hev_dbus_object_idcard_reader_manager_new("/hev/idcard/Reader/Manager");
	g_object_set_data(dbus_obj, "dbus-object-manager-server", server);
	dbus_iface = hev_dbus_interface_idcard_reader_manager_new(G_DBUS_OBJECT_SKELETON(dbus_obj));
	g_dbus_object_manager_server_export(server, G_DBUS_OBJECT_SKELETON(dbus_obj));
	g_object_unref(dbus_obj);

	g_bus_own_name(G_BUS_TYPE_SYSTEM, "hev.idcard.Reader",
				G_BUS_NAME_OWNER_FLAGS_REPLACE | G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
				bus_acquired_handler, NULL, bus_name_lost_handler, server, NULL);

	g_unix_signal_add(SIGINT, unix_signal_handler, main_loop);
	g_unix_signal_add(SIGTERM, unix_signal_handler, main_loop);

	g_main_loop_run(main_loop);

	g_object_unref(server);

	g_main_loop_unref(main_loop);

	return 0;
}

