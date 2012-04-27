/*
 ============================================================================
 Name        : hev-dbus-object-idcard-reader-manager.h
 Author      : Heiher <admin@heiher.info>
 Version     : 0.0.1
 Copyright   : Copyright (C) 2011 everyone.
 Description : 
 ============================================================================
 */

#ifndef __HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_H__
#define __HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define HEV_TYPE_DBUS_OBJECT_IDCARD_READER_MANAGER	(hev_dbus_object_idcard_reader_manager_get_type())
#define HEV_DBUS_OBJECT_IDCARD_READER_MANAGER(obj)	(G_TYPE_CHECK_INSTANCE_CAST((obj), HEV_TYPE_DBUS_OBJECT_IDCARD_READER_MANAGER, HevDBusObjectIDCardReaderManager))
#define HEV_IS_DBUS_OBJECT_IDCARD_READER_MANAGER(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), HEV_TYPE_DBUS_OBJECT_IDCARD_READER_MANAGER))
#define HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), HEV_TYPE_DBUS_OBJECT_IDCARD_READER_MANAGER, HevDBusObjectIDCardReaderManagerClass))
#define HEV_IS_DBUS_OBJECT_IDCARD_READER_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), HEV_TYPE_DBUS_OBJECT_IDCARD_READER_MANAGER))
#define HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), HEV_TYPE_DBUS_OBJECT_IDCARD_READER_MANAGER, HevDBusObjectIDCardReaderManagerClass))

typedef struct _HevDBusObjectIDCardReaderManager HevDBusObjectIDCardReaderManager;
typedef struct _HevDBusObjectIDCardReaderManagerClass HevDBusObjectIDCardReaderManagerClass;

struct _HevDBusObjectIDCardReaderManager
{
	GDBusObjectSkeleton parent_instance;
};

struct _HevDBusObjectIDCardReaderManagerClass
{
	GDBusObjectSkeletonClass parent_class;

	/* Signal handlers */
	void (*add)(HevDBusObjectIDCardReaderManager *self, const gchar *path);
	void (*remove)(HevDBusObjectIDCardReaderManager *self, const gchar *path);
};

GType hev_dbus_object_idcard_reader_manager_get_type(void);

GObject * hev_dbus_object_idcard_reader_manager_new(const gchar *object_path);

void hev_dbus_object_idcard_reader_manager_request_remove(HevDBusObjectIDCardReaderManager *self,
			GObject *reader);

GList * hev_dbus_object_idcard_reader_manager_enumerate_devices(HevDBusObjectIDCardReaderManager *self);

G_END_DECLS

#endif /* __HEV_DBUS_OBJECT_IDCARD_READER_MANAGER_H__ */

