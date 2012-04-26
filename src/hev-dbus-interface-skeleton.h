/*
 ============================================================================
 Name        : hev-dbus-interface-skeleton.h
 Author      : Heiher <admin@heiher.info>
 Version     : 0.0.1
 Copyright   : Copyright (C) 2011 everyone.
 Description : 
 ============================================================================
 */

#ifndef __HEV_DBUS_INTERFACE_SKELETON_H__
#define __HEV_DBUS_INTERFACE_SKELETON_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define HEV_TYPE_DBUS_INTERFACE_SKELETON	(hev_dbus_interface_skeleton_get_type())
#define HEV_DBUS_INTERFACE_SKELETON(obj)	(G_TYPE_CHECK_INSTANCE_CAST((obj), HEV_TYPE_DBUS_INTERFACE_SKELETON, HevDBusInterfaceSkeleton))
#define HEV_IS_DBUS_INTERFACE_SKELETON(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), HEV_TYPE_DBUS_INTERFACE_SKELETON))
#define HEV_DBUS_INTERFACE_SKELETON_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), HEV_TYPE_DBUS_INTERFACE_SKELETON, HevDBusInterfaceSkeletonClass))
#define HEV_IS_DBUS_INTERFACE_SKELETON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), HEV_TYPE_DBUS_INTERFACE_SKELETON))
#define HEV_DBUS_INTERFACE_SKELETON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), HEV_TYPE_DBUS_INTERFACE_SKELETON, HevDBusInterfaceSkeletonClass))

typedef struct _HevDBusInterfaceSkeleton HevDBusInterfaceSkeleton;
typedef struct _HevDBusInterfaceSkeletonClass HevDBusInterfaceSkeletonClass;

struct _HevDBusInterfaceSkeleton
{
	GDBusInterfaceSkeleton parent_instance;
};

struct _HevDBusInterfaceSkeletonClass
{
	GDBusInterfaceSkeletonClass parent_class;
};

GType hev_dbus_interface_skeleton_get_type(void);

G_END_DECLS

#endif /* __HEV_DBUS_INTERFACE_SKELETON_H__ */

