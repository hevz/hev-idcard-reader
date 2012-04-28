/*
 ============================================================================
 Name        : hev-serial-port.h
 Author      : Heiher <admin@heiher.info>
 Version     : 0.0.2
 Copyright   : Copyright (C) 2012 everyone.
 Description : 
 ============================================================================
 */

#ifndef __HEV_SERIAL_PORT_H__
#define __HEV_SERIAL_PORT_H__

#include <gio/gio.h>
#include <termios.h>

G_BEGIN_DECLS

#define HEV_TYPE_SERIAL_PORT	(hev_serial_port_get_type())
#define HEV_SERIAL_PORT(obj)	(G_TYPE_CHECK_INSTANCE_CAST((obj), HEV_TYPE_SERIAL_PORT, HevSerialPort))
#define HEV_IS_SERIAL_PORT(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), HEV_TYPE_SERIAL_PORT))
#define HEV_SERIAL_PORT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), HEV_TYPE_SERIAL_PORT, HevSerialPortClass))
#define HEV_IS_SERIAL_PORT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), HEV_TYPE_SERIAL_PORT))
#define HEV_SERIAL_PORT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), HEV_TYPE_SERIAL_PORT, HevSerialPortClass))

typedef gsize (*HevSerialPortReadSizeCallback)(const gpointer buffer,
			gssize size, gpointer user_data);

typedef struct _HevSerialPort HevSerialPort;
typedef struct _HevSerialPortClass HevSerialPortClass;

struct _HevSerialPort
{
	GObject parent_instance;
};

struct _HevSerialPortClass
{
	GObjectClass parent_class;
};

GType hev_serial_port_get_type(void);

void hev_serial_port_new_async(const gchar *path,
			GCancellable *cancellable, GAsyncReadyCallback callback,
			gpointer user_data);
GObject * hev_serial_port_new_finish(GAsyncResult *res, GError **error);

void hev_serial_port_close_async(HevSerialPort *self,
			GCancellable *cancellable, GAsyncReadyCallback callback,
			gpointer user_data);
gboolean hev_serial_port_close_finish(HevSerialPort *self,
			GAsyncResult *res, GError **error);

void hev_serial_port_config_async(HevSerialPort *self, struct termios *options,
			GCancellable *cancellable, GAsyncReadyCallback callback,
			gpointer user_data);
gboolean hev_serial_port_config_finish(HevSerialPort *self,
			GAsyncResult *res, GError **error);

void hev_serial_port_queue_command_async(HevSerialPort *self, GByteArray *command,
			HevSerialPortReadSizeCallback read_callback, GCancellable *cancellable,
			GAsyncReadyCallback callback, gpointer user_data);
GByteArray * hev_serial_port_queue_command_finish(HevSerialPort *self,
			GAsyncResult *res, GError **error);

G_END_DECLS

#endif /* __HEV_SERIAL_PORT_H__ */

