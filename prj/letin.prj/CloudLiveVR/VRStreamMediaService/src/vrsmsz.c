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

static void create_stream (const gchar * descr, describer_t* des, gint type)
{
  GstElement *bin;
  GError *error = NULL;

  bin = gst_parse_launch (descr, &error);
  if (error) {
    g_print ("pipeline could not be constructed: %s\n", error->message);
    g_error_free (error);
    return ;
  }

  /* add the bin to the pipeline now, this will set the current base_time of the
   * pipeline on the new bin. */
  gst_bin_add (GST_BIN_CAST (vrsmsz->pipeline), bin);

  des->bin = bin;
  des->type = type;
  memcpy(des->descr, descr, strlen(descr)+1);
  des->streamid = vrsmsz->stream_ids;
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

void vrsmsz_add_stream(){
   //create_stream();
   vrsmsz->stream_ids++;
}

void vrsmsz_remove_stream(){
   vrsmsz->stream_ids--;
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

static void vrsmsz_null_channel(){
  describer_t *bin = vrsmsz->bins[0];
  create_stream("( videotestsrc is-live=1 ! videoconvert !  compositor name=Compositer ! timeoverlay ! clockoverlay ! queue ! x264enc ! flvmux name=muxer ! rtmpsink location=rtmp://192.168.0.134/live/chan10 audiotestsrc ! audioconvert ! queue ! voaacenc ! muxer. )", bin, 0);
  
  //vrsmsz->comp->bin= gst_element_get_by_name(bin->bin);
}

void vrsmsz_init(int argc, char **argv){

  vrsmsz =  g_malloc0 ( sizeof(vrsmsz_t) );
  if( !vrsmsz ){
    g_error("vrsmsz init failed\n");
  }
  
  enable_factory("nvh265dec",TRUE);
  enable_factory("nvh264dec",TRUE);
  
  for(int i=0; i<MAX_CHANNEL; i++){

    vrsmsz->video_src[i] = g_malloc0 ( sizeof(describer_t) );
    if(!vrsmsz->video_src[i])
      g_error("vrsmsz init failed\n");

    vrsmsz->audio_src[i] = g_malloc0 ( sizeof(describer_t) );
    if(!vrsmsz->audio_src[i])
      g_error("vrsmsz init failed\n");

    vrsmsz->video_filter[i] = g_malloc0 ( sizeof(describer_t) );
    if(!vrsmsz->video_filter[i])
      g_error("vrsmsz init failed\n");

    vrsmsz->audio_filter[i] = g_malloc0 ( sizeof(describer_t) );
    if(!vrsmsz->audio_filter[i])
      g_error("vrsmsz init failed\n");

    vrsmsz->video_encoder[i] = g_malloc0 ( sizeof(describer_t) );
    if(!vrsmsz->video_encoder[i])
      g_error("vrsmsz init failed\n");

    vrsmsz->audio_encoder[i] = g_malloc0 ( sizeof(describer_t) );
    if(!vrsmsz->video_encoder[i])
      g_error("vrsmsz init failed\n");

    vrsmsz->video_out[i] = g_malloc0 ( sizeof(describer_t) );
    if(!vrsmsz->video_encoder[i])
      g_error("vrsmsz init failed\n");

    vrsmsz->audio_out[i] = g_malloc0 ( sizeof(describer_t) );
    if(!vrsmsz->video_encoder[i])
      g_error("vrsmsz init failed\n");

    vrsmsz->bins[i] = g_malloc0 ( sizeof(describer_t) );
    if(!vrsmsz->bins[i])
      g_error("vrsmsz init failed\n");
  }

  vrsmsz->v_director= 0;
  vrsmsz->a_director= 0;
  vrsmsz->director_path = g_malloc0 ( sizeof(describer_t) );
  if(!vrsmsz->director_path)
    g_error("vrsmsz init failed\n");

  vrsmsz->comp = g_malloc0 ( sizeof(describer_t) );
  if(!vrsmsz->comp)
    g_error("vrsmsz init failed\n");

  vrsmsz->mixer = g_malloc0 ( sizeof(describer_t) );
  if(!vrsmsz->mixer)
    g_error("vrsmsz init failed\n");

  gst_init (&argc, &argv);
  vrsmsz->loop = g_main_loop_new (NULL, TRUE);
  vrsmsz->pipeline = gst_pipeline_new ("pipeline");
  if(!vrsmsz->loop || !vrsmsz->pipeline )
    g_error("vrsmsz init failed\n");

  vrsmsz->theclock = gst_element_get_clock (vrsmsz->pipeline);
  vrsmsz->bus = gst_pipeline_get_bus (GST_PIPELINE (vrsmsz->pipeline));
  vrsmsz->stream_ids = 1;
  vrsmsz_null_channel();
}
