/*
 ============================================================================
 Name        : hev-dbus-interface-idcard-reader-manager.h
 Author      : Heiher <admin@heiher.info>
 Version     : 0.0.1
 Copyright   : Copyright (C) 2012 everyone.
 Description : 
 ============================================================================
 */

#ifndef __HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_H__
#define __HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_H__

#include "hev-dbus-interface-skeleton.h"

G_BEGIN_DECLS

#define HEV_TYPE_DBUS_INTERFACE_IDCARD_READER_MANAGER	(hev_dbus_interface_idcard_reader_manager_get_type())
#define HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER(obj)	(G_TYPE_CHECK_INSTANCE_CAST((obj), HEV_TYPE_DBUS_INTERFACE_IDCARD_READER_MANAGER, HevDBusInterfaceIDCardReaderManager))
#define HEV_IS_DBUS_INTERFACE_IDCARD_READER_MANAGER(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), HEV_TYPE_DBUS_INTERFACE_IDCARD_READER_MANAGER))
#define HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), HEV_TYPE_DBUS_INTERFACE_IDCARD_READER_MANAGER, HevDBusInterfaceIDCardReaderManagerClass))
#define HEV_IS_DBUS_INTERFACE_IDCARD_READER_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), HEV_TYPE_DBUS_INTERFACE_IDCARD_READER_MANAGER))
#define HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), HEV_TYPE_DBUS_INTERFACE_IDCARD_READER_MANAGER, HevDBusInterfaceIDCardReaderManagerClass))

typedef struct _HevDBusInterfaceIDCardReaderManager HevDBusInterfaceIDCardReaderManager;
typedef struct _HevDBusInterfaceIDCardReaderManagerClass HevDBusInterfaceIDCardReaderManagerClass;

struct _HevDBusInterfaceIDCardReaderManager
{
	HevDBusInterfaceSkeleton parent_instance;
};

struct _HevDBusInterfaceIDCardReaderManagerClass
{
	HevDBusInterfaceSkeletonClass parent_class;
};

GType hev_dbus_interface_idcard_reader_manager_get_type(void);

GObject * hev_dbus_interface_idcard_reader_manager_new(GDBusObjectSkeleton *object);

G_END_DECLS

#endif /* __HEV_DBUS_INTERFACE_IDCARD_READER_MANAGER_H__ */

