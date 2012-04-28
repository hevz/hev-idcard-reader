/*
 ============================================================================
 Name        : hev-serial-port.c
 Author      : Heiher <admin@heiher.info>
 Version     : 0.0.2
 Copyright   : Copyright (C) 2012 everyone.
 Description : 
 ============================================================================
 */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <glib/gstdio.h>

#include "hev-serial-port.h"

#define HEV_SERIAL_PORT_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE((obj), HEV_TYPE_SERIAL_PORT, HevSerialPortPrivate))

enum
{
	PROP_ZERO,
	PROP_PATH,
	N_PROPERTIES
};

static GParamSpec * hev_serial_port_properties[N_PROPERTIES] = { NULL };

typedef struct _HevSerialPortPrivate HevSerialPortPrivate;

struct _HevSerialPortPrivate
{
	gchar *path;
	gint fd;
	GQueue *queue;
};

typedef struct _HevSerialPortReadWriteData HevSerialPortReadWriteData;

struct _HevSerialPortReadWriteData
{
	gpointer buffer;
	gsize count;
};

typedef struct _HevSerialPortQueueCommandData HevSerialPortQueueCommandData;

struct _HevSerialPortQueueCommandData
{
	GByteArray *command;
	HevSerialPortReadSizeCallback read_callback;
	GCancellable *cancellable;
	gpointer read_buffer;
	gsize read_size;
	gpointer user_data;
};

static void hev_serial_port_async_initable_iface_init(GAsyncInitableIface *iface);

static void hev_serial_port_async_initable_init_async(GAsyncInitable *initable,
			gint io_priority, GCancellable *cancellable,
			GAsyncReadyCallback callback, gpointer user_data);
static gboolean hev_serial_port_async_initable_init_finish(GAsyncInitable *initable,
			GAsyncResult *result, GError **error);

static void hev_serial_port_read_async(HevSerialPort *self, gpointer buffer, gsize count,
			GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
static gssize hev_serial_port_read_finish(HevSerialPort *self, GAsyncResult *result, GError **error);

static void hev_serial_port_write_async(HevSerialPort *self, const gpointer buffer, gsize count,
			GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
static gssize hev_serial_port_write_finish(HevSerialPort *self, GAsyncResult *result, GError **error);

G_DEFINE_TYPE_WITH_CODE(HevSerialPort, hev_serial_port, G_TYPE_OBJECT,
		G_IMPLEMENT_INTERFACE(G_TYPE_ASYNC_INITABLE, hev_serial_port_async_initable_iface_init));

static void g_simple_async_init_thread_handler(GSimpleAsyncResult *simple,
			GObject *object, GCancellable *cancellable);
static void g_simple_async_close_thread_handler(GSimpleAsyncResult *simple,
			GObject *object, GCancellable *cancellable);
static void g_simple_async_config_thread_handler(GSimpleAsyncResult *simple,
			GObject *object, GCancellable *cancellable);
static void g_simple_async_read_thread_handler(GSimpleAsyncResult *simple,
			GObject *object, GCancellable *cancellable);
static void g_simple_async_write_thread_handler(GSimpleAsyncResult *simple,
			GObject *object, GCancellable *cancellable);

static void hev_serial_port_queue_command_handler(HevSerialPort *self);
static void hev_serial_port_write_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data);
static void hev_serial_port_read_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data);

static void hev_serial_port_dispose(GObject *obj)
{
	HevSerialPort *self = HEV_SERIAL_PORT(obj);
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	G_OBJECT_CLASS(hev_serial_port_parent_class)->dispose(obj);
}

static void hev_serial_port_finalize(GObject *obj)
{
	HevSerialPort *self = HEV_SERIAL_PORT(obj);
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(priv->queue)
	{
		g_queue_free(priv->queue);
		priv->queue = NULL;
	}

	if(priv->path)
	{
		g_free(priv->path);
		priv->path = NULL;
	}

	if(0 <= priv->fd)
	{
		close(priv->fd);
		priv->fd = -1;
	}

	G_OBJECT_CLASS(hev_serial_port_parent_class)->finalize(obj);
}

static GObject * hev_serial_port_constructor(GType type, guint n, GObjectConstructParam *param)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	return G_OBJECT_CLASS(hev_serial_port_parent_class)->constructor(type, n, param);
}

static void hev_serial_port_constructed(GObject *obj)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	G_OBJECT_CLASS(hev_serial_port_parent_class)->constructed(obj);
}

static void hev_serial_port_get_property(GObject *obj, guint id,
			GValue *value, GParamSpec *pspec)
{
	HevSerialPort *self = HEV_SERIAL_PORT(obj);
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	switch(id)
	{
	case PROP_PATH:
		g_value_set_string(value, priv->path);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, id, pspec);
		break;
	}
}

static void hev_serial_port_set_property(GObject *obj, guint id,
			const GValue *value, GParamSpec *pspec)
{
	HevSerialPort *self = HEV_SERIAL_PORT(obj);
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	switch(id)
	{
	case PROP_PATH:
		if(priv->path)
		  g_free(priv->path);
		priv->path = g_strdup(g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, id, pspec);
		break;
	}
}

static void hev_serial_port_class_init(HevSerialPortClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	obj_class->constructor = hev_serial_port_constructor;
	obj_class->constructed = hev_serial_port_constructed;
	obj_class->dispose = hev_serial_port_dispose;
	obj_class->finalize = hev_serial_port_finalize;
	obj_class->get_property = hev_serial_port_get_property;
	obj_class->set_property = hev_serial_port_set_property;

	/* Properties */
	hev_serial_port_properties[PROP_PATH] =
		g_param_spec_string("path", "Path", "Device path",
					NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_properties(obj_class, N_PROPERTIES,
				hev_serial_port_properties);

	g_type_class_add_private(klass, sizeof(HevSerialPortPrivate));
}

static void hev_serial_port_async_initable_iface_init(GAsyncInitableIface *iface)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	iface->init_async = hev_serial_port_async_initable_init_async;
	iface->init_finish = hev_serial_port_async_initable_init_finish;
}

static void hev_serial_port_init(HevSerialPort *self)
{
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	/* Queue */
	priv->queue = g_queue_new();

	/* fd */
	priv->fd = -1;
}

void hev_serial_port_new_async(const gchar *path,
			GCancellable *cancellable, GAsyncReadyCallback callback,
			gpointer user_data)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_async_initable_new_async(HEV_TYPE_SERIAL_PORT, G_PRIORITY_DEFAULT,
				cancellable, callback, user_data, "path", path, NULL);
}

GObject * hev_serial_port_new_finish(GAsyncResult *res, GError **error)
{
	GObject *object = NULL, *source_object = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	source_object = g_async_result_get_source_object(res);
	object = g_async_initable_new_finish(G_ASYNC_INITABLE(source_object),
				res, error);
	g_object_unref(source_object);

	return object;
}

void hev_serial_port_close_async(HevSerialPort *self,
			GCancellable *cancellable, GAsyncReadyCallback callback,
			gpointer user_data)
{
	HevSerialPortPrivate *priv = NULL;
	GSimpleAsyncResult *simple = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_return_if_fail(HEV_IS_SERIAL_PORT(self));
	priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	/* Simple async result */
	simple = g_simple_async_result_new(G_OBJECT(self),
				callback, user_data, hev_serial_port_close_async);
	g_simple_async_result_set_check_cancellable(simple, cancellable);

	g_simple_async_result_run_in_thread(simple, g_simple_async_close_thread_handler,
				G_PRIORITY_DEFAULT, cancellable);

	g_object_unref(simple);
}

gboolean hev_serial_port_close_finish(HevSerialPort *self,
			GAsyncResult *res, GError **error)
{
	HevSerialPortPrivate *priv = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_return_val_if_fail(HEV_IS_SERIAL_PORT(self), FALSE);
	priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	g_return_val_if_fail(g_simple_async_result_is_valid(res,
					G_OBJECT(self), hev_serial_port_close_async),
				FALSE);
	if(g_simple_async_result_propagate_error(G_SIMPLE_ASYNC_RESULT(res),
					error))
	  return FALSE;

	return g_simple_async_result_get_op_res_gboolean(G_SIMPLE_ASYNC_RESULT(res));
}

void hev_serial_port_config_async(HevSerialPort *self, struct termios *options,
			GCancellable *cancellable, GAsyncReadyCallback callback,
			gpointer user_data)
{
	HevSerialPortPrivate *priv = NULL;
	GSimpleAsyncResult *simple = NULL;
	struct termios *opts = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_return_if_fail(HEV_IS_SERIAL_PORT(self));
	g_return_if_fail(NULL != options);
	priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	/* Simple async result */
	simple = g_simple_async_result_new(G_OBJECT(self),
				callback, user_data, hev_serial_port_config_async);
	g_simple_async_result_set_check_cancellable(simple, cancellable);

	opts = g_malloc0(sizeof(struct termios));
	g_memmove(opts, options, sizeof(struct termios));
	g_simple_async_result_set_op_res_gpointer(simple, opts, NULL);

	g_simple_async_result_run_in_thread(simple, g_simple_async_config_thread_handler,
				G_PRIORITY_DEFAULT, cancellable);

	g_object_unref(simple);
}

gboolean hev_serial_port_config_finish(HevSerialPort *self,
			GAsyncResult *res, GError **error)
{
	HevSerialPortPrivate *priv = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_return_val_if_fail(HEV_IS_SERIAL_PORT(self), FALSE);
	priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	g_return_val_if_fail(g_simple_async_result_is_valid(res,
					G_OBJECT(self), hev_serial_port_config_async),
				FALSE);
	if(g_simple_async_result_propagate_error(G_SIMPLE_ASYNC_RESULT(res),
					error))
	  return FALSE;

	return g_simple_async_result_get_op_res_gboolean(G_SIMPLE_ASYNC_RESULT(res));
}

void hev_serial_port_queue_command_async(HevSerialPort *self, GByteArray *command,
			HevSerialPortReadSizeCallback read_callback, GCancellable *cancellable,
			GAsyncReadyCallback callback, gpointer user_data)
{
	HevSerialPortPrivate *priv = NULL;
	GSimpleAsyncResult *simple = NULL;
	HevSerialPortQueueCommandData *data = NULL;
	gboolean empty = FALSE;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_return_if_fail(HEV_IS_SERIAL_PORT(self));
	g_return_if_fail(NULL != command);
	g_return_if_fail(NULL != read_callback);
	priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	/* Queue is empty */
	empty = g_queue_is_empty(priv->queue);
	
	/* Simple async result */
	simple = g_simple_async_result_new(G_OBJECT(self),
				callback, user_data, hev_serial_port_queue_command_async);
	g_simple_async_result_set_check_cancellable(simple, cancellable);

	/* Queue command data */
	data = g_malloc0(sizeof(HevSerialPortQueueCommandData));
	data->command = g_byte_array_ref(command);
	data->read_callback = read_callback;
	data->cancellable = cancellable;
	data->user_data = user_data;
	g_simple_async_result_set_op_res_gpointer(simple, data, NULL);

	/* Push head */
	g_queue_push_head(priv->queue, simple);

	if(empty)
	  hev_serial_port_queue_command_handler(self);
}

GByteArray * hev_serial_port_queue_command_finish(HevSerialPort *self,
			GAsyncResult *res, GError **error)
{
	HevSerialPortPrivate *priv = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_return_val_if_fail(HEV_IS_SERIAL_PORT(self), NULL);
	priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	/* Pop tail */
	g_queue_pop_tail(priv->queue);
	if(!g_queue_is_empty(priv->queue))
	  hev_serial_port_queue_command_handler(self);

	g_return_val_if_fail(g_simple_async_result_is_valid(res,
					G_OBJECT(self), hev_serial_port_queue_command_async),
				NULL);
	if(g_simple_async_result_propagate_error(G_SIMPLE_ASYNC_RESULT(res),
					error))
	  return NULL;

	return g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(res));
}

static void g_simple_async_init_thread_handler(GSimpleAsyncResult *simple,
			GObject *object, GCancellable *cancellable)
{
	HevSerialPort *self = HEV_SERIAL_PORT(object);
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	/* Open */
	priv->fd = g_open(priv->path, O_RDWR | O_EXCL | O_NONBLOCK | O_NOCTTY, 0);
	if(-1 == priv->fd)
	{
		GError *error = NULL;

		error = g_error_new(G_IO_ERROR, g_io_error_from_errno(errno),
					"%s", strerror(errno));
		g_simple_async_result_take_error(simple, error);
	}
}

static void g_simple_async_close_thread_handler(GSimpleAsyncResult *simple,
			GObject *object, GCancellable *cancellable)
{
	HevSerialPort *self = HEV_SERIAL_PORT(object);
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(-1 == close(priv->fd))
	{
		g_simple_async_result_set_error(simple, G_IO_ERROR,
					g_io_error_from_errno(errno), "%s", strerror(errno));
		g_simple_async_result_set_op_res_gboolean(simple, FALSE);
	}
	else
	{
		priv->fd = -1;
		g_simple_async_result_set_op_res_gboolean(simple, TRUE);
	}
}

static void g_simple_async_config_thread_handler(GSimpleAsyncResult *simple,
			GObject *object, GCancellable *cancellable)
{
	HevSerialPort *self = HEV_SERIAL_PORT(object);
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);
	struct termios *options = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	options = g_simple_async_result_get_op_res_gpointer(simple);

	if(-1 == tcflush(priv->fd, TCIFLUSH))
	{
		GError *error = NULL;

		error = g_error_new(G_IO_ERROR, g_io_error_from_errno(errno),
					"%s", strerror(errno));
		g_simple_async_result_take_error(simple, error);
		g_simple_async_result_set_op_res_gboolean(simple, FALSE);

		goto ret;
	}
	if(-1 == tcsetattr(priv->fd, TCSANOW, options))
	{
		GError *error = NULL;

		error = g_error_new(G_IO_ERROR, g_io_error_from_errno(errno),
					"%s", strerror(errno));
		g_simple_async_result_take_error(simple, error);
		g_simple_async_result_set_op_res_gboolean(simple, FALSE);

		goto ret;
	}

	g_simple_async_result_set_op_res_gboolean(simple, TRUE);

ret:
	g_free(options);
}

static void g_simple_async_read_thread_handler(GSimpleAsyncResult *simple,
			GObject *object, GCancellable *cancellable)
{
	HevSerialPort *self = HEV_SERIAL_PORT(object);
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);
	HevSerialPortReadWriteData *data = NULL;
	gssize i = 0, r = 0;
	GPollFD fds[] =
	{
		{priv->fd, G_IO_IN, G_IO_NVAL}
	};

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	data = g_simple_async_result_get_op_res_gpointer(simple);

	while(1)
	{
		gint rfds = g_poll(fds, 1, 200);
		
		if(0 < rfds)
		{
			i = read(priv->fd, data->buffer+r, data->count-r);

			if(-1 == i)
			{
				g_simple_async_result_set_error(simple, G_IO_ERROR,
							g_io_error_from_errno(errno), "%s", strerror(errno));
				break;
			}
			else
			  r += i;
		}

		if(data->count == r)
		  break;

		if(g_cancellable_is_cancelled(cancellable))
		  goto ret;
	}

	g_simple_async_result_set_op_res_gssize(simple, r);

ret:
	g_free(data);
}

static void g_simple_async_write_thread_handler(GSimpleAsyncResult *simple,
			GObject *object, GCancellable *cancellable)
{
	HevSerialPort *self = HEV_SERIAL_PORT(object);
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);
	HevSerialPortReadWriteData *data = NULL;
	gssize i = 0, w = 0;
	GPollFD fds[] =
	{
		{priv->fd, G_IO_OUT, G_IO_NVAL}
	};

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	data = g_simple_async_result_get_op_res_gpointer(simple);

	while(1)
	{
		gint rfds = g_poll(fds, 1, 200);
		
		if(0 < rfds)
		{
			i = write(priv->fd, data->buffer+w, data->count-w);

			if(-1 == i)
			{
				g_simple_async_result_set_error(simple, G_IO_ERROR,
							g_io_error_from_errno(errno), "%s", strerror(errno));
				break;
			}
			else
			  w += i;
		}

		if(data->count == w)
		  break;

		if(g_cancellable_is_cancelled(cancellable))
		  goto ret;
	}

	g_simple_async_result_set_op_res_gssize(simple, w);

ret:
	g_free(data);
}

static void hev_serial_port_async_initable_init_async(GAsyncInitable *initable,
			gint io_priority, GCancellable *cancellable,
			GAsyncReadyCallback callback, gpointer user_data)
{
	HevSerialPort *self = HEV_SERIAL_PORT(initable);
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);
	GSimpleAsyncResult *simple = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	/* Simple async result */
	simple = g_simple_async_result_new(G_OBJECT(initable),
				callback, user_data, hev_serial_port_async_initable_init_async);
	g_simple_async_result_set_check_cancellable(simple, cancellable);

	g_simple_async_result_run_in_thread(simple, g_simple_async_init_thread_handler,
				io_priority, cancellable);

	g_object_unref(simple);
}

static gboolean hev_serial_port_async_initable_init_finish(GAsyncInitable *initable,
			GAsyncResult *result, GError **error)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_return_val_if_fail(g_simple_async_result_is_valid(result,
					G_OBJECT(initable), hev_serial_port_async_initable_init_async),
				FALSE);
	if(g_simple_async_result_propagate_error(G_SIMPLE_ASYNC_RESULT(result),
					error))
	  return FALSE;

	return TRUE;
}

static void hev_serial_port_read_async(HevSerialPort *self, gpointer buffer, gsize count,
			GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	HevSerialPortPrivate *priv = NULL;
	GSimpleAsyncResult *simple = NULL;
	HevSerialPortReadWriteData *data = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_return_if_fail(HEV_IS_SERIAL_PORT(self));
	priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	/* Simple async result */
	simple = g_simple_async_result_new(G_OBJECT(self),
				callback, user_data, hev_serial_port_read_async);
	g_simple_async_result_set_check_cancellable(simple, cancellable);

	/* ReadWrite data */
	data = g_malloc0(sizeof(HevSerialPortReadWriteData));
	data->buffer = buffer;
	data->count = count;
	g_simple_async_result_set_op_res_gpointer(simple, data, NULL);

	g_simple_async_result_run_in_thread(simple, g_simple_async_read_thread_handler,
				G_PRIORITY_DEFAULT, cancellable);

	g_object_unref(simple);
}

static gssize hev_serial_port_read_finish(HevSerialPort *self, GAsyncResult *result, GError **error)
{
	HevSerialPortPrivate *priv = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_return_val_if_fail(HEV_IS_SERIAL_PORT(self), -1);
	priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	g_return_val_if_fail(g_simple_async_result_is_valid(result,
					G_OBJECT(self), hev_serial_port_read_async),
				-1);
	if(g_simple_async_result_propagate_error(G_SIMPLE_ASYNC_RESULT(result),
					error))
	  return -1;

	return g_simple_async_result_get_op_res_gssize(G_SIMPLE_ASYNC_RESULT(result));
}

static void hev_serial_port_write_async(HevSerialPort *self, const gpointer buffer, gsize count,
			GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	HevSerialPortPrivate *priv = NULL;
	GSimpleAsyncResult *simple = NULL;
	HevSerialPortReadWriteData *data = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_return_if_fail(HEV_IS_SERIAL_PORT(self));
	priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	/* Simple async result */
	simple = g_simple_async_result_new(G_OBJECT(self),
				callback, user_data, hev_serial_port_write_async);
	g_simple_async_result_set_check_cancellable(simple, cancellable);

	/* ReadWrite data */
	data = g_malloc0(sizeof(HevSerialPortReadWriteData));
	data->buffer = buffer;
	data->count = count;
	g_simple_async_result_set_op_res_gpointer(simple, data, NULL);

	g_simple_async_result_run_in_thread(simple, g_simple_async_write_thread_handler,
				G_PRIORITY_DEFAULT, cancellable);

	g_object_unref(simple);
}

static gssize hev_serial_port_write_finish(HevSerialPort *self, GAsyncResult *result, GError **error)
{
	HevSerialPortPrivate *priv = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	g_return_val_if_fail(HEV_IS_SERIAL_PORT(self), -1);
	priv = HEV_SERIAL_PORT_GET_PRIVATE(self);

	g_return_val_if_fail(g_simple_async_result_is_valid(result,
					G_OBJECT(self), hev_serial_port_write_async),
				-1);
	if(g_simple_async_result_propagate_error(G_SIMPLE_ASYNC_RESULT(result),
					error))
	  return -1;

	return g_simple_async_result_get_op_res_gssize(G_SIMPLE_ASYNC_RESULT(result));
}

static void hev_serial_port_queue_command_handler(HevSerialPort *self)
{
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);
	GSimpleAsyncResult *simple = NULL;
	HevSerialPortQueueCommandData *data = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	simple = g_queue_peek_tail(priv->queue);
	data = g_simple_async_result_get_op_res_gpointer(simple);

	/* Write command */
	hev_serial_port_write_async(self, data->command->data, data->command->len,
				data->cancellable, hev_serial_port_write_async_handler, simple);
}

static void hev_serial_port_write_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data)
{
	HevSerialPort *self = HEV_SERIAL_PORT(source_object);
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);
	GSimpleAsyncResult *simple = user_data;
	HevSerialPortQueueCommandData *data = NULL;
	gssize size = 0;
	GError *error = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	data = g_simple_async_result_get_op_res_gpointer(simple);
	g_byte_array_unref(data->command);

	size = hev_serial_port_write_finish(self, res, &error);
	if(-1 == size)
	{
		g_simple_async_result_take_error(simple, error);
		g_simple_async_result_set_op_res_gpointer(simple, NULL, NULL);
		g_simple_async_result_complete_in_idle(simple);
		g_object_unref(simple);

		g_free(data);

		return;
	}

	/* Get read size */
	size = data->read_callback(NULL, 0, data->user_data);
	if(0 < size)
	{
		data->read_buffer = g_malloc0(size);
		if(data->read_buffer)
		{
			data->read_size = size;
			/* Read data */
			hev_serial_port_read_async(self, data->read_buffer, data->read_size,
						data->cancellable, hev_serial_port_read_async_handler, simple);
		}
		else
		{
			g_simple_async_result_set_error(simple, g_quark_from_static_string("Memory"),
						0, "Memory not enough!");
			g_simple_async_result_set_op_res_gpointer(simple, NULL, NULL);
			g_simple_async_result_complete_in_idle(simple);
			g_object_unref(simple);

			g_free(data);
		}
	}
	else
	{
		GByteArray *rdata = NULL;

		rdata = g_byte_array_new();
		g_simple_async_result_set_op_res_gpointer(simple, rdata, NULL);
		g_simple_async_result_complete(simple);
		g_object_unref(simple);

		g_free(data);
	}
}

static void hev_serial_port_read_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data)
{
	HevSerialPort *self = HEV_SERIAL_PORT(source_object);
	HevSerialPortPrivate *priv = HEV_SERIAL_PORT_GET_PRIVATE(self);
	GSimpleAsyncResult *simple = user_data;
	HevSerialPortQueueCommandData *data = NULL;
	gssize size = 0;
	GError *error = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	data = g_simple_async_result_get_op_res_gpointer(simple);
	size = hev_serial_port_read_finish(self, res, &error);
	if(-1 == size)
	{
		g_simple_async_result_take_error(simple, error);
		g_simple_async_result_set_op_res_gpointer(simple, NULL, NULL);
		g_simple_async_result_complete_in_idle(simple);
		g_object_unref(simple);

		if(data->read_buffer)
		  g_free(data->read_buffer);
		g_free(data);

		return;
	}

	/* Get read size */
	size = data->read_callback(data->read_buffer, data->read_size,
				data->user_data);
	if(0 < size)
	{
		gpointer old_buffer = data->read_buffer;
		gsize new_size = data->read_size+size;

		data->read_buffer = g_malloc0(new_size);
		if(data->read_buffer)
		{
			g_memmove(data->read_buffer, old_buffer, data->read_size);

			/* Read data */
			hev_serial_port_read_async(self, data->read_buffer+data->read_size, size,
						data->cancellable, hev_serial_port_read_async_handler,simple);

			data->read_size = new_size;
		}
		else
		{
			g_simple_async_result_set_error(simple, g_quark_from_static_string("Memory"),
						0, "Memory not enough!");
			g_simple_async_result_set_op_res_gpointer(simple, NULL, NULL);
			g_simple_async_result_complete_in_idle(simple);
			g_object_unref(simple);

			g_free(data);
		}

		g_free(old_buffer);
	}
	else
	{
		GByteArray *rdata = NULL;

		rdata = g_byte_array_new();
		g_byte_array_append(rdata, data->read_buffer, data->read_size);
		g_simple_async_result_set_op_res_gpointer(simple, rdata, NULL);
		g_simple_async_result_complete(simple);
		g_object_unref(simple);

		if(data->read_buffer)
		  g_free(data->read_buffer);
		g_free(data);
	}
}

