/*
 ============================================================================
 Name        : hev-dbus-interface-idcard-reader.h
 Author      : Heiher <admin@heiher.info>
 Version     : 0.0.1
 Copyright   : Copyright (C) 2012 everyone.
 Description : 
 ============================================================================
 */

#ifndef __HEV_DBUS_INTERFACE_IDCARD_READER_H__
#define __HEV_DBUS_INTERFACE_IDCARD_READER_H__

#include "hev-dbus-interface-skeleton.h"

G_BEGIN_DECLS

#define HEV_TYPE_DBUS_INTERFACE_IDCARD_READER	(hev_dbus_interface_idcard_reader_get_type())
#define HEV_DBUS_INTERFACE_IDCARD_READER(obj)	(G_TYPE_CHECK_INSTANCE_CAST((obj), HEV_TYPE_DBUS_INTERFACE_IDCARD_READER, HevDBusInterfaceIDCardReader))
#define HEV_IS_DBUS_INTERFACE_IDCARD_READER(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), HEV_TYPE_DBUS_INTERFACE_IDCARD_READER))
#define HEV_DBUS_INTERFACE_IDCARD_READER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), HEV_TYPE_DBUS_INTERFACE_IDCARD_READER, HevDBusInterfaceIDCardReaderClass))
#define HEV_IS_DBUS_INTERFACE_IDCARD_READER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), HEV_TYPE_DBUS_INTERFACE_IDCARD_READER))
#define HEV_DBUS_INTERFACE_IDCARD_READER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), HEV_TYPE_DBUS_INTERFACE_IDCARD_READER, HevDBusInterfaceIDCardReaderClass))

typedef struct _HevDBusInterfaceIDCardReader HevDBusInterfaceIDCardReader;
typedef struct _HevDBusInterfaceIDCardReaderClass HevDBusInterfaceIDCardReaderClass;

struct _HevDBusInterfaceIDCardReader
{
	HevDBusInterfaceSkeleton parent_instance;
};

struct _HevDBusInterfaceIDCardReaderClass
{
	HevDBusInterfaceSkeletonClass parent_class;
};

GType hev_dbus_interface_idcard_reader_get_type(void);

GObject * hev_dbus_interface_idcard_reader_new(GDBusObjectSkeleton *object);

G_END_DECLS

#endif /* __HEV_DBUS_INTERFACE_IDCARD_READER_H__ */

