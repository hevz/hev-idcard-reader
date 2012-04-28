/*
 ============================================================================
 Name        : hev-dbus-object-idcard-reader.c
 Author      : Heiher <admin@heiher.info>
 Version     : 0.0.1
 Copyright   : Copyright (C) 2012 everyone.
 Description : 
 ============================================================================
 */

#include "hev-dbus-object-idcard-reader.h"
#include "hev-dbus-object-idcard-reader-manager.h"
#include "hev-serial-port.h"

#define HEV_DBUS_OBJECT_IDCARD_READER_GET_STATUS_DELAY			3000
#define HEV_DBUS_OBJECT_IDCARD_READER_GET_CARD_STATUS_DELAY		1000
#define HEV_DBUS_OBJECT_IDCARD_READER_QUEUE_COMMAND_TIMEOUT		5000

#define HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE((obj), HEV_TYPE_DBUS_OBJECT_IDCARD_READER, HevDBusObjectIDCardReaderPrivate))

enum
{
	PROP_ZERO,
	PROP_SERIAL_PORT,
	N_PROPERTIES
};

enum
{
	SIG_STATUS_CHANGED,
	SIG_CARD_STATUS_CHANGED,
	SIG_CARD_INFO_CHANGED,
	LAST_SIGNAL
};

/* Get reader status cmd */
static const guchar cmd_get_status[] = {0xAA, 0xAA, 0xAA, 0x96, 0x69, 0x00, 0x03, 0x11, 0xFF, 0xED};
/* Get card status cmd */
static const guchar cmd_get_card_status[] = {0xAA, 0xAA, 0xAA, 0x96, 0x69, 0x00, 0x03, 0x20, 0x01, 0x22};
/* Select card cmd */
static const guchar cmd_select_card[] = {0xAA, 0xAA, 0xAA, 0x96, 0x69, 0x00, 0x03, 0x20, 0x02, 0x21};
/* Read base info cmd */
static const guchar cmd_read_base_info[] = {0xAA, 0xAA, 0xAA, 0x96, 0x69, 0x00, 0x03, 0x30, 0x01, 0x32};
/* Set max rf byte cmd */
static const guchar cmd_set_max_rf_byte[] = {0xAA, 0xAA, 0xAA, 0x96, 0x69, 0x00, 0x04, 0x61, 0xFF, 0x50, 0xCA};

/* Nation database */
static const gchar *nations[] =
{
	"解码错",	/* 00 */
	"汉",		/* 01 */
	"蒙古",		/* 02 */
	"回",		/* 03 */
	"藏",		/* 04 */
	"维吾尔",	/* 05 */
	"苗",		/* 06 */
	"彝",		/* 07 */
	"壮",		/* 08 */
	"布依",		/* 09 */
	"朝鲜",		/* 10 */
	"满",		/* 11 */
	"侗",		/* 12 */
	"瑶",		/* 13 */
	"白",		/* 14 */
	"土家",		/* 15 */
	"哈尼",		/* 16 */
	"哈萨克",	/* 17 */
	"傣",		/* 18 */
	"黎",		/* 19 */
	"傈僳",		/* 20 */
	"佤",		/* 21 */
	"畲",		/* 22 */
	"高山",		/* 23 */
	"拉祜",		/* 24 */
	"水",		/* 25 */
	"东乡",		/* 26 */
	"纳西",		/* 27 */
	"景颇",		/* 28 */
	"柯尔克孜",	/* 29 */
	"土",		/* 30 */
	"达斡尔",	/* 31 */
	"仫佬",		/* 32 */
	"羌",		/* 33 */
	"布朗",		/* 34 */
	"撒拉",		/* 35 */
	"毛南",		/* 36 */
	"仡佬",		/* 37 */
	"锡伯",		/* 38 */
	"阿昌",		/* 39 */
	"普米",		/* 40 */
	"塔吉克",	/* 41 */
	"怒",		/* 42 */
	"乌孜别克",	/* 43 */
	"俄罗斯",	/* 44 */
	"鄂温克",	/* 45 */
	"德昴",		/* 46 */
	"保安",		/* 47 */
	"裕固",		/* 48 */
	"京",		/* 49 */
	"塔塔尔",	/* 50 */
	"独龙",		/* 51 */
	"鄂伦春",	/* 52 */
	"赫哲",		/* 53 */
	"门巴",		/* 54 */
	"珞巴",		/* 55 */
	"基诺",		/* 56 */
	"编码错",	/* 57 */
	"其他",		/* 97 */
	"外国血统"	/* 98 */
};

static guint hev_dbus_object_idcard_reader_signals[LAST_SIGNAL] = { 0 };
static GParamSpec * hev_dbus_object_idcard_reader_properties[N_PROPERTIES] = { NULL };

typedef struct _HevDBusObjectIDCardReaderPrivate HevDBusObjectIDCardReaderPrivate;

struct _HevDBusObjectIDCardReaderPrivate
{
	GObject *serial_port;
	gboolean status;
	gboolean card_status;
	gboolean init_reader;
	GVariant *card_info;
};

typedef struct _HevDBusObjectIDCardReaderCmdData HevDBusObjectIDCardReaderCmdData;

struct _HevDBusObjectIDCardReaderCmdData
{
	GAsyncReadyCallback callback;
	GCancellable *cancellable;
	GSourceFunc timeout_callback;
	guint timeout_id;
	gpointer user_data;
};

typedef struct _HevDBusObjectIDCardReaderCardInfo HevDBusObjectIDCardReaderCardInfo;

struct _HevDBusObjectIDCardReaderCardInfo
{
	gushort name[15];
	gushort sex;
	gushort nation[2];
	gushort birthday[8];
	gushort address[35];
	gushort serial_number[18];
	gushort grant_dept[15];
	gushort begin_date[8];
	gushort end_date[8];
};

static gboolean g_timeout_handler(gpointer user_data);
static void hev_serial_port_queue_command_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data);
static void hev_dbus_object_idcard_reader_queue_command_async(HevDBusObjectIDCardReader *self,
			GByteArray *command, GAsyncReadyCallback callback, HevSerialPortReadSizeCallback rs_callback,
			guint timeout, GSourceFunc timeout_callback, gpointer user_data);

G_DEFINE_TYPE(HevDBusObjectIDCardReader, hev_dbus_object_idcard_reader, G_TYPE_DBUS_OBJECT_SKELETON);

static gsize hev_serial_port_read_size_handler(const gpointer buffer, gssize size,
			gpointer user_data);
static gboolean hev_dbus_object_idcard_reader_queue_command_timeout_handler(gpointer user_data);
static gboolean hev_dbus_object_idcard_reader_queue_command_card_timeout_handler(gpointer user_data);
static gboolean hev_dbus_object_idcard_reader_queue_command_null_timeout_handler(gpointer user_data);

static void hev_dbus_object_idcard_reader_get_status_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data);
static gboolean hev_dbus_object_idcard_reader_install_get_status_timeout_handler(gpointer user_data);

static void hev_dbus_object_idcard_reader_get_card_status_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data);
static gboolean hev_dbus_object_idcard_reader_install_get_card_status_timeout_handler(gpointer user_data);

static void hev_dbus_object_idcard_reader_set_max_rf_byte_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data);

static void hev_dbus_object_idcard_reader_select_card_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data);

static void hev_dbus_object_idcard_reader_read_base_info_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data);

static void hev_dbus_object_idcard_reader_dispose(GObject *obj)
{
	HevDBusObjectIDCardReader *self = HEV_DBUS_OBJECT_IDCARD_READER(obj);
	HevDBusObjectIDCardReaderPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	G_OBJECT_CLASS(hev_dbus_object_idcard_reader_parent_class)->dispose(obj);
}

static void hev_dbus_object_idcard_reader_finalize(GObject *obj)
{
	HevDBusObjectIDCardReader *self = HEV_DBUS_OBJECT_IDCARD_READER(obj);
	HevDBusObjectIDCardReaderPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(priv->card_info)
	{
		g_variant_unref(priv->card_info);
		priv->card_info = NULL;
	}

	G_OBJECT_CLASS(hev_dbus_object_idcard_reader_parent_class)->finalize(obj);
}

static GObject * hev_dbus_object_idcard_reader_constructor(GType type, guint n, GObjectConstructParam *param)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);
	return G_OBJECT_CLASS(hev_dbus_object_idcard_reader_parent_class)->constructor(type, n, param);
}

static void hev_dbus_object_idcard_reader_constructed(GObject *obj)
{
	HevDBusObjectIDCardReader *self = HEV_DBUS_OBJECT_IDCARD_READER(obj);
	HevDBusObjectIDCardReaderPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);
	GByteArray *command = g_byte_array_new();
	g_byte_array_append(command, cmd_get_status, sizeof(cmd_get_status));

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	G_OBJECT_CLASS(hev_dbus_object_idcard_reader_parent_class)->constructed(obj);
	
	/* First get reader status */
	hev_dbus_object_idcard_reader_queue_command_async(self, command,
				hev_dbus_object_idcard_reader_get_status_async_handler,
				hev_serial_port_read_size_handler, HEV_DBUS_OBJECT_IDCARD_READER_QUEUE_COMMAND_TIMEOUT,
				hev_dbus_object_idcard_reader_queue_command_timeout_handler,
				self);

	g_byte_array_unref(command);
}

static void hev_dbus_object_idcard_reader_set_property(GObject *obj, guint id,
			const GValue *value, GParamSpec *pspec)
{
	HevDBusObjectIDCardReader *self = HEV_DBUS_OBJECT_IDCARD_READER(obj);
	HevDBusObjectIDCardReaderPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	switch(id)
	{
	case PROP_SERIAL_PORT:
		priv->serial_port = g_value_get_object(value);
		g_object_notify_by_pspec(obj, hev_dbus_object_idcard_reader_properties[id]);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, id, pspec);
		break;
	}
}

static void hev_dbus_object_idcard_reader_get_property(GObject *obj, guint id,
			GValue *value, GParamSpec *pspec)
{
	HevDBusObjectIDCardReader *self = HEV_DBUS_OBJECT_IDCARD_READER(obj);
	HevDBusObjectIDCardReaderPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	switch(id)
	{
	case PROP_SERIAL_PORT:
		g_value_set_object(value, priv->serial_port);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, id, pspec);
		break;
	}
}

static void hev_dbus_object_idcard_reader_class_init(HevDBusObjectIDCardReaderClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS(klass);

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	obj_class->constructor = hev_dbus_object_idcard_reader_constructor;
	obj_class->constructed = hev_dbus_object_idcard_reader_constructed;
	obj_class->dispose = hev_dbus_object_idcard_reader_dispose;
	obj_class->finalize = hev_dbus_object_idcard_reader_finalize;
	obj_class->set_property = hev_dbus_object_idcard_reader_set_property;
	obj_class->get_property = hev_dbus_object_idcard_reader_get_property;

	/* Properties */
	hev_dbus_object_idcard_reader_properties[PROP_SERIAL_PORT] =
		g_param_spec_object("serial-port", "Serial port", "Serial port",
					G_TYPE_OBJECT, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
	g_object_class_install_properties(obj_class, N_PROPERTIES,
				hev_dbus_object_idcard_reader_properties);

	/* Signals */
	hev_dbus_object_idcard_reader_signals[SIG_STATUS_CHANGED] =
		g_signal_new("status-changed",
				G_TYPE_FROM_CLASS(klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET(HevDBusObjectIDCardReaderClass, status_changed),
				NULL, NULL,
				g_cclosure_marshal_VOID__BOOLEAN,
				G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	hev_dbus_object_idcard_reader_signals[SIG_CARD_STATUS_CHANGED] =
		g_signal_new("card-status-changed",
				G_TYPE_FROM_CLASS(klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET(HevDBusObjectIDCardReaderClass, card_status_changed),
				NULL, NULL,
				g_cclosure_marshal_VOID__BOOLEAN,
				G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	hev_dbus_object_idcard_reader_signals[SIG_CARD_INFO_CHANGED] =
		g_signal_new("card-info-changed",
				G_TYPE_FROM_CLASS(klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET(HevDBusObjectIDCardReaderClass, card_info_changed),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);

	g_type_class_add_private(klass, sizeof(HevDBusObjectIDCardReaderPrivate));
}

static void hev_dbus_object_idcard_reader_init(HevDBusObjectIDCardReader *self)
{
	HevDBusObjectIDCardReaderPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);
	GVariantBuilder *builder = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	priv->status = FALSE;
	priv->card_status = FALSE;
	priv->init_reader = TRUE;

	builder = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
	priv->card_info = g_variant_builder_end(builder);
	g_variant_builder_unref(builder);
}

GObject *hev_dbus_object_idcard_reader_new(const gchar *object_path, GObject *serial_port)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);
	return g_object_new(HEV_TYPE_DBUS_OBJECT_IDCARD_READER,
				"g-object-path", object_path,
				"serial-port", serial_port,
				NULL);
}

gboolean hev_dbus_object_idcard_reader_get_cached_status(HevDBusObjectIDCardReader *self, GError **error)
{
	HevDBusObjectIDCardReaderPrivate *priv = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(!HEV_IS_DBUS_OBJECT_IDCARD_READER(self))
	{
		if(error)
		  *error = g_error_new(g_quark_from_static_string("GObject"), 0,
					  "The %s isn't %s!", "self", "HevDBusObjectIDCardReader");
		return FALSE;
	}

	priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	return priv->status;
}

gboolean hev_dbus_object_idcard_reader_get_cached_card_status(HevDBusObjectIDCardReader *self, GError **error)
{
	HevDBusObjectIDCardReaderPrivate *priv = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(!HEV_IS_DBUS_OBJECT_IDCARD_READER(self))
	{
		if(error)
		  *error = g_error_new(g_quark_from_static_string("GObject"), 0,
					  "The %s isn't %s!", "self", "HevDBusObjectIDCardReader");
		return FALSE;
	}

	priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	return priv->card_status;
}

GVariant * hev_dbus_object_idcard_reader_get_cached_card_info(HevDBusObjectIDCardReader *self, GError **error)
{
	HevDBusObjectIDCardReaderPrivate *priv = NULL;
	GVariantBuilder *builder = NULL;
	GVariant *variant = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(!HEV_IS_DBUS_OBJECT_IDCARD_READER(self))
	{
		if(error)
		  *error = g_error_new(g_quark_from_static_string("GObject"), 0,
					  "The %s isn't %s!", "self", "HevDBusObjectIDCardReader");
		return FALSE;
	}

	priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	/* Save cached card info */
	variant = priv->card_info;

	/* Reset cached card info */
	builder = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
	priv->card_info = g_variant_builder_end(builder);
	g_variant_builder_unref(builder);

	return variant;
}

static gboolean g_timeout_handler(gpointer user_data)
{
	HevDBusObjectIDCardReaderCmdData *cmd_data = user_data;
	GCancellable *cancellable = cmd_data->cancellable;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	/* Cancel */
	g_cancellable_cancel(cancellable);

	return cmd_data->timeout_callback(cmd_data->user_data);
}

static void hev_serial_port_queue_command_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data)
{
	HevDBusObjectIDCardReaderCmdData *cmd_data = user_data;
	guint timeout_id = cmd_data->timeout_id;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	/* Remove timeout */
	g_source_remove(timeout_id);

	cmd_data->callback(source_object, res, cmd_data->user_data);

	g_slice_free(HevDBusObjectIDCardReaderCmdData, cmd_data);
}

static void hev_dbus_object_idcard_reader_queue_command_async(HevDBusObjectIDCardReader *self,
			GByteArray *command, GAsyncReadyCallback callback, HevSerialPortReadSizeCallback rs_callback,
			guint timeout, GSourceFunc timeout_callback, gpointer user_data)
{
	HevDBusObjectIDCardReaderPrivate *priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);
	HevDBusObjectIDCardReaderCmdData *cmd_data = NULL;
	GCancellable *cancellable = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	/* Cmd data */
	cmd_data = g_slice_new0(HevDBusObjectIDCardReaderCmdData);
	cmd_data->callback = callback;
	cmd_data->timeout_callback = timeout_callback;
	cmd_data->user_data = user_data;

	/* Cancellable */
	cancellable = g_cancellable_new();
	cmd_data->cancellable = cancellable;

	/* Timeout */
	cmd_data->timeout_id = g_timeout_add(timeout,
				g_timeout_handler, cmd_data);

	/* Queue command */
	hev_serial_port_queue_command_async(HEV_SERIAL_PORT(priv->serial_port), command,
				rs_callback, cancellable, hev_serial_port_queue_command_async_handler,
				cmd_data);

	g_object_unref(cancellable);
}

static gsize hev_serial_port_read_size_handler(const gpointer buffer, gssize size,
			gpointer user_data)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(0 == size)
	  return 7;
	else if(7 == size)
	  return ((guint8 *)buffer)[5] * 256 + ((guint8 *)buffer)[6];
	else
	  return 0;
}

static gboolean hev_dbus_object_idcard_reader_queue_command_timeout_handler(gpointer user_data)
{
	HevDBusObjectIDCardReader *self = NULL;
	HevDBusObjectIDCardReaderPrivate *priv = NULL;
	HevDBusObjectIDCardReaderManager *manager = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(!HEV_IS_DBUS_OBJECT_IDCARD_READER(user_data))
	  return FALSE;

	self = HEV_DBUS_OBJECT_IDCARD_READER(user_data);
	priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	if(priv->status)
	{
		priv->status = FALSE;
		g_signal_emit(self, hev_dbus_object_idcard_reader_signals[SIG_STATUS_CHANGED],
					0, priv->status);
	}

	if(priv->card_status)
	{
		priv->card_status = FALSE;
		g_signal_emit(self, hev_dbus_object_idcard_reader_signals[SIG_CARD_STATUS_CHANGED],
					0, priv->card_status);
	}

	manager = g_object_get_data(G_OBJECT(self), "manager");
	hev_dbus_object_idcard_reader_manager_request_remove(manager, G_OBJECT(self));
	
	return FALSE;
}

static gboolean hev_dbus_object_idcard_reader_queue_command_card_timeout_handler(gpointer user_data)
{
	HevDBusObjectIDCardReader *self = NULL;
	HevDBusObjectIDCardReaderPrivate *priv = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(!HEV_IS_DBUS_OBJECT_IDCARD_READER(user_data))
	  return FALSE;

	self = HEV_DBUS_OBJECT_IDCARD_READER(user_data);
	priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	if(priv->card_status)
	{
		priv->card_status = FALSE;
		g_signal_emit(self, hev_dbus_object_idcard_reader_signals[SIG_CARD_STATUS_CHANGED],
					0, priv->card_status);
	}

	return FALSE;
}

static gboolean hev_dbus_object_idcard_reader_queue_command_null_timeout_handler(gpointer user_data)
{
	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	return FALSE;
}

static void hev_dbus_object_idcard_reader_get_status_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data)
{
	HevDBusObjectIDCardReader *self = NULL;
	HevDBusObjectIDCardReaderPrivate *priv = NULL;
	GByteArray *data = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	data = hev_serial_port_queue_command_finish(HEV_SERIAL_PORT(source_object),
				res, NULL);
	if(!HEV_IS_DBUS_OBJECT_IDCARD_READER(user_data))
	{
		if(data)
		  g_byte_array_unref(data);

		return;
	}

	self = HEV_DBUS_OBJECT_IDCARD_READER(user_data);
	priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	if(data)
	{
		/* Status */
		if((9<=data->len) && (0x90==data->data[9]))
		{
			if(!priv->status)
			{
				priv->status = TRUE;
				g_signal_emit(self, hev_dbus_object_idcard_reader_signals[SIG_STATUS_CHANGED],
							0, priv->status);
			}

			if(priv->init_reader)
			{
				GByteArray *command = g_byte_array_new();
				g_byte_array_append(command, cmd_set_max_rf_byte,
							sizeof(cmd_set_max_rf_byte));

				/* Set max rf byte */
				hev_dbus_object_idcard_reader_queue_command_async(self, command,
							hev_dbus_object_idcard_reader_set_max_rf_byte_async_handler,
							hev_serial_port_read_size_handler, HEV_DBUS_OBJECT_IDCARD_READER_QUEUE_COMMAND_TIMEOUT,
							hev_dbus_object_idcard_reader_queue_command_null_timeout_handler,
							self);
				
				g_byte_array_unref(command);

				/* Install get card status timeout */
				g_timeout_add(HEV_DBUS_OBJECT_IDCARD_READER_GET_CARD_STATUS_DELAY,
							hev_dbus_object_idcard_reader_install_get_card_status_timeout_handler,
							self);

				priv->init_reader = FALSE;
			}
			else
			{
				/* Install get reader status timeout */
				g_timeout_add(HEV_DBUS_OBJECT_IDCARD_READER_GET_STATUS_DELAY,
							hev_dbus_object_idcard_reader_install_get_status_timeout_handler,
							self);
			}
		}
		else
		{
			HevDBusObjectIDCardReaderManager *manager = NULL;
			
			if(priv->status)
			{
				priv->status = FALSE;
				g_signal_emit(self, hev_dbus_object_idcard_reader_signals[SIG_STATUS_CHANGED],
							0, priv->status);
			}

			if(priv->card_status)
			{
				priv->card_status = FALSE;
				g_signal_emit(self, hev_dbus_object_idcard_reader_signals[SIG_CARD_STATUS_CHANGED],
							0, priv->card_status);
			}

			manager = g_object_get_data(G_OBJECT(self), "manager");
			hev_dbus_object_idcard_reader_manager_request_remove(manager, G_OBJECT(self));
		}

		g_byte_array_unref(data);
	}
	else
	{
		HevDBusObjectIDCardReaderManager *manager = NULL;
		
		if(priv->status)
		{
			priv->status = FALSE;
			g_signal_emit(self, hev_dbus_object_idcard_reader_signals[SIG_STATUS_CHANGED],
						0, priv->status);
		}

		if(priv->card_status)
		{
			priv->card_status = FALSE;
			g_signal_emit(self, hev_dbus_object_idcard_reader_signals[SIG_CARD_STATUS_CHANGED],
						0, priv->card_status);
		}

		manager = g_object_get_data(G_OBJECT(self), "manager");
		hev_dbus_object_idcard_reader_manager_request_remove(manager, G_OBJECT(self));
	}
}

static gboolean hev_dbus_object_idcard_reader_install_get_status_timeout_handler(gpointer user_data)
{
	HevDBusObjectIDCardReader *self = NULL;
	HevDBusObjectIDCardReaderPrivate *priv = NULL;
	GByteArray *command = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(!HEV_IS_DBUS_OBJECT_IDCARD_READER(user_data))
	  return FALSE;

	self = HEV_DBUS_OBJECT_IDCARD_READER(user_data);
	priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	/* Command */
	command = g_byte_array_new();
	g_byte_array_append(command, cmd_get_status, sizeof(cmd_get_status));

	/* Get reader status */
	hev_dbus_object_idcard_reader_queue_command_async(self, command,
				hev_dbus_object_idcard_reader_get_status_async_handler,
				hev_serial_port_read_size_handler, HEV_DBUS_OBJECT_IDCARD_READER_QUEUE_COMMAND_TIMEOUT,
				hev_dbus_object_idcard_reader_queue_command_timeout_handler,
				self);

	g_byte_array_unref(command);

	return FALSE;
}

static void hev_dbus_object_idcard_reader_get_card_status_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data)
{
	HevDBusObjectIDCardReader *self = NULL;
	HevDBusObjectIDCardReaderPrivate *priv = NULL;
	GByteArray *data = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	data = hev_serial_port_queue_command_finish(HEV_SERIAL_PORT(source_object),
				res, NULL);
	if(!HEV_IS_DBUS_OBJECT_IDCARD_READER(user_data))
	{
		if(data)
		  g_byte_array_unref(data);

		return;
	}

	self = HEV_DBUS_OBJECT_IDCARD_READER(user_data);
	priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	if(data)
	{
		/* Status */
		if((9<=data->len) && (0x9F==data->data[9]))
		{
			if(!priv->card_status)
			{
				GByteArray *command = g_byte_array_new();
				g_byte_array_append(command, cmd_select_card,
							sizeof(cmd_select_card));

				/* Select card */
				hev_dbus_object_idcard_reader_queue_command_async(self, command,
							hev_dbus_object_idcard_reader_select_card_async_handler,
							hev_serial_port_read_size_handler, HEV_DBUS_OBJECT_IDCARD_READER_QUEUE_COMMAND_TIMEOUT,
							hev_dbus_object_idcard_reader_queue_command_null_timeout_handler,
							self);
				
				g_byte_array_unref(command);

				priv->card_status = TRUE;
				g_signal_emit(self, hev_dbus_object_idcard_reader_signals[SIG_CARD_STATUS_CHANGED],
							0, priv->card_status);
			}
		}
		else
		{
			if(priv->card_status)
			{
				priv->card_status = FALSE;
				g_signal_emit(self, hev_dbus_object_idcard_reader_signals[SIG_CARD_STATUS_CHANGED],
							0, priv->card_status);
			}
		}

		/* Install get card status timeout */
		g_timeout_add(HEV_DBUS_OBJECT_IDCARD_READER_GET_CARD_STATUS_DELAY,
					hev_dbus_object_idcard_reader_install_get_card_status_timeout_handler,
					self);

		g_byte_array_unref(data);
	}
}

static gboolean hev_dbus_object_idcard_reader_install_get_card_status_timeout_handler(gpointer user_data)
{
	HevDBusObjectIDCardReader *self = NULL;
	HevDBusObjectIDCardReaderPrivate *priv = NULL;
	GByteArray *command = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	if(!HEV_IS_DBUS_OBJECT_IDCARD_READER(user_data))
	  return FALSE;

	self = HEV_DBUS_OBJECT_IDCARD_READER(user_data);
	priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);
	
	/* Command */
	command = g_byte_array_new();
	g_byte_array_append(command, cmd_get_card_status, sizeof(cmd_get_card_status));

	/* Get card status */
	hev_dbus_object_idcard_reader_queue_command_async(self, command,
				hev_dbus_object_idcard_reader_get_card_status_async_handler,
				hev_serial_port_read_size_handler, HEV_DBUS_OBJECT_IDCARD_READER_QUEUE_COMMAND_TIMEOUT,
				hev_dbus_object_idcard_reader_queue_command_card_timeout_handler,
				self);

	g_byte_array_unref(command);

	return FALSE;
}

static void hev_dbus_object_idcard_reader_set_max_rf_byte_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data)
{
	HevDBusObjectIDCardReader *self = NULL;
	HevDBusObjectIDCardReaderPrivate *priv = NULL;
	GByteArray *data = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	data = hev_serial_port_queue_command_finish(HEV_SERIAL_PORT(source_object),
				res, NULL);
	if(!HEV_IS_DBUS_OBJECT_IDCARD_READER(user_data))
	{
		if(data)
		  g_byte_array_unref(data);

		return;
	}

	self = HEV_DBUS_OBJECT_IDCARD_READER(user_data);
	priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	if(data)
	{
		/* Status */
		if((9<=data->len) && (0x90==data->data[9]))
		{
			/* Install get reader status timeout */
			g_timeout_add(HEV_DBUS_OBJECT_IDCARD_READER_GET_STATUS_DELAY,
						hev_dbus_object_idcard_reader_install_get_status_timeout_handler,
						self);

			/* Install get card status timeout */
			g_timeout_add(HEV_DBUS_OBJECT_IDCARD_READER_GET_CARD_STATUS_DELAY,
						hev_dbus_object_idcard_reader_install_get_card_status_timeout_handler,
						self);
		}

		g_byte_array_unref(data);
	}
}

static void hev_dbus_object_idcard_reader_select_card_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data)
{
	HevDBusObjectIDCardReader *self = NULL;
	HevDBusObjectIDCardReaderPrivate *priv = NULL;
	GByteArray *data = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	data = hev_serial_port_queue_command_finish(HEV_SERIAL_PORT(source_object),
				res, NULL);
	if(!HEV_IS_DBUS_OBJECT_IDCARD_READER(user_data))
	{
		if(data)
		  g_byte_array_unref(data);

		return;
	}

	self = HEV_DBUS_OBJECT_IDCARD_READER(user_data);
	priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	if(data)
	{
		/* Status */
		if((9<=data->len) && (0x90==data->data[9]))
		{
			GByteArray *command = g_byte_array_new();
			g_byte_array_append(command, cmd_read_base_info,
						sizeof(cmd_read_base_info));

			/* Read base info */
			hev_dbus_object_idcard_reader_queue_command_async(self, command,
						hev_dbus_object_idcard_reader_read_base_info_async_handler,
						hev_serial_port_read_size_handler, HEV_DBUS_OBJECT_IDCARD_READER_QUEUE_COMMAND_TIMEOUT,
						hev_dbus_object_idcard_reader_queue_command_null_timeout_handler,
						self);
			
			g_byte_array_unref(command);
		}

		g_byte_array_unref(data);
	}
}

static void hev_dbus_object_idcard_reader_read_base_info_async_handler(GObject *source_object,
			GAsyncResult *res, gpointer user_data)
{
	HevDBusObjectIDCardReader *self = NULL;
	HevDBusObjectIDCardReaderPrivate *priv = NULL;
	GByteArray *data = NULL;

	g_debug("%s:%d[%s]", __FILE__, __LINE__, __FUNCTION__);

	data = hev_serial_port_queue_command_finish(HEV_SERIAL_PORT(source_object),
				res, NULL);
	if(!HEV_IS_DBUS_OBJECT_IDCARD_READER(user_data))
	{
		if(data)
		  g_byte_array_unref(data);

		return;
	}

	self = HEV_DBUS_OBJECT_IDCARD_READER(user_data);
	priv = HEV_DBUS_OBJECT_IDCARD_READER_GET_PRIVATE(self);

	if(data)
	{
		/* Status */
		if((9<=data->len) && (0x90==data->data[9]))
		{
			HevDBusObjectIDCardReaderCardInfo *info = NULL;
			GVariantBuilder *builder = NULL;
			GVariant *variant = NULL;
			gchar *value = NULL, *p = NULL;
			glong wlen = 0; gint i = 0;

			info = (HevDBusObjectIDCardReaderCardInfo *)(data->data+14);

			builder = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
			/* Name */
			value = g_utf16_to_utf8(info->name, sizeof(info->name)/2, NULL, &wlen, NULL);
			p = g_strstr_len(value, wlen, " ");
			if(p) p[0] = '\0'; /* fix */
			g_variant_builder_add(builder, "{sv}", "name", g_variant_new_string(value));
			g_free(value);
			/* Sex */
			if('1' == info->sex)
			  value = "男";
			else
			  value = "女";
			g_variant_builder_add(builder, "{sv}", "sex", g_variant_new_string(value));
			/* Nation */
			value = g_utf16_to_utf8(info->nation, sizeof(info->nation)/2, NULL, &wlen, NULL);
			i = g_ascii_strtod(value, NULL);
			g_free(value);
			if((0<=i) && (57>=i))
			  value = (gchar *)nations[i];
			else if(97 == i)
			  value = (gchar *)nations[58];
			else if(98 == i)
			  value = (gchar *)nations[59];
			else
			  value = (gchar *)nations[0];
			g_variant_builder_add(builder, "{sv}", "nation", g_variant_new_string(value));
			/* Birthday */
			value = g_utf16_to_utf8(info->birthday, sizeof(info->birthday)/2, NULL, &wlen, NULL);
			p = g_strstr_len(value, wlen, " ");
			if(p) p[0] = '\0'; /* fix */
			g_variant_builder_add(builder, "{sv}", "birthday", g_variant_new_string(value));
			g_free(value);
			/* Address */
			value = g_utf16_to_utf8(info->address, sizeof(info->address)/2, NULL, &wlen, NULL);
			p = g_strstr_len(value, wlen, " ");
			if(p) p[0] = '\0'; /* fix */
			g_variant_builder_add(builder, "{sv}", "address", g_variant_new_string(value));
			g_free(value);
			/* Serial number */
			value = g_utf16_to_utf8(info->serial_number, sizeof(info->serial_number)/2, NULL, &wlen, NULL);
			p = g_strstr_len(value, wlen, " ");
			if(p) p[0] = '\0'; /* fix */
			g_variant_builder_add(builder, "{sv}", "serial-number", g_variant_new_string(value));
			g_free(value);
			/* Grant dept */
			value = g_utf16_to_utf8(info->grant_dept, sizeof(info->grant_dept)/2, NULL, &wlen, NULL);
			p = g_strstr_len(value, wlen, " ");
			if(p) p[0] = '\0'; /* fix */
			g_variant_builder_add(builder, "{sv}", "grant-dept", g_variant_new_string(value));
			g_free(value);
			/* Begin date */
			value = g_utf16_to_utf8(info->begin_date, sizeof(info->begin_date)/2, NULL, &wlen, NULL);
			p = g_strstr_len(value, wlen, " ");
			if(p) p[0] = '\0'; /* fix */
			g_variant_builder_add(builder, "{sv}", "begin-date", g_variant_new_string(value));
			g_free(value);
			/* End date */
			value = g_utf16_to_utf8(info->end_date, sizeof(info->end_date)/2, NULL, &wlen, NULL);
			p = g_strstr_len(value, wlen, " ");
			if(p) p[0] = '\0'; /* fix */
			g_variant_builder_add(builder, "{sv}", "end-date", g_variant_new_string(value));
			g_free(value);
			variant = g_variant_builder_end(builder);
			g_variant_builder_unref(builder);

			if(priv->card_info)
			  g_variant_unref(priv->card_info);

			priv->card_info = variant;
			g_signal_emit(self, hev_dbus_object_idcard_reader_signals[SIG_CARD_INFO_CHANGED], 0);
		}

		g_byte_array_unref(data);
	}
}

