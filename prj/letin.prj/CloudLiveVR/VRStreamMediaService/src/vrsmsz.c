#include <stdio.h>
#include "vrsmsz.h"

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
      g_print ("Got EOS\n");
      //g_main_loop_quit (loop);
      break;
    default:
      //g_print("==> %s\n",gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
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
  vrstream_t* vs = data;

   //g_printf("==>stream %d(%x) info: video_id %d, \naudio_id %d,\nsrc_url %s,\n pre_url %s,\ndis= %d,\nuridecodebin %x,\nvdec_tee %x,\nvdec_tee_queue %x,\nvideo_capsfilter %x,\nvideo_encoder %x,\naudio_encoder %x\n",vrsmsz->stream_nbs-1, vrsmsz->streams+(vrsmsz->stream_nbs-1),vs->video_id, vs->audio_id, vs->src_url, vs->pre_sink_url, vs->dis, vs->uridecodebin,vs->vdec_tee, vs->vdec_tee_queue,vs->video_capsfilter, vs->video_encoder, vs->audio_encoder);
  g_print ("url= %s Received new pad '%s' from '%s':\n",vs->pre_sink_url,GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(decodebin));
 
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
}
/*
 *
 * uridecodebin uri=rtmp://192.168.0.134/live/ch1 name=src ! tee name=t1 ! queue ! videoscale ! video/x-raw, width=320, height=240 ! x264enc ! flvmux name=muxer ! rtmpsink location=rtmp://192.168.0.134/live/0 src. ! audioconvert ! voaacenc ! tee ! queue ! muxer.
 *
*/
gboolean vrsmsz_add_stream(gpointer data){
   if(vrsmsz->stream_nbs > 16) {
     g_print("sorry streams is full\n");
     return FALSE;
   }
   /**********************dispatch stream id ***********************/
   gchar* uri = data;
   g_print("start pull stream uri= %s\n",uri);
   vrstream_t* vs = NULL;
   for(int i=0; i<MAX_CHANNEL; i++){
       if(vrsmsz->streams_id[i] == -1){
          vrsmsz->streams_id[i] = i;
          vs = vrsmsz->streams + vrsmsz->streams_id[i];
          vs->stream_id = vrsmsz->streams_id[i];
	  break;
	}
   }
   if(!vs) return FALSE;
   vs->video_id = vs->stream_id;
   vs->audio_id = vs->stream_id;
   sprintf(vs->src_url,"%s",uri);
   vs->dis = 1998;
   gchar name[1024];

   if(!vs->uridecodebin){
     sprintf(name,"vs%d-uridecodebin",vs->video_id);
     vs->uridecodebin = gst_element_factory_make("uridecodebin", name);
     g_object_set (vs->uridecodebin, "uri", uri, "expose-all-streams", TRUE, NULL);
     g_signal_connect (vs->uridecodebin, "pad-added", (GCallback) _pad_added_cb, vs);
   }

   if(!vs->vdec_tee){
     sprintf(name,"vs%d-vdec_tee",vs->video_id);
    vs->vdec_tee = gst_element_factory_make("tee",name);
    if(!vs->vdec_tee){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->vdec_tee_queue){
     sprintf(name,"vs%d-vdec_tee_queue",vs->video_id);
    vs->vdec_tee_queue = gst_element_factory_make("queue",name);
    if(!vs->vdec_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->video_scale){
     sprintf(name,"vs%d-video_scale",vs->video_id);
    vs->video_scale = gst_element_factory_make("videoscale",name);
    if(!vs->video_scale){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->video_capsfilter){
     sprintf(name,"vs%d-video_capsfilter",vs->video_id);
     vs->video_capsfilter = gst_element_factory_make("capsfilter", name);
    if(!vs->video_capsfilter){
      g_print("error make\n");
      return FALSE;
    }
     gst_util_set_object_arg (G_OBJECT (vs->video_capsfilter), "caps",
      "video/x-raw, width=320, height=240");
   }
   if(!vs->video_encoder){
     sprintf(name,"vs%d-video_encoder",vs->video_id);
     vs->video_encoder = gst_element_factory_make("x264enc", name);
    if(!vs->video_encoder){
      g_print("error make\n");
      return FALSE;
    }
    g_object_set (vs->video_encoder, "byte-stream", TRUE, "key-int-max", 25, NULL);
   }
   if(!vs->audio_convert){
     sprintf(name,"vs%d-audio_convert",vs->audio_id);
     vs->audio_convert = gst_element_factory_make("audioconvert", name);
    if(!vs->audio_convert){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->audio_encoder){
     sprintf(name,"vs%d-audio_encoder",vs->audio_id);
     vs->audio_encoder = gst_element_factory_make("voaacenc", name);
    if(!vs->audio_encoder){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->aenc_tee){
     sprintf(name,"vs%d-aenc_tee",vs->audio_id);
     vs->aenc_tee = gst_element_factory_make("tee", name);
    if(!vs->aenc_tee){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->aenc_tee_queue){
     sprintf(name,"vs%d-aenc_tee_queue",vs->audio_id);
     vs->aenc_tee_queue = gst_element_factory_make("queue", name);
    if(!vs->aenc_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->muxer){
     sprintf(name,"vs%d-muxer",vs->video_id);
     vs->muxer = gst_element_factory_make("flvmux", name);
    if(!vs->muxer){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->outer){
     sprintf(name,"vs%d-outer",vs->video_id);
     vs->outer= gst_element_factory_make("rtmpsink", name);
    if(!vs->outer){
      g_print("error make\n");
      return FALSE;
    }
     g_print("out put uri = %s\n",vs->pre_sink_url);
     g_object_set (vs->outer, "location", vs->pre_sink_url, NULL);
   }

   gst_bin_add_many(GST_BIN_CAST(vrsmsz->pipeline), vs->uridecodebin, vs->vdec_tee, vs->vdec_tee_queue,vs->video_scale, vs->video_capsfilter, vs->video_encoder, vs->audio_convert, vs->audio_encoder, vs->aenc_tee, vs->aenc_tee_queue, vs->muxer,vs->outer,NULL);

   if(!gst_element_link_many(vs->vdec_tee, vs->vdec_tee_queue, vs->video_scale, vs->video_capsfilter, vs->video_encoder, vs->muxer,vs->outer,NULL)){
      g_print("push link failed\n");
      return FALSE;
   }

   if(!gst_element_link_many(vs->audio_convert,vs->audio_encoder,vs->aenc_tee, vs->aenc_tee_queue, vs->muxer,NULL)){
      g_print("push link failed\n");
      return FALSE;
   }

   (vrsmsz->stream_nbs)++;
   vrsmsz_start();
   g_free(uri);
   /**** put stream_id to queue ***/
   g_print("Stream_id is %d\n", vs->stream_id);
   //g_print("++++++++++++++ old base time %lu\n\n",gst_element_get_base_time (vs->uridecodebin));
   //base_time = gst_element_get_base_time (vs->uridecodebin);
   //running_time = now - base_time;
   //g_print("++++++++++++++ running_time  %"GST_TIME_FORMAT", base_time = %"GST_TIME_FORMAT"\n\n",GST_TIME_ARGS(running_time / GST_MSECOND) ,GST_TIME_ARGS(base_time / GST_MSECOND));

#if 1
   GstClockTime now ;//,base_time,running_time;
   now = gst_clock_get_time (vrsmsz->theclock);
    //now = gst_clock_get_time (vrsmsz->theclock);
    //base_time = now - running_time;

    gst_element_set_base_time (vs->uridecodebin, now);
   g_print("\n++++++++++++++ base time %"GST_TIME_FORMAT"\n\n",GST_TIME_ARGS(now / GST_MSECOND));
#endif

   //g_print("==>stream %d(%x) info: video_id %d, \naudio_id %d,\nsrc_url %s,\n pre_url %s,\ndis= %d,\nuridecodebin %x,\nvdec_tee %x,\nvdec_tee_queue %x,\nvideo_capsfilter %x,\nvideo_encoder %x,\naudio_encoder %x\n",vrsmsz->stream_nbs-1, vrsmsz->streams+(vrsmsz->stream_nbs-1),vs->video_id, vs->audio_id, vs->src_url, vs->pre_sink_url, vs->dis, vs->uridecodebin,vs->vdec_tee, vs->vdec_tee_queue,vs->video_capsfilter, vs->video_encoder, vs->audio_encoder);
   return FALSE;
}

/*******************************************************************************/
gboolean vrsmsz_remove_stream(gpointer data){
   gint streamid = atoi((gchar*)data);
   vrstream_t* vs = NULL;

   if(!vrsmsz->stream_nbs) {
      g_print("no streams found\n");
      return FALSE;
   }
   g_print("remove stream = %s\n",vrsmsz->streams[streamid].src_url);
   for(int i=0; i<MAX_CHANNEL; i++){
       if(vrsmsz->streams_id[i] == streamid){
          vs = vrsmsz->streams + vrsmsz->streams_id[i];
          vs->stream_id = -1;
          vrsmsz->streams_id[i] = -1;
	  break;
	}
   }
   if(!vs) return FALSE;
   /*
    * deadlock ????
   gst_element_set_state(vs->uridecodebin,GST_STATE_NULL);
   gst_element_set_state(vs->vdec_tee,GST_STATE_NULL);
   gst_element_set_state(vs->vdec_tee_queue, GST_STATE_NULL);
   gst_element_set_state(vs->video_scale,GST_STATE_NULL);
   gst_element_set_state(vs->video_capsfilter,GST_STATE_NULL);
   gst_element_set_state(vs->video_encoder,GST_STATE_NULL);
   gst_element_set_state(vs->audio_convert,GST_STATE_NULL);
   gst_element_set_state(vs->audio_encoder,GST_STATE_NULL);
   gst_element_set_state(vs->aenc_tee,GST_STATE_NULL);
   gst_element_set_state(vs->aenc_tee_queue,GST_STATE_NULL);
   gst_element_set_state(vs->muxer,GST_STATE_NULL);
   *
   */
   g_print("close rtmp\n");
   gst_element_set_state(vs->outer,GST_STATE_NULL); // sink will not deadlock
   gst_bin_remove_many(GST_BIN (vrsmsz->pipeline),vs->uridecodebin,vs->vdec_tee,vs->vdec_tee_queue,vs->video_scale,vs->video_capsfilter, vs->video_encoder,vs->audio_convert,vs->audio_encoder,vs->aenc_tee,vs->aenc_tee_queue,vs->muxer,vs->outer,NULL);
   vs->uridecodebin=NULL,vs->vdec_tee=NULL,vs->vdec_tee_queue=NULL,vs->video_scale=NULL,vs->video_capsfilter=NULL, vs->video_encoder=NULL,vs->audio_convert=NULL,vs->audio_encoder=NULL,vs->aenc_tee=NULL,vs->aenc_tee_queue=NULL,vs->muxer=NULL,vs->outer=NULL;
   vrsmsz->stream_nbs--;
   vs->video_id = -1;
   vs->audio_id = -1;
   memset(vs->src_url,0,sizeof(vs->src_url));
   vs->dis = -1;
   g_print("remove stream = %s\n",vrsmsz->streams[streamid].src_url);
   g_free(data);
   vrsmsz_start();
   return FALSE;
}

/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/
gboolean vrsmsz_switch_stream(gpointer data){
  gint streamid = atoi((gchar*)data);
  GstPadLinkReturn ret;
  vrstream_t* vs = vrsmsz->streams+streamid;
  g_print("switch %d \n",streamid);

  if(vrsmsz->stream_nbs == 0){
    g_print("no stream \n");
    return FALSE;
  }
  if(streamid == vrsmsz->v_director){
    g_print("switch in same stream\n");
    return FALSE;
  }
  int i ;
  for(i=0; i<MAX_CHANNEL; i++){
    if(streamid == vrsmsz->streams_id[i]){
       break;
    }
  }
  if(i == MAX_CHANNEL){
    g_print("streamid is error");
    return FALSE;
  }
  if(!vrsmsz->director_stream_publish_url[0]){
    g_print("publish url = NULL\n");
    //return FALSE;
    gchar pub_url[1024]={"rtmp://192.168.0.134/live/pub"};
    g_print("use default url %s\n",pub_url);
    memcpy(vrsmsz->director_stream_publish_url,pub_url,strlen(pub_url)+1);
  }else{
    g_print("\n");
  }

  g_free(data);
  g_print("%s %s\n",vrsmsz->director_stream_publish_url,vrsmsz->director_stream_preview_url);

  gchar name[1024];
  if(!vrsmsz->pre_vdec_tee_queue){
     sprintf(name,"%s-pre_vdec_tee_queue","vrsmsz");
     vrsmsz->pre_vdec_tee_queue= gst_element_factory_make("queue", name);
    if(!vrsmsz->pre_vdec_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->pre_video_scale){
     sprintf(name,"%s-pre_video_scale","vrsmsz");
    vrsmsz->pre_video_scale= gst_element_factory_make("videoscale", name);
    if(!vrsmsz->pre_video_scale){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->pre_capsfilter){
     sprintf(name,"%s-pre_capsfilter","vrsmsz");
    vrsmsz->pre_capsfilter= gst_element_factory_make("capsfilter", name);
    if(!vrsmsz->pre_capsfilter){
      g_print("error make\n");
      return FALSE;
    }
     gst_util_set_object_arg (G_OBJECT (vrsmsz->pre_capsfilter), "caps",
      "video/x-raw, width=320, height=240");
  }
  if(!vrsmsz->pre_video_encoder){
     sprintf(name,"%s-pre_video_encoder","vrsmsz");
    vrsmsz->pre_video_encoder= gst_element_factory_make("x264enc", name);
    if(!vrsmsz->pre_video_encoder){
      g_print("error make\n");
      return FALSE;
    }
    g_object_set (vs->video_encoder, "byte-stream", TRUE, "key-int-max", 25, NULL);
  }
  if(!vrsmsz->pre_aenc_tee_queue){
     sprintf(name,"%s-pre_aenc_tee_queue","vrsmsz");
    vrsmsz->pre_aenc_tee_queue= gst_element_factory_make("queue", name);
    if(!vrsmsz->pre_aenc_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->pre_muxer){
     sprintf(name,"%s-pre_muxer","vrsmsz");
    vrsmsz->pre_muxer= gst_element_factory_make("flvmux", name);
    if(!vrsmsz->pre_muxer){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->pre_outer){
     sprintf(name,"%s-pre_outer","vrsmsz");
    vrsmsz->pre_outer= gst_element_factory_make("rtmpsink", name);
    if(!vrsmsz->pre_outer){
      g_print("error make\n");
      return FALSE;
    }
    g_object_set (vrsmsz->pre_outer, "location", vrsmsz->director_stream_preview_url, NULL);
  }

  if(!vrsmsz->pub_vdec_tee_queue){
     sprintf(name,"%s-pub_vdec_tee_queue","vrsmsz");
    vrsmsz->pub_vdec_tee_queue= gst_element_factory_make("queue", name);
    if(!vrsmsz->pub_vdec_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
  }

  if(!vrsmsz->pub_video_encoder){
     sprintf(name,"%s-pub_video_encoder","vrsmsz");
    vrsmsz->pub_video_encoder= gst_element_factory_make("x264enc", name); // nvh264enc have no avc
    if(!vrsmsz->pub_video_encoder){
      g_print("error make\n");
      return FALSE;
    }
    g_object_set (vs->video_encoder, "byte-stream", TRUE, "key-int-max", 25, NULL);
  }
  if(!vrsmsz->pub_aenc_tee_queue){
     sprintf(name,"%s-pub_aenc_tee_queue","vrsmsz");
    vrsmsz->pub_aenc_tee_queue= gst_element_factory_make("queue", name);
    if(!vrsmsz->pub_aenc_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->pub_muxer){
     sprintf(name,"%s-pub_muxer","vrsmsz");
    vrsmsz->pub_muxer= gst_element_factory_make("flvmux", name);
    if(!vrsmsz->pub_muxer){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->pub_outer){
     sprintf(name,"%s-pub_outer","vrsmsz");
    vrsmsz->pub_outer= gst_element_factory_make("rtmpsink", name);
    if(!vrsmsz->pub_outer){
      g_print("error make\n");
      return FALSE;
    }
    g_object_set (vrsmsz->pub_outer, "location", vrsmsz->director_stream_publish_url, NULL);
  }
  /*
  if(!vrsmsz->){
    vrsmsz->= gst_element_factory_make("", NULL);
    if(!vrsmsz->){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->){
    vrsmsz->= gst_element_factory_make("", NULL);
    if(!vrsmsz->){
      g_print("error make\n");
      return FALSE;
    }
  }
  */

/*
*
*
 uridecodebin uri=rtmp://192.168.0.134/live/ch1 name=src ! tee name=t1 ! queue ! videoscale ! video/x-raw, width=320, height=240 ! x264enc ! flvmux name=muxer ! rtmpsink location=rtmp://192.168.0.134/live/0 src. ! audioconvert ! voaacenc ! tee name=t2 ! queue ! muxer. t1. ! queue ! videoscale ! video/x-raw, width=320, height=240 ! x264enc ! flvmux name=pre_muxer ! rtmpsink location=rtmp://192.168.0.134/live/preview t1. ! queue ! x264enc ! flvmux name=pub_muxer ! rtmpsink location=rtmp://192.168.0.134/live/pub t2. ! queue ! pre_muxer. t2. ! queue ! pub_muxer.
*
*/

/****************************************************************************************/
  //gst_element_set_state (vrsmsz->pipeline, GST_STATE_PAUSED);
  if(vrsmsz->canSwitch){// 
    vrstream_t* vr = vrsmsz->streams + vrsmsz->v_director;
    ret = gst_pad_unlink(vr->pre_aenc_tee_srcpad, vrsmsz->pre_aenc_tee_queue_sinkpad);
    if (GST_PAD_LINK_FAILED (ret)) {
       g_print ("unLink failed.\n");
    } else {
       g_print ("unLink succeeded %d.\n",vr->video_id);
    }
    //gst_object_unref(vr->pre_aenc_tee_srcpad);
    //vr->pre_aenc_tee_srcpad = NULL;
    ret = gst_pad_unlink (vr->pre_vdec_tee_srcpad,vrsmsz->pre_vdec_tee_queue_sinkpad);
    if (GST_PAD_LINK_FAILED (ret)) {
       g_print ("unLink failed.\n");
    } else {
       g_print ("unLink succeeded %d.\n",vr->video_id);
    }
    //gst_object_unref(vr->pre_vdec_tee_srcpad);
    //vr->pre_vdec_tee_srcpad= NULL;
    ret = gst_pad_unlink (vr->pub_vdec_tee_srcpad, vrsmsz->pub_vdec_tee_queue_sinkpad);
    if (GST_PAD_LINK_FAILED (ret)) {
       g_print ("unLink failed.\n");
    } else {
       g_print ("unLink succeeded %d.\n",vr->video_id);
    }
    //gst_object_unref(vr->pub_vdec_tee_srcpad);
    //vr->pub_vdec_tee_srcpad= NULL;
    ret = gst_pad_unlink (vr->pub_aenc_tee_srcpad, vrsmsz->pub_aenc_tee_queue_sinkpad);
    if (GST_PAD_LINK_FAILED (ret)) {
       g_print ("unLink failed.\n");
    } else {
       g_print ("unLink succeeded %d.\n",vr->video_id);
    }
    //gst_object_unref(vr->pub_aenc_tee_srcpad);
    //vr->pub_aenc_tee_srcpad= NULL;

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


  }else{
     gst_bin_add_many(GST_BIN_CAST(vrsmsz->pipeline), vrsmsz->pre_vdec_tee_queue, vrsmsz->pre_video_scale, vrsmsz->pre_capsfilter, vrsmsz->pre_video_encoder, vrsmsz->pre_aenc_tee_queue, vrsmsz->pre_muxer, vrsmsz->pre_outer,vrsmsz->pub_vdec_tee_queue,vrsmsz->pub_video_encoder,vrsmsz->pub_aenc_tee_queue,vrsmsz->pub_muxer,vrsmsz->pub_outer, NULL);
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


   vrsmsz->canSwitch = TRUE;
  }
  vrsmsz->v_director = streamid;
  vrsmsz->a_director = streamid;
  gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
  gst_debug_bin_to_dot_file (vrsmsz->pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");
  return FALSE;
}

void vrsmsz_stop(){
  gst_element_set_state (vrsmsz->pipeline, GST_STATE_NULL);
}

void vrsmsz_play(){
  g_main_loop_run (vrsmsz->loop);
}

void vrsmsz_quit(){
  g_main_loop_quit (vrsmsz->loop);
}

void vrsmsz_start(){
  gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
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
  g_print("command--> %s %s\n",argv[0],argv[1]);
  gchar* arg2 = g_strdup(argv[1]);

  if(!memcmp(argv[0],"pull",4)){
    g_idle_add(vrsmsz_add_stream,arg2);
  }else if(!memcmp(argv[0],"remove",6)){
    g_idle_add(vrsmsz_remove_stream,arg2);
  }else if(!memcmp(argv[0],"switch",6)){
    g_idle_add(vrsmsz_switch_stream,arg2);
  }else if(!memcmp(argv[0],"quit",4)){
    exit(atoi(argv[1]));
  }else if(!memcmp(argv[0],"recover",7)){
    gst_element_set_state (vrsmsz->pipeline, GST_STATE_READY);
    gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
    gst_debug_bin_to_dot_file (vrsmsz->pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");
  }
  g_free(command);
}

static gboolean get_command(){
   gpointer command;
#if 0
   ret = g_strdup("pull rtmp://192.168.0.134/live/ch1");
#else
   command = g_async_queue_try_pop(vrsmsz->queue);
#endif
   if(!command) return TRUE;
   vrsmsz_run_command(command);
   return TRUE;
}

void vrsmsz_init(int argc, char **argv){

  vrsmsz =  g_malloc0 ( sizeof(vrsmsz_t) );
  if( !vrsmsz ){
    g_error("vrsmsz init failed\n");
  }
#if 1
  vrsmsz->queue = g_async_queue_new ();
  create_http_server(8888,NULL,NULL,vrsmsz->queue);
  //g_idle_add(get_command,NULL);
#endif
  
  gst_init (&argc, &argv);
  vrsmsz->loop = g_main_loop_new (NULL, TRUE);
  vrsmsz->pipeline = gst_pipeline_new ("pipeline");
  if(!vrsmsz->loop || !vrsmsz->pipeline )
    g_error("vrsmsz init failed\n");
  //return ;

  sprintf(vrsmsz->director_stream_preview_url,"rtmp://%s:%s/live/preview",argv[1],argv[2]);
  memset(vrsmsz->director_stream_publish_url,0,URL_LEN);
  vrsmsz->mode = atoi(argv[3]);
  vrsmsz->pre_vdec_tee_queue = NULL;
  vrsmsz->pre_vdec_tee_queue_sinkpad = NULL;
  vrsmsz->pre_video_scale = NULL;
  vrsmsz->pre_capsfilter = NULL;
  vrsmsz->pre_video_encoder = NULL;
  vrsmsz->pre_aenc_tee_queue = NULL;
  vrsmsz->pre_aenc_tee_queue_sinkpad= NULL;
  vrsmsz->pre_muxer= NULL;
  vrsmsz->pre_outer= NULL;
  vrsmsz->pub_vdec_tee_queue= NULL;
  vrsmsz->pub_vdec_tee_queue_sinkpad= NULL;
  vrsmsz->pub_video_encoder= NULL;
  vrsmsz->pub_aenc_tee_queue= NULL;
  vrsmsz->pub_aenc_tee_queue_sinkpad= NULL;
  vrsmsz->pub_muxer= NULL;
  vrsmsz->pub_outer= NULL;
  vrsmsz->mixer= NULL;
  vrsmsz->comp= NULL;
#if 0
  vrsmsz->videoconverter = gst_element_factory_make ("videoconvert", NULL);
  vrsmsz->comp = gst_element_factory_make ("compositor", NULL);
  vrsmsz->tee  = gst_element_factory_make ("tee", NULL);
  if(vrsmsz->mode == 0)// 8k
    vrsmsz->video_encoder = gst_element_factory_make("nvh265enc", NULL); 
  else if(vrsmsz->mode == 1)// 4k
    vrsmsz->video_encoder = gst_element_factory_make("nvh264enc", NULL); 
  else if(vrsmsz->mode == 2)// 2k
    vrsmsz->video_encoder = gst_element_factory_make("x264enc",   NULL); 
  vrsmsz->audio_encoder = gst_element_factory_make("voaacenc",   NULL); 
  vrsmsz->muxer = gst_element_factory_make("flvmux",   NULL);
  vrsmsz->outer = gst_element_factory_make("rtmpsink",   NULL);
#endif
  enable_factory("nvh265dec",TRUE);
  enable_factory("nvh264dec",TRUE);
  
  for( int i=0; i<MAX_CHANNEL; i++){ 
    memset(vrsmsz->streams + i, 0, sizeof(vrsmsz->streams[i]));
    vrsmsz->streams_id[i] = -1;
#if 0
    vrsmsz->streams[i].video_id = i;
    vrsmsz->streams[i].audio_id = i;
    vrsmsz->streams[i].uridecodebin = gst_element_factory_make("uridecodebin", NULL);
    vrsmsz->streams[i].muxer = gst_element_factory_make("flvmux", NULL);
    vrsmsz->streams[i].outer = gst_element_factory_make("rtmpsink", NULL);
#endif
    sprintf(vrsmsz->streams[i].pre_sink_url,"rtmp://%s:%s/live/%d", argv[1], argv[2], i);
    g_print("%s \n",vrsmsz->streams[i].pre_sink_url);
    //g_object_set(vrsmsz->streams[i].outer, "location", vrsmsz->streams[0].pre_sink_url,NULL);
  }
  g_print("%s\n",vrsmsz->director_stream_preview_url);
#if 0
  gst_bin_add_many(GST_BIN (vrsmsz->pipeline),vrsmsz->comp,vrsmsz->video_encoder,vrsmsz->audio_encoder,vrsmsz->muxer,vrsmsz->tee, NULL);
  g_print("example: %s","gst-launch-1.0 -e  -v  videotestsrc is-live=1 ! videoconvert ! timeoverlay ! queue ! compositor ! x264enc ! flvmux name=muxer ! rtmpsink location=rtmp://192.168.0.134/live/chan10 audiotestsrc ! audioconvert ! queue ! voaacenc ! muxer.\n\n gst-launch-1.0 -e  -v  videotestsrc is-live=1 ! videoconvert ! timeoverlay ! queue ! compositor ! x264enc ! flvmux name=muxer ! tee name=tee ! queue ! rtmpsink location=rtmp://192.168.0.134/live/chan10 audiotestsrc ! audioconvert ! queue ! voaacenc ! muxer. tee. ! queue ! rtmpsink location=rtmp://192.168.0.134/live/chan11\n\n");
  if(!gst_element_link_many(vrsmsz->comp,vrsmsz->video_encoder,vrsmsz->muxer,vrsmsz->tee, NULL)){
    g_error("link video failed\n");
  }
  if(!gst_element_link_many(vrsmsz->audio_encoder,vrsmsz->muxer,NULL)){
    g_error("link audio failed\n");
  }
#endif
  vrsmsz->v_director= -1;
  vrsmsz->a_director= -1;
  vrsmsz->stream_nbs = 0;
  vrsmsz->canSwitch = FALSE;
  memset(vrsmsz->comp_sink0_pad_name,0,16);
  memset(vrsmsz->comp_sink1_pad_name,0,16);

  vrsmsz->bus = gst_pipeline_get_bus (GST_PIPELINE (vrsmsz->pipeline));
  gst_bus_add_signal_watch (vrsmsz->bus);
  g_signal_connect (G_OBJECT (vrsmsz->bus), "message", G_CALLBACK (message_cb), NULL);

  vrsmsz_start();// playing
  vrsmsz->theclock = gst_element_get_clock (vrsmsz->pipeline);
  if(!vrsmsz->theclock) g_print("the clock false\n");
  g_timeout_add(10, get_command,NULL);
  //vrsmsz_null_channel();
}
