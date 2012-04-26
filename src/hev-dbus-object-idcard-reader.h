/*
 ============================================================================
 Name        : hev-dbus-object-idcard-reader.h
 Author      : Heiher <admin@heiher.info>
 Version     : 0.0.1
 Copyright   : Copyright (C) 2012 everyone.
 Description : 
 ============================================================================
 */

#ifndef __HEV_DBUS_OBJECT_IDCARD_READER_H__
#define __HEV_DBUS_OBJECT_IDCARD_READER_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define HEV_TYPE_DBUS_OBJECT_IDCARD_READER	(hev_dbus_object_idcard_reader_get_type())
#define HEV_DBUS_OBJECT_IDCARD_READER(obj)	(G_TYPE_CHECK_INSTANCE_CAST((obj), HEV_TYPE_DBUS_OBJECT_IDCARD_READER, HevDBusObjectIDCardReader))
#define HEV_IS_DBUS_OBJECT_IDCARD_READER(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), HEV_TYPE_DBUS_OBJECT_IDCARD_READER))
#define HEV_DBUS_OBJECT_IDCARD_READER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), HEV_TYPE_DBUS_OBJECT_IDCARD_READER, HevDBusObjectIDCardReaderClass))
#define HEV_IS_DBUS_OBJECT_IDCARD_READER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), HEV_TYPE_DBUS_OBJECT_IDCARD_READER))
#define HEV_DBUS_OBJECT_IDCARD_READER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), HEV_TYPE_DBUS_OBJECT_IDCARD_READER, HevDBusObjectIDCardReaderClass))

typedef struct _HevDBusObjectIDCardReader HevDBusObjectIDCardReader;
typedef struct _HevDBusObjectIDCardReaderClass HevDBusObjectIDCardReaderClass;

struct _HevDBusObjectIDCardReader
{
	GDBusObjectSkeleton parent_instance;
};

struct _HevDBusObjectIDCardReaderClass
{
	GDBusObjectSkeletonClass parent_class;

	/* Signal handlers */
	void (*status_changed)(HevDBusObjectIDCardReader *self, gboolean status);
	void (*card_status_changed)(HevDBusObjectIDCardReader *self, gboolean status);
	void (*card_info_changed)(HevDBusObjectIDCardReader *self);
};

GType hev_dbus_object_idcard_reader_get_type(void);

GObject * hev_dbus_object_idcard_reader_new(const gchar *object_path, GObject *serial_port);

gboolean hev_dbus_object_idcard_reader_get_cached_status(HevDBusObjectIDCardReader *self, GError **error);
gboolean hev_dbus_object_idcard_reader_get_cached_card_status(HevDBusObjectIDCardReader *self, GError **error);
GVariant * hev_dbus_object_idcard_reader_get_cached_card_info(HevDBusObjectIDCardReader *self, GError **error);

G_END_DECLS

#endif /* __HEV_DBUS_OBJECT_IDCARD_READER_H__ */

