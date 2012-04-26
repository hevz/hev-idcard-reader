/*
 ============================================================================
 Name        : hev-dbus-interface-skeleton.c
 Author      : Heiher <admin@heiher.info>
 Version     : 0.0.1
 Copyright   : Copyright (C) 2011 everyone.
 Description : 
 ============================================================================
 */

#include "hev-dbus-interface-skeleton.h"

#define HEV_DBUS_INTERFACE_SKELETON_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE((obj), HEV_TYPE_DBUS_INTERFACE_SKELETON, HevDBusInterfaceSkeletonPrivate))

enum
{
	PROP_ZERO,
	PROP_OBJECT,
	N_PROPERTIES
};

static GParamSpec * hev_dbus_interface_skeleton_properties[N_PROPERTIES] = { NULL };

typedef struct _HevDBusInterfaceSkeletonPrivate HevDBusInterfaceSkeletonPrivate;

struct _HevDBusInterfaceSkeletonPrivate
{
	GObject *object;
};

G_DEFINE_TYPE(HevDBusInterfaceSkeleton, hev_dbus_interface_skeleton, G_TYPE_DBUS_INTERFACE_SKELETON);

static void hev_dbus_interface_skeleton_dispose(GObject *obj)
{
	HevDBusInterfaceSkeleton *self = HEV_DBUS_INTERFACE_SKELETON(obj);
	HevDBusInterfaceSkeletonPrivate *priv = HEV_DBUS_INTERFACE_SKELETON_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	G_OBJECT_CLASS(hev_dbus_interface_skeleton_parent_class)->dispose(obj);
}

static void hev_dbus_interface_skeleton_finalize(GObject *obj)
{
	HevDBusInterfaceSkeleton *self = HEV_DBUS_INTERFACE_SKELETON(obj);
	HevDBusInterfaceSkeletonPrivate *priv = HEV_DBUS_INTERFACE_SKELETON_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	G_OBJECT_CLASS(hev_dbus_interface_skeleton_parent_class)->finalize(obj);
}

static GObject * hev_dbus_interface_skeleton_constructor(GType type, guint n, GObjectConstructParam *param)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);
	return G_OBJECT_CLASS(hev_dbus_interface_skeleton_parent_class)->constructor(type, n, param);
}

static void hev_dbus_interface_skeleton_constructed(GObject *obj)
{
	GDBusObject *dbus_obj = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	G_OBJECT_CLASS(hev_dbus_interface_skeleton_parent_class)->constructed(obj);

	g_object_get(obj, "object", &dbus_obj, NULL);
	g_dbus_object_skeleton_add_interface(G_DBUS_OBJECT_SKELETON(dbus_obj),
				G_DBUS_INTERFACE_SKELETON(obj));
	g_object_unref(dbus_obj);
}

static void hev_dbus_interface_skeleton_set_property(GObject *obj, guint id,
			const GValue *value, GParamSpec *pspec)
{
	HevDBusInterfaceSkeleton *self = HEV_DBUS_INTERFACE_SKELETON(obj);
	HevDBusInterfaceSkeletonPrivate *priv = HEV_DBUS_INTERFACE_SKELETON_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	switch(id)
	{
	case PROP_OBJECT:
		priv->object = g_value_get_object(value);
		g_object_notify_by_pspec(obj,
					hev_dbus_interface_skeleton_properties[PROP_OBJECT]);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, id, pspec);
		break;
	}
}

static void hev_dbus_interface_skeleton_get_property(GObject *obj, guint id,
			GValue *value, GParamSpec *pspec)
{
	HevDBusInterfaceSkeleton *self = HEV_DBUS_INTERFACE_SKELETON(obj);
	HevDBusInterfaceSkeletonPrivate *priv = HEV_DBUS_INTERFACE_SKELETON_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	switch(id)
	{
	case PROP_OBJECT:
		g_value_set_object(value, priv->object);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, id, pspec);
		break;
	}
}

static void hev_dbus_interface_skeleton_class_init(HevDBusInterfaceSkeletonClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	obj_class->constructor = hev_dbus_interface_skeleton_constructor;
	obj_class->constructed = hev_dbus_interface_skeleton_constructed;
	obj_class->dispose = hev_dbus_interface_skeleton_dispose;
	obj_class->finalize = hev_dbus_interface_skeleton_finalize;
	obj_class->get_property = hev_dbus_interface_skeleton_get_property;
	obj_class->set_property = hev_dbus_interface_skeleton_set_property;

	/* Properties */
	hev_dbus_interface_skeleton_properties[PROP_OBJECT] =
		g_param_spec_object("object", "Object", "DBus Object",
					G_TYPE_DBUS_OBJECT_SKELETON, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_properties(obj_class, N_PROPERTIES,
				hev_dbus_interface_skeleton_properties);
	
	g_type_class_add_private(klass, sizeof(HevDBusInterfaceSkeletonPrivate));
}

static void hev_dbus_interface_skeleton_init(HevDBusInterfaceSkeleton *self)
{
	HevDBusInterfaceSkeletonPrivate *priv = HEV_DBUS_INTERFACE_SKELETON_GET_PRIVATE(self);
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);
}

