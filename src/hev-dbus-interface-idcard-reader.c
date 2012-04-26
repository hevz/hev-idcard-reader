/*
 ============================================================================
 Name        : hev-dbus-interface-idcard-reader.c
 Author      : Heiher <admin@heiher.info>
 Version     : 0.0.1
 Copyright   : Copyright (C) 2012 everyone.
 Description : 
 ============================================================================
 */

#include "hev-dbus-interface-idcard-reader.h"
#include "hev-dbus-object-idcard-reader.h"

#define HEV_DBUS_INTERFACE_IDCARD_READER_NAME	"hev.idcard.Reader"

#define HEV_DBUS_INTERFACE_IDCARD_READER_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE((obj), HEV_TYPE_DBUS_INTERFACE_IDCARD_READER, HevDBusInterfaceIDCardReaderPrivate))

static const gchar introspection_xml[] =
"<node>"
"  <interface name='"HEV_DBUS_INTERFACE_IDCARD_READER_NAME"'>"
"    <method name='GetStatus'>"
"      <arg type='b' name='status' direction='out' />"
"    </method>"
"    <method name='GetCardStatus'>"
"      <arg type='b' name='status' direction='out' />"
"    </method>"
"    <method name='GetCardInfo'>"
"      <arg type='a{sv}' name='info' direction='out' />"
"    </method>"
"    <signal name='StatusChanged'>"
"      <arg type='b' name='status' />"
"    </signal>"
"    <signal name='CardStatusChanged'>"
"      <arg type='b' name='status' />"
"    </signal>"
"    <signal name='CardInfoChanged' />"
"  </interface>"
"</node>";

typedef struct _HevDBusInterfaceIDCardReaderPrivate HevDBusInterfaceIDCardReaderPrivate;

struct _HevDBusInterfaceIDCardReaderPrivate
{
	GDBusNodeInfo *node_info;
	GDBusInterfaceVTable vtable;
};

G_DEFINE_TYPE(HevDBusInterfaceIDCardReader, hev_dbus_interface_idcard_reader, HEV_TYPE_DBUS_INTERFACE_SKELETON);

static void hev_dbus_object_idcard_reader_status_changed_handler(GObject *obj,
			gboolean status, gpointer user_data);
static void hev_dbus_object_idcard_reader_card_status_changed_handler(GObject *obj,
			gboolean status, gpointer user_data);
static void hev_dbus_object_idcard_reader_card_info_changed_handler(GObject *obj,
			gpointer user_data);

static void hev_dbus_interface_idcard_reader_dispose(GObject *obj)
{
	HevDBusInterfaceIDCardReader *self = HEV_DBUS_INTERFACE_IDCARD_READER(obj);
	HevDBusInterfaceIDCardReaderPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	G_OBJECT_CLASS(hev_dbus_interface_idcard_reader_parent_class)->dispose(obj);
}

static void hev_dbus_interface_idcard_reader_finalize(GObject *obj)
{
	HevDBusInterfaceIDCardReader *self = HEV_DBUS_INTERFACE_IDCARD_READER(obj);
	HevDBusInterfaceIDCardReaderPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(priv->node_info)
	{
		g_dbus_node_info_unref(priv->node_info);
		priv->node_info = NULL;
	}

	G_OBJECT_CLASS(hev_dbus_interface_idcard_reader_parent_class)->finalize(obj);
}

static GObject * hev_dbus_interface_idcard_reader_constructor(GType type, guint n, GObjectConstructParam *param)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);
	return G_OBJECT_CLASS(hev_dbus_interface_idcard_reader_parent_class)->constructor(type, n, param);
}

static void hev_dbus_interface_idcard_reader_constructed(GObject *obj)
{
	GDBusObject *dbus_obj = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	G_OBJECT_CLASS(hev_dbus_interface_idcard_reader_parent_class)->constructed(obj);

	dbus_obj = g_dbus_interface_get_object(G_DBUS_INTERFACE(obj));

	/* Connect object signals */
	g_signal_connect(G_OBJECT(dbus_obj), "status-changed",
				G_CALLBACK(hev_dbus_object_idcard_reader_status_changed_handler), obj);
	g_signal_connect(G_OBJECT(dbus_obj), "card-status-changed",
				G_CALLBACK(hev_dbus_object_idcard_reader_card_status_changed_handler), obj);
	g_signal_connect(G_OBJECT(dbus_obj), "card-info-changed",
				G_CALLBACK(hev_dbus_object_idcard_reader_card_info_changed_handler), obj);
}

static GDBusInterfaceInfo * hev_dbus_interface_idcard_reader_get_info(GDBusInterfaceSkeleton *skeleton)
{
	HevDBusInterfaceIDCardReader *self = HEV_DBUS_INTERFACE_IDCARD_READER(skeleton);
	HevDBusInterfaceIDCardReaderPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	return g_dbus_node_info_lookup_interface(priv->node_info, HEV_DBUS_INTERFACE_IDCARD_READER_NAME);
}

static GDBusInterfaceVTable * hev_dbus_interface_idcard_reader_get_vtable(GDBusInterfaceSkeleton *skeleton)
{
	HevDBusInterfaceIDCardReader *self = HEV_DBUS_INTERFACE_IDCARD_READER(skeleton);
	HevDBusInterfaceIDCardReaderPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	return &(priv->vtable);
}

static GVariant * hev_dbus_interface_idcard_reader_get_properties(GDBusInterfaceSkeleton *skeleton)
{
	GVariantBuilder *builder = NULL;
	GVariant *variant = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	builder = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
	variant = g_variant_builder_end(builder);
	g_variant_builder_unref(builder);

	return variant;
}

static void hev_dbus_interface_idcard_reader_class_init(HevDBusInterfaceIDCardReaderClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GDBusInterfaceSkeletonClass *skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS(klass);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	obj_class->constructor = hev_dbus_interface_idcard_reader_constructor;
	obj_class->constructed = hev_dbus_interface_idcard_reader_constructed;
	obj_class->dispose = hev_dbus_interface_idcard_reader_dispose;
	obj_class->finalize = hev_dbus_interface_idcard_reader_finalize;

	skeleton_class->get_info = hev_dbus_interface_idcard_reader_get_info;
	skeleton_class->get_vtable = hev_dbus_interface_idcard_reader_get_vtable;
	skeleton_class->get_properties = hev_dbus_interface_idcard_reader_get_properties;

	g_type_class_add_private(klass, sizeof(HevDBusInterfaceIDCardReaderPrivate));
}

static void hev_dbus_interface_idcard_reader_method_call(GDBusConnection *connection,
			const gchar *sender, const gchar *object_path,
			const gchar *interface_name, const gchar *method_name,
			GVariant *parameters, GDBusMethodInvocation *invocation,
			gpointer user_data)
{
	HevDBusInterfaceIDCardReader *self = HEV_DBUS_INTERFACE_IDCARD_READER(user_data);
	HevDBusObjectIDCardReader *obj = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	obj = HEV_DBUS_OBJECT_IDCARD_READER(g_dbus_interface_get_object(G_DBUS_INTERFACE(self)));

	if(0 == g_strcmp0(method_name, "GetStatus"))
	{
		gboolean status = FALSE;
		GError *error = NULL;

		status = hev_dbus_object_idcard_reader_get_cached_status(obj, &error);
		if(error)
		  g_dbus_method_invocation_return_gerror(invocation, error);
		else
		{
			GVariant *variant = NULL;

			variant = g_variant_new("(b)", status);
			g_dbus_method_invocation_return_value(invocation, variant);
		}
	}
	else if(0 == g_strcmp0(method_name, "GetCardStatus"))
	{
		gboolean status = FALSE;
		GError *error = NULL;

		status = hev_dbus_object_idcard_reader_get_cached_card_status(obj, &error);
		if(error)
		  g_dbus_method_invocation_return_gerror(invocation, error);
		else
		{
			GVariant *variant = NULL;

			variant = g_variant_new("(b)", status);
			g_dbus_method_invocation_return_value(invocation, variant);
		}
	}
	else if(0 == g_strcmp0(method_name, "GetCardInfo"))
	{
		GError *error = NULL;

		if(error)
		  g_dbus_method_invocation_return_gerror(invocation, error);
		else
		{
			GVariant *variant = NULL;
			variant = hev_dbus_object_idcard_reader_get_cached_card_info(obj, &error);
			variant = g_variant_new("(@a{sv})", variant);

			g_dbus_method_invocation_return_value(invocation, variant);
		}
	}
}

static void hev_dbus_interface_idcard_reader_init(HevDBusInterfaceIDCardReader *self)
{
	HevDBusInterfaceIDCardReaderPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	/* Introspection */
	priv->node_info = g_dbus_node_info_new_for_xml(introspection_xml, NULL);

	/* VTable */
	priv->vtable.method_call = hev_dbus_interface_idcard_reader_method_call;
}

GObject *hev_dbus_interface_idcard_reader_new(GDBusObjectSkeleton *object)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);
	return g_object_new(HEV_TYPE_DBUS_INTERFACE_IDCARD_READER,
				"object", object, NULL);
}

static void hev_dbus_object_idcard_reader_status_changed_handler(GObject *obj,
			gboolean status, gpointer user_data)
{
	HevDBusInterfaceIDCardReader *self = HEV_DBUS_INTERFACE_IDCARD_READER(user_data);
	HevDBusInterfaceIDCardReaderPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_GET_PRIVATE(self);
	GDBusConnection *connection = NULL;
	const gchar *object_path = NULL;
	GVariant *variant = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	connection = g_dbus_interface_skeleton_get_connection(G_DBUS_INTERFACE_SKELETON(self));
	object_path = g_dbus_interface_skeleton_get_object_path(G_DBUS_INTERFACE_SKELETON(self));

	variant = g_variant_new("(b)", status);

	g_dbus_connection_emit_signal(connection, NULL, object_path,
				HEV_DBUS_INTERFACE_IDCARD_READER_NAME, "StatusChanged",
				variant, NULL);
}

static void hev_dbus_object_idcard_reader_card_status_changed_handler(GObject *obj,
			gboolean status, gpointer user_data)
{
	HevDBusInterfaceIDCardReader *self = HEV_DBUS_INTERFACE_IDCARD_READER(user_data);
	HevDBusInterfaceIDCardReaderPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_GET_PRIVATE(self);
	GDBusConnection *connection = NULL;
	const gchar *object_path = NULL;
	GVariant *variant = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	connection = g_dbus_interface_skeleton_get_connection(G_DBUS_INTERFACE_SKELETON(self));
	object_path = g_dbus_interface_skeleton_get_object_path(G_DBUS_INTERFACE_SKELETON(self));

	variant = g_variant_new("(b)", status);

	g_dbus_connection_emit_signal(connection, NULL, object_path,
				HEV_DBUS_INTERFACE_IDCARD_READER_NAME, "CardStatusChanged",
				variant, NULL);
}

static void hev_dbus_object_idcard_reader_card_info_changed_handler(GObject *obj,
			gpointer user_data)
{
	HevDBusInterfaceIDCardReader *self = HEV_DBUS_INTERFACE_IDCARD_READER(user_data);
	GDBusConnection *connection = NULL;
	const gchar *object_path = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	connection = g_dbus_interface_skeleton_get_connection(G_DBUS_INTERFACE_SKELETON(self));
	object_path = g_dbus_interface_skeleton_get_object_path(G_DBUS_INTERFACE_SKELETON(self));

	g_dbus_connection_emit_signal(connection, NULL, object_path,
				HEV_DBUS_INTERFACE_IDCARD_READER_NAME, "CardInfoChanged",
				NULL, NULL);
}

