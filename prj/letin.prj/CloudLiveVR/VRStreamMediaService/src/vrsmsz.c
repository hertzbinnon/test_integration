#include "vrsmsz.h"

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
     g_print("what is run\n");
     return FALSE;
   }
   gchar* uri = data;
   g_printf("start push stream uri= %s\n",uri);
   vrstream_t* vs = &(vrsmsz->streams[vrsmsz->stream_nbs]);
   vs->video_id = vrsmsz->stream_nbs;
   vs->audio_id = vrsmsz->stream_nbs;
   sprintf(vs->src_url,"%s",uri);
   vs->dis = 1998;
   if(!vs->uridecodebin){
     vs->uridecodebin = gst_element_factory_make("uridecodebin", NULL);
     g_object_set (vs->uridecodebin, "uri", uri, "expose-all-streams", TRUE, NULL);
     g_signal_connect (vs->uridecodebin, "pad-added", (GCallback) _pad_added_cb, vs);
   }
   if(!vs->vdec_tee){
    vs->vdec_tee = gst_element_factory_make("tee",NULL);
    if(!vs->vdec_tee){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->vdec_tee_queue){
    vs->vdec_tee_queue = gst_element_factory_make("queue",NULL);
    if(!vs->vdec_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->video_scale){
    vs->video_scale = gst_element_factory_make("videoscale",NULL);
    if(!vs->video_scale){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->video_capsfilter){
     vs->video_capsfilter = gst_element_factory_make("capsfilter", NULL);
    if(!vs->video_capsfilter){
      g_print("error make\n");
      return FALSE;
    }
     gst_util_set_object_arg (G_OBJECT (vs->video_capsfilter), "caps",
      "video/x-raw, width=320, height=240");
   }
   if(!vs->video_encoder){
     vs->video_encoder = gst_element_factory_make("x264enc", NULL);
    if(!vs->video_encoder){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->audio_convert){
     vs->audio_convert = gst_element_factory_make("audioconvert", NULL);
    if(!vs->audio_convert){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->audio_encoder){
     vs->audio_encoder = gst_element_factory_make("voaacenc", NULL);
    if(!vs->audio_encoder){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->aenc_tee){
     vs->aenc_tee = gst_element_factory_make("tee", NULL);
    if(!vs->aenc_tee){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->aenc_tee_queue){
     vs->aenc_tee_queue = gst_element_factory_make("queue", NULL);
    if(!vs->aenc_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->muxer){
     vs->muxer = gst_element_factory_make("flvmux", NULL);
    if(!vs->muxer){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->outer){
     vs->outer= gst_element_factory_make("rtmpsink", NULL);
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

   g_printf("==>stream %d(%x) info: video_id %d, \naudio_id %d,\nsrc_url %s,\n pre_url %s,\ndis= %d,\nuridecodebin %x,\nvdec_tee %x,\nvdec_tee_queue %x,\nvideo_capsfilter %x,\nvideo_encoder %x,\naudio_encoder %x\n",vrsmsz->stream_nbs-1, vrsmsz->streams+(vrsmsz->stream_nbs-1),vs->video_id, vs->audio_id, vs->src_url, vs->pre_sink_url, vs->dis, vs->uridecodebin,vs->vdec_tee, vs->vdec_tee_queue,vs->video_capsfilter, vs->video_encoder, vs->audio_encoder);
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

void vrsmsz_deinit(){

}
void vrsmsz_remove_stream(){
   vrsmsz->stream_nbs--;
}

void vrsmsz_add_track(){

}

void vrsmsz_switch_stream(){

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
  gchar* arg2 = g_strdup(argv[1]);
  g_print("command--> %s %s\n",argv[0],arg2);
  if(!memcmp(argv[0],"pull",4)){
    g_idle_add(vrsmsz_add_stream,arg2);
  }
}

static gboolean get_command(){
   gpointer ret;
   ret = g_async_queue_try_pop(vrsmsz->queue);
   if(!ret) return TRUE;
   vrsmsz_run_command(ret);
   g_free(ret);
   return TRUE;
}

void vrsmsz_init(int argc, char **argv){

  vrsmsz =  g_malloc0 ( sizeof(vrsmsz_t) );
  if( !vrsmsz ){
    g_error("vrsmsz init failed\n");
  }
  vrsmsz->queue = g_async_queue_new ();
  //g_idle_add(get_command,NULL);
  g_timeout_add(10, get_command,NULL);
  create_http_server(8888,NULL,NULL,vrsmsz->queue);
  
  gst_init (&argc, &argv);
  vrsmsz->loop = g_main_loop_new (NULL, TRUE);
  vrsmsz->pipeline = gst_pipeline_new ("pipeline");
  if(!vrsmsz->loop || !vrsmsz->pipeline )
    g_error("vrsmsz init failed\n");

  sprintf(vrsmsz->director_stream_preview_url,"rtmp://%s:%s/live/preview",argv[1],argv[2]);
  vrsmsz->mode = atoi(argv[3]);
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
  vrsmsz->v_director= 0;
  vrsmsz->a_director= 0;
  vrsmsz->stream_nbs = 0;
  memset(vrsmsz->comp_sink0_pad_name,0,16);
  memset(vrsmsz->comp_sink1_pad_name,0,16);

  vrsmsz->theclock = gst_element_get_clock (vrsmsz->pipeline);
  vrsmsz->bus = gst_pipeline_get_bus (GST_PIPELINE (vrsmsz->pipeline));
  //vrsmsz_null_channel();
}
