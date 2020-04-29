/* GStreamer
 * 
 */
/**
 * SECTION:element-gstatshttpserversink
 *
 * The atshttpserversink element sends pipeline data to an HTTP server
 * using HTTP PUT commands.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v \
 *     videotestsrc num-buffers=300 name=2 ! x264enc ! mpegtesmux ! atshttpserversink location=/program1 name=server \
 *     videotestsrc num-buffers=300 name=1 ! x264enc ! mpegtesmux ! server.
 * ]|
 * 
 * This example encodes 10 seconds of video and sends it to the HTTP
 * server "server" using HTTP PUT commands.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
//#include <gst/base/gstbasesink.h>
#include "gstatshttpserversink.h"
#include "EventName.h"

/* prototypes */


static void gst_ats_http_server_sink_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_ats_http_server_sink_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_ats_http_server_sink_dispose (GObject * object);
static void gst_ats_http_server_sink_finalize (GObject * object);

static void gst_ats_http_server_sink_reset (GstAtsHttpServerSink *
    atshttpsink);

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_USER_AGENT,
  PROP_AUTOMATIC_REDIRECT,
  PROP_PROXY,
  PROP_USER_ID,
  PROP_USER_PW,
  PROP_PROXY_ID,
  PROP_PROXY_PW,
  PROP_COOKIES,
  PROP_SESSION,
  PROP_ATS_LOG_LEVEL,
  PROP_RETRY_DELAY,
  PROP_RETRIES
};

GST_DEBUG_CATEGORY_STATIC (gst_plugin_template_debug);
#define GST_CAT_DEFAULT gst_plugin_template_debug

#define DEFAULT_USER_AGENT           "GStreamer atshttpserversink "
#define DEFAULT_ATS_LOG_LEVEL       ATS_LOGGER_LOG_NONE

//G_DEFINE_TYPE (GstAtsHttpServerPad, gst_ats_http_server_pad, GST_TYPE_GHOST_PAD);

/* pad templates */

static GstStaticPadTemplate gst_ats_http_server_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);


/* class initialization */

#define gst_ats_http_server_sink_parent_class parent_class
G_DEFINE_TYPE (GstAtsHttpServerSink, gst_ats_http_server_sink,GST_TYPE_ELEMENT);

static gpointer
thread_func (gpointer ptr)
{
  GstAtsHttpServerSink* atshttpsink= GST_ATS_HTTP_SERVER_SINK(ptr);

  Note("Ats main thread start");
  atshttpsink->ats_context.main_thread->execute();
  Note("Ats main thread quit");

  return NULL;
}
/*
static void
ats_pad_reset (AtsHttpServerPadData* pad_data){
  pad_data->dts = 0;
  pad_data->url = NULL;
  pad_data->mime = NULL;
}
*/


static GstFlowReturn
gst_ats_http_server_sink_chain (GstPad * pad, GstObject * parent, GstBuffer * buffer)
{
  GstFlowReturn ret;
  GstAtsHttpServerSink *server = GST_ATS_HTTP_SERVER_SINK(parent);
  
  AtsHttpServerPadData *context;
  context = gst_pad_get_element_private (pad);

  if(context->is_live > 0){
     for(int i=0;i<1024;i++){
		if(context->queue[i]){
    		g_async_queue_push(context->queue[i], gst_buffer_ref(buffer));
    		//Note("inqueue length %d",g_async_queue_length (context->queue[i]));
	 	}
  	}
  }else{
    for(int i=0;i<1024;i++){
		if(context->queue[i]){
  			//guint length = g_async_queue_length (context->queue[i]); 
			//if(length != 0){
       			for(;;){
					GstBuffer* tmp=g_async_queue_try_pop(context->queue[i]);
					if(!tmp) break;
  					gst_buffer_unref(tmp);
  					gst_buffer_unref(tmp);
				}
		//	}
		}
	}
  }
  gst_buffer_unref(buffer);
  return ret;
}

static void
free_buffer(gpointer item)
{
  GstBuffer * buffer = item;
  gst_buffer_unref(buffer);
}

#define BLOCKSIZE 2048
static void
gst_ats_http_server_loop (GstPad *pad/*GstAtsHttpServerSink * server*/)
{
   GstFlowReturn ret;
   guint64 len;
   GstFormat fmt = GST_FORMAT_BYTES;
   GstBuffer *buffer = NULL;
   AtsHttpServerPadData *context;
   context = gst_pad_get_element_private (pad);
/*
   if (!gst_pad_query_duration (pad, fmt, &len)) {
     GST_DEBUG_OBJECT (pad,"failed to query duration, pausing");
      goto stop;
   }
   if (pad->offset >= len) {
      Note("at end of input, sending EOS, pausing");
      gst_pad_push_event (pad, gst_event_new_eos ());
      goto stop;
   }
*/
   /* now, read BLOCKSIZE bytes from byte offset server->offset */
   ret = gst_pad_pull_range (pad, context->offset,BLOCKSIZE, &buffer);

   if (ret != GST_FLOW_OK) {
     GST_DEBUG_OBJECT (pad,"pull_range failed: %s", gst_flow_get_name (ret));
      goto stop;
   }

  if(context->is_live > 0){
     for(int i=0;i<1024;i++){
		if(context->queue[i]){
    		g_async_queue_push(context->queue[i], gst_buffer_ref(buffer));
    		//Note("inqueue length %d",g_async_queue_length (context->queue[i]));
	 	}
  	}
  }else{
    for(int i=0;i<1024;i++){
		if(context->queue[i]){
  			//guint length = g_async_queue_length (context->queue[i]); 
			//if(length != 0){
       			for(;;){
					GstBuffer* tmp=g_async_queue_try_pop(context->queue[i]);
					if(!tmp) break;
  					gst_buffer_unref(tmp);
  					gst_buffer_unref(tmp);
				}
		//	}
		}
	}
  }
  gst_buffer_unref(buffer);

   /* now push buffer downstream */
   //ret = gst_pad_push (pad, buf);
   //buf = NULL; /* gst_pad_push() took ownership of buffer */
   //if (ret != GST_FLOW_OK) {
   //   Note("pad_push failed: %s", gst_flow_get_name (ret));
   //   goto stop;
   //}
   /* everything is fine, increase offset and wait for us to be called again */
   context->offset += BLOCKSIZE;
   return;

stop:
   Note("pausing task");
   gst_pad_pause_task (pad);
}

static gboolean 
gst_ats_http_server_activate(GstPad * pad, GstObject * parent)
{
  GstQuery *query;
  gboolean pull_mode;

  /* first check what upstream scheduling is supported */
  query = gst_query_new_scheduling ();

  if (!gst_pad_peer_query (pad, query)) {
    gst_query_unref (query);
    goto activate_push;
  }

  /* see if pull-mode is supported */
  pull_mode = gst_query_has_scheduling_mode_with_flags (query,
      GST_PAD_MODE_PULL, GST_SCHEDULING_FLAG_SEEKABLE);
  gst_query_unref (query);

  if (!pull_mode)
    goto activate_push;

  Note("PULL mode");
  /* now we can activate in pull-mode. GStreamer will also
 *    * activate the upstream peer in pull-mode */
  return gst_pad_activate_mode (pad, GST_PAD_MODE_PULL, TRUE);

activate_push:
  {
    /* something not right, we fallback to push-mode */
    Note("PUSH mode");
    return gst_pad_activate_mode (pad, GST_PAD_MODE_PUSH, TRUE);
  }
}

static gboolean
gst_ats_http_server_activate_pull(GstPad  * pad,
                 GstObject * parent,
                 GstPadMode  mode,
                 gboolean    active)
{
  gboolean res;
  GstAtsHttpServerSink *server = GST_ATS_HTTP_SERVER_SINK(parent);

  switch (mode) {
    case GST_PAD_MODE_PUSH:
      res = TRUE;
      break;
    case GST_PAD_MODE_PULL:
      if (active) {
        //server->offset = 0;
        res = gst_pad_start_task (pad,
            (GstTaskFunction) gst_ats_http_server_loop, pad, NULL);
      } else {
        res = gst_pad_stop_task (pad);
      }
      break;
    default:
      /* unknown scheduling mode */
      res = FALSE;
      break;
  }
  return res;
}

static GstPad *     
gst_ats_http_server_request_new_pad(GstElement * element, GstPadTemplate * templ,  
    const gchar * name, const GstCaps * caps){
  GstAtsHttpServerSink *server = GST_ATS_HTTP_SERVER_SINK(element);
  GstPad *sinkpad = NULL;
  gchar *pad_name = NULL;
  gboolean do_notify = FALSE;
  //AtsHttpServerPadData* pad_data=NULL;

  g_mutex_lock (&server->mutex);                            
  pad_name = g_strdup_printf ("sink_%u", server->pad_count);    
  server->pad_count++;           
  g_mutex_unlock (&server->mutex);

  AtsHttpServerPadData *context;
  context = g_new0 (AtsHttpServerPadData, 1);
  g_snprintf(context->url,sizeof(context->url),"stream_%u",server->pad_count);
  memset(context->queue,0,sizeof(context->queue));
  context->is_live = 0;
  context->offset = 0;
  sinkpad = gst_pad_new_from_template (templ, pad_name);
  gst_pad_set_element_private (sinkpad, context);
/*  sinkpad = GST_PAD_CAST (g_object_new (GST_TYPE_ATS_HTTP_SERVER_SINK,
          "name", pad_name, "direction", templ->direction, "template", templ,
          NULL));*/
  g_free (pad_name);
	
  gst_pad_set_chain_function (sinkpad,
      GST_DEBUG_FUNCPTR (gst_ats_http_server_sink_chain));
  /*gst_pad_set_event_function (sinkpad,
      GST_DEBUG_FUNCPTR (gst_ats_http_server_sink_event));
  gst_pad_set_query_function (sinkpad,
      GST_DEBUG_FUNCPTR (gst_ats_http_server_sink_query));
  GST_OBJECT_FLAG_SET (sinkpad, GST_PAD_FLAG_PROXY_CAPS);
  GST_OBJECT_FLAG_SET (sinkpad, GST_PAD_FLAG_PROXY_ALLOCATION);*/
  gst_pad_set_activate_function(sinkpad, gst_ats_http_server_activate);
  gst_pad_set_activatemode_function (sinkpad, gst_ats_http_server_activate_pull);

  //gst_pad_set_active (sinkpad, TRUE);

  g_mutex_lock (&server->mutex);
  server->sinkpads = g_list_prepend (server->sinkpads, gst_object_ref (sinkpad));
  /*if (!server->current_sinkpad) {
    do_notify = TRUE;
    server->current_sinkpad = gst_object_ref (sinkpad);
  }*/
  g_mutex_unlock (&server->mutex);

  gst_element_add_pad (element, sinkpad);

  /*if (do_notify)
    gst_concat_notify_active_pad (server);
  */
  Note("requested pad ...\n");

  return sinkpad;
}

static void gst_ats_http_sink_detach_from_queue(_AtsHttpServerPadData* context,guint queue_index){
	context->queue[queue_index]=NULL;
}
static guint gst_ats_http_server_attach_queue_to_pad(GAsyncQueue* queue,_AtsHttpServerPadData* context){
	for(int i = 0; i<1024; i++){
		if(!context->queue[i]){
			context->queue[i] = queue;
			return i;
		}
	}
}
static GstPad* gst_ats_http_server_get_pad_from_url(GstAtsHttpServerSink* server,
												gchar* url){
	GList *it=server->sinkpads;
	AtsHttpServerPadData* context;

	if(!it) return NULL;

	for(; it; it=it->next){
		context = gst_pad_get_element_private((GstPad*)(it->data));
		if(g_strcmp0(context->url,url)==0){
			return it->data;
		}
	}

	return NULL;
}

static void
gst_ats_http_server_release_pad(GstElement * element, GstPad * pad)
{
	
}

static void                                                                
check_config_directories(void)                                             
{                                                                          
  ats_scoped_str rundir(RecConfigReadRuntimeDir());                        
  ats_scoped_str sysconfdir(RecConfigReadConfigDir());                     
                                                                           
  if (access(sysconfdir, R_OK) == -1) {                                    
    fprintf(stderr, "unable to access() config dir '%s': %d, %s\n", (const char *)sysconfdir, errno, strerror(errno));                                
    fprintf(stderr, "please set the 'TS_ROOT' environment variable\n");    
    _exit(1);
  } 
  
  if (access(rundir, R_OK | W_OK) == -1) {                                 
    fprintf(stderr, "unable to access() local state dir '%s': %d, %s\n", (const char *)rundir, errno, strerror(errno));                               
    fprintf(stderr, "please set 'proxy.config.local_state_dir'\n");
    _exit(1);
  } 
} 

static void
initialize_process_manager()
{
  mgmt_use_syslog();

  if (getenv("PROXY_REMOTE_MGMT")) {
    remote_management_flag = true;
  }

  if (remote_management_flag) {
    EnableDeathSignal(SIGTERM);
  }

  RecProcessInit(remote_management_flag ? RECM_CLIENT : RECM_STAND_ALONE, diags);
  LibRecordsConfigInit();

  pmgmt = new ProcessManager(remote_management_flag);

  pmgmt->start();
  RecProcessInitMessage(remote_management_flag ? RECM_CLIENT : RECM_STAND_ALONE);
  pmgmt->reconfigure();
  check_config_directories();

  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.short", appVersionInfo.VersionStr, RECP_NON_PERSISTENT);
  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.long", appVersionInfo.FullVersionInfoStr, RECP_NON_PERSISTENT);
  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.build_number", appVersionInfo.BldNumStr, RECP_NON_PERSISTENT);
  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.build_time", appVersionInfo.BldTimeStr, RECP_NON_PERSISTENT);
  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.build_date", appVersionInfo.BldDateStr, RECP_NON_PERSISTENT);
  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.build_machine", appVersionInfo.BldMachineStr,
                        RECP_NON_PERSISTENT);
  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.build_person", appVersionInfo.BldPersonStr,
                        RECP_NON_PERSISTENT);
}

struct NetTesterSM: public Continuation {                             
  GstAtsHttpServerSink *server;
  AtsHttpServerPadData *context;
  GAsyncQueue* queue;
  guint queue_index;
  NetVConnection *vc;
  VIO *read_vio;
  VIO *write_vio;
  IOBufferReader *reader, *resp_reader;
  MIOBuffer *req_buf, *resp_buf;
  IOBufferBlock *resp_blk;
  char request[2000];
  int req_len;
  int doc_len =28;
  const char *format  = "HTTP/1.0 200 OK\n"
					    "Content-type: application/octet-stream\n"
						"Cache-Control: no-cache\r\n\r\n";
  char response_hdr[1024];
  GstPad* sinkpad ; 

  NetTesterSM(ProxyMutex *_mutex, NetVConnection *_vc, GstAtsHttpServerSink* server) : Continuation(_mutex) {
    //Note("Accepted a connection\n");                                       
	this->server = server;
    vc = _vc;                           
    SET_HANDLER(&NetTesterSM::handle_read);
    req_buf  = new_MIOBuffer(1); 
    reader   = req_buf->alloc_reader(); 
    read_vio = vc->do_io_read(this, INT64_MAX, req_buf); 
    req_len     = 0; 

	g_snprintf(response_hdr, sizeof(response_hdr),format);
	resp_blk = new_IOBufferBlock();
	resp_blk->alloc(6);
    char *b = resp_blk->start();
 	ink_strlcpy(b, response_hdr, resp_blk->block_size());
  	//memcpy(b + strlen(response_hdr), bufdata, bufsize);
  	resp_blk->fill(doc_len = strlen(response_hdr) /*+ bufsize*/);

    resp_buf = new_empty_MIOBuffer(6); 
    resp_buf->append_block(resp_blk->clone()); 
    resp_reader = resp_buf->alloc_reader();

    //vc->set_inactivity_timeout(HRTIME_SECONDS(60)); 

  }
  
  ~NetTesterSM()
  {
	resp_blk->clear();
	resp_blk->free();

    req_buf->dealloc_all_readers();
    req_buf->clear();
    free_MIOBuffer(req_buf);

    resp_buf->dealloc_all_readers();
    resp_buf->clear();
    free_MIOBuffer(resp_buf);
	context->is_live--;
	gst_ats_http_sink_detach_from_queue(context,queue_index);
  }

  int handle_read(int event, void *data){
    int r;
    char *str;
    char buffer[32];
    const char *msg = event_int_to_string(event, sizeof(buffer), buffer);
	Note("event: %s",msg);

    switch (event) {
    case VC_EVENT_READ_READY:
      r = reader->read_avail();
      reader->read(&request[req_len], r);
      req_len += r;
      request[req_len] = 0;
      //Note("thread=0x%llx  %s\n",(unsigned long long)pthread_self(), request);
      fflush(stdout);
      if (strcmp(&request[req_len - 4], "\r\n\r\n") == 0) {
        Note( "The request header is :\n%s\n", request);
		// TODO 
		char* blank = strchr(&request[5],' ');
		*blank='\0';
		sinkpad = gst_ats_http_server_get_pad_from_url(server,&request[5]) ;
		if( !sinkpad ){
			break;
		}
		Note("Request URL %s", request+5);
  		context = gst_pad_get_element_private (sinkpad);
  	    context->is_live++;
    	queue = g_async_queue_new_full(free_buffer);
    	queue_index = gst_ats_http_server_attach_queue_to_pad(queue,context);
  		/*const char *format  = "HTTP/1.0 200 OK\n"
                             "Content-Type: video/mp2t\n"
                             "Content-Length: %d\r\n\r\n";*/
		//g_snprintf(response_hdr, sizeof(response_hdr),format, bufsize);
        SET_HANDLER(&NetTesterSM::handle_write);
        ink_assert(doc_len == resp_reader->read_avail());
        write_vio=vc->do_io_write(this, doc_len, resp_reader);
		//resp_buf->remove_append(resp_reader);
	   }
       break;
    case VC_EVENT_READ_COMPLETE:
    case VC_EVENT_EOS:
      r   = reader->read_avail();
      str = new char[r + 10];
      reader->read(str, r);
      Note("net_test,%s", str);
      fflush(stdout);
    case VC_EVENT_ERROR:
    case VC_EVENT_INACTIVITY_TIMEOUT:
      vc->do_io_close();
      break;
    default:
      ink_release_assert(!"unknown event");
    }
    return EVENT_CONT;
  }

  int handle_write(int event, Event *e){
    //Note("thread=0x%llx WRITE\n",(unsigned long long)pthread_self());
    char eventmsg[32];
    const char *msg = event_int_to_string(event, sizeof(eventmsg), eventmsg);
	GstBuffer *buffer;
	//Note("event: %s %d",msg,event);
    fflush(stdout);

    switch (event) {
		case VC_EVENT_WRITE_READY:
		  Note("WRITE_READY");
		  //TSVIOReenable(write_vio);
			//buffer = g_async_queue_try_pop (queue);
			//gst_buffer_unref(buffer);
		  //break;
		case VC_EVENT_WRITE_COMPLETE:
		  //Note("WRITE_COMPLETE");
		  {
			buffer = g_async_queue_timeout_pop (queue,800000);
    		if(!buffer) {
    		    Note("dequeue length %d",g_async_queue_length (queue));
       			//SET_HANDLER(&NetTesterSM::handle_write);
		  		//write_vio->reenable();
		  		vc->do_io_close();
		  		delete this;
		  		return EVENT_DONE;
			};
  	
   			gint bufsize = gst_buffer_get_size(buffer);
   			gchar *bufdata = NULL;
			GstMemory *memory = gst_buffer_get_all_memory(buffer);
			GstMapInfo meminfo;
   			gst_memory_map(memory,&meminfo,GST_MAP_READ);
			bufdata = meminfo.data;
   			//Note("ts stream %x\n",bufdata[0]);
  			/*const char *format  = "HTTP/1.0 200 OK\n"
                             "Content-Type: video/mp2t\n"
                             "Content-Length: %d\r\n\r\n";
				g_snprintf(response_hdr, sizeof(response_hdr),format, bufsize);*/
			resp_blk->reset();
   			char *b = resp_blk->start();
 				//ink_strlcpy(b, response_hdr, resp_blk->block_size());
            memcpy(b ,bufdata, bufsize);
  			resp_blk->fill(doc_len = bufsize);
    		resp_buf->append_block(resp_blk->clone()); 
    			//resp_reader = resp_buf->alloc_reader();
       		SET_HANDLER(&NetTesterSM::handle_write);
       		ink_assert(doc_len == resp_reader->read_avail());
       		write_vio = vc->do_io_write(this, doc_len, resp_reader);
			gst_memory_unmap(memory, &meminfo);
			gst_memory_unref(memory);
			gst_buffer_unref(buffer);
		  }
		  break;
		case VC_EVENT_EOS:
		case VC_EVENT_ERROR:
		case VC_EVENT_INACTIVITY_TIMEOUT:
		  Note("WRITE_EOS");
		  vc->do_io_close();
		  delete this;
		  return EVENT_DONE;
		  break;
		default:
		  ink_release_assert(!"unknown event");
	}
	return EVENT_CONT;
  }

};

struct NetTesterAccept : public Continuation {
  GstAtsHttpServerSink* server;
  NetTesterAccept(ProxyMutex *_mutex, GstAtsHttpServerSink* server) : Continuation(_mutex) {
	this->server = server;
	SET_HANDLER(&NetTesterAccept::handle_accept); 
  }
  int handle_accept(int event, void *data){
    Note("Accepted a connection\n");
    fflush(stdout);
    NetVConnection *vc = (NetVConnection *)data;
    new NetTesterSM(new_ProxyMutex(), vc, server);
    return EVENT_CONT;
  }
};

static void
gst_ats_http_server_sink_class_init (GstAtsHttpServerSinkClass * klass)
{
  signal(SIGPIPE, SIG_IGN);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  //GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS (klass);

  gobject_class->set_property = gst_ats_http_server_sink_set_property;
  gobject_class->get_property = gst_ats_http_server_sink_get_property;
  gobject_class->dispose = gst_ats_http_server_sink_dispose;
  gobject_class->finalize = gst_ats_http_server_sink_finalize;

  gstelement_class->request_new_pad = gst_ats_http_server_request_new_pad;
  gstelement_class->release_pad = gst_ats_http_server_release_pad;

  /*g_object_class_install_property (gobject_class, PROP_ATS_LOG_LEVEL,
      g_param_spec_enum ("http-log-level", "HTTP log level",
          "Set log level for ats's HTTP session log",
          ATS_TYPE_LOGGER_LOG_LEVEL, DEFAULT_ATS_LOG_LEVEL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));*/

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_ats_http_server_sink_template);

  gst_element_class_set_static_metadata (gstelement_class, "HTTP server sink",
      "Generic", "Sends streams to HTTP server via PUT",
      "David Schleef <ds@entropywave.com>");

  //base_sink_class->set_caps =NULL;
      //GST_DEBUG_FUNCPTR (gst_ats_http_server_sink_set_caps);

  //base_sink_class->start = GST_DEBUG_FUNCPTR (gst_ats_http_server_sink_start);
  //base_sink_class->stop = GST_DEBUG_FUNCPTR (gst_ats_http_server_sink_stop);
  //base_sink_class->unlock =
  //    GST_DEBUG_FUNCPTR (gst_ats_http_server_sink_unlock);
  //base_sink_class->event = GST_DEBUG_FUNCPTR (gst_ats_http_server_sink_event);
  //base_sink_class->render =
  //    GST_DEBUG_FUNCPTR (gst_ats_http_server_sink_render);


}

static void
gst_ats_http_server_sink_init (GstAtsHttpServerSink * atshttpsink)
{ 
  g_mutex_init (&atshttpsink->mutex);
  Layout::create();
  init_diags("ATS",NULL);
  initialize_process_manager();
  RecModeT mode_type =  RECM_STAND_ALONE;
  RecProcessInit(mode_type);
  ink_event_system_init(EVENT_SYSTEM_MODULE_VERSION);
  ink_net_init(NET_SYSTEM_MODULE_VERSION);
  eventProcessor.start(ink_number_of_processors()/8);
  RecProcessStart();
  netProcessor.start(0, 2*1024*1024);
  atshttpsink->ats_context.main_thread = new EThread;
  atshttpsink->ats_context.main_thread->set_specific();

  atshttpsink->ats_context.event_processor = &eventProcessor;
  atshttpsink->ats_context.net_processor = &netProcessor;
  atshttpsink->ats_context.shutdown_event_system= false;

  NetProcessor::AcceptOptions opt;
  opt.local_port=8080;
  opt.localhost_only=false;
  opt.accept_threads=1;
  atshttpsink->ats_context.opt =opt ;
  netProcessor.accept(new NetTesterAccept(new_ProxyMutex(),atshttpsink), opt); 
  Note("listen to opt.local_port");
  //g_cond_init (&klass->ats_context.cond);
  GError *error = NULL;
  atshttpsink->ats_context.thread=g_thread_try_new("atshttpserversinkclass-thread",
      thread_func, atshttpsink, &error);
  if (error != NULL) {
    printf("error start thread\n");
    g_error_free (error);
  } //gst_ats_http_server_sink_reset (atshttpsink);
}

static void
gst_ats_http_server_sink_reset (GstAtsHttpServerSink * atshttpsink)
{
}

void
gst_ats_http_server_sink_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAtsHttpServerSink *atshttpsink = GST_ATS_HTTP_SERVER_SINK (object);

  g_mutex_lock (&atshttpsink->mutex);
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
done:
  g_mutex_unlock (&atshttpsink->mutex);
}

void
gst_ats_http_server_sink_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstAtsHttpServerSink *atshttpsink = GST_ATS_HTTP_SERVER_SINK (object);

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_ats_http_server_sink_dispose (GObject * object)
{
  GstAtsHttpServerSink *atshttpsink = GST_ATS_HTTP_SERVER_SINK (object);

  /* clean up as possible.  may be called multiple times */
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

void
gst_ats_http_server_sink_finalize (GObject * object)
{
  GstAtsHttpServerSink *atshttpsink = GST_ATS_HTTP_SERVER_SINK (object);

  /* clean up object here */
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template plugin' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_plugin_template_debug, "atshttpserversink",
      0, "ATS http server");

  return gst_element_register (plugin, "atshttpserversink", GST_RANK_NONE,
      GST_TYPE_ATS_HTTP_SERVER_SINK);
}

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    atshttpserversink,
    "ATS http server",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://hertz.org/"
)
