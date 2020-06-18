#include <stdio.h>
#include "vrsmsz.h"
/*
static gboolean  vrsmsz_remove()
{
   vrchan_t* vc = vrsmsz->remove_chan;
   vrstream_t* vs = &vc->vs;
   gst_element_set_state(vs->bin,GST_STATE_NULL); // sink will not deadlock

   gst_bin_remove_many(GST_BIN (vrsmsz->pipeline),vs->bin,NULL);

   vs->uridecodebin=NULL,vs->vdec_tee=NULL,vs->vdec_tee_queue=NULL,vs->video_scale=NULL,vs->video_capsfilter=NULL, vs->video_encoder=NULL,vs->audio_convert=NULL,vs->audio_encoder=NULL,vs->aenc_tee=NULL,vs->aenc_tee_queue=NULL,vs->muxer=NULL,vs->outer=NULL;
   vrsmsz->stream_nbs--;
   vc->video_id = -1;
   vc->audio_id = -1;
   memset(vc->in_url,0,sizeof(vc->in_url));
   vc->resolution = -1;
   g_print("remove bin successful!!!\n");
   // recover 
   gst_element_set_state (vrsmsz->pipeline, GST_STATE_READY);
   gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
    
   gst_debug_bin_to_dot_file (GST_BIN_CAST(vrsmsz->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");
   return FALSE;
}
*/
void vrsmsz_set_play(GstElement* bin){
  GstStateChangeReturn r;
  r = gst_element_set_state (bin, GST_STATE_PAUSED);
  switch (r) {
    case GST_STATE_CHANGE_NO_PREROLL:
      /* live source, timestamps are running_time of the pipeline clock. */
	    g_print("this bin is live stream\n");
      break;
    case GST_STATE_CHANGE_SUCCESS:
      /* success, no async state changes, same as async, timestamps start
       * from 0 */
    case GST_STATE_CHANGE_ASYNC:
      /* no live source, bin will preroll. We have to punch it in because in
       * this situation timestamps start from 0.  */
	    g_print("this bin is not live stream\n");
      break;
    case GST_STATE_CHANGE_FAILURE:
      /* fall through to return */
    default:
	    g_print("this bin status change is failed\n");
  }
  r = gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
  switch (r) {
    case GST_STATE_CHANGE_NO_PREROLL:
      /* live source, timestamps are running_time of the pipeline clock. */
	    g_print("this bin is live stream\n");
      break;
    case GST_STATE_CHANGE_SUCCESS:
      /* success, no async state changes, same as async, timestamps start
       * from 0 */
    case GST_STATE_CHANGE_ASYNC:
      /* no live source, bin will preroll. We have to punch it in because in
       * this situation timestamps start from 0.  */
	    g_print("this bin is not live stream\n");
      break;
    case GST_STATE_CHANGE_FAILURE:
      /* fall through to return */
    default:
	    g_print("this bin status change is failed\n");
  }
}

static gboolean
message_cb (GstBus * bus, GstMessage * message, gpointer user_data)
{
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *name, *debug = NULL;

      name = gst_object_get_path_string (message->src);
      gst_message_parse_error (message, &err, &debug);

      g_printerr ("ERROR: from element %s: %s\n", name, err->message);
      if (debug != NULL)
        g_printerr ("Additional debug info:\n%s\n", debug);

      g_error_free (err);
      g_free (debug);
      g_free (name);

      //g_main_loop_quit (loop);
      break;
    }
    case GST_MESSAGE_WARNING:{
      GError *err = NULL;
      gchar *name, *debug = NULL;

      name = gst_object_get_path_string (message->src);
      gst_message_parse_warning (message, &err, &debug);

      g_printerr ("ERROR: from element %s: %s\n", name, err->message);
      if (debug != NULL)
        g_printerr ("Additional debug info:\n%s\n", debug);

      g_error_free (err);
      g_free (debug);
      g_free (name);
      break;
    }
    case GST_MESSAGE_EOS:
      g_print ("======> Got EOS\n");
      //vrsmsz_remove();
      //g_main_loop_quit (loop);
      break;
    default:
      g_print("==> %s\n",gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
      break;
  }

  return TRUE;
}

vrsmsz_t* vrsmsz;
static void enable_factory (const gchar *name, gboolean enable) {
    GstRegistry *registry = NULL;
    GstElementFactory *factory = NULL;

    registry = gst_registry_get();
    if (!registry) return;

    factory = gst_element_factory_find (name);
    if (!factory) return;

    if (enable) {
        gst_plugin_feature_set_rank (GST_PLUGIN_FEATURE (factory), GST_RANK_PRIMARY + 1);
    }
    else {
        gst_plugin_feature_set_rank (GST_PLUGIN_FEATURE (factory), GST_RANK_NONE);
    }

    gst_registry_add_feature (registry, GST_PLUGIN_FEATURE (factory));
    return;
}
#if 0
static GstElement* create_stream (const gchar * descr)
{
  GstElement *bin;
  GError *error = NULL;

  bin = gst_parse_launch (descr, &error);
  if (error) {
    g_print ("pipeline could not be constructed: %s\n", error->message);
    g_error_free (error);
    return NULL;
  }

  /* add the bin to the pipeline now, this will set the current base_time of the
   * pipeline on the new bin. */
  gst_bin_add (GST_BIN_CAST (vrsmsz->pipeline), bin);
  return bin;
}
#endif
static void
_pad_added_cb (GstElement * decodebin, GstPad * new_pad, gpointer data)
{
  GstPad *sink_pad=NULL ;
  GstPadLinkReturn ret;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;
  vrchan_t* vc = data;
  vrstream_t* vs = &(vc->vs);

   //g_printf("==>stream %d(%x) info: video_id %d, \naudio_id %d,\nsrc_url %s,\n pre_url %s,\ndis= %d,\nuridecodebin %x,\nvdec_tee %x,\nvdec_tee_queue %x,\nvideo_capsfilter %x,\nvideo_encoder %x,\naudio_encoder %x\n",vrsmsz->stream_nbs-1, vrsmsz->streams+(vrsmsz->stream_nbs-1),vs->video_id, vs->audio_id, vs->src_url, vs->pre_sink_url, vs->dis, vs->uridecodebin,vs->vdec_tee, vs->vdec_tee_queue,vs->video_capsfilter, vs->video_encoder, vs->audio_encoder);
  g_print ("url= %s Received new pad '%s' from '%s':\n",vc->preview_url,GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(decodebin));
 
  /* If our converter is already linked, we have nothing to do here */
  /*if (gst_pad_is_linked (sink_pad)) {
    g_print ("We are already linked. Ignoring.\n");
  }*/
  /* Check the new pad's type */
  new_pad_caps = gst_pad_get_current_caps (new_pad);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_struct);
  if (g_str_has_prefix (new_pad_type, "audio/x-raw")) {
    g_print ("It has type '%s' which is raw audio. \n", new_pad_type);
    sink_pad = gst_element_get_static_pad (GST_ELEMENT (vs->audio_convert), "sink");
    ret = gst_pad_link (new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("Type is '%s' but link failed.\n", new_pad_type);
    } else {
      g_print ("Link succeeded (type '%s').\n", new_pad_type);
    }
  }else if(g_str_has_prefix (new_pad_type, "video/x-raw")){
    g_print ("It has type '%s' which is raw video. \n", new_pad_type);
    sink_pad = gst_element_get_static_pad (GST_ELEMENT (vs->vdec_tee), "sink");
    ret = gst_pad_link (new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("Type is '%s' but link failed.\n", new_pad_type);
    } else {
      g_print ("Link succeeded (type '%s').\n", new_pad_type);
    }
  }else{
    g_print ("It has type '%s' which is not supported. \n", new_pad_type);
  }
  GstStateChangeReturn r;
  r = gst_element_set_state (vrsmsz->pipeline, GST_STATE_PAUSED);
  switch (r) {
    case GST_STATE_CHANGE_NO_PREROLL:
      /* live source, timestamps are running_time of the pipeline clock. */
	    g_print("this is live stream\n");
      break;
    case GST_STATE_CHANGE_SUCCESS:
      /* success, no async state changes, same as async, timestamps start
       * from 0 */
    case GST_STATE_CHANGE_ASYNC:
      /* no live source, bin will preroll. We have to punch it in because in
       * this situation timestamps start from 0.  */
	    g_print("this is not live stream\n");
      break;
    case GST_STATE_CHANGE_FAILURE:
      /* fall through to return */
    default:
      return;
  }
  r = gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);

  gst_debug_bin_to_dot_file (GST_BIN_CAST(vrsmsz->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");
}
/*
 *
 * uridecodebin uri=rtmp://192.168.0.134/live/ch1 name=src ! tee name=t1 ! queue ! videoscale ! video/x-raw, width=320, height=240 ! x264enc ! flvmux name=muxer ! rtmpsink location=rtmp://192.168.0.134/live/0 src. ! audioconvert ! voaacenc ! tee ! queue ! muxer.
 *
*/
gboolean vrsmsz_add_stream(gpointer data){
   gchar name[1024];
   gchar* uri = data;
   vrstream_t* vs = NULL;
   vrchan_t* vc=NULL;

   if(vrsmsz->stream_nbs > 16) {
     g_print("sorry streams is full\n");
     return FALSE;
   }
   /**********************dispatch stream id ***********************/
   g_print("start pull stream uri= %s\n",uri);
   for(int i=0; i<MAX_CHANNEL; i++){
       if(vrsmsz->streams_id[i] == -1){
          vrsmsz->streams_id[i] = i;
          vc = vrsmsz->streams + vrsmsz->streams_id[i];
          vc->stream_id = vrsmsz->streams_id[i];
	  break;
	}
   }
   if(!vc) return FALSE;
   vc->video_id = vc->stream_id;
   vc->audio_id = vc->stream_id;
   sprintf(vc->in_url, "%s", uri);
   vc->resolution = 2;

   g_print("Stream_id is %d\n", vc->stream_id);
   sprintf(name,"chan-%d-bin",vc->stream_id);

   vs = &vc->vs;
   vs->bin = gst_bin_new(name);

   if(!vs->uridecodebin){
     sprintf(name,"vs%d-uridecodebin",vc->video_id);
     vs->uridecodebin = gst_element_factory_make("uridecodebin", name);
     g_object_set (vs->uridecodebin,"uri", uri, "expose-all-streams", TRUE, NULL);
     g_signal_connect (vs->uridecodebin, "pad-added", (GCallback) _pad_added_cb, vc);

   }

   if(!vs->vdec_tee){
     sprintf(name,"vs%d-vdec_tee",vc->video_id);
    vs->vdec_tee = gst_element_factory_make("tee",name);
    if(!vs->vdec_tee){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->vdec_tee_queue){
     sprintf(name,"vs%d-vdec_tee_queue",vc->video_id);
    vs->vdec_tee_queue = gst_element_factory_make("queue",name);
    if(!vs->vdec_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->video_scale){
     sprintf(name,"vs%d-video_scale",vc->video_id);
    vs->video_scale = gst_element_factory_make("videoscale",name);
    if(!vs->video_scale){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->video_capsfilter){
     sprintf(name,"vs%d-video_capsfilter",vc->video_id);
     vs->video_capsfilter = gst_element_factory_make("capsfilter", name);
    if(!vs->video_capsfilter){
      g_print("error make\n");
      return FALSE;
    }
     gst_util_set_object_arg (G_OBJECT (vs->video_capsfilter), "caps",
      "video/x-raw, width=320, height=240");
   }
   if(!vs->video_encoder){
     sprintf(name,"vs%d-video_encoder",vc->video_id);
     vs->video_encoder = gst_element_factory_make("x264enc", name);
    if(!vs->video_encoder){
      g_print("error make\n");
      return FALSE;
    }
    g_object_set (vs->video_encoder, "byte-stream", TRUE, "key-int-max", 25, NULL);
   }
   if(!vs->audio_convert){
     sprintf(name,"vs%d-audio_convert",vc->audio_id);
     vs->audio_convert = gst_element_factory_make("audioconvert", name);
    if(!vs->audio_convert){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->audio_encoder){
     sprintf(name,"vs%d-audio_encoder",vc->audio_id);
     vs->audio_encoder = gst_element_factory_make("voaacenc", name);
    if(!vs->audio_encoder){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->aenc_tee){
     sprintf(name,"vs%d-aenc_tee",vc->audio_id);
     vs->aenc_tee = gst_element_factory_make("tee", name);
     if(!vs->aenc_tee){
       g_print("error make\n");
      return FALSE;
     }
   }
   if(!vs->aenc_tee_queue){
     sprintf(name,"vs%d-aenc_tee_queue",vc->audio_id);
     vs->aenc_tee_queue = gst_element_factory_make("queue", name);
     if(!vs->aenc_tee_queue){
       g_print("error make\n");
       return FALSE;
     }
   }
   if(!vs->muxer){
     sprintf(name,"vs%d-muxer",vc->video_id);
     vs->muxer = gst_element_factory_make("flvmux", name);
     if(!vs->muxer){
       g_print("error make\n");
       return FALSE;
     }
   }
   if(!vs->outer){
     sprintf(name,"vs%d-outer",vc->video_id);
     vs->outer= gst_element_factory_make("rtmpsink", name);
     if(!vs->outer){
       g_print("error make\n");
       return FALSE;
     }
     g_print("preview put uri = %s\n",vc->preview_url);
     g_object_set (vs->outer, "location", vc->preview_url, NULL);
   }

   gst_bin_add_many(GST_BIN(vs->bin), vs->uridecodebin, vs->vdec_tee, vs->vdec_tee_queue,vs->video_scale, vs->video_capsfilter, vs->video_encoder, vs->audio_convert, vs->audio_encoder, vs->aenc_tee, vs->aenc_tee_queue, vs->muxer,vs->outer,NULL);
   gst_bin_add(GST_BIN(vrsmsz->pipeline),vs->bin);

   if(!gst_element_link_many(vs->vdec_tee, vs->vdec_tee_queue, vs->video_scale, vs->video_capsfilter, vs->video_encoder, vs->muxer,vs->outer,NULL)){
      g_print("push link failed\n");
      return FALSE;
   }

   if(!gst_element_link_many(vs->audio_convert,vs->audio_encoder,vs->aenc_tee, vs->aenc_tee_queue, vs->muxer,NULL)){
      g_print("push link failed\n");
      return FALSE;
   }

   (vrsmsz->stream_nbs)++;
   vrsmsz_play();
   g_free(uri);

#if 1
   GstClockTime now ;
   now = gst_clock_get_time (vrsmsz->theclock);

   gst_element_set_base_time (vs->uridecodebin, now);
   //gst_element_set_base_time (vs->bin, now);
   g_print("\n++++++++++++++ base time %"GST_TIME_FORMAT"\n\n",GST_TIME_ARGS(now / GST_MSECOND));
#endif

   //g_print("==>stream %d(%x) info: video_id %d, \naudio_id %d,\nsrc_url %s,\n pre_url %s,\ndis= %d,\nuridecodebin %x,\nvdec_tee %x,\nvdec_tee_queue %x,\nvideo_capsfilter %x,\nvideo_encoder %x,\naudio_encoder %x\n",vrsmsz->stream_nbs-1, vrsmsz->streams+(vrsmsz->stream_nbs-1),vs->video_id, vs->audio_id, vs->src_url, vs->pre_sink_url, vs->dis, vs->uridecodebin,vs->vdec_tee, vs->vdec_tee_queue,vs->video_capsfilter, vs->video_encoder, vs->audio_encoder);
  gst_element_set_state (vs->bin, GST_STATE_PLAYING);
  gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
   return FALSE;
}

/*******************************************************************************/
gboolean vrsmsz_remove_stream(gpointer data){
   gint streamid = atoi((gchar*)data);
   vrchan_t* vc = NULL;
   vrstream_t* vs = NULL;

   g_free(data);
   if(!vrsmsz->stream_nbs) {
      return FALSE;
   }
   for(int i=0; i<MAX_CHANNEL; i++){
       if(vrsmsz->streams_id[i] == streamid){
          vc = vrsmsz->streams + vrsmsz->streams_id[i];
	  break;
	}
   }
   if(!vc){ 
      g_print("streams not found\n");
      return FALSE;
   }
   g_print("streamd_id %d director_id %d\n", vc->stream_id,vrsmsz->director.stream_id);
   if(vc->stream_id == vrsmsz->director.stream_id && vrsmsz->stream_nbs != 1){
      g_print("this stream is live playing, cant be removed\n");
      return FALSE;
   }
   if(streamid == vrsmsz->director.stream_id){
     g_print("remove preview and pub");
   }
   g_print("remove stream = %s\n",vc->in_url);
#if 0
   //GstEvent*  event = gst_event_new_eos();
   //vrsmsz->remove_chan = vc;
   //GstElement* source = NULL;
   //g_object_get(vs->uridecodebin,"source",&source,NULL);
   //if(!gst_element_send_event(source, event)){
   //if(!gst_element_send_event(vs->uridecodebin, event)){
   //if(!gst_element_send_event(vrsmsz->pipeline, event)){
   //  g_print("send events failed\n");
   //}
     g_print("send events\n");
   //vrsmsz_remove();
#endif

   vs = &vc->vs;
   gst_element_set_state(vs->bin,GST_STATE_NULL); // sink will not deadlock
   //gst_object_unref(vs->uridecodebin);
   //gst_object_unref(vs->vdec_tee);
   //gst_object_unref(vs->vdec_tee_queue);
   //gst_object_unref(vs->video_scale);
   //gst_object_unref(vs->video_capsfilter);
   //gst_object_unref(vs->video_encoder);
   //gst_object_unref(vs->audio_convert);
   //gst_object_unref(vs->audio_encoder);
   //gst_object_unref(vs->aenc_tee);
   //gst_object_unref(vs->aenc_tee_queue);
   //gst_object_unref(vs->muxer);
   //gst_object_unref(vs->outer);
   gst_bin_remove_many(GST_BIN (vrsmsz->pipeline),vs->bin,NULL);
   vs->uridecodebin=NULL,vs->vdec_tee=NULL,vs->vdec_tee_queue=NULL,vs->video_scale=NULL,vs->video_capsfilter=NULL, vs->video_encoder=NULL,vs->audio_convert=NULL,vs->audio_encoder=NULL,vs->aenc_tee=NULL,vs->aenc_tee_queue=NULL,vs->muxer=NULL,vs->outer=NULL;
   vrsmsz->stream_nbs--;
   vc->tracks = 0;
   vc->video_id = -1;
   vc->audio_id = -1;
   memset(vc->in_url,0,sizeof(vc->in_url));
   //memset(vc->preview_url,0,sizeof(vc->preview_url));
   vc->resolution = -1;
   vrsmsz->streams_id[vc->stream_id] = -1;
   vc->stream_id = -1;
   g_print("remove bin successful!!!\n");

   gst_debug_bin_to_dot_file (GST_BIN_CAST(vrsmsz->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");

   return FALSE;
}


/*****************************************************************************************************/
gboolean director_publish_create(gpointer uri){
  gchar name[1024];
  if(!vrsmsz->director.publish_url[0]){
    //g_print("publish url = NULL\n");
    gchar pub_url[1024]={"rtmp://192.168.0.134/live/pub"};
    g_print("use default url %s\n",pub_url);
    memcpy(vrsmsz->director.publish_url,pub_url,strlen(pub_url)+1);
  }else{
    g_print("\n");
  }

  if(!vrsmsz->director.ds.pub_vdec_tee_queue){
     sprintf(name,"%s-pub_vdec_tee_queue","vrsmsz");
    vrsmsz->director.ds.pub_vdec_tee_queue= gst_element_factory_make("queue", name);
    if(!vrsmsz->director.ds.pub_vdec_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
  }

  if(!vrsmsz->director.ds.pub_video_encoder){
     sprintf(name,"%s-pub_video_encoder","vrsmsz");
    vrsmsz->director.ds.pub_video_encoder= gst_element_factory_make("x264enc", name); // nvh264enc have no avc
    if(!vrsmsz->director.ds.pub_video_encoder){
      g_print("error make\n");
      return FALSE;
    }
    g_object_set (vrsmsz->director.ds.pub_video_encoder, "byte-stream", TRUE, "key-int-max", 25, NULL);
  }
  if(!vrsmsz->director.ds.pub_aenc_tee_queue){
     sprintf(name,"%s-pub_aenc_tee_queue","vrsmsz");
    vrsmsz->director.ds.pub_aenc_tee_queue= gst_element_factory_make("queue", name);
    if(!vrsmsz->director.ds.pub_aenc_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->director.ds.pub_muxer){
     sprintf(name,"%s-pub_muxer","vrsmsz");
    vrsmsz->director.ds.pub_muxer= gst_element_factory_make("flvmux", name);
    if(!vrsmsz->director.ds.pub_muxer){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->director.ds.pub_outer){
     sprintf(name,"%s-pub_outer","vrsmsz");
    vrsmsz->director.ds.pub_outer= gst_element_factory_make("rtmpsink", name);
    if(!vrsmsz->director.ds.pub_outer){
      g_print("error make\n");
      return FALSE;
    }
    g_object_set (vrsmsz->director.ds.pub_outer, "location", vrsmsz->director.publish_url, NULL);
  }

  GstCaps *caps = gst_caps_from_string("audio/mpeg");
  gst_bin_add_many(GST_BIN(vrsmsz->director.pub_bin), vrsmsz->director.ds.pub_vdec_tee_queue,vrsmsz->director.ds.pub_video_encoder,vrsmsz->director.ds.pub_aenc_tee_queue,vrsmsz->director.ds.pub_muxer,vrsmsz->director.ds.pub_outer, NULL);

  if(!gst_element_link_many(vrsmsz->director.ds.pub_vdec_tee_queue, vrsmsz->director.ds.pub_video_encoder, vrsmsz->director.ds.pub_muxer,vrsmsz->director.ds.pub_outer, NULL)){
    g_print("link director pub video failed");
    return FALSE;
  }

  if(!gst_element_link_filtered(vrsmsz->director.ds.pub_aenc_tee_queue, vrsmsz->director.ds.pub_muxer, caps)){
    g_print("link director pub audio failed");
    return FALSE;
  }
 
  return FALSE;
}

/*****************************************************************************************************/
/*****************************************************************************************************/
void director_preview_link_vs(vrstream_t* vs){
  GstPadLinkReturn ret;
  // audio 
  if(!vs->pre_aenc_tee_srcpad){
      vs->pre_aenc_tee_srcpad = gst_element_get_request_pad (vs->aenc_tee, "src_%u");
      if(!vs->pre_aenc_tee_srcpad)
	g_print ("pre aenc failed. \n");
  }
  if(!vrsmsz->director.ds.pre_aenc_tee_queue_sinkpad){
      vrsmsz->director.ds.pre_aenc_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->director.ds.pre_aenc_tee_queue, "sink");
      if(!vrsmsz->director.ds.pre_aenc_tee_queue_sinkpad)
	g_print ("pre aenc tee queue failed. \n");
  }
  if(!vs->pre_aenc_tee_ghost_srcpad){
     vs->pre_aenc_tee_ghost_srcpad = gst_ghost_pad_new ("pre-aenc-tee-srcpad", vs->pre_aenc_tee_srcpad);
     gst_element_add_pad (vs->bin, vs->pre_aenc_tee_ghost_srcpad);
  }

  if(!vrsmsz->director.ds.pre_aenc_tee_queue_ghost_sinkpad){
    vrsmsz->director.ds.pre_aenc_tee_queue_ghost_sinkpad = gst_ghost_pad_new ("pre-aenc-tee-queue-sinkpad", vrsmsz->director.ds.pre_aenc_tee_queue_sinkpad);
    gst_element_add_pad (vrsmsz->director.pre_bin,vrsmsz->director.ds.pre_aenc_tee_queue_ghost_sinkpad);
  }
  ret = gst_pad_link (vs->pre_aenc_tee_ghost_srcpad,vrsmsz->director.ds.pre_aenc_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
     g_print ("pre aenc tee Link failed.\n");
  } else {
     g_print ("pre Link succeeded .\n");
  }
  // video
  if(!vs->pre_vdec_tee_srcpad)
    vs->pre_vdec_tee_srcpad = gst_element_get_request_pad (vs->vdec_tee, "src_%u");
  if(!vrsmsz->director.ds.pre_vdec_tee_queue_sinkpad)
    vrsmsz->director.ds.pre_vdec_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->director.ds.pre_vdec_tee_queue, "sink");

  if(!vrsmsz->director.ds.pre_vdec_tee_queue_ghost_sinkpad){
    vrsmsz->director.ds.pre_vdec_tee_queue_ghost_sinkpad = gst_ghost_pad_new ("pre-vdec-tee-queue-sinkpad", vrsmsz->director.ds.pre_vdec_tee_queue_sinkpad);
    gst_element_add_pad (vrsmsz->director.pre_bin,vrsmsz->director.ds.pre_vdec_tee_queue_ghost_sinkpad );
  }

  vs->pre_vdec_tee_ghost_srcpad = gst_ghost_pad_new ("pre-vdec-tee-srcpad", vs->pre_vdec_tee_srcpad );
  gst_element_add_pad (vs->bin, vs->pre_vdec_tee_ghost_srcpad);
  ret = gst_pad_link(vs->pre_vdec_tee_ghost_srcpad, vrsmsz->director.ds.pre_vdec_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
    g_print ("pre vdec tee Link failed.\n");
  } else {
    g_print ("pre Link succeeded .\n");
  }

  vrsmsz_set_play(vrsmsz->director.pre_bin); // this must be used
  vrsmsz_play();
  gst_debug_bin_to_dot_file (GST_BIN_CAST(vrsmsz->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");
}
/*****************************************************************************************************/
gboolean director_preview_create(vrstream_t* vs){
  gchar name[1024];
  g_print("%s \n",vrsmsz->director.preview_url);

  if(!vrsmsz->director.ds.pre_vdec_tee_queue){
     sprintf(name,"%s-pre_vdec_tee_queue","vrsmsz");
     vrsmsz->director.ds.pre_vdec_tee_queue= gst_element_factory_make("queue", name);
     if(!vrsmsz->director.ds.pre_vdec_tee_queue){
       g_print("error make\n");
       return FALSE;
     }
  }
  if(!vrsmsz->director.ds.pre_video_scale){
     sprintf(name,"%s-pre_video_scale","vrsmsz");
    vrsmsz->director.ds.pre_video_scale= gst_element_factory_make("videoscale", name);
    if(!vrsmsz->director.ds.pre_video_scale){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->director.ds.pre_capsfilter){
     sprintf(name,"%s-pre_capsfilter","vrsmsz");
    vrsmsz->director.ds.pre_capsfilter= gst_element_factory_make("capsfilter", name);
    if(!vrsmsz->director.ds.pre_capsfilter){
      g_print("error make\n");
      return FALSE;
    }
     gst_util_set_object_arg (G_OBJECT (vrsmsz->director.ds.pre_capsfilter), "caps",
      "video/x-raw, width=320, height=240");
  }
  if(!vrsmsz->director.ds.pre_video_encoder){
     sprintf(name,"%s-pre_video_encoder","vrsmsz");
    vrsmsz->director.ds.pre_video_encoder= gst_element_factory_make("x264enc", name);
    if(!vrsmsz->director.ds.pre_video_encoder){
      g_print("error make\n");
      return FALSE;
    }
    g_object_set (vrsmsz->director.ds.pre_video_encoder, "byte-stream", TRUE, "key-int-max", 25, NULL);
  }
  if(!vrsmsz->director.ds.pre_aenc_tee_queue){
     sprintf(name,"%s-pre_aenc_tee_queue","vrsmsz");
    vrsmsz->director.ds.pre_aenc_tee_queue= gst_element_factory_make("queue", name);
    if(!vrsmsz->director.ds.pre_aenc_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->director.ds.pre_muxer){
     sprintf(name,"%s-pre_muxer","vrsmsz");
    vrsmsz->director.ds.pre_muxer= gst_element_factory_make("flvmux", name);
    if(!vrsmsz->director.ds.pre_muxer){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->director.ds.pre_outer){
     sprintf(name,"%s-pre_outer","vrsmsz");
    vrsmsz->director.ds.pre_outer= gst_element_factory_make("rtmpsink", name);
    if(!vrsmsz->director.ds.pre_outer){
      g_print("error make\n");
      return FALSE;
    }
    g_object_set (vrsmsz->director.ds.pre_outer, "location", vrsmsz->director.preview_url, NULL);
  }
   vrsmsz->director.pre_bin = gst_bin_new("pre_bin");
  gst_bin_add_many(GST_BIN(vrsmsz->director.pre_bin), vrsmsz->director.ds.pre_vdec_tee_queue, vrsmsz->director.ds.pre_video_scale, vrsmsz->director.ds.pre_capsfilter, vrsmsz->director.ds.pre_video_encoder, vrsmsz->director.ds.pre_aenc_tee_queue, vrsmsz->director.ds.pre_muxer, vrsmsz->director.ds.pre_outer, NULL);
  gst_bin_add(GST_BIN(vrsmsz->pipeline),vrsmsz->director.pre_bin);

  if(!gst_element_link_many(vrsmsz->director.ds.pre_vdec_tee_queue, vrsmsz->director.ds.pre_video_scale, vrsmsz->director.ds.pre_capsfilter, vrsmsz->director.ds.pre_video_encoder, vrsmsz->director.ds.pre_muxer, vrsmsz->director.ds.pre_outer, NULL)){
     g_print("link director pre viedo failed");
     return FALSE;
  }
  GstCaps *caps = gst_caps_from_string("audio/mpeg");
  if(!gst_element_link_filtered(vrsmsz->director.ds.pre_aenc_tee_queue, vrsmsz->director.ds.pre_muxer, caps)){
     g_print("link director pre audio failed");
     return FALSE;
  }

  return FALSE;
}

gboolean director_switch_effect_create(vrstream_t* vs){
  gchar name[1024];
  GstPadLinkReturn ret;
  if(!vrsmsz->director.ds.video_filter_queue){
      sprintf(name,"%s-video_filter_queue","vrsmsz");
      vrsmsz->director.ds.video_filter_queue = gst_element_factory_make("queue", name);
      if(!vrsmsz->director.ds.video_filter_queue){
        g_print("error make\n");
        return FALSE;
      }
    }
    if(!vrsmsz->director.ds.videoconvert){
      sprintf(name,"%s-videoconvert","vrsmsz");
      vrsmsz->director.ds.videoconvert= gst_element_factory_make("videoconvert", name);
      if(!vrsmsz->director.ds.videoconvert){
        g_print("error make\n");
        return FALSE;
      }
    }
    if(!vrsmsz->director.ds.video_filter_chain){
      sprintf(name,"%s-video_filter_chain","vrsmsz");
      vrsmsz->director.ds.video_filter_chain= gst_element_factory_make("compositor", name);
      if(!vrsmsz->director.ds.video_filter_chain){
        g_print("error make\n");
        return FALSE;
      }
    }
    if(!vrsmsz->director.ds.video_filter_tee){
      sprintf(name,"%s-video_filter_tee","vrsmsz");
      vrsmsz->director.ds.video_filter_tee= gst_element_factory_make("tee", name);
      if(!vrsmsz->director.ds.video_filter_tee){
        g_print("error make\n");
        return FALSE;
      }
    }
    gst_bin_add_many(GST_BIN(vrsmsz->director.swt_bin),vrsmsz->director.ds.video_filter_queue,vrsmsz->director.ds.videoconvert,vrsmsz->director.ds.video_filter_chain,vrsmsz->director.ds.video_filter_tee,NULL);

    if(!gst_element_link_many(vrsmsz->director.ds.pre_vdec_tee_queue, vrsmsz->director.ds.pre_video_scale, vrsmsz->director.ds.pre_capsfilter, vrsmsz->director.ds.pre_video_encoder, vrsmsz->director.ds.pre_muxer, vrsmsz->director.ds.pre_outer, NULL)){
     g_print("link director pre viedo failed");
     return FALSE;
    }
    
    GstCaps *caps = gst_caps_from_string("audio/mpeg");
    if(!gst_element_link_filtered(vrsmsz->director.ds.pre_aenc_tee_queue, vrsmsz->director.ds.pre_muxer, caps)){
     g_print("link director pre audio failed");
     return FALSE;
    }
    
    if(!vs->pre_aenc_tee_srcpad){
      vs->pre_aenc_tee_srcpad = gst_element_get_request_pad (vs->aenc_tee, "src_%u");
    }
    if(!vrsmsz->director.ds.pre_aenc_tee_queue_sinkpad)
      vrsmsz->director.ds.pre_aenc_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->director.ds.pre_aenc_tee_queue, "sink");
    ret = gst_pad_link (vs->pre_aenc_tee_srcpad, vrsmsz->director.ds.pre_aenc_tee_queue_sinkpad);
    if (GST_PAD_LINK_FAILED (ret)) {
       g_print ("Link failed.\n");
    } else {
       g_print ("Link succeeded .\n");
    }

    if(!vs->pre_vdec_tee_srcpad)
      vs->pre_vdec_tee_srcpad = gst_element_get_request_pad (vs->vdec_tee, "src_%u");
    if(!vrsmsz->director.ds.pre_vdec_tee_queue_sinkpad)
      vrsmsz->director.ds.pre_vdec_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->director.ds.pre_vdec_tee_queue, "sink");
    ret = gst_pad_link (vs->pre_vdec_tee_srcpad,vrsmsz->director.ds.pre_vdec_tee_queue_sinkpad);
    if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("Link failed.\n");
    } else {
      g_print ("Link succeeded .\n");
    }

    return FALSE;
    /*
    if(!vrsmsz->){
      sprintf(name,"%s-","vrsmsz");
      vrsmsz->= gst_element_factory_make("", name);
      if(!vrsmsz->){
        g_print("error make\n");
        return FALSE;
      }
    }
    */
}

gboolean director_preview_unlink_vs(vrstream_t* vr){
  GstPadLinkReturn ret;
  ret = gst_pad_unlink(vr->pre_aenc_tee_ghost_srcpad, vrsmsz->director.ds.pre_aenc_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
     g_print ("unLink aenc tee ghost failed.\n");
  } else {
     g_print ("unLink succeeded .\n");
  }
  gst_element_release_request_pad(vr->aenc_tee,vr->pre_aenc_tee_srcpad);
  gst_element_release_request_pad(vr->bin,vr->pre_aenc_tee_ghost_srcpad);
  gst_object_unref(vr->pre_aenc_tee_srcpad);
  //gst_object_unref(vr->pre_aenc_tee_ghost_srcpad);
  vr->pre_aenc_tee_srcpad = NULL;
  vr->pre_aenc_tee_ghost_srcpad = NULL;

  ret = gst_pad_unlink (vr->pre_vdec_tee_ghost_srcpad,vrsmsz->director.ds.pre_vdec_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
    g_print ("unLink vdec tee failed.\n");
  } else {
    g_print ("unLink succeeded .\n");
  }
  gst_element_release_request_pad(vr->vdec_tee,vr->pre_vdec_tee_srcpad);
  gst_element_release_request_pad(vr->bin,vr->pre_vdec_tee_ghost_srcpad);
  gst_object_unref(vr->pre_vdec_tee_srcpad);
  //gst_object_unref(vr->pre_vdec_tee_ghost_srcpad);
  vr->pre_vdec_tee_srcpad= NULL;
  vr->pre_vdec_tee_ghost_srcpad= NULL;


/*
  ret = gst_pad_unlink (vr->pub_vdec_tee_ghost_srcpad, vrsmsz->director.ds.pub_vdec_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("unLink failed.\n");
      } else {
         g_print ("unLink succeeded .\n");
      }
      gst_element_release_request_pad(vr->vdec_tee,vr->pub_vdec_tee_srcpad);
      gst_element_release_request_pad(vr->vdec_tee,vr->pub_vdec_tee_ghost_srcpad);
      gst_object_unref(vr->pub_vdec_tee_srcpad);
      //gst_object_unref(vr->pub_vdec_tee_ghost_srcpad);
      vr->pub_vdec_tee_srcpad= NULL;
      vr->pub_vdec_tee_ghost_srcpad= NULL;

  ret = gst_pad_unlink (vr->pub_aenc_tee_ghost_srcpad, vrsmsz->director.ds.pub_aenc_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("unLink failed.\n");
      } else {
         g_print ("unLink succeeded .\n");
      }
      gst_element_release_request_pad(vr->aenc_tee,vr->pub_aenc_tee_srcpad);
      gst_element_release_request_pad(vr->aenc_tee,vr->pub_aenc_tee_ghost_srcpad);
      gst_object_unref(vr->pub_aenc_tee_srcpad);
      //gst_object_unref(vr->pub_aenc_tee_ghost_srcpad);
      vr->pub_aenc_tee_srcpad= NULL;
      vr->pub_aenc_tee_ghost_srcpad= NULL;


      if(!vs->pub_vdec_tee_srcpad)
        vs->pub_vdec_tee_srcpad = gst_element_get_request_pad (vs->vdec_tee, "src_%u");
      if(!vrsmsz->director.ds.pub_vdec_tee_queue_sinkpad)
        vrsmsz->director.ds.pub_vdec_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->director.ds.pub_vdec_tee_queue, "sink");
      ret = gst_pad_link (vs->pub_vdec_tee_srcpad, vrsmsz->director.ds.pub_vdec_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
        g_print ("Link failed.\n");
      } else {
        g_print ("Link succeeded .\n");
      }

      if(!vs->pub_aenc_tee_srcpad)
        vs->pub_aenc_tee_srcpad = gst_element_get_request_pad (vs->aenc_tee, "src_%u");
      if(!vrsmsz->director.ds.pub_aenc_tee_queue_sinkpad)
        vrsmsz->director.ds.pub_aenc_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->director.ds.pub_aenc_tee_queue, "sink");
      ret = gst_pad_link (vs->pub_aenc_tee_srcpad, vrsmsz->director.ds.pub_aenc_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("Link failed.\n");
       } else {
         g_print ("Link succeeded .\n");
      }
*/
     return FALSE;   
}
/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/
gboolean vrsmsz_switch_stream(gpointer data){
#if 0// effect switch
  //gboolean is_fade = TRUE;
  gboolean is_fade = FALSE;
#endif
  gint streamid = atoi((gchar*)data);
  //GstPadLinkReturn ret;
  //gchar name[1024];
  g_print("Switch %d \n",streamid);

  if(vrsmsz->stream_nbs == 0){
    g_print("no stream \n");
    return FALSE;
  }
  if(streamid == vrsmsz->director.stream_id){
    g_print("switch in same stream\n");
    return FALSE;
  }
  int i;
  for(i=0; i<MAX_CHANNEL; i++){
    if(streamid == vrsmsz->streams_id[i]){
       break;
    }
  }
  if(i == MAX_CHANNEL){
    g_print("streamid is error");
    return FALSE;
  }
 
  vrchan_t* vc = vrsmsz->streams+streamid;
  g_free(data);
  if(!vrsmsz->isSwitched){// 
    g_print("first switch %d\n",vc->stream_id);
    director_preview_create(&(vc->vs));
  g_print("create preview \n");
    director_preview_link_vs(&(vc->vs));
  g_print("link vs to preview \n");
    vrsmsz->isSwitched = TRUE;
    vrsmsz->director.stream_id = vc->stream_id;
    gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
    return FALSE;
  }
  
  vrstream_t* vr = &(vrsmsz->streams[vrsmsz->director.stream_id].vs);
  g_print("unlink vs to preview \n");
  director_preview_unlink_vs(vr);
  g_print("link new vs to preview \n");
  director_preview_link_vs(&vc->vs);
  vrsmsz->director.stream_id = vc->stream_id;
  gst_debug_bin_to_dot_file (GST_BIN_CAST(vrsmsz->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");
  return FALSE;
#if 0
  director_publish_create();
  director_switch_effect_create();

  if(vrsmsz->isSwitched && is_fade){
      if(!gst_element_link_many(vrsmsz->director.video_filter_queue, vrsmsz->director.videoconvert, vrsmsz->director.video_filter_chain, vrsmsz->director.video_filter_tee, NULL)){
        g_print("link director pre viedo failed");
        return FALSE;
      }
  }
/*
*
*
 uridecodebin uri=rtmp://192.168.0.134/live/ch1 name=src ! tee name=t1 ! queue ! videoscale ! video/x-raw, width=320, height=240 ! x264enc ! flvmux name=muxer ! rtmpsink location=rtmp://192.168.0.134/live/0 src. ! audioconvert ! voaacenc ! tee name=t2 ! queue ! muxer. t1. ! queue ! videoscale ! video/x-raw, width=320, height=240 ! x264enc ! flvmux name=pre_muxer ! rtmpsink location=rtmp://192.168.0.134/live/preview t1. ! queue ! x264enc ! flvmux name=pub_muxer ! rtmpsink location=rtmp://192.168.0.134/live/pub t2. ! queue ! pre_muxer. t2. ! queue ! pub_muxer.
*
*/

/****************************************************************************************/
  //gst_element_set_state (vrsmsz->pipeline, GST_STATE_PAUSED);
  if(vrsmsz->isSwitched){// 
    vrstream_t* vr = vrsmsz->streams + vrsmsz->v_director;
    if(is_fade){

      ret = gst_pad_unlink (vr->pre_vdec_tee_srcpad,vrsmsz->pre_vdec_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("unLink failed.\n");
      } else {
         g_print ("unLink succeeded %d.\n",vr->video_id);
      }
      gst_element_release_request_pad(vr->vdec_tee,vr->pre_vdec_tee_srcpad);
      gst_object_unref(vr->pre_vdec_tee_srcpad);
      vr->pre_vdec_tee_srcpad= NULL;

      ret = gst_pad_unlink (vr->pub_vdec_tee_srcpad, vrsmsz->pub_vdec_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("unLink failed.\n");
      } else {
         g_print ("unLink succeeded %d.\n",vr->video_id);
      }
      gst_element_release_request_pad(vr->vdec_tee,vr->pub_vdec_tee_srcpad);
      gst_object_unref(vr->pub_vdec_tee_srcpad);
      vr->pub_vdec_tee_srcpad= NULL;

      if(!vrsmsz->video_filter_queue_sinkpad)
        vrsmsz->video_filter_queue_sinkpad = gst_element_get_static_pad (vrsmsz->video_filter_queue, "sink");
      if(!vr->vdec_tee_filter_srcpad)
        vr->vdec_tee_filter_srcpad = gst_element_get_request_pad (vr->vdec_tee, "src_%u");
      ret = gst_pad_link (vr->vdec_tee_filter_srcpad ,vrsmsz->video_filter_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("Link failed.\n");
      } else {
         g_print ("Link succeeded .\n");
      }

      if(!vrsmsz->pre_video_filter_tee_srcpad)
        vrsmsz->pre_video_filter_tee_srcpad = gst_element_get_request_pad (vrsmsz->video_filter_tee, "src_%u");
      ret = gst_pad_link (vrsmsz->pre_video_filter_tee_srcpad ,vrsmsz->pre_vdec_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("unLink failed.\n");
      } else {
         g_print ("unLink succeeded %d.\n",vr->video_id);
      }
      if(!vrsmsz->pub_video_filter_tee_srcpad)
        vrsmsz->pub_video_filter_tee_srcpad = gst_element_get_request_pad (vrsmsz->video_filter_tee, "src_%u");
      ret = gst_pad_link (vrsmsz->pub_video_filter_tee_srcpad ,vrsmsz->pub_vdec_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("unLink failed.\n");
      } else {
         g_print ("unLink succeeded %d.\n",vr->video_id);
      }
//// --- 
      if(!vs->vdec_tee_filter_srcpad)
        vs->vdec_tee_filter_srcpad = gst_element_get_request_pad (vs->vdec_tee, "src_%u");

      if(!vs->vdec_filter_tee_queue){
         sprintf(name,"vs%d-%s",vs->stream_id,"vdec_filter_tee_queue");
         vs->vdec_filter_tee_queue= gst_element_factory_make("queue", name);
         if(!vs->vdec_filter_tee_queue){
           g_print("error make\n");
           return FALSE;
         }
      }
      if(!vs->vdec_filter_tee_convert){
         sprintf(name,"vs%d-%s",vs->stream_id,"vdec_filter_tee_convert");
         vs->vdec_filter_tee_convert= gst_element_factory_make("videoconvert", name);
         if(!vs->vdec_filter_tee_convert){
           g_print("error make\n");
           return FALSE;
         }
      }
      gst_bin_add_many(GST_BIN_CAST(vrsmsz->pipeline),vs->vdec_filter_tee_queue,vs->vdec_filter_tee_convert,NULL);
      if(!gst_element_link_many(vs->vdec_filter_tee_queue,vs->vdec_filter_tee_convert,vrsmsz->video_filter_chain, NULL)){
        g_print("link director pre viedo failed");
        return FALSE;
      }

      if(!vs->vdec_filter_tee_queue_sinkpad)
        vs->vdec_filter_tee_queue_sinkpad= gst_element_get_static_pad (vs->vdec_filter_tee_queue, "sink");

      ret = gst_pad_link (vs->vdec_tee_filter_srcpad,vs->vdec_filter_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("unLink failed.\n");
      } else {
         g_print ("unLink succeeded %d.\n",vr->video_id);
      }

      ///// set comp operater 
      //
      //
      //
      //
    }else{
      ret = gst_pad_unlink(vr->pre_aenc_tee_srcpad, vrsmsz->pre_aenc_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("unLink failed.\n");
      } else {
         g_print ("unLink succeeded %d.\n",vr->video_id);
      }
      gst_element_release_request_pad(vr->aenc_tee,vr->pre_aenc_tee_srcpad);
      gst_object_unref(vr->pre_aenc_tee_srcpad);
      vr->pre_aenc_tee_srcpad = NULL;

      ret = gst_pad_unlink (vr->pre_vdec_tee_srcpad,vrsmsz->pre_vdec_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("unLink failed.\n");
      } else {
         g_print ("unLink succeeded %d.\n",vr->video_id);
      }
      gst_element_release_request_pad(vr->vdec_tee,vr->pre_vdec_tee_srcpad);
      gst_object_unref(vr->pre_vdec_tee_srcpad);
      vr->pre_vdec_tee_srcpad= NULL;

      ret = gst_pad_unlink (vr->pub_vdec_tee_srcpad, vrsmsz->pub_vdec_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("unLink failed.\n");
      } else {
         g_print ("unLink succeeded %d.\n",vr->video_id);
      }
      gst_element_release_request_pad(vr->vdec_tee,vr->pub_vdec_tee_srcpad);
      gst_object_unref(vr->pub_vdec_tee_srcpad);
      vr->pub_vdec_tee_srcpad= NULL;

      ret = gst_pad_unlink (vr->pub_aenc_tee_srcpad, vrsmsz->pub_aenc_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("unLink failed.\n");
      } else {
         g_print ("unLink succeeded %d.\n",vr->video_id);
      }
      gst_element_release_request_pad(vr->aenc_tee,vr->pub_aenc_tee_srcpad);
      gst_object_unref(vr->pub_aenc_tee_srcpad);
      vr->pub_aenc_tee_srcpad= NULL;

      if(!vs->pre_aenc_tee_srcpad){
        vs->pre_aenc_tee_srcpad = gst_element_get_request_pad (vs->aenc_tee, "src_%u");
      }
      if(!vrsmsz->pre_aenc_tee_queue_sinkpad)
        vrsmsz->pre_aenc_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->pre_aenc_tee_queue, "sink");
      ret = gst_pad_link (vs->pre_aenc_tee_srcpad, vrsmsz->pre_aenc_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("Link failed.\n");
      } else {
         g_print ("Link succeeded .\n");
      }

      if(!vs->pre_vdec_tee_srcpad)
        vs->pre_vdec_tee_srcpad = gst_element_get_request_pad (vs->vdec_tee, "src_%u");
      if(!vrsmsz->pre_vdec_tee_queue_sinkpad)
        vrsmsz->pre_vdec_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->pre_vdec_tee_queue, "sink");
      ret = gst_pad_link (vs->pre_vdec_tee_srcpad,vrsmsz->pre_vdec_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
        g_print ("Link failed.\n");
      } else {
        g_print ("Link succeeded .\n");
      }

      if(!vs->pub_vdec_tee_srcpad)
        vs->pub_vdec_tee_srcpad = gst_element_get_request_pad (vs->vdec_tee, "src_%u");
      if(!vrsmsz->pub_vdec_tee_queue_sinkpad)
        vrsmsz->pub_vdec_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->pub_vdec_tee_queue, "sink");
      ret = gst_pad_link (vs->pub_vdec_tee_srcpad, vrsmsz->pub_vdec_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
        g_print ("Link failed.\n");
      } else {
        g_print ("Link succeeded .\n");
      }

      if(!vs->pub_aenc_tee_srcpad)
        vs->pub_aenc_tee_srcpad = gst_element_get_request_pad (vs->aenc_tee, "src_%u");
      if(!vrsmsz->pub_aenc_tee_queue_sinkpad)
        vrsmsz->pub_aenc_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->pub_aenc_tee_queue, "sink");
      ret = gst_pad_link (vs->pub_aenc_tee_srcpad, vrsmsz->pub_aenc_tee_queue_sinkpad);
      if (GST_PAD_LINK_FAILED (ret)) {
         g_print ("Link failed.\n");
       } else {
         g_print ("Link succeeded .\n");
      }
    }
  }else{
    if(!gst_element_link_many(vrsmsz->pre_vdec_tee_queue, vrsmsz->pre_video_scale, vrsmsz->pre_capsfilter, vrsmsz->pre_video_encoder, vrsmsz->pre_muxer, vrsmsz->pre_outer, NULL)){
     g_print("link director pre viedo failed");
     return FALSE;
    }
    if(!gst_element_link_many(vrsmsz->pub_vdec_tee_queue, vrsmsz->pub_video_encoder, vrsmsz->pub_muxer,vrsmsz->pub_outer, NULL)){
      g_print("link director pub video failed");
     return FALSE;
    }
    GstCaps *caps = gst_caps_from_string("audio/mpeg");
    if(!gst_element_link_filtered(vrsmsz->pre_aenc_tee_queue, vrsmsz->pre_muxer, caps)){
     g_print("link director pre audio failed");
     return FALSE;
    }
    if(!gst_element_link_filtered(vrsmsz->pub_aenc_tee_queue, vrsmsz->pub_muxer, caps)){
     g_print("link director pub audio failed");
     return FALSE;
    }
    

    if(!vs->pre_aenc_tee_srcpad){
      vs->pre_aenc_tee_srcpad = gst_element_get_request_pad (vs->aenc_tee, "src_%u");
    }
    if(!vrsmsz->pre_aenc_tee_queue_sinkpad)
      vrsmsz->pre_aenc_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->pre_aenc_tee_queue, "sink");
    ret = gst_pad_link (vs->pre_aenc_tee_srcpad, vrsmsz->pre_aenc_tee_queue_sinkpad);
    if (GST_PAD_LINK_FAILED (ret)) {
       g_print ("Link failed.\n");
    } else {
       g_print ("Link succeeded .\n");
    }
    if(!vs->pre_vdec_tee_srcpad)
      vs->pre_vdec_tee_srcpad = gst_element_get_request_pad (vs->vdec_tee, "src_%u");
    if(!vrsmsz->pre_vdec_tee_queue_sinkpad)
      vrsmsz->pre_vdec_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->pre_vdec_tee_queue, "sink");
    ret = gst_pad_link (vs->pre_vdec_tee_srcpad,vrsmsz->pre_vdec_tee_queue_sinkpad);
    if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("Link failed.\n");
    } else {
      g_print ("Link succeeded .\n");
    }

    if(!vs->pub_vdec_tee_srcpad)
      vs->pub_vdec_tee_srcpad = gst_element_get_request_pad (vs->vdec_tee, "src_%u");
    if(!vrsmsz->pub_vdec_tee_queue_sinkpad)
      vrsmsz->pub_vdec_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->pub_vdec_tee_queue, "sink");
    ret = gst_pad_link (vs->pub_vdec_tee_srcpad, vrsmsz->pub_vdec_tee_queue_sinkpad);
    if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("Link failed.\n");
    } else {
      g_print ("Link succeeded .\n");
    }

    if(!vs->pub_aenc_tee_srcpad)
      vs->pub_aenc_tee_srcpad = gst_element_get_request_pad (vs->aenc_tee, "src_%u");
    if(!vrsmsz->pub_aenc_tee_queue_sinkpad)
      vrsmsz->pub_aenc_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->pub_aenc_tee_queue, "sink");
    ret = gst_pad_link (vs->pub_aenc_tee_srcpad, vrsmsz->pub_aenc_tee_queue_sinkpad);
    if (GST_PAD_LINK_FAILED (ret)) {
       g_print ("Link failed.\n");
     } else {
       g_print ("Link succeeded .\n");
    }


   vrsmsz->isSwitched = TRUE;
  }
  vrsmsz->v_director = streamid;
  vrsmsz->a_director = streamid;
  gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
  gst_debug_bin_to_dot_file (GST_BIN_CAST(vrsmsz->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");
  return FALSE;
#endif
}

void vrsmsz_stop(){
  gst_element_set_state (vrsmsz->pipeline, GST_STATE_NULL);
}

void vrsmsz_start(){
  g_main_loop_run (vrsmsz->loop);
}

void vrsmsz_quit(){
  g_main_loop_quit (vrsmsz->loop);
}

void vrsmsz_play(){
  GstStateChangeReturn r;
  r = gst_element_set_state (vrsmsz->pipeline, GST_STATE_PAUSED);
  switch (r) {
    case GST_STATE_CHANGE_NO_PREROLL:
      /* live source, timestamps are running_time of the pipeline clock. */
	    g_print("this is live stream\n");
      break;
    case GST_STATE_CHANGE_SUCCESS:
      /* success, no async state changes, same as async, timestamps start
       * from 0 */
    case GST_STATE_CHANGE_ASYNC:
      /* no live source, bin will preroll. We have to punch it in because in
       * this situation timestamps start from 0.  */
	    g_print("this is not live stream\n");
      break;
    case GST_STATE_CHANGE_FAILURE:
      /* fall through to return */
    default:
	    g_print("this status change is failed\n");
  }
  r = gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
  switch (r) {
    case GST_STATE_CHANGE_NO_PREROLL:
      /* live source, timestamps are running_time of the pipeline clock. */
	    g_print("this is live stream\n");
      break;
    case GST_STATE_CHANGE_SUCCESS:
      /* success, no async state changes, same as async, timestamps start
       * from 0 */
    case GST_STATE_CHANGE_ASYNC:
      /* no live source, bin will preroll. We have to punch it in because in
       * this situation timestamps start from 0.  */
	    g_print("this is not live stream\n");
      break;
    case GST_STATE_CHANGE_FAILURE:
      /* fall through to return */
    default:
	    g_print("this status change is failed\n");
  }
}

void vrsmsz_paused(){
  gst_element_set_state (vrsmsz->pipeline, GST_STATE_PAUSED);
}

void vrsmsz_deinit(){

}

void vrsmsz_add_track(){

}

void vrsmsz_switch_track(){

}

void vrsmsz_switch_stream_crossfade(){

}

void vrsmsz_add_text(){

}

void vrsmsz_add_pic(){

}

void vrsmsz_add_clip(){

}

void vrsmsz_publish(){

}
#if 0
static void vrsmsz_null_channel(){
  /*create_stream("(audiotestsrc ! audioconvert ! queue ! voaacenc ! muxer.  videotestsrc is-live=1 ! videoconvert !  compositor name=Compositer !  clockoverlay ! queue ! x264enc ! flvmux name=muxer ! tee name=teer ! rtmpsink location=rtmp://192.168.0.134/live/chan10 )");*/
  
  //vrsmsz->comp = gst_bin_get_by_name("Compositer");
}
#endif 
static void vrsmsz_run_command(gchar* command){
  gchar argv[2][1024];
  sscanf(command,"%s %s",argv[0],argv[1]);
  //g_print("command--> %s %s\n",argv[0],argv[1]);
  gchar* arg2 = g_strdup(argv[1]);

  if(!memcmp(argv[0],"pull",4)){
    g_idle_add(vrsmsz_add_stream, arg2);
  }else if(!memcmp(argv[0],"remove",6)){
    g_idle_add(vrsmsz_remove_stream, arg2);
  }else if(!memcmp(argv[0],"switch",6)){
    g_idle_add(vrsmsz_switch_stream, arg2);
  }else if(!memcmp(argv[0],"quit",4)){
    exit(atoi(argv[1]));
  }else if(!memcmp(argv[0],"recover",7)){
    gst_element_set_state (vrsmsz->pipeline, GST_STATE_READY);
    gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
    gst_debug_bin_to_dot_file (GST_BIN_CAST(vrsmsz->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");
  }
  g_free(command);
}

static gboolean get_command(){
   gpointer command;
   command = g_async_queue_try_pop(vrsmsz->queue);
   if(!command) return TRUE;
   vrsmsz_run_command(command);
   return TRUE;
}

void chan_stream_init(vrstream_t* vs){
  vs->uridecodebin = NULL;  
  vs->vdec_tee= NULL;
  vs->vdec_tee_queue= NULL;
  vs->pre_vdec_tee_srcpad= NULL;
  vs->pub_vdec_tee_srcpad= NULL;
  vs->vdec_tee_filter_srcpad= NULL;
  vs->vdec_filter_tee_queue_sinkpad= NULL;
  vs->vdec_filter_tee_queue= NULL;
  vs->vdec_filter_tee_convert= NULL;
  vs->video_scale= NULL;
  vs->video_capsfilter= NULL;
  vs->video_encoder= NULL;
  vs->audio_convert= NULL;
  vs->audio_encoder= NULL;
  vs->aenc_tee= NULL;
  vs->pre_aenc_tee_srcpad= NULL;
  vs->pub_aenc_tee_srcpad= NULL;
  vs->aenc_tee_queue= NULL;
  vs->muxer= NULL;  
  vs->outer= NULL;  
}

void director_stream_init(drstream_t* ds){
  ds->video_filter_queue_sinkpad= NULL;
  ds->video_filter_queue= NULL;
  ds->videoconvert= NULL;
  ds->video_filter_chain= NULL;
  ds->video_filter_tee= NULL;
  ds->pre_video_filter_tee_srcpad= NULL;
  ds->pub_video_filter_tee_srcpad= NULL;
  ds->pre_vdec_tee_queue = NULL;
  ds->pre_vdec_tee_queue_sinkpad = NULL;
  ds->pre_video_scale = NULL;
  ds->pre_capsfilter = NULL;
  ds->pre_video_encoder = NULL;
  ds->pre_aenc_tee_queue = NULL;
  ds->pre_aenc_tee_queue_sinkpad= NULL;
  ds->pre_muxer= NULL;
  ds->pre_outer= NULL;
  ds->pub_vdec_tee_queue= NULL;
  ds->pub_vdec_tee_queue_sinkpad= NULL;
  ds->pub_video_encoder= NULL;
  ds->pub_aenc_tee_queue= NULL;
  ds->pub_aenc_tee_queue_sinkpad= NULL;
  ds->pub_muxer= NULL;
  ds->pub_outer= NULL;
  ds->mixer= NULL;
  //vrsmsz->comp= NULL;
}

void vrsmsz_init(int argc, char **argv){

  vrsmsz =  g_malloc0 ( sizeof(vrsmsz_t) );
  if( !vrsmsz ){
    g_error("vrsmsz init failed\n");
  }

  vrsmsz->queue = g_async_queue_new ();
  create_http_server(8888,NULL,NULL,vrsmsz->queue);
  
  gst_init (&argc, &argv);
  enable_factory("nvh265dec",TRUE); // may be use uridecodebin force 
  enable_factory("nvh264dec",TRUE);
  vrsmsz->loop = g_main_loop_new (NULL, TRUE);
  vrsmsz->pipeline = gst_pipeline_new ("vrsmsz-pipeline");
  if(!vrsmsz->loop || !vrsmsz->pipeline )
    g_error("vrsmsz init failed\n");
  vrsmsz->bus = gst_pipeline_get_bus (GST_PIPELINE (vrsmsz->pipeline));
  gst_bus_add_signal_watch (vrsmsz->bus);
  g_signal_connect (G_OBJECT (vrsmsz->bus), "message", G_CALLBACK (message_cb), NULL);
  
  vrsmsz->mode = atoi(argv[3]);

  vrsmsz->stream_nbs = 0;
  vrsmsz->isSwitched= FALSE;
  for( int i=0; i<MAX_CHANNEL; i++){ 
    memset(vrsmsz->streams + i, 0, sizeof(vrsmsz->streams[i]));
    vrsmsz->streams_id[i] = -1;
    sprintf(vrsmsz->streams[i].preview_url,"rtmp://%s:%s/live/%d", argv[1], argv[2], i);
    g_print("%s \n",vrsmsz->streams[i].preview_url);
    vrsmsz->streams[i].tracks = 0;
    vrsmsz->streams[i].video_id = -1;
    vrsmsz->streams[i].audio_id = -1;
    vrsmsz->streams[i].stream_id = -1;
    vrsmsz->streams[i].subs_id = -1;
    vrsmsz->streams[i].resolution = 0;

    chan_stream_init(&vrsmsz->streams[i].vs);
  }

  sprintf(vrsmsz->director.preview_url,"rtmp://%s:%s/live/preview",argv[1],argv[2]);
  memset(vrsmsz->director.publish_url,0,URL_LEN);
  vrsmsz->director.pre_bin = NULL;
  vrsmsz->director.pub_bin = NULL;
  vrsmsz->director.swt_bin = NULL;
  vrsmsz->director.eff_bin = NULL;
  vrsmsz->director.stream_id = -1;
  vrsmsz->director.video_id = -1;
  vrsmsz->director.audio_id = -1;

  director_stream_init(&vrsmsz->director.ds);

  vrsmsz_play(); 
  vrsmsz->theclock = gst_element_get_clock (vrsmsz->pipeline);
  if(!vrsmsz->theclock) g_print("the clock false\n");
  g_timeout_add(10, get_command,NULL);
  //vrsmsz_null_channel();
}
