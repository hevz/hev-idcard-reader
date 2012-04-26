/*
 ============================================================================
 Name        : hev-dbus-object-idcard_reader_manager.c
 Author      : Heiher <admin@heiher.info>
 Version     : 0.0.1
 Copyright   : Copyright (C) 2011 everyone.
 Description : 
 ============================================================================
 */

#include <gudev/gudev.h>

#include "hev-dbus-object-idcard-reader-manager.h"
#include "hev-dbus-object-idcard-reader.h"
#include "hev-dbus-interface-idcard-reader.h"
#include "hev-serial-port.h"

#define HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE((obj), HEV_TYPE_DBUS_OBJECT_IDCARD_READER_MANAGER, HevDBusObjectIDCardReaderManagerPrivate))

enum
{
	SIG_ADD,
	SIG_REMOVE,
	LAST_SIGNAL
};

static guint hev_dbus_object_idcard_reader_manager_signals[LAST_SIGNAL] = { 0 };

typedef struct _HevDBusObjectIDCardReaderManagerPrivate HevDBusObjectIDCardReaderManagerPrivate;

struct _HevDBusObjectIDCardReaderManagerPrivate
{
	GUdevClient *udev_client;
	GList *device_list;
	guint8 device_max_id;
};

static void hev_serial_port_try_open(HevDBusObjectIDCardReaderManager *self,
			const gchar *device_path);
static void hev_serial_port_try_remove(HevDBusObjectIDCardReaderManager *self,
			const gchar *device_path);

G_DEFINE_TYPE(HevDBusObjectIDCardReaderManager, hev_dbus_object_idcard_reader_manager, G_TYPE_DBUS_OBJECT_SKELETON);

static void hev_serial_port_new_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data);
static void g_udev_client_uevent_handler(GUdevClient *client, gchar *action,
			GUdevDevice *device, gpointer user_data);

static void hev_dbus_object_idcard_reader_manager_dispose(GObject *obj)
{
	HevDBusObjectIDCardReaderManager *self = HEV_DBUS_OBJECT_IDCARD_READER_MANAGER(obj);
	HevDBusObjectIDCardReaderManagerPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(priv->device_list)
	{
		g_list_free_full(priv->device_list, g_object_unref);
		priv->device_list = NULL;
	}

	if(priv->udev_client)
	{
		g_object_unref(priv->udev_client);
		priv->udev_client = NULL;
	}

	G_OBJECT_CLASS(hev_dbus_object_idcard_reader_manager_parent_class)->dispose(obj);
}

static void hev_dbus_object_idcard_reader_manager_finalize(GObject *obj)
{
	HevDBusObjectIDCardReaderManager *self = HEV_DBUS_OBJECT_IDCARD_READER_MANAGER(obj);
	HevDBusObjectIDCardReaderManagerPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	G_OBJECT_CLASS(hev_dbus_object_idcard_reader_manager_parent_class)->finalize(obj);
}

static GObject * hev_dbus_object_idcard_reader_manager_constructor(GType type, guint n, GObjectConstructParam *param)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);
	return G_OBJECT_CLASS(hev_dbus_object_idcard_reader_manager_parent_class)->constructor(type, n, param);
}

static void hev_dbus_object_idcard_reader_manager_constructed(GObject *obj)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	G_OBJECT_CLASS(hev_dbus_object_idcard_reader_manager_parent_class)->constructed(obj);
}

static void hev_dbus_object_idcard_reader_manager_class_init(HevDBusObjectIDCardReaderManagerClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	obj_class->constructor = hev_dbus_object_idcard_reader_manager_constructor;
	obj_class->constructed = hev_dbus_object_idcard_reader_manager_constructed;
	obj_class->dispose = hev_dbus_object_idcard_reader_manager_dispose;
	obj_class->finalize = hev_dbus_object_idcard_reader_manager_finalize;

	/* Signals */
	hev_dbus_object_idcard_reader_manager_signals[SIG_ADD] = g_signal_new("add",
				G_TYPE_FROM_CLASS(klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET(HevDBusObjectIDCardReaderManagerClass, add),
				NULL, NULL,
				g_cclosure_marshal_VOID__STRING,
				G_TYPE_NONE, 1, G_TYPE_STRING);
	hev_dbus_object_idcard_reader_manager_signals[SIG_REMOVE] = g_signal_new("remove",
				G_TYPE_FROM_CLASS(klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET(HevDBusObjectIDCardReaderManagerClass, remove),
				NULL, NULL,
				g_cclosure_marshal_VOID__STRING,
				G_TYPE_NONE, 1, G_TYPE_STRING);

	g_type_class_add_private(klass, sizeof(HevDBusObjectIDCardReaderManagerPrivate));
}

static void hev_dbus_object_idcard_reader_manager_init(HevDBusObjectIDCardReaderManager *self)
{
	HevDBusObjectIDCardReaderManagerPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_GET_PRIVATE(self);
	const gchar *subsystems[] =
	{
		"tty",
		NULL
	};
	GList *devices = NULL, *sl = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	/* Udev client */
	priv->udev_client = g_udev_client_new(subsystems);
	g_signal_connect(priv->udev_client, "uevent",
				G_CALLBACK(g_udev_client_uevent_handler),
				self);

	/* Query devices" */
	devices = g_udev_client_query_by_subsystem(priv->udev_client, "tty");
	for(sl=devices; sl; sl=g_list_next(sl))
	{
		if(g_udev_device_has_property(G_UDEV_DEVICE(sl->data), "ID_VENDOR"))
		{
			const gchar *path = NULL;

			path = g_udev_device_get_device_file(G_UDEV_DEVICE(sl->data));
			hev_serial_port_try_open(self, path);
		}
	}
	g_list_free_full(devices, g_object_unref);
}

GObject *hev_dbus_object_idcard_reader_manager_new(const gchar *object_path)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);
	return g_object_new(HEV_TYPE_DBUS_OBJECT_IDCARD_READER_MANAGER,
				"g-object-path", object_path, NULL);
}

GList * hev_dbus_object_idcard_reader_manager_enumerate_devices(HevDBusObjectIDCardReaderManager *self)
{
	HevDBusObjectIDCardReaderManagerPrivate *priv = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_return_val_if_fail(HEV_IS_DBUS_OBJECT_IDCARD_READER_MANAGER(self), NULL);
	priv = HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_GET_PRIVATE(self);

	return priv->device_list;
}

static void hev_serial_port_try_open(HevDBusObjectIDCardReaderManager *self,
			const gchar *device_path)
{
	HevDBusObjectIDCardReaderManagerPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_debug("%s:%d[%s]=>(Try to open %s ...)",
				__FILE__, __LINE__, __FUNCTION__, device_path);

	hev_serial_port_new_async(device_path, NULL,
				hev_serial_port_new_async_handler, self);
}

static void hev_serial_port_try_remove(HevDBusObjectIDCardReaderManager *self,
			const gchar *device_path)
{
	HevDBusObjectIDCardReaderManagerPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_GET_PRIVATE(self);
	GList *sl = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_debug("%s:%d[%s]=>(Try to remove %s ...)",
						__FILE__, __LINE__, __FUNCTION__, device_path);
	for(sl=priv->device_list; sl; sl=g_list_next(sl))
	{
		GObject *serial_port = NULL;
		gchar *p = NULL;
		gint t = 0;

		g_object_get(G_OBJECT(sl->data), "serial-port", &serial_port, NULL);
		g_object_unref(serial_port);
		g_object_get(serial_port, "path", &p, NULL);
		t = g_strcmp0(device_path, p);
		g_free(p);
		if(0 == t)
		{
			GDBusObjectManagerServer *server = g_object_get_data(G_OBJECT(self),
						"dbus-object-manager-server");

			g_signal_emit(self, hev_dbus_object_idcard_reader_manager_signals[SIG_REMOVE],
						0, g_dbus_object_get_object_path(G_DBUS_OBJECT(sl->data)));
			g_dbus_object_manager_server_unexport(server,
						g_dbus_object_get_object_path(G_DBUS_OBJECT(sl->data)));
			g_object_unref(sl->data);
			g_object_unref(serial_port);
			priv->device_list = g_list_remove(priv->device_list, sl->data);

			break;
		}
	}
}

static void hev_serial_port_queue_command_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data)
{
	HevDBusObjectIDCardReaderManager *self = HEV_DBUS_OBJECT_IDCARD_READER_MANAGER(user_data);
	HevDBusObjectIDCardReaderManagerPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_GET_PRIVATE(self);
	GByteArray *data = NULL;
	guint source_id = 0;
	gchar *path = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	/* Remove timeout */
	source_id = GPOINTER_TO_UINT(g_object_get_data(source_object,
					"source-id"));
	g_source_remove(source_id);

	g_object_get(source_object, "path", &path, NULL);

	data = hev_serial_port_queue_command_finish(HEV_SERIAL_PORT(source_object),
				res, NULL);
	if(data)
	{
		/* Status */
		if((9<=data->len) && (0x90==data->data[9]))
		{
			GDBusObjectManagerServer *server = NULL;
			GObject *dbus_obj = NULL, *dbus_iface = NULL;
			gchar *dbus_obj_path = NULL;

			g_debug("%s:%d[%s]=>(%s is a reader, worked.)",
						__FILE__, __LINE__, __FUNCTION__, path);

			/* DBus object */
			dbus_obj_path = g_strdup_printf("/hev/idcard/Reader/%u",
						priv->device_max_id++);
			dbus_obj = hev_dbus_object_idcard_reader_new(dbus_obj_path,
						source_object);
			g_free(dbus_obj_path);

			/* DBus interface */
			dbus_iface = hev_dbus_interface_idcard_reader_new(
						G_DBUS_OBJECT_SKELETON(dbus_obj));

			/* Export DBus object */
			server = g_object_get_data(G_OBJECT(self),
						"dbus-object-manager-server");
			g_dbus_object_manager_server_export(server,
						G_DBUS_OBJECT_SKELETON(dbus_obj));

			g_signal_emit(self, hev_dbus_object_idcard_reader_manager_signals[SIG_ADD],
						0, g_dbus_object_get_object_path(G_DBUS_OBJECT(dbus_obj)));
			priv->device_list = g_list_append(priv->device_list, dbus_obj);
		}
		else
		{
			g_debug("%s:%d[%s]=>(%s is a reader, but not work!)",
						__FILE__, __LINE__, __FUNCTION__, path);
			g_object_unref(source_object);
		}

		g_byte_array_unref(data);
	}
	else
	{
		g_debug("%s:%d[%s]=>(%s is not a reader!)", __FILE__, __LINE__,
					__FUNCTION__, path);
		g_object_unref(source_object);
	}

	g_free(path);
}

static gsize hev_serial_port_read_size_handler(const gpointer buffer, gssize size,
			gpointer user_data)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(0 == size)
	  return 7;
	else if(7 == size)
	  return ((guint8 *)buffer)[5] * 256 + ((guint8 *)buffer)[6];
	else
	  return 0;
}

static gboolean g_timeout_handler(gpointer user_data)
{
	GCancellable *cancellable = user_data;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_cancellable_cancel(cancellable);

	return FALSE;
}

static void hev_serial_port_new_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data)
{
	GObject *serial_port = NULL;
	GByteArray *command = g_byte_array_new();
	/* Get reader status */
	guchar cmd[] = {0xAA, 0xAA, 0xAA, 0x96, 0x69, 0x00, 0x03, 0x11, 0xFF, 0xED};
	GCancellable *cancellable = NULL;
	guint source_id = 0;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	serial_port = hev_serial_port_new_finish(res, NULL);
	g_byte_array_append(command, cmd, sizeof(cmd));

	cancellable = g_cancellable_new();
	/* Add timeout */
	source_id = g_timeout_add_seconds(2, g_timeout_handler, cancellable);
	g_object_set_data(serial_port, "source-id", GUINT_TO_POINTER(source_id));

	hev_serial_port_queue_command_async(HEV_SERIAL_PORT(serial_port), command,
				hev_serial_port_read_size_handler, cancellable,
				hev_serial_port_queue_command_async_handler, user_data);

	g_byte_array_unref(command);
	g_object_unref(cancellable);
}

static void g_udev_client_uevent_handler(GUdevClient *client, gchar *action,
			GUdevDevice *device, gpointer user_data)
{
	HevDBusObjectIDCardReaderManager *self = HEV_DBUS_OBJECT_IDCARD_READER_MANAGER(user_data);
	HevDBusObjectIDCardReaderManagerPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_GET_PRIVATE(self);
	const gchar *path = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	path = g_udev_device_get_device_file(device);

	if(0 == g_strcmp0(action, "add"))
	  hev_serial_port_try_open(self, path);
	else if(0 == g_strcmp0(action, "remove"))
	  hev_serial_port_try_remove(self, path);
}

