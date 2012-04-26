/*
 ============================================================================
 Name        : hev-dbus-interface-idcard-reader-manager.c
 Author      : Heiher <admin@heiher.info>
 Version     : 0.0.1
 Copyright   : Copyright (C) 2012 everyone.
 Description : 
 ============================================================================
 */

#include "hev-dbus-interface-idcard-reader-manager.h"
#include "hev-dbus-object-idcard-reader-manager.h"

#define HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_NAME	"hev.idcard.Reader.Manager"

#define HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE((obj), HEV_TYPE_DBUS_INTERFACE_IDCARD_READER_MANAGER, HevDBusInterfaceIDCardReaderManagerPrivate))

static const gchar introspection_xml[] =
"<node>"
"  <interface name='"HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_NAME"'>"
"    <method name='EnumerateDevices'>"
"      <arg type='ao' name='devices' direction='out' />"
"    </method>"
"    <signal name='Add'>"
"      <arg type='o' name='device' />"
"    </signal>"
"    <signal name='Remove'>"
"      <arg type='o' name='device' />"
"    </signal>"
"  </interface>"
"</node>";

typedef struct _HevDBusInterfaceIDCardReaderManagerPrivate HevDBusInterfaceIDCardReaderManagerPrivate;

struct _HevDBusInterfaceIDCardReaderManagerPrivate
{
	GDBusNodeInfo *node_info;
	GDBusInterfaceVTable vtable;
};

G_DEFINE_TYPE(HevDBusInterfaceIDCardReaderManager, hev_dbus_interface_idcard_reader_manager, HEV_TYPE_DBUS_INTERFACE_SKELETON);

static void hev_dbus_object_idcard_reader_manager_add_handler(GObject *obj,
			const gchar *path, gpointer user_data);
static void hev_dbus_object_idcard_reader_manager_remove_handler(GObject *obj,
			const gchar *path, gpointer user_data);

static void hev_dbus_interface_idcard_reader_manager_dispose(GObject *obj)
{
	HevDBusInterfaceIDCardReaderManager *self = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER(obj);
	HevDBusInterfaceIDCardReaderManagerPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	G_OBJECT_CLASS(hev_dbus_interface_idcard_reader_manager_parent_class)->dispose(obj);
}

static void hev_dbus_interface_idcard_reader_manager_finalize(GObject *obj)
{
	HevDBusInterfaceIDCardReaderManager *self = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER(obj);
	HevDBusInterfaceIDCardReaderManagerPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(priv->node_info)
	{
		g_dbus_node_info_unref(priv->node_info);
		priv->node_info = NULL;
	}

	G_OBJECT_CLASS(hev_dbus_interface_idcard_reader_manager_parent_class)->finalize(obj);
}

static GObject * hev_dbus_interface_idcard_reader_manager_constructor(GType type, guint n, GObjectConstructParam *param)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);
	return G_OBJECT_CLASS(hev_dbus_interface_idcard_reader_manager_parent_class)->constructor(type, n, param);
}

static void hev_dbus_interface_idcard_reader_manager_constructed(GObject *obj)
{
	GDBusObject *dbus_obj = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	G_OBJECT_CLASS(hev_dbus_interface_idcard_reader_manager_parent_class)->constructed(obj);

	dbus_obj = g_dbus_interface_get_object(G_DBUS_INTERFACE(obj));

	/* Connect object signals */
	g_signal_connect(G_OBJECT(dbus_obj), "add",
				G_CALLBACK(hev_dbus_object_idcard_reader_manager_add_handler), obj);
	g_signal_connect(G_OBJECT(dbus_obj), "remove",
				G_CALLBACK(hev_dbus_object_idcard_reader_manager_remove_handler), obj);
}

static GDBusInterfaceInfo * hev_dbus_interface_idcard_reader_manager_get_info(GDBusInterfaceSkeleton *skeleton)
{
	HevDBusInterfaceIDCardReaderManager *self = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER(skeleton);
	HevDBusInterfaceIDCardReaderManagerPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	return g_dbus_node_info_lookup_interface(priv->node_info, HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_NAME);
}

static GDBusInterfaceVTable * hev_dbus_interface_idcard_reader_manager_get_vtable(GDBusInterfaceSkeleton *skeleton)
{
	HevDBusInterfaceIDCardReaderManager *self = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER(skeleton);
	HevDBusInterfaceIDCardReaderManagerPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	return &(priv->vtable);
}

static GVariant * hev_dbus_interface_idcard_reader_manager_get_properties(GDBusInterfaceSkeleton *skeleton)
{
	GVariantBuilder *builder = NULL;
	GVariant *variant = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	builder = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
	variant = g_variant_builder_end(builder);
	g_variant_builder_unref(builder);

	return variant;
}

static void hev_dbus_interface_idcard_reader_manager_class_init(HevDBusInterfaceIDCardReaderManagerClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);
	GDBusInterfaceSkeletonClass *skeleton_class = G_DBUS_INTERFACE_SKELETON_CLASS(klass);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	obj_class->constructor = hev_dbus_interface_idcard_reader_manager_constructor;
	obj_class->constructed = hev_dbus_interface_idcard_reader_manager_constructed;
	obj_class->dispose = hev_dbus_interface_idcard_reader_manager_dispose;
	obj_class->finalize = hev_dbus_interface_idcard_reader_manager_finalize;

	skeleton_class->get_info = hev_dbus_interface_idcard_reader_manager_get_info;
	skeleton_class->get_vtable = hev_dbus_interface_idcard_reader_manager_get_vtable;
	skeleton_class->get_properties = hev_dbus_interface_idcard_reader_manager_get_properties;

	g_type_class_add_private(klass, sizeof(HevDBusInterfaceIDCardReaderManagerPrivate));
}

static void hev_dbus_interface_idcard_reader_manager_method_call(GDBusConnection *connection,
			const gchar *sender, const gchar *object_path,
			const gchar *interface_name, const gchar *method_name,
			GVariant *parameters, GDBusMethodInvocation *invocation,
			gpointer user_data)
{
	HevDBusInterfaceIDCardReaderManager *self = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER(user_data);
	HevDBusObjectIDCardReaderManager *obj = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	obj = HEV_DBUS_OBJECT_IDCARD_READER_MANAGER(g_dbus_interface_get_object(G_DBUS_INTERFACE(self)));

	if(0 == g_strcmp0(method_name, "EnumerateDevices"))
	{
		GList *sl = NULL, *devices = NULL;
		GVariantBuilder *builder = NULL;
		GVariant *variant = NULL;

		devices = hev_dbus_object_idcard_reader_manager_enumerate_devices(obj);

		builder = g_variant_builder_new(G_VARIANT_TYPE_OBJECT_PATH_ARRAY);
		for(sl=devices; sl; sl=g_list_next(sl))
		  g_variant_builder_add(builder, "o",
					  g_dbus_object_get_object_path(sl->data));
		variant = g_variant_builder_end(builder);
		g_variant_builder_unref(builder);
		variant = g_variant_new("(@ao)", variant);
		g_dbus_method_invocation_return_value(invocation, variant);
	}
}

static void hev_dbus_interface_idcard_reader_manager_init(HevDBusInterfaceIDCardReaderManager *self)
{
	HevDBusInterfaceIDCardReaderManagerPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	/* Introspection */
	priv->node_info = g_dbus_node_info_new_for_xml(introspection_xml, NULL);

	/* VTable */
	priv->vtable.method_call = hev_dbus_interface_idcard_reader_manager_method_call;
}

GObject *hev_dbus_interface_idcard_reader_manager_new(GDBusObjectSkeleton *object)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);
	return g_object_new(HEV_TYPE_DBUS_INTERFACE_IDCARD_READER_MANAGER,
				"object", object, NULL);
}

static void hev_dbus_object_idcard_reader_manager_add_handler(GObject *obj,
			const gchar *path, gpointer user_data)
{
	HevDBusInterfaceIDCardReaderManager *self = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER(user_data);
	HevDBusInterfaceIDCardReaderManagerPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_GET_PRIVATE(self);
	GDBusConnection *connection = NULL;
	const gchar *object_path = NULL;
	GVariant *variant = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	connection = g_dbus_interface_skeleton_get_connection(G_DBUS_INTERFACE_SKELETON(self));
	object_path = g_dbus_interface_skeleton_get_object_path(G_DBUS_INTERFACE_SKELETON(self));

	variant = g_variant_new("(o)", path);

	g_dbus_connection_emit_signal(connection, NULL, object_path,
				HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_NAME, "Add",
				variant, NULL);
}

static void hev_dbus_object_idcard_reader_manager_remove_handler(GObject *obj,
			const gchar *path, gpointer user_data)
{
	HevDBusInterfaceIDCardReaderManager *self = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER(user_data);
	HevDBusInterfaceIDCardReaderManagerPrivate *priv = HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_GET_PRIVATE(self);
	GDBusConnection *connection = NULL;
	const gchar *object_path = NULL;
	GVariant *variant = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	connection = g_dbus_interface_skeleton_get_connection(G_DBUS_INTERFACE_SKELETON(self));
	object_path = g_dbus_interface_skeleton_get_object_path(G_DBUS_INTERFACE_SKELETON(self));

	variant = g_variant_new("(o)", path);

	g_dbus_connection_emit_signal(connection, NULL, object_path,
				HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_NAME, "Remove",
				variant, NULL);
}

