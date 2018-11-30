#ifndef _GST_ATS_HTTP_SERVER_SINK_
#define _GST_ATS_HTTP_SERVER_SINK_

#include <gst/gst.h>
//#include <gst/base/gstbasesink.h>
#include "P_Net.h"
#include <netdb.h>
#include "diags.i"
#include "I_EventSystem.h"
#include "ts/I_Layout.h"
#include "RecordsConfig.h"
//#include <gst/base/gstcollectpads.h> 

G_BEGIN_DECLS
/*
GType gst_ats_http_server_pad_get_type(void);
#define GST_TYPE_ATS_HTTP_SERVER_PAD            (gst_ats_http_server_pad_get_type())
#define GST_ATS_HTTP_SERVER_PAD(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ATS_HTTP_SERVER_PAD,GstAtsHttpServerPad))
#define GST_IS_ATS_HTTP_SERVER_PAD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ATS_HTTP_SERVER_PAD)) 
#define GST_ATS_HTTP_SERVER_PAD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GST_TYPE_ATS_HTTP_SERVER_PAD,GstAtsHttpServerPadClass))
#define GST_IS_ATS_HTTP_SERVER_PAD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GST_TYPE_ATS_HTTP_SERVER_PAD))
#define GST_ATS_HTTP_SERVER_PAD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_ATS_HTTP_SERVER_PAD,GstAtsHttpServerPadClass))

typedef struct _GstAtsHttpServerPad GstAtsHttpServerPad;
typedef struct _GstAtsHttpServerPadClass GstAtsHttpServerPadClass;                          

struct _GstAtsHttpServerPad                                                             
{ 
  GstGhostPad           parent;                                                     
  char *location;//  mount point
  
  //guint                 mlineindex;                                                 
  
  //GstWebRTCRTPTransceiver *trans;                                                   
  //gulong                block_id;                                                   
  
  GstCaps              *received_caps;                                              
};

struct _GstAtsHttpServerPadClass
{
  GstGhostPadClass      parent_class;
};
*/
GType gst_ats_http_server_sink_get_type (void);
#define GST_TYPE_ATS_HTTP_SERVER_SINK 			(gst_ats_http_server_sink_get_type())
#define GST_ATS_HTTP_SERVER_SINK(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ATS_HTTP_SERVER_SINK,GstAtsHttpServerSink))
#define GST_ATS_HTTP_SERVER_SINK_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ATS_HTTP_SERVER_SINK,GstAtsHttpServerSinkClass))
#define GST_IS_ATS_HTTP_SERVER_SINK(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ATS_HTTP_SERVER_SINK))
#define GST_IS_ATS_HTTP_SERVER_SINK_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ATS_HTTP_SERVER_SINK))

typedef struct _GstAtsHttpServerSink GstAtsHttpServerSink;
typedef struct _GstAtsHttpServerSinkClass GstAtsHttpServerSinkClass;

struct _GstAtsHttpServerSink
{
  GstElement base_atshttpsink;
  //GstBaseSink base_atshttpsink;

  GMutex mutex;
  guint pad_count;
  GList *sinkpads;  /* last is earliest */
  GstPad *current_sinkpad;
  GstPad *srcpad;
  GstFormat format;

  struct _AtsContext {
    Thread *        main_thread;
	EventProcessor* event_processor;
	NetProcessor*   net_processor;
    bool            shutdown_event_system;
    NetProcessor::AcceptOptions opt;
    GThread *thread;
    GSource *timer;
    //GCond cond;
  } ats_context;
  //GstCollectPads *collect;

};

struct _GstAtsHttpServerSinkClass
{
  GstElementClass base_class;
  //GstBaseSinkClass base_class;
  // ATS context

};

typedef struct _AtsHttpServerPadData AtsHttpServerPadData;
struct _AtsHttpServerPadData{
	//GstCollectData collect;
	gchar url[1024];
    guint is_live;
	//gint64 dts;
    //GMutex mutex;
	GAsyncQueue *queue[1024]; //the number of client
    guint64  offset; 
	
/*
	video/3gpp                                       3gpp 3gp;
    video/mp2t                                       ts;
    video/mp4                                        mp4;
    video/mpeg                                       mpeg mpg;
    video/quicktime                                  mov;
    video/webm                                       webm;
    video/x-flv                                      flv;
    video/x-m4v                                      m4v;
    video/x-mng                                      mng;
    video/x-ms-asf                                   asx asf;
    video/x-ms-wmv                                   wmv;
    video/x-msvideo                                  avi;

		
*/
	gchar *mime;
};

G_END_DECLS
#endif
