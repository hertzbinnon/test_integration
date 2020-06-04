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

static void
_pad_added_cb (GstElement * decodebin, GstPad * pad)
{
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

void vrsmsz_add_stream(gchar* uri){
   //create_stream();
   vrstream_t vs = vrsmsz->streams[vrsmsz->stream_nbs];
   if(!vs.uridecodebin) 
     vs.uridecodebin = gst_element_factory_make("uridecodebin", NULL);
   g_object_set (vs.uridecodebin, "uri", uri, "expose-all-streams", TRUE, NULL);
   g_signal_connect (vs.uridecodebin, "pad-added", (GCallback) _pad_added_cb, NULL);
   gst_bin_add(GST_BIN_CAST(vrsmsz->pipeline), vs.uridecodebin);
   vrsmsz->stream_nbs++;
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

static void vrsmsz_null_channel(){
  /*create_stream("(audiotestsrc ! audioconvert ! queue ! voaacenc ! muxer.  videotestsrc is-live=1 ! videoconvert !  compositor name=Compositer !  clockoverlay ! queue ! x264enc ! flvmux name=muxer ! tee name=teer ! rtmpsink location=rtmp://192.168.0.134/live/chan10 )");*/
  
  //vrsmsz->comp = gst_bin_get_by_name("Compositer");
}

void vrsmsz_init(int argc, char **argv){

  gst_init (&argc, &argv);
  vrsmsz =  g_malloc0 ( sizeof(vrsmsz_t) );
  if( !vrsmsz ){
    g_error("vrsmsz init failed\n");
  }
  
  sprintf(vrsmsz->director_stream_preview_url,"rtmp://%s:%s/live/preview",argv[1],argv[2]);
  vrsmsz->mode = atoi(argv[3]);
  //vrsmsz->videoconverter = gst_element_factory_make ("videoconvert", NULL);
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

  enable_factory("nvh265dec",TRUE);
  enable_factory("nvh264dec",TRUE);
  
  for( int i=0; i<MAX_CHANNEL; i++){ 
    vrsmsz->streams[i].video_id = i;
    vrsmsz->streams[i].audio_id = i;
    vrsmsz->streams[i].uridecodebin = gst_element_factory_make("uridecodebin", NULL);
    vrsmsz->streams[i].muxer = gst_element_factory_make("flvmux", NULL);
    vrsmsz->streams[i].outer = gst_element_factory_make("rtmpsink", NULL);
    sprintf(vrsmsz->streams[i].pre_sink_url,"rtmp://%s:%s/live/%d", argv[1], argv[2], i);
    g_print("%s\n",vrsmsz->streams[0].pre_sink_url);
  }
  g_print("%s\n",vrsmsz->director_stream_preview_url);
  gst_bin_add_many(vrsmsz->comp,vrsmsz->video_encoder,vrsmsz->audio_encoder,vrsmsz->muxer,vrsmsz->tee,vrsmsz->outer);

  vrsmsz->v_director= 0;
  vrsmsz->a_director= 0;
  vrsmsz->stream_nbs = 0;

  vrsmsz->loop = g_main_loop_new (NULL, TRUE);
  vrsmsz->pipeline = gst_pipeline_new ("pipeline");
  if(!vrsmsz->loop || !vrsmsz->pipeline )
    g_error("vrsmsz init failed\n");
  vrsmsz->theclock = gst_element_get_clock (vrsmsz->pipeline);
  vrsmsz->bus = gst_pipeline_get_bus (GST_PIPELINE (vrsmsz->pipeline));
  //vrsmsz_null_channel();
}
