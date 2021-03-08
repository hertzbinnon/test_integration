#include <stdio.h>
#include "vrsmsz.h"
#include <libsoup/soup.h>
#include <json-glib/json-glib.h>
#include <string.h>
#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>


#define TEST_RENDER
//#define TEST_FADE
#define MEDIA_PATH "/tmp"
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
static SoupWebsocketConnection *ws_conn = NULL;
static enum AppState app_state = 0;
static const gchar *server_url = "wss://192.168.0.134:9443";
#define STUN_SERVER "stun://stun.l.google.com:19302 "
void vrsmsz_set_play(GstElement* bin){
  GstStateChangeReturn r;
#if 1
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
#endif
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

vrsmsz_t* vrsmsz;
GList* message_list;

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
    case GST_MESSAGE_EOS:{
      GError *err = NULL;
      gchar *name, *debug = NULL;

      name = gst_object_get_path_string (message->src);
      gst_message_parse_warning (message, &err, &debug);

      g_print ("======> Got EOS name=%s\n",name);
      if( err )
        g_printerr ("ERROR: from element %s: %s\n", name, err->message);
      if (debug )
        g_printerr ("Additional debug info:\n%s\n", debug);
      //gst_element_set_state (vrsmsz->pipeline, GST_STATE_READY);
      //gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
      //vrsmsz_remove();
      //g_main_loop_quit (loop);
      break;
    }
    default:
      //g_print("==> %s\n",gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
      break;
  }

  return TRUE;
}


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
      vs->audio_src_pad = new_pad;
      if(vs->audio_time != 0)
        gst_pad_set_offset(vs->audio_src_pad, vs->audio_time );
    }
  }else if(g_str_has_prefix (new_pad_type, "video/x-raw")){
    g_print ("It has type '%s' which is raw video. \n", new_pad_type);
    sink_pad = gst_element_get_static_pad (GST_ELEMENT (vs->vdec_tee), "sink");
    ret = gst_pad_link (new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("Type is '%s' but link failed.\n", new_pad_type);
    } else {
      g_print ("Link succeeded (type '%s').\n", new_pad_type);
      vs->video_src_pad = new_pad;
      if(vs->video_time != 0)
        gst_pad_set_offset(vs->video_src_pad, vs->video_time );
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
  vc->status = 2;

  gst_debug_bin_to_dot_file (GST_BIN_CAST(vrsmsz->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");
}

gint _autoplug_select_cb(GstElement* decodebin, GstPad* pad, GstCaps*caps, GValueArray* factories){
    GstStructure *new_pad_struct = gst_caps_get_structure (caps, 0);
    const gchar * new_pad_type = gst_structure_get_name (new_pad_struct);
    g_print("===== select for %s:%s, %s\n",GST_DEBUG_PAD_NAME (pad), new_pad_type);
    g_print("checking factory %s\n", GST_OBJECT_NAME (factories));
    return 0;
}
/*
 *
 * uridecodebin uri=rtmp://192.168.0.134/live/ch1 name=src ! tee name=t1 ! queue ! videoscale ! video/x-raw, width=320, height=240 ! x264enc ! flvmux name=muxer ! rtmpsink location=rtmp://192.168.0.134/live/0 src. ! audioconvert ! voaacenc ! tee ! queue ! muxer.
 *
*/

static gboolean
register_with_server (vrchan_t* vc){
  gchar *hello;

  if (soup_websocket_connection_get_state (ws_conn) !=
      SOUP_WEBSOCKET_STATE_OPEN)
    return FALSE;

  hello = g_strdup_printf ("HELLO %s", vc->ps.our_id);

  app_state = SERVER_REGISTERING;

  /* Register with the server with a random integer id. Reply will be received
   * by on_server_message() */
  soup_websocket_connection_send_text (ws_conn, hello);
  g_free (hello);

  return TRUE;
}

static gboolean
cleanup_and_quit_loop (const gchar * msg, enum AppState state)
{
  if (msg)
    gst_printerr ("%s\n", msg);
  if (state > 0)
    app_state = state;

  if (ws_conn) {
    if (soup_websocket_connection_get_state (ws_conn) ==
        SOUP_WEBSOCKET_STATE_OPEN)
      /* This will call us again */
      soup_websocket_connection_close (ws_conn, 1000, "");
    else
      g_clear_object (&ws_conn);
  }

  /* To allow usage as a GSourceFunc */
  return G_SOURCE_REMOVE;
}

static gchar *
get_string_from_json_object (JsonObject * object)
{
  JsonNode *root;
  JsonGenerator *generator;
  gchar *text;

  /* Make it the root node */
  root = json_node_init_object (json_node_alloc (), object);
  generator = json_generator_new ();
  json_generator_set_root (generator, root);
  text = json_generator_to_data (generator, NULL);

  /* Release everything */
  g_object_unref (generator);
  json_node_free (root);
  return text;
}

static void
data_channel_on_error (GObject * dc, gpointer user_data)
{
  cleanup_and_quit_loop ("Data channel error", 0);
}

static void
data_channel_on_open (GObject * dc, gpointer user_data)
{
  GBytes *bytes = g_bytes_new ("data", strlen ("data"));
  gst_print ("data channel opened\n");
  g_signal_emit_by_name (dc, "send-string", "Hi! from GStreamer");
  g_signal_emit_by_name (dc, "send-data", bytes);
  g_bytes_unref (bytes);
}

static void
data_channel_on_close (GObject * dc, gpointer user_data)
{
  cleanup_and_quit_loop ("Data channel closed", 0);
}

static void
data_channel_on_message_string (GObject * dc, gchar * str, gpointer user_data)
{
  gst_print ("Received data channel message: %s\n", str);
}

static void
connect_data_channel_signals (GObject * data_channel)
{
  g_signal_connect (data_channel, "on-error",
      G_CALLBACK (data_channel_on_error), NULL);
  g_signal_connect (data_channel, "on-open", G_CALLBACK (data_channel_on_open),
      NULL);
  g_signal_connect (data_channel, "on-close",
      G_CALLBACK (data_channel_on_close), NULL);
  g_signal_connect (data_channel, "on-message-string",
      G_CALLBACK (data_channel_on_message_string), NULL);
}

static void
on_data_channel (GstElement * webrtc, GObject * data_channel,
    gpointer user_data)
{
  PeerStruct *ps = user_data;
  connect_data_channel_signals (data_channel);
  ps->receive_channel = data_channel;
}

static void
send_sdp_to_peer (GstWebRTCSessionDescription * desc)
{
  gchar *text;
  JsonObject *msg, *sdp;

  if (app_state < PEER_CALL_NEGOTIATING) {
    cleanup_and_quit_loop ("Can't send SDP to peer, not in call",
        APP_STATE_ERROR);
    return;
  }

  text = gst_sdp_message_as_text (desc->sdp);
  sdp = json_object_new ();

  if (desc->type == GST_WEBRTC_SDP_TYPE_OFFER) {
    gst_print ("Sending offer:\n%s\n", text);
    json_object_set_string_member (sdp, "type", "offer");
  } else if (desc->type == GST_WEBRTC_SDP_TYPE_ANSWER) {
    gst_print ("Sending answer:\n%s\n", text);
    json_object_set_string_member (sdp, "type", "answer");
  } else {
    g_assert_not_reached ();
  }

  json_object_set_string_member (sdp, "sdp", text);
  g_free (text);

  msg = json_object_new ();
  json_object_set_object_member (msg, "sdp", sdp);
  text = get_string_from_json_object (msg);
  json_object_unref (msg);

  soup_websocket_connection_send_text (ws_conn, text);
  g_free (text);
}


static void
on_answer_created (GstPromise * promise, gpointer user_data)
{
  GstElement* webrtc = user_data;
  GstWebRTCSessionDescription *answer = NULL;
  const GstStructure *reply;

  g_assert_cmphex (app_state, ==, PEER_CALL_NEGOTIATING);

  g_assert_cmphex (gst_promise_wait (promise), ==, GST_PROMISE_RESULT_REPLIED);
  reply = gst_promise_get_reply (promise);
  gst_structure_get (reply, "answer",
      GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, NULL);
  gst_promise_unref (promise);

  promise = gst_promise_new ();
  g_print("--> 4\n");
  g_signal_emit_by_name (webrtc, "set-local-description", answer, promise);
  gst_promise_interrupt (promise);
  gst_promise_unref (promise);

  /* Send answer to peer */
  send_sdp_to_peer (answer);
  gst_webrtc_session_description_free (answer);
}


static void
on_offer_set (GstPromise * promise, gpointer user_data)
{
  GstElement* webrtc = user_data;
  gst_promise_unref (promise);
  promise = gst_promise_new_with_change_func (on_answer_created, webrtc, NULL);
  g_print("--> 3\n");
  g_signal_emit_by_name (webrtc, "create-answer", NULL, promise);
}

static void
on_offer_received (GstSDPMessage * sdp, GstElement* webrtc)
{
  GstWebRTCSessionDescription *offer = NULL;
  GstPromise *promise;

  offer = gst_webrtc_session_description_new (GST_WEBRTC_SDP_TYPE_OFFER, sdp);
  g_assert_nonnull (offer);

  /* Set remote description on our pipeline */
  {
    promise = gst_promise_new_with_change_func (on_offer_set, webrtc, NULL);
    g_print("--> 2 %x \n",webrtc);
    g_signal_emit_by_name (webrtc, "set-remote-description", offer, promise);
  }
  gst_webrtc_session_description_free (offer);
}

gboolean vrsmsz_add_stream(gpointer data){
   gchar name[1024];
   message_t* msg = data;
   gchar* uri = msg->command.in_url;
   vrstream_t* vs = NULL;
   vrchan_t* vc=NULL;

   if(vrsmsz->stream_nbs > 16) {
     g_print("sorry streams is full\n");
     return FALSE;
   }
   /**********************dispatch stream id ***********************/
   g_print("start pull stream uri= %s\n",uri);
   int i = 0;
   for(i=0; i<MAX_CHANNEL; i++){
       if(vrsmsz->streams_id[i] == -1){
          vrsmsz->streams_id[i] = i;
          vc = vrsmsz->streams + vrsmsz->streams_id[i];
          vc->stream_id = vrsmsz->streams_id[i];
	  break;
	}
   }

   if(!vc) return FALSE;
   if(strncmp(uri,"rtmp",4)) {msg->vc = vc;vc->stream_id = -1;vrsmsz->streams_id[i] = -1;msg->errcode=-1;return FALSE;}
   vc->video_id = vc->stream_id;
   vc->audio_id = vc->stream_id;
   sprintf(vc->in_url, "%s", uri);
   vc->resolution = 2;
   vc->status = 1;

   g_print("Stream_id is %d\n", vc->stream_id);
   sprintf(name,"chan-%d-bin",vc->stream_id);

   vs = &vc->vs;
   vs->bin = gst_bin_new(name);

   if(!vs->uridecodebin){
     sprintf(name,"vs%d-uridecodebin",vc->video_id);
     vs->uridecodebin = gst_element_factory_make("uridecodebin", name);
     g_object_set (vs->uridecodebin,"uri", uri, "expose-all-streams", TRUE, NULL);
     g_signal_connect (vs->uridecodebin, "pad-added", (GCallback) _pad_added_cb, vc);

     g_signal_connect (vs->uridecodebin, "autoplug-select", (GCallback) _autoplug_select_cb, vc);

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
    g_object_set(vs->vdec_tee_queue,"min-threshold-time", 0,NULL);
    g_object_set(vs->vdec_tee_queue,"max-size-time",      0,NULL);
    g_object_set(vs->vdec_tee_queue,"max-size-buffers",   0,          NULL);
    g_object_set(vs->vdec_tee_queue,"max-size-bytes",     0,          NULL);
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
      "video/x-raw, width=1280, height=720");
   }
#if 0
   if(!vs->video_encoder){
     sprintf(name,"vs%d-video_encoder",vc->video_id);
     vs->video_encoder = gst_element_factory_make("x264enc", name);
    if(!vs->video_encoder){
      g_print("error make\n");
      return FALSE;
    }
    g_object_set (vs->video_encoder, "byte-stream", TRUE, "key-int-max", 30, "speed-preset",1,NULL);
   }
#else
  if(!vs->video_encoder){
     sprintf(name,"vs%d-video_encoder",vc->video_id);
     if(vrsmsz->mode == 4 || vrsmsz->mode==8){
        vs->video_encoder= gst_element_factory_make("nvh264enc", name); // 
        if(!vs->video_encoder){
          g_print("error make\n");
          return FALSE;
        }
        g_object_set (vs->video_encoder, "preset", 4, "bitrate", 1000, NULL);
     }
  }
#if 0
  if(!vs->video_enccapsfilter){
     sprintf(name,"vs%d-video_enccapsfilter",vc->video_id);
     vs->video_enccapsfilter = gst_element_factory_make("capsfilter", name);
    if(!vs->video_enccapsfilter){
      g_print("error make\n");
      return FALSE;
    }
     gst_util_set_object_arg (G_OBJECT (vs->video_enccapsfilter), "caps",
      "video/x-h264, profile=high,framerate=29/1");
  }
#endif
  if(!vs->venc_tee){
     sprintf(name,"vs%d-venc_tee",vc->video_id);
     if(vrsmsz->mode == 4 || vrsmsz->mode==8){
        vs->venc_tee= gst_element_factory_make("tee", name); // 
        if(!vs->venc_tee){
          g_print("error make\n");
          return FALSE;
        }
     }
  }

  if(!vs->video_encoder_queue){
     sprintf(name,"vs%d-video_encoder_queue",vc->video_id);
    vs->video_encoder_queue= gst_element_factory_make("queue", name);
    if(!vs->video_encoder_queue){
      g_print("error make\n");
      return FALSE;
    }
  }

  if(!vs->video_encoder_parser){
         sprintf(name,"vs%d-video_encoder_parser",vc->video_id);
         vs->video_encoder_parser= gst_element_factory_make("h264parse", name); // nvh264enc have no avc
        if(!vs->video_encoder_parser){
          g_print("error make\n");
          return FALSE;
        }
  }
#endif
   if(!vs->audio_convert){
     sprintf(name,"vs%d-audio_convert",vc->audio_id);
     vs->audio_convert = gst_element_factory_make("audioconvert", name);
    if(!vs->audio_convert){
      g_print("error make\n");
      return FALSE;
    }
   }
#if 0
   if(!vs->audio_encoder){
     sprintf(name,"vs%d-audio_encoder",vc->audio_id);
     vs->audio_encoder = gst_element_factory_make("voaacenc", name);
    if(!vs->audio_encoder){
      g_print("error make\n");
      return FALSE;
    }
#else
   if(!vs->audio_encoder){
     sprintf(name,"vs%d-audio_encoder",vc->audio_id);
     vs->audio_encoder = gst_element_factory_make("opusenc", name);
    if(!vs->audio_encoder){
      g_print("error make\n");
      return FALSE;
    }
   }
#endif
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
     g_object_set(vs->aenc_tee_queue,"min-threshold-time", 0,NULL);
     g_object_set(vs->aenc_tee_queue,"max-size-time",      0,NULL);
     g_object_set(vs->aenc_tee_queue,"max-size-buffers",   0,          NULL);
     g_object_set(vs->aenc_tee_queue,"max-size-bytes",     0,          NULL);
   }
#if 0
   if(!vs->muxer){
     sprintf(name,"vs%d-muxer",vc->video_id);
     vs->muxer = gst_element_factory_make("flvmux", name);
     if(!vs->muxer){
       g_print("error make\n");
       return FALSE;
     }
     g_object_set (vs->muxer, "encoder", "LetinVR 1.0.0", NULL);
     g_object_set (vs->muxer, "metadatacreator", "LetinVR 1.0.0", NULL);
   }
   if(!vs->outer){
     sprintf(name,"vs%d-outer",vc->video_id);
     vs->outer= gst_element_factory_make("rtmp2sink", name);
     if(!vs->outer){
       g_print("error make\n");
       return FALSE;
     }
     g_print("preview put uri = %s\n",vc->preview_url);
     g_object_set (vs->outer, "location", vc->preview_url, NULL);
   }
#else
   if(!vs->audio_encoder_parser){
     sprintf(name,"vs%d-audio_encoder_parser",vc->video_id);
     vs->audio_encoder_parser= gst_element_factory_make("opusparse", name);
     if(!vs->audio_encoder_parser){
       g_print("error make\n");
       return FALSE;
     }
   }
   if(!vs->muxer){
     sprintf(name,"vs%d-muxer",vc->video_id);
     vs->muxer = gst_element_factory_make("mpegtsmux", name);
     if(!vs->muxer){
       g_print("error make\n");
       return FALSE;
     }
   }
   if(!vs->outer){
     sprintf(name,"vs%d-outer",vc->video_id);
     vs->outer= gst_element_factory_make("udpsink", name);
     if(!vs->outer){
       g_print("error make\n");
       return FALSE;
     }
     g_object_set (vs->outer, "host", "127.0.0.1", NULL);
     g_object_set (vs->outer, "port", 12346, NULL);
   }
#endif
   gst_bin_add_many(GST_BIN(vs->bin), vs->uridecodebin, vs->vdec_tee, vs->vdec_tee_queue,vs->video_scale, vs->video_capsfilter, vs->video_encoder, /*vs->video_enccapsfilter, */vs->venc_tee,vs->video_encoder_queue,vs->video_encoder_parser, vs->audio_convert, vs->audio_encoder,vs->audio_encoder_parser, vs->aenc_tee, vs->aenc_tee_queue, vs->muxer,vs->outer,NULL);
   gst_bin_add(GST_BIN(vrsmsz->pipeline),vs->bin);

   if(!gst_element_link_many(vs->vdec_tee, vs->vdec_tee_queue, vs->video_scale, vs->video_capsfilter, vs->video_encoder, /*vs->video_enccapsfilter, */vs->venc_tee,vs->video_encoder_queue,vs->video_encoder_parser, vs->muxer,vs->outer,NULL)){
      g_print("push link failed\n");
      return FALSE;
   }

   if(!gst_element_link_many(vs->audio_convert,vs->audio_encoder,vs->audio_encoder_parser,vs->aenc_tee, vs->aenc_tee_queue, vs->muxer,NULL)){
      g_print("push link failed\n");
      return FALSE;
   }

   (vrsmsz->stream_nbs)++;
   vrsmsz_play();

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
   
   msg->vc = vc;
   vc->ps.src_id = vc->stream_id;
   sprintf(vc->ps.our_id,"%d",vc->ps.src_id);
   register_with_server (vc);

   return FALSE;
}

gboolean  vrsmsz_remove_all(){
   vrchan_t* vc;
   vrstream_t* vs;
   gst_element_set_state(vrsmsz->pipeline,GST_STATE_NULL); // sink will not deadlock
   for(int i=0; i<MAX_CHANNEL; i++){
       if( vrsmsz->streams_id[i] != -1 ){
          vc = vrsmsz->streams + vrsmsz->streams_id[i];
          vc->tracks = 0;
          vc->video_id = -1;
          vc->audio_id = -1;
          memset(vc->in_url,0,sizeof(vc->in_url));
   //memset(vc->preview_url,0,sizeof(vc->preview_url));
          vc->resolution = -1;
          vc->stream_id = -1;

	  vs = &(vc->vs);
          gst_element_set_state(vs->bin,GST_STATE_NULL); // sink will not deadlock
          gst_bin_remove_many(GST_BIN (vrsmsz->pipeline),vs->bin,NULL);
          vs->uridecodebin=NULL,vs->vdec_tee=NULL,vs->vdec_tee_queue=NULL,vs->video_scale=NULL,vs->video_capsfilter=NULL, vs->video_encoder=NULL,vs->venc_tee=NULL,vs->video_encoder_queue=NULL,vs->video_encoder_parser=NULL,vs->audio_convert=NULL,vs->audio_encoder=NULL,vs->aenc_tee=NULL,vs->aenc_tee_queue=NULL,vs->muxer=NULL,vs->outer=NULL;
	  vs->bin = NULL;
	  vrsmsz->streams_id[i] = -1; 
	}
   }

   if(vrsmsz->director.pub_bin){
	   g_print("remove pub bin");
     gst_element_set_state(vrsmsz->director.pub_bin,GST_STATE_NULL); // sink will not deadlock
     gst_bin_remove_many(GST_BIN (vrsmsz->pipeline),vrsmsz->director.pub_bin,NULL);
     vrsmsz->director.pub_bin = NULL;
     vrsmsz->director.ds.pub_video_filter_tee_srcpad = NULL;
     vrsmsz->director.ds.pub_vdec_tee_queue= NULL; // points
     vrsmsz->director.ds.pub_vdec_tee_queue_sinkpad= NULL;
     vrsmsz->director.ds.pub_vdec_tee_queue_ghost_sinkpad= NULL;
     vrsmsz->director.ds.pub_fade = NULL;
     vrsmsz->director.ds.pub_render1 = NULL;
     vrsmsz->director.ds.pub_render2 = NULL;
     vrsmsz->director.ds.pub_video_encoder= NULL;
     vrsmsz->director.ds.pub_video_encoder_queue= NULL;
     vrsmsz->director.ds.pub_video_encoder_parser= NULL;
     vrsmsz->director.ds.pub_aenc_tee_queue= NULL;
     vrsmsz->director.ds.pub_aenc_tee_queue_sinkpad= NULL;
     vrsmsz->director.ds.pub_aenc_tee_queue_ghost_sinkpad= NULL;
     vrsmsz->director.ds.pub_muxer= NULL;
     vrsmsz->director.ds.pub_outer= NULL; // 
     vrsmsz->director.ds.mixer= NULL; // audio mix
   }
   if(vrsmsz->director.pre_bin){
	g_print("remove pre bin");
     gst_element_set_state(vrsmsz->director.pre_bin,GST_STATE_NULL); // sink will not deadlock
     gst_bin_remove_many(GST_BIN (vrsmsz->pipeline),vrsmsz->director.pre_bin,NULL);
     vrsmsz->director.pre_bin = NULL;
     vrsmsz->director.ds.pre_video_filter_tee_srcpad = NULL;
     vrsmsz->director.ds.pre_vdec_tee_queue = NULL; // points
     vrsmsz->director.ds.pre_vdec_tee_queue_sinkpad = NULL;
     vrsmsz->director.ds.pre_vdec_tee_queue_ghost_sinkpad = NULL;
     vrsmsz->director.ds.pre_render1= NULL;
     vrsmsz->director.ds.pre_render2= NULL;
     vrsmsz->director.ds.pre_video_scale = NULL;
     vrsmsz->director.ds.pre_capsfilter = NULL;
     vrsmsz->director.ds.pre_video_encoder = NULL;
     vrsmsz->director.ds.pre_video_encoder_queue = NULL;
     vrsmsz->director.ds.pre_video_encoder_parser = NULL;
     vrsmsz->director.ds.pre_aenc_tee_queue = NULL;
     vrsmsz->director.ds.pre_aenc_tee_queue_sinkpad = NULL;
     vrsmsz->director.ds.pre_aenc_tee_queue_ghost_sinkpad = NULL;
     vrsmsz->director.ds.pre_muxer = NULL;
     vrsmsz->director.ds.pre_outer = NULL; // 
   }
   vrsmsz->stream_nbs = 0;
   vrsmsz->director.stream_id = -1;
   gst_debug_bin_to_dot_file (GST_BIN_CAST(vrsmsz->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");
}
/*******************************************************************************/
gboolean vrsmsz_remove_stream(gpointer data){
   message_t* msg = data;
   gint streamid = atoi(msg->command.stream_id);
   vrchan_t* vc = NULL;
   vrstream_t* vs = NULL;

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
   vs->uridecodebin=NULL,vs->vdec_tee=NULL,vs->vdec_tee_queue=NULL,vs->video_scale=NULL,vs->video_capsfilter=NULL, vs->video_encoder=NULL,vs->venc_tee=NULL,vs->video_encoder_queue=NULL;vs->video_encoder_parser=NULL;vs->audio_convert=NULL,vs->audio_encoder=NULL,vs->aenc_tee=NULL,vs->aenc_tee_queue=NULL,vs->muxer=NULL,vs->outer=NULL;
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
gboolean director_publish_create(gchar* url){
  gchar name[1024];
  if(!vrsmsz->director.publish_url[0]){
    g_print("publish url = NULL\n");
  }
  memcpy(vrsmsz->director.publish_url, url, strlen(url)+1);

  if(!vrsmsz->director.ds.pub_vdec_tee_queue){
     sprintf(name,"%s-pub_vdec_tee_queue","vrsmsz");
    vrsmsz->director.ds.pub_vdec_tee_queue= gst_element_factory_make("queue", name);
    if(!vrsmsz->director.ds.pub_vdec_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
  }
#ifdef TEST_RENDER
#ifdef TEST_FADE
  if(!vrsmsz->director.ds.pub_fade){
     sprintf(name,"%s-pub_fade","vrsmsz");
    vrsmsz->director.ds.pub_fade= gst_element_factory_make("videomixer", name);
    if(!vrsmsz->director.ds.pub_fade){
      g_print("error make\n");
      return FALSE;
    }
  }
#endif
  if(!vrsmsz->director.ds.pub_render1){
     sprintf(name,"%s-pub_render1","vrsmsz");
    vrsmsz->director.ds.pub_render1= gst_element_factory_make("gdkpixbufoverlay", name);
    if(!vrsmsz->director.ds.pub_render1){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->director.ds.pub_render2){
     sprintf(name,"%s-pub_render2","vrsmsz");
    vrsmsz->director.ds.pub_render2= gst_element_factory_make("gdkpixbufoverlay", name);
    if(!vrsmsz->director.ds.pub_render2){
      g_print("error make\n");
      return FALSE;
    }
  }
#endif
  if(!vrsmsz->director.ds.pub_video_encoder){
     sprintf(name,"%s-pub_video_encoder","vrsmsz");
     if(vrsmsz->mode == 4 ){
        vrsmsz->director.ds.pub_video_encoder= gst_element_factory_make("nvh264enc", name); // nvh264enc have no avc
        //vrsmsz->director.ds.pub_video_encoder= gst_element_factory_make("nvh264device1enc", name); // nvh264enc have no avc
        if(!vrsmsz->director.ds.pub_video_encoder){
          g_print("error make\n");
          return FALSE;
        }
        g_object_set (vrsmsz->director.ds.pub_video_encoder, "preset", 4, "bitrate", 10000, NULL);
     }else if(vrsmsz->mode==8){
        vrsmsz->director.ds.pub_video_encoder= gst_element_factory_make("nvh265enc", name); // nvh264enc have no avc
        //vrsmsz->director.ds.pub_video_encoder= gst_element_factory_make("nvh265device1enc", name); // nvh264enc have no avc
        if(!vrsmsz->director.ds.pub_video_encoder){
          g_print("error make\n");
          return FALSE;
        }
        g_object_set (vrsmsz->director.ds.pub_video_encoder, "preset", 1, "bitrate", 80000, "gop-size",30,NULL);
     }else{
        vrsmsz->director.ds.pub_video_encoder= gst_element_factory_make("x264enc", name); // nvh264enc have no avc
        if(!vrsmsz->director.ds.pub_video_encoder){
          g_print("error make\n");
          return FALSE;
        }
        g_object_set (vrsmsz->director.ds.pub_video_encoder, "byte-stream", TRUE, "key-int-max", 25, NULL);
     }
  }

  if(!vrsmsz->director.ds.pub_video_encoder_queue){
     sprintf(name,"%s-pub_video_encoder_queue","vrsmsz");
    vrsmsz->director.ds.pub_video_encoder_queue= gst_element_factory_make("queue", name);
    if(!vrsmsz->director.ds.pub_video_encoder_queue){
      g_print("error make\n");
      return FALSE;
    }
  }

  if(vrsmsz->mode == 8){
    if(!vrsmsz->director.ds.pub_video_encoder_parser){
         sprintf(name,"%s-pub_video_encoder_parser","vrsmsz");
         vrsmsz->director.ds.pub_video_encoder_parser= gst_element_factory_make("h265parse", name); // nvh264enc have no avc
        if(!vrsmsz->director.ds.pub_video_encoder_parser){
          g_print("error make\n");
          return FALSE;
        }
    }
  }else{
    if(!vrsmsz->director.ds.pub_video_encoder_parser){
         sprintf(name,"%s-pub_video_encoder_parser","vrsmsz");
         vrsmsz->director.ds.pub_video_encoder_parser= gst_element_factory_make("h264parse", name); // nvh264enc have no avc
        if(!vrsmsz->director.ds.pub_video_encoder_parser){
          g_print("error make\n");
          return FALSE;
        }
    }
  }

  if(!vrsmsz->director.ds.pub_aenc_tee_queue){
     sprintf(name,"%s-pub_aenc_tee_queue","vrsmsz");
    vrsmsz->director.ds.pub_aenc_tee_queue= gst_element_factory_make("queue", name);
    if(!vrsmsz->director.ds.pub_aenc_tee_queue){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(vrsmsz->mode == 8){
     if(!vrsmsz->director.ds.pub_muxer){
       sprintf(name,"%s-pub_muxer","vrsmsz");
      vrsmsz->director.ds.pub_muxer= gst_element_factory_make("mpegtsmux", name);
      if(!vrsmsz->director.ds.pub_muxer){
        g_print("error make\n");
        return FALSE;
      }
    }
    if(!vrsmsz->director.ds.pub_outer){
       sprintf(name,"%s-pub_outer","vrsmsz");
      vrsmsz->director.ds.pub_outer= gst_element_factory_make("hlssink", name);
      //vrsmsz->director.ds.pub_outer= gst_element_factory_make("udpsink", name);
      if(!vrsmsz->director.ds.pub_outer){
        g_print("error make\n");
        return FALSE;
      }
      //g_object_set (vrsmsz->director.ds.pub_outer,"host","192.168.1.32","port",12346,NULL);
      g_object_set (vrsmsz->director.ds.pub_outer, "location", "/tmp/hls/%05d.ts", "playlist-length", 6, "playlist-location","/tmp/hls/publish.m3u8","max-files",10,"target-duration",3,NULL);
   }   
  }else{
     if(!vrsmsz->director.ds.pub_muxer){
      sprintf(name,"%s-pub_muxer","vrsmsz");
      vrsmsz->director.ds.pub_muxer= gst_element_factory_make("flvmux", name);
      if(!vrsmsz->director.ds.pub_muxer){
        g_print("error make\n");
        return FALSE;
      }
     g_object_set (vrsmsz->director.ds.pub_muxer, "encoder", "LetinVR 1.0.0", NULL);
     g_object_set (vrsmsz->director.ds.pub_muxer, "metadatacreator", "LetinVR 1.0.0", NULL);
    }
    if(!vrsmsz->director.ds.pub_outer){
      sprintf(name,"%s-pub_outer","vrsmsz");
      vrsmsz->director.ds.pub_outer= gst_element_factory_make("rtmp2sink", name);
      if(!vrsmsz->director.ds.pub_outer){
        g_print("error make\n");
        return FALSE;
      }
      g_object_set (vrsmsz->director.ds.pub_outer, "location", vrsmsz->director.publish_url, NULL);
    }
  }
  vrsmsz->director.pub_bin = gst_bin_new("pub_bin");
  gst_bin_add_many(GST_BIN(vrsmsz->director.pub_bin), vrsmsz->director.ds.pub_vdec_tee_queue,
#ifdef TEST_FADE
                          vrsmsz->director.ds.pub_fade,
#endif 
#ifdef TEST_RENDER 
                          vrsmsz->director.ds.pub_render1,vrsmsz->director.ds.pub_render2,
#endif 
		  vrsmsz->director.ds.pub_video_encoder,vrsmsz->director.ds.pub_video_encoder_queue,vrsmsz->director.ds.pub_video_encoder_parser,vrsmsz->director.ds.pub_aenc_tee_queue,vrsmsz->director.ds.pub_muxer,vrsmsz->director.ds.pub_outer, NULL);
  gst_bin_add(GST_BIN(vrsmsz->pipeline),vrsmsz->director.pub_bin);

  if(!gst_element_link_many(vrsmsz->director.ds.pub_vdec_tee_queue, 
#ifdef TEST_FADE
                          vrsmsz->director.ds.pub_fade,
#endif 
#ifdef TEST_RENDER 
                          vrsmsz->director.ds.pub_render1,vrsmsz->director.ds.pub_render2,
#endif 
			  vrsmsz->director.ds.pub_video_encoder, vrsmsz->director.ds.pub_video_encoder_queue, vrsmsz->director.ds.pub_video_encoder_parser, vrsmsz->director.ds.pub_muxer,vrsmsz->director.ds.pub_outer, NULL)){
    g_print("link director pub video failed");
    return FALSE;
  }

  GstCaps *caps = gst_caps_from_string("audio/mpeg");
  if(!gst_element_link_filtered(vrsmsz->director.ds.pub_aenc_tee_queue, vrsmsz->director.ds.pub_muxer, caps)){
    g_print("link director pub audio failed");
    return FALSE;
  }
 
  return FALSE;
}
/*****************************************************************************************************/

void director_publish_link_vs(vrstream_t* vsv, vrstream_t* vsa){
  GstPadLinkReturn ret;
  // audio 
  if(vsa){
  if(!vsa->pub_aenc_tee_srcpad){
      vsa->pub_aenc_tee_srcpad = gst_element_get_request_pad (vsa->aenc_tee, "src_%u");
      if(!vsa->pub_aenc_tee_srcpad)
	g_print ("pub aenc failed. \n");
  }
  if(!vrsmsz->director.ds.pub_aenc_tee_queue_sinkpad){
      vrsmsz->director.ds.pub_aenc_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->director.ds.pub_aenc_tee_queue, "sink");
      if(!vrsmsz->director.ds.pub_aenc_tee_queue_sinkpad)
	g_print ("pub aenc tee queue failed. \n");
  }
  if(!vsa->pub_aenc_tee_ghost_srcpad){
     vsa->pub_aenc_tee_ghost_srcpad = gst_ghost_pad_new ("pub-aenc-tee-srcpad", vsa->pub_aenc_tee_srcpad);
     gst_element_add_pad (vsa->bin, vsa->pub_aenc_tee_ghost_srcpad);
  }

  if(!vrsmsz->director.ds.pub_aenc_tee_queue_ghost_sinkpad){
    vrsmsz->director.ds.pub_aenc_tee_queue_ghost_sinkpad = gst_ghost_pad_new ("pub-aenc-tee-queue-sinkpad", vrsmsz->director.ds.pub_aenc_tee_queue_sinkpad);
    gst_element_add_pad (vrsmsz->director.pub_bin,vrsmsz->director.ds.pub_aenc_tee_queue_ghost_sinkpad);
  }
  ret = gst_pad_link (vsa->pub_aenc_tee_ghost_srcpad,vrsmsz->director.ds.pub_aenc_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
     g_print ("pub aenc tee Link failed.\n");
  } else {
     g_print ("pub Link succeeded .\n");
  }
 }
  // video
  if(vsv){
  if(!vsv->pub_vdec_tee_srcpad)
    vsv->pub_vdec_tee_srcpad = gst_element_get_request_pad (vsv->vdec_tee, "src_%u");
  if(!vrsmsz->director.ds.pub_vdec_tee_queue_sinkpad)
    vrsmsz->director.ds.pub_vdec_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->director.ds.pub_vdec_tee_queue, "sink");

  if(!vrsmsz->director.ds.pub_vdec_tee_queue_ghost_sinkpad){
    vrsmsz->director.ds.pub_vdec_tee_queue_ghost_sinkpad = gst_ghost_pad_new("pub-vdec-tee-queue-sinkpad", vrsmsz->director.ds.pub_vdec_tee_queue_sinkpad);
    gst_element_add_pad( vrsmsz->director.pub_bin,vrsmsz->director.ds.pub_vdec_tee_queue_ghost_sinkpad );
  }

  vsv->pub_vdec_tee_ghost_srcpad = gst_ghost_pad_new ( "pub-vdec-tee-srcpad", vsv->pub_vdec_tee_srcpad );
  gst_element_add_pad (vsv->bin, vsv->pub_vdec_tee_ghost_srcpad);
  ret = gst_pad_link(vsv->pub_vdec_tee_ghost_srcpad,vrsmsz->director.ds.pub_vdec_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
    g_print ("pub vdec tee Link failed.\n");
  } else {
    g_print ("pub Link succeeded .\n");
  }
  }
  vrsmsz_set_play(vrsmsz->director.pub_bin); // this must be used
  vrsmsz_play();
  gst_debug_bin_to_dot_file (GST_BIN_CAST(vrsmsz->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");
}

/*****************************************************************************************************/
void director_preview_link_vs(vrstream_t* vsv,vrstream_t* vsa){
  GstPadLinkReturn ret;
  // audio 
  if(vsa){
  if(!vsa->pre_aenc_tee_srcpad){
      vsa->pre_aenc_tee_srcpad = gst_element_get_request_pad (vsa->aenc_tee, "src_%u");
      if(!vsa->pre_aenc_tee_srcpad)
	g_print ("pre aenc failed. \n");
  }
  if(!vrsmsz->director.ds.pre_aenc_tee_queue_sinkpad){
      vrsmsz->director.ds.pre_aenc_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->director.ds.pre_aenc_tee_queue, "sink");
      if(!vrsmsz->director.ds.pre_aenc_tee_queue_sinkpad)
	g_print ("pre aenc tee queue failed. \n");
  }
  if(!vsa->pre_aenc_tee_ghost_srcpad){
     vsa->pre_aenc_tee_ghost_srcpad = gst_ghost_pad_new ("pre-aenc-tee-srcpad", vsa->pre_aenc_tee_srcpad);
     gst_element_add_pad (vsa->bin, vsa->pre_aenc_tee_ghost_srcpad);
  }

  if(!vrsmsz->director.ds.pre_aenc_tee_queue_ghost_sinkpad){
    vrsmsz->director.ds.pre_aenc_tee_queue_ghost_sinkpad = gst_ghost_pad_new ("pre-aenc-tee-queue-sinkpad", vrsmsz->director.ds.pre_aenc_tee_queue_sinkpad);
    gst_element_add_pad (vrsmsz->director.pre_bin,vrsmsz->director.ds.pre_aenc_tee_queue_ghost_sinkpad);
  }
  ret = gst_pad_link (vsa->pre_aenc_tee_ghost_srcpad,vrsmsz->director.ds.pre_aenc_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
     g_print ("pre aenc tee Link failed.\n");
  } else {
     g_print ("pre Link succeeded .\n");
  }
  }
  // video
  if(vsv){
  if(!vsv->pre_vdec_tee_srcpad)
    vsv->pre_vdec_tee_srcpad = gst_element_get_request_pad (vsv->vdec_tee, "src_%u");
  if(!vrsmsz->director.ds.pre_vdec_tee_queue_sinkpad)
    vrsmsz->director.ds.pre_vdec_tee_queue_sinkpad = gst_element_get_static_pad (vrsmsz->director.ds.pre_vdec_tee_queue, "sink");

  if(!vrsmsz->director.ds.pre_vdec_tee_queue_ghost_sinkpad){
    vrsmsz->director.ds.pre_vdec_tee_queue_ghost_sinkpad = gst_ghost_pad_new ("pre-vdec-tee-queue-sinkpad", vrsmsz->director.ds.pre_vdec_tee_queue_sinkpad);
    gst_element_add_pad (vrsmsz->director.pre_bin,vrsmsz->director.ds.pre_vdec_tee_queue_ghost_sinkpad );
  }

  vsv->pre_vdec_tee_ghost_srcpad = gst_ghost_pad_new ("pre-vdec-tee-srcpad", vsv->pre_vdec_tee_srcpad );
  gst_element_add_pad (vsv->bin, vsv->pre_vdec_tee_ghost_srcpad);
  ret = gst_pad_link(vsv->pre_vdec_tee_ghost_srcpad, vrsmsz->director.ds.pre_vdec_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
    g_print ("pre vdec tee Link failed.\n");
  } else {
    g_print ("pre Link succeeded .\n");
  }
  }

  vrsmsz_set_play(vrsmsz->director.pre_bin); // this must be used
  vrsmsz_play();
  gst_debug_bin_to_dot_file (GST_BIN_CAST(vrsmsz->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");
}
/****************************************************************************************/
gboolean director_publish_unlink_vs(vrstream_t* vrv,vrstream_t* vra){
  GstPadLinkReturn ret;
  if(vra){
  ret = gst_pad_unlink(vra->pub_aenc_tee_ghost_srcpad, vrsmsz->director.ds.pub_aenc_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
     g_print ("unLink aenc tee ghost failed.\n");
  } else {
     g_print ("unLink succeeded .\n");
  }
  gst_element_release_request_pad(vra->aenc_tee,vra->pub_aenc_tee_srcpad);
  gst_element_release_request_pad(vra->bin,vra->pub_aenc_tee_ghost_srcpad);
  gst_object_unref(vra->pub_aenc_tee_srcpad);
  //gst_object_unref(vr->pre_aenc_tee_ghost_srcpad);
  vra->pub_aenc_tee_srcpad = NULL;
  vra->pub_aenc_tee_ghost_srcpad = NULL;
  }
  if(vrv){
  ret = gst_pad_unlink (vrv->pub_vdec_tee_ghost_srcpad,vrsmsz->director.ds.pub_vdec_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
    g_print ("unLink vdec tee failed.\n");
  } else {
    g_print ("unLink succeeded .\n");
  }
  gst_element_release_request_pad(vrv->vdec_tee,vrv->pub_vdec_tee_srcpad);
  gst_element_release_request_pad(vrv->bin,vrv->pub_vdec_tee_ghost_srcpad);
  gst_object_unref(vrv->pub_vdec_tee_srcpad);
  //gst_object_unref(vr->pre_vdec_tee_ghost_srcpad);
  vrv->pub_vdec_tee_srcpad= NULL;
  vrv->pub_vdec_tee_ghost_srcpad= NULL;
  }

   return FALSE;   
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
 #ifdef TEST_RENDER
  if(!vrsmsz->director.ds.pre_render1){
     sprintf(name,"%s-pre_render1","vrsmsz");
    vrsmsz->director.ds.pre_render1 = gst_element_factory_make("gdkpixbufoverlay", name);
    if(!vrsmsz->director.ds.pre_render1){
      g_print("error make\n");
      return FALSE;
    }
  }
  if(!vrsmsz->director.ds.pre_render2){
     sprintf(name,"%s-pre_render2","vrsmsz");
    vrsmsz->director.ds.pre_render2 = gst_element_factory_make("gdkpixbufoverlay", name);
    if(!vrsmsz->director.ds.pre_render2){
      g_print("error make\n");
      return FALSE;
    }
  }
#endif
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
      "video/x-raw, width=1280, height=720");
  }

  if(!vrsmsz->director.ds.pre_video_encoder){
     sprintf(name,"%s-pre_video_encoder","vrsmsz");
#ifndef X264_ENC
     if(vrsmsz->mode == 4 || vrsmsz->mode==8){
        vrsmsz->director.ds.pre_video_encoder= gst_element_factory_make("nvh264enc", name); // 
        //vrsmsz->director.ds.pre_video_encoder= gst_element_factory_make("nvh264device1enc", name); // nvh264enc have no avc
        if(!vrsmsz->director.ds.pre_video_encoder){
          g_print("error make\n");
          return FALSE;
        }
        g_object_set (vrsmsz->director.ds.pre_video_encoder, "preset", 4, "bitrate", 1000, NULL);

	
     }else 
#endif
        if(vrsmsz->mode == 4 || vrsmsz->mode==8){
        vrsmsz->director.ds.pre_video_encoder= gst_element_factory_make("x264enc", name); // nvh264enc have no avc
        if(!vrsmsz->director.ds.pre_video_encoder){
          g_print("error make\n");
          return FALSE;
        }
        g_object_set (vrsmsz->director.ds.pre_video_encoder, "byte-stream", TRUE, "key-int-max", 25, "speed-preset", 1,"bitrate", 2000, NULL);
     }
  }


  if(!vrsmsz->director.ds.pre_video_encoder_queue){
     sprintf(name,"%s-pre_video_encoder_queue","vrsmsz");
    vrsmsz->director.ds.pre_video_encoder_queue = gst_element_factory_make("queue", name);
    if(!vrsmsz->director.ds.pre_video_encoder_queue){
      g_print("error make\n");
      return FALSE;
    }
  }

  if(!vrsmsz->director.ds.pre_video_encoder_parser){
         sprintf(name,"%s-pre_video_encoder_parser","vrsmsz");
         vrsmsz->director.ds.pre_video_encoder_parser= gst_element_factory_make("h264parse", name); // 
        if(!vrsmsz->director.ds.pre_video_encoder_parser){
          g_print("error make\n");
          return FALSE;
        }
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
     g_object_set (vrsmsz->director.ds.pre_muxer, "encoder", "LetinVR 1.0.0", NULL);
     g_object_set (vrsmsz->director.ds.pre_muxer, "metadatacreator", "LetinVR 1.0.0", NULL);
  }
  if(!vrsmsz->director.ds.pre_outer){
     sprintf(name,"%s-pre_outer","vrsmsz");
    vrsmsz->director.ds.pre_outer= gst_element_factory_make("rtmp2sink", name);
    if(!vrsmsz->director.ds.pre_outer){
      g_print("error make\n");
      return FALSE;
    }
    g_object_set (vrsmsz->director.ds.pre_outer, "location", vrsmsz->director.preview_url, NULL);
  }
   vrsmsz->director.pre_bin = gst_bin_new("pre_bin");
  gst_bin_add_many(GST_BIN(vrsmsz->director.pre_bin), vrsmsz->director.ds.pre_vdec_tee_queue,
#ifdef TEST_RENDER 
			  vrsmsz->director.ds.pre_render1,vrsmsz->director.ds.pre_render2,
#endif
		  vrsmsz->director.ds.pre_video_scale, vrsmsz->director.ds.pre_capsfilter, vrsmsz->director.ds.pre_video_encoder, vrsmsz->director.ds.pre_video_encoder_queue,vrsmsz->director.ds.pre_video_encoder_parser, vrsmsz->director.ds.pre_aenc_tee_queue, vrsmsz->director.ds.pre_muxer, vrsmsz->director.ds.pre_outer, NULL);
  gst_bin_add(GST_BIN(vrsmsz->pipeline),vrsmsz->director.pre_bin);

  if(!gst_element_link_many(vrsmsz->director.ds.pre_vdec_tee_queue, 
#ifdef TEST_RENDER 
			  vrsmsz->director.ds.pre_render1,vrsmsz->director.ds.pre_render2,
#endif
			  vrsmsz->director.ds.pre_video_scale, vrsmsz->director.ds.pre_capsfilter, vrsmsz->director.ds.pre_video_encoder,vrsmsz->director.ds.pre_video_encoder_queue, vrsmsz->director.ds.pre_video_encoder_parser, vrsmsz->director.ds.pre_muxer, vrsmsz->director.ds.pre_outer, NULL)){
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

    if(!gst_element_link_many(vrsmsz->director.ds.pre_vdec_tee_queue, 
#ifdef TEST_RENDER
                            vrsmsz->director.ds.pre_render1,vrsmsz->director.ds.pre_render2,
#endif
			    vrsmsz->director.ds.pre_video_scale, vrsmsz->director.ds.pre_capsfilter, vrsmsz->director.ds.pre_video_encoder, 
#if 1
			    vrsmsz->director.ds.pre_video_encoder_queue,vrsmsz->director.ds.pre_video_encoder_parser,
#endif
			    vrsmsz->director.ds.pre_muxer, vrsmsz->director.ds.pre_outer, NULL)){
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

gboolean director_preview_unlink_vs(vrstream_t* vrv,vrstream_t* vra){
  GstPadLinkReturn ret;
  if(vra){
  ret = gst_pad_unlink(vra->pre_aenc_tee_ghost_srcpad, vrsmsz->director.ds.pre_aenc_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
     g_print ("unLink aenc tee ghost failed.\n");
  } else {
     g_print ("unLink succeeded .\n");
  }
  gst_element_release_request_pad(vra->aenc_tee,vra->pre_aenc_tee_srcpad);
  gst_element_release_request_pad(vra->bin,vra->pre_aenc_tee_ghost_srcpad);
  gst_object_unref(vra->pre_aenc_tee_srcpad);
  //gst_object_unref(vr->pre_aenc_tee_ghost_srcpad);
  vra->pre_aenc_tee_srcpad = NULL;
  vra->pre_aenc_tee_ghost_srcpad = NULL;
  }

  if(vrv){
  ret = gst_pad_unlink (vrv->pre_vdec_tee_ghost_srcpad,vrsmsz->director.ds.pre_vdec_tee_queue_ghost_sinkpad);
  if (GST_PAD_LINK_FAILED (ret)) {
    g_print ("unLink vdec tee failed.\n");
  } else {
    g_print ("unLink succeeded .\n");
  }
  gst_element_release_request_pad(vrv->vdec_tee,vrv->pre_vdec_tee_srcpad);
  gst_element_release_request_pad(vrv->bin,vrv->pre_vdec_tee_ghost_srcpad);
  gst_object_unref(vrv->pre_vdec_tee_srcpad);
  //gst_object_unref(vr->pre_vdec_tee_ghost_srcpad);
  vrv->pre_vdec_tee_srcpad= NULL;
  vrv->pre_vdec_tee_ghost_srcpad= NULL;
  }

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
gboolean vrsmsz_publish_stop(){
   if(vrsmsz->director.stream_id == -1){
     g_print("no director streams is selector \n");
     return FALSE;
   }
   if(vrsmsz->stream_nbs == 0){
     g_print("the streams is null  \n");
     return FALSE;
   }
   vrchan_t *vcv = vrsmsz->streams + vrsmsz->director.video_id;
   vrchan_t *vca = vrsmsz->streams + vrsmsz->director.audio_id;
   vrsmsz->isPubSwitched = FALSE;
   director_publish_unlink_vs(&vcv->vs,&vca->vs);
   memset(vrsmsz->director.publish_url,0,URL_LEN);
   gst_element_set_state (vrsmsz->director.pub_bin, GST_STATE_NULL);
   gst_bin_remove_many(GST_BIN (vrsmsz->pipeline),vrsmsz->director.pub_bin,NULL);
   vrsmsz->director.ds.pub_video_filter_tee_srcpad = NULL;
   vrsmsz->director.ds.pub_vdec_tee_queue = NULL; 
   vrsmsz->director.ds.pub_vdec_tee_queue_sinkpad = NULL;
   vrsmsz->director.ds.pub_vdec_tee_queue_ghost_sinkpad = NULL;
   vrsmsz->director.ds.pub_fade = NULL;
   vrsmsz->director.ds.pub_render1 = NULL;
   vrsmsz->director.ds.pub_render2 = NULL;
   vrsmsz->director.ds.pub_video_encoder = NULL;
   vrsmsz->director.ds.pub_video_encoder_queue = NULL;
   vrsmsz->director.ds.pub_video_encoder_parser= NULL;
   vrsmsz->director.ds.pub_aenc_tee_queue = NULL;
   vrsmsz->director.ds.pub_aenc_tee_queue_sinkpad = NULL;
   vrsmsz->director.ds.pub_aenc_tee_queue_ghost_sinkpad = NULL;
   vrsmsz->director.ds.pub_muxer = NULL;
   vrsmsz->director.ds.pub_outer = NULL; 
   vrsmsz->director.pub_bin = NULL; 
   return FALSE;
}

gboolean vrsmsz_publish_stream (gpointer data){
   message_t* msg = data;
   gchar* uri = msg->command.pub_url;
   if(vrsmsz->director.stream_id == -1){
     g_print("no director streams is selector \n");
     return FALSE;
   }
   if(vrsmsz->stream_nbs == 0){
     g_print("the streams is null  \n");
     return FALSE;
   }
   g_print("current publish url:%s\n", vrsmsz->director.publish_url);
   if(!strncmp(vrsmsz->director.publish_url, uri, strlen(uri))){ // some question error
     g_print("the stream already published\n");
     return FALSE;
   }

   vrchan_t *vcv = vrsmsz->streams + vrsmsz->director.video_id;
   vrchan_t *vca = vrsmsz->streams + vrsmsz->director.audio_id;
   if(!vrsmsz->isPubSwitched ){ // first pub
     director_publish_create(uri);
     director_publish_link_vs(&(vcv->vs),&(vca->vs));
     vrsmsz->isPubSwitched= TRUE;
   }else{
     vrsmsz_publish_stop();
     director_publish_create(uri);
     director_publish_link_vs(&(vcv->vs),&(vca->vs));
   }
   if(vrsmsz->director.render[0].url[0] != 0){
     g_object_set(vrsmsz->director.ds.pub_render1,"location",vrsmsz->director.render[0].url,NULL);
     g_object_set(vrsmsz->director.ds.pub_render1,"offset-x",vrsmsz->director.render[0].left,NULL);
     g_object_set(vrsmsz->director.ds.pub_render1,"offset-y",vrsmsz->director.render[0].top,NULL);
   }
   if(vrsmsz->director.render[1].url[0] != 0){
     g_object_set(vrsmsz->director.ds.pub_render2,"location",vrsmsz->director.render[1].url,NULL);
     g_object_set(vrsmsz->director.ds.pub_render2,"offset-x",vrsmsz->director.render[1].left,NULL);
     g_object_set(vrsmsz->director.ds.pub_render2,"offset-y",vrsmsz->director.render[1].top,NULL);
   }
   return FALSE;
}
/*****************************************************************************************************/
gboolean vrsmsz_stream_delay(gpointer data){
  message_t* msg = data;
  gint streamid = atoi(msg->command.stream_id);
  gint msecs = msg->command.delay_time;

  gint videoid = atoi(msg->command.video_id);
  gint audioid = atoi(msg->command.audio_id);
  g_print("stream id %d videoid %d audioid %d delay %d ms \n",streamid,videoid ,audioid,msecs);

  if(vrsmsz->stream_nbs == 0){
    g_print("no stream \n");
    return FALSE;
  }
  if(streamid == vrsmsz->director.stream_id){
    g_print("Delaying in director stream is not permitted \n");
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
  vrstream_t* vs = &(vc->vs);
 
  if(videoid > 0 || videoid == 0){
     gst_pad_set_offset(vs->video_src_pad, msecs * 1000000);
     vs->video_time = msecs * 1000000;
     g_print("--> %lld\n",gst_pad_get_offset(vs->video_src_pad));
  }
  if(audioid > 0|| audioid == 0){
     gst_pad_set_offset(vs->audio_src_pad, msecs * 1000000);
     vs->audio_time = msecs * 1000000;
     g_print("--> %lld\n",gst_pad_get_offset(vs->audio_src_pad));
  }
  return FALSE;
  if(msecs < 0){
    if(videoid > 0){
    g_object_set(vs->vdec_tee_queue,"min-threshold-time", abs(msecs) * 1000000,NULL);
    g_object_set(vs->vdec_tee_queue,"max-size-time",      3000000000,NULL);
    g_object_set(vs->vdec_tee_queue,"max-size-buffers",   0,          NULL);
    g_object_set(vs->vdec_tee_queue,"max-size-bytes",     0,          NULL);
    }
    
    if(audioid > 0){
    g_object_set(vs->aenc_tee_queue,"min-threshold-time", abs(msecs) * 1000000,NULL);
    g_object_set(vs->aenc_tee_queue,"max-size-time",      3000000000,NULL);
    g_object_set(vs->aenc_tee_queue,"max-size-buffers",   0,          NULL);
    g_object_set(vs->aenc_tee_queue,"max-size-bytes",     0,          NULL);
    }
  }else{
    //gst_element_set_state (vs->bin, GST_STATE_READY);

    //if(videoid > 0){
    g_object_set(vs->vdec_tee_queue,"min-threshold-time", msecs * 1000000,NULL);
    g_object_set(vs->vdec_tee_queue,"max-size-time",      3000000000,NULL);
    g_object_set(vs->vdec_tee_queue,"max-size-buffers",   0,          NULL);
    g_object_set(vs->vdec_tee_queue,"max-size-bytes",     0,          NULL);
    //}
    
    //if(audioid > 0){
    g_object_set(vs->aenc_tee_queue,"min-threshold-time", msecs * 1000000,NULL);
    g_object_set(vs->aenc_tee_queue,"max-size-time",      3000000000,NULL);
    g_object_set(vs->aenc_tee_queue,"max-size-buffers",   0,          NULL);
    g_object_set(vs->aenc_tee_queue,"max-size-bytes",     0,          NULL);
    //}

    //gst_element_set_state (vs->bin, GST_STATE_PLAYING);
  }


  //g_object_set(vrsmsz->director.ds.pre_vdec_tee_queue,min-threshold-time,msecs * 1000000,NULL);
  //g_object_set(vrsmsz->director.ds.pre_aenc_tee_queue,min-threshold-time,msecs * 1000000,NULL);

  //g_object_set(vrsmsz->director.ds.pub_vdec_tee_queue,min-threshold-time,msecs * 1000000,NULL);
  //g_object_set(vrsmsz->director.ds.pub_aenc_tee_queue,min-threshold-time,msecs * 1000000,NULL);
  return FALSE;
}

/*****************************************************************************************************/
gboolean  vrsmsz_stream_logo(gpointer data){
  message_t* msg = data;
  gint streamid = atoi(msg->command.stream_id);
  
  g_print("logo process %d \n",streamid);
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

  if(vrsmsz->director.stream_id == -1){
    return FALSE;
  }
  if(!strcmp(msg->command.action, "add")){
    if(vrsmsz->director.num_render == 2) return FALSE;
    for(int i=0; i<2;i++){
       if(vrsmsz->director.render[i].id == -1){
	 sprintf(vrsmsz->director.render[i].url, "%s/%s",MEDIA_PATH,msg->command.logo_path);
         vrsmsz->director.render[i].left   = msg->command.left;
         vrsmsz->director.render[i].top    = msg->command.top;
         vrsmsz->director.render[i].width  = msg->command.width;
         vrsmsz->director.render[i].height = msg->command.height;
         vrsmsz->director.render[i].id = i;
	 msg->command.render_id = i;
         g_print("add logo render_id %d\n",i);
         break;
       }
    }
    vrsmsz->director.num_render++;
  }else if(!strcmp(msg->command.action, "update")){
    if(vrsmsz->director.num_render == 0) return FALSE;
    int i = msg->command.render_id ;
    g_print("update logo %d\n",i);
    if(i == -1){ msg->command.render_id = -2;return FALSE;}
    vrsmsz->director.render[msg->command.render_id].left   = msg->command.left;
    vrsmsz->director.render[msg->command.render_id].top    = msg->command.top;
    vrsmsz->director.render[msg->command.render_id].width  = msg->command.width;
    vrsmsz->director.render[msg->command.render_id].height = msg->command.height;
    g_object_set(vrsmsz->director.ds.pre_render1,"offset-x",vrsmsz->director.render[msg->command.render_id].left,NULL);
    g_object_set(vrsmsz->director.ds.pre_render1,"offset-y",vrsmsz->director.render[msg->command.render_id].top,NULL);
    g_object_set(vrsmsz->director.ds.pub_render1,"offset-x",vrsmsz->director.render[msg->command.render_id].left,NULL);
    g_object_set(vrsmsz->director.ds.pub_render1,"offset-y",vrsmsz->director.render[msg->command.render_id].top,NULL);

    g_object_set(vrsmsz->director.ds.pre_render2,"offset-x",vrsmsz->director.render[msg->command.render_id].left,NULL);
    g_object_set(vrsmsz->director.ds.pre_render2,"offset-y",vrsmsz->director.render[msg->command.render_id].top,NULL);
    g_object_set(vrsmsz->director.ds.pub_render2,"offset-x",vrsmsz->director.render[msg->command.render_id].left,NULL);
    g_object_set(vrsmsz->director.ds.pub_render2,"offset-y",vrsmsz->director.render[msg->command.render_id].top,NULL);
  }else{
    if(vrsmsz->director.num_render == 0) return FALSE;
    int i = msg->command.render_id ;
    g_print("delete logo %d\n",i);
    if(i == -1){ msg->command.render_id = -2;return FALSE;}
    memset(vrsmsz->director.render[i].url,0,URL_LEN);
    vrsmsz->director.render[i].left = 0;
    vrsmsz->director.render[i].top = 0;
    vrsmsz->director.render[i].width = 0;
    vrsmsz->director.render[i].height = 0;
    vrsmsz->director.render[i].duration = 0;
    vrsmsz->director.render[i].type = 0;
    vrsmsz->director.render[i].id = -1;
    if(i == 0 ){
      g_object_set(vrsmsz->director.ds.pre_render1,"location","/var/local/vrsmsz/image.jpg",NULL);
      g_object_set(vrsmsz->director.ds.pre_render1,"offset-x",vrsmsz->director.render[0].left,NULL);
      g_object_set(vrsmsz->director.ds.pre_render1,"offset-y",vrsmsz->director.render[0].top,NULL);

      g_object_set(vrsmsz->director.ds.pub_render1,"location","/var/local/vrsmsz/image.jpg",NULL);
      g_object_set(vrsmsz->director.ds.pub_render1,"offset-x",vrsmsz->director.render[0].left,NULL);
      g_object_set(vrsmsz->director.ds.pub_render1,"offset-y",vrsmsz->director.render[0].top,NULL);
      //g_object_set(vrsmsz->director.ds.pre_render1,"alpha",0,NULL);
      //g_object_set(vrsmsz->director.ds.pub_render1,"alpha",0,NULL);
    }else {
      g_object_set(vrsmsz->director.ds.pre_render2,"location","/var/local/vrsmsz/image.jpg",NULL);
      g_object_set(vrsmsz->director.ds.pre_render2,"offset-x",vrsmsz->director.render[0].left,NULL);
      g_object_set(vrsmsz->director.ds.pre_render2,"offset-y",vrsmsz->director.render[0].top,NULL);

      g_object_set(vrsmsz->director.ds.pub_render2,"location","/var/local/vrsmsz/image.jpg",NULL);
      g_object_set(vrsmsz->director.ds.pub_render2,"offset-x",vrsmsz->director.render[0].left,NULL);
      g_object_set(vrsmsz->director.ds.pub_render2,"offset-y",vrsmsz->director.render[0].top,NULL);
      //g_object_set(vrsmsz->director.ds.pre_render2,"alpha",0,NULL);
      //g_object_set(vrsmsz->director.ds.pub_render2,"alpha",0,NULL);
    }
    vrsmsz->director.num_render--;
  }
  
  if(vrsmsz->director.pre_bin){
      if(vrsmsz->director.render[0].url[0] != 0){
        g_object_set(vrsmsz->director.ds.pre_render1,"location",vrsmsz->director.render[0].url,NULL);
        g_object_set(vrsmsz->director.ds.pre_render1,"offset-x",vrsmsz->director.render[0].left,NULL);
        g_object_set(vrsmsz->director.ds.pre_render1,"offset-y",vrsmsz->director.render[0].top,NULL);
        //g_object_set(vrsmsz->director.ds.pre_render1,"alpha",1,NULL);
      }
      if(vrsmsz->director.render[1].url[0] != 0){
        g_object_set(vrsmsz->director.ds.pre_render2,"location",vrsmsz->director.render[1].url,NULL);
        g_object_set(vrsmsz->director.ds.pre_render2,"offset-x",vrsmsz->director.render[1].left,NULL);
        g_object_set(vrsmsz->director.ds.pre_render2,"offset-y",vrsmsz->director.render[1].top,NULL);
        //g_object_set(vrsmsz->director.ds.pre_render2,"alpha",1,NULL);
      }
  }

  if(vrsmsz->director.pub_bin){
      if(vrsmsz->director.render[0].url[0] != 0){
        g_object_set(vrsmsz->director.ds.pub_render1,"location",vrsmsz->director.render[0].url,NULL);
        g_object_set(vrsmsz->director.ds.pub_render1,"offset-x",vrsmsz->director.render[0].left,NULL);
        g_object_set(vrsmsz->director.ds.pub_render1,"offset-y",vrsmsz->director.render[0].top,NULL);
        //g_object_set(vrsmsz->director.ds.pub_render1,"alpha",1,NULL);
      }
      if(vrsmsz->director.render[1].url[0] != 0){
        g_object_set(vrsmsz->director.ds.pub_render2,"location",vrsmsz->director.render[1].url,NULL);
        g_object_set(vrsmsz->director.ds.pub_render2,"offset-x",vrsmsz->director.render[1].left,NULL);
        g_object_set(vrsmsz->director.ds.pub_render2,"offset-y",vrsmsz->director.render[1].top,NULL);
        //g_object_set(vrsmsz->director.ds.pub_render2,"alpha",1,NULL);
      }
  }
  return FALSE;
}

//#define TEST_8K
gboolean vrsmsz_switch_stream(gpointer data){
#if 0// effect switch
  //gboolean is_fade = TRUE;
  gboolean is_fade = FALSE;
#endif
  message_t* msg = data;
  gint streamid = atoi(msg->command.stream_id);
  gint videoid = atoi(msg->command.video_id);
  gint audioid = atoi(msg->command.audio_id);
  vrchan_t* vcv = NULL;
  vrchan_t* vca = NULL;
  if(streamid != videoid && streamid != audioid){
    g_print("error videoid == ? audioid = ?\n");
    return FALSE;
  }
  //GstPadLinkReturn ret;
  //gchar name[1024];
  //GstPadLinkReturn ret;
  //gchar name[1024];
  //GstPadLinkReturn ret;
  //gchar name[1024];
  g_print("Switch %d \n",streamid);

  if(vrsmsz->stream_nbs == 0){
    g_print("no stream \n");
    return FALSE;
  }
  if(streamid == vrsmsz->director.stream_id && videoid == vrsmsz->director.video_id && audioid == vrsmsz->director.audio_id ){
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
  if(videoid != -1 ){
    vcv = vrsmsz->streams+videoid;
  }
  if(audioid != -1 ){
    vca = vrsmsz->streams+audioid;
  }

  if(!vrsmsz->isPreSwitched){// 
	  if(streamid & videoid & audioid != streamid){
             g_print("streamid or videoid or audioid is error");
             return FALSE;
	  }
#ifndef TEST_8K
    director_preview_create(&(vcv->vs));
    director_preview_link_vs(&(vcv->vs),&(vca->vs));
    gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
    vrsmsz->director.stream_id = streamid;
    vrsmsz->director.video_id = videoid;
    vrsmsz->director.audio_id = audioid;
#endif
    vrsmsz->isPreSwitched = TRUE;
    return FALSE;
  }
  
  vrstream_t* vrv = &(vrsmsz->streams[vrsmsz->director.video_id].vs);
  vrstream_t* vra = &(vrsmsz->streams[vrsmsz->director.audio_id].vs);

  if(!vcv) { 
          vcv = vrsmsz->streams+vrsmsz->director.video_id;
	  g_print("audio switch\n");
  }
  if(!vca) { 
          vca = vrsmsz->streams+vrsmsz->director.audio_id;
	  g_print("video switch\n");
  }
#ifndef TEST_8K
  director_preview_unlink_vs(vrv,vra);
  director_preview_link_vs(&(vcv->vs),&(vca->vs));
#endif

  if(vrsmsz->isPubSwitched){ // first pub
    g_print("publish switch");
    director_publish_unlink_vs(vrv,vra);
    director_publish_link_vs(&(vcv->vs),&(vca->vs));
  }

  vrsmsz->director.stream_id = streamid;
  if(videoid != -1 )
    vrsmsz->director.video_id = videoid;
  if(audioid != -1 )
    vrsmsz->director.audio_id = audioid;
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
 gst-launch-1.0 uridecodebin uri=rtmp://192.168.1.31/live/ch0 name=src ! tee name=vt ! queue ! videoscale ! video/x-raw, width=320, height=240 ! x264enc ! queue ! flvmux name=muxer ! rtmpsink location=rtmp://192.168.1.31/live/0 src. ! audioconvert ! voaacenc ! tee name=at ! queue ! muxer. vt. ! queue ! videoscale ! video/x-raw, width=320, height=240 ! x264enc ! queue ! flvmux name=pre_muxer ! rtmpsink location=rtmp://192.168.1.31/live/preview at. ! queue ! pre_muxer.  vt. ! queue ! nvh264enc bitrate=1000 preset=4 ! queue !  h264parse ! queue ! flvmux name=pub_muxer ! rtmpsink location=rtmp://192.168.1.31/live/publish at. ! queue ! pub_muxer.

 gst-launch-1.0 uridecodebin uri=rtmp://192.168.1.31/live/ch0 name=src ! tee name=vt ! queue ! videoscale ! video/x-raw, width=320, height=240 ! x264enc ! queue ! flvmux name=muxer ! queue ! rtmpsink location=rtmp://192.168.1.31/live/0 src. ! audioconvert ! voaacenc ! tee name=at ! queue ! muxer. vt. ! queue ! videoscale ! video/x-raw, width=320, height=240 ! x264enc ! queue ! flvmux name=pre_muxer ! queue ! rtmpsink location=rtmp://192.168.1.31/live/preview at. ! queue ! pre_muxer.  vt. ! queue ! nvh264enc bitrate=1000 preset=4 ! queue !  h264parse ! queue ! flvmux name=pub_muxer ! queue ! rtmpsink location=rtmp://192.168.1.31/live/publish at. ! queue ! pub_muxer.

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
    if(!gst_element_link_many(vrsmsz->pre_vdec_tee_queue,
#ifdef TEST_RENDER
                            vrsmsz->pre_render1,vrsmsz->pre_render2,
#endif
			    vrsmsz->pre_video_scale, vrsmsz->pre_capsfilter, vrsmsz->pre_video_encoder, 
#if 1
			    vrsmsz->pre_video_encoder_queue, vrsmsz->pre_video_encoder_parser, 
#endif
			    vrsmsz->pre_muxer, vrsmsz->pre_outer, NULL)){
     g_print("link director pre viedo failed");
     return FALSE;
    }
    if(!gst_element_link_many(vrsmsz->pub_vdec_tee_queue, vrsmsz->pub_video_encoder,vrsmsz->pub_video_encoder_queue, vrsmsz->pub_video_encoder_parser, vrsmsz->pub_muxer,vrsmsz->pub_outer, NULL)){
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


   vrsmsz->isPreSwitched = TRUE;
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

static void print_cb(JsonObject *obj, const gchar *key, JsonNode *val, gpointer    user_data)
{
  gchar *rel_val = "";
  GType type = json_node_get_value_type(val);
  g_message("typename:%s", g_type_name(type));

  if (g_type_is_a(type, G_TYPE_STRING)) {
     rel_val =(gchar *)json_node_get_string(val);
  }

  g_message("%s:%s", key, rel_val);
}

message_t* parse_json_msg(gchar* msg){

  JsonParser *parser = json_parser_new();
  GError *error = NULL;
  message_t* message = g_malloc(sizeof(*message));
  if(!message || !parser ){
    g_error("new msg or parser error");
    return NULL;
  }
  memset(message,0,sizeof(*message));
  message->timeout = 3;
  message->is_responsed = FALSE;

  //g_print("len = %lu\n",strlen(msg));
  memcpy(message->req, msg, strlen(msg)+1);
  json_parser_load_from_data (parser,message->req,strlen(message->req),&error);
  if(error) {
    g_error("Unable to parse %s", msg);
    g_error_free(error);
    g_object_unref(parser);
    return NULL;
  }
  JsonNode *root = json_parser_get_root(parser);
  if(!root) g_error("get root node failed\n");
  const JsonObject *obj = json_node_get_object(root);
  const gchar* ret = json_object_get_string_member(obj,"cmd");
  int r=-1;
  memcpy(message->command.cmd,ret,strlen(ret)+1);
  r = json_object_get_int_member (obj,"id"); // refresh will be error
  message->command.id = r;

  if(!strcmp(message->command.cmd, "pull")){
    ret = json_object_get_string_member (obj,"url");
    memcpy(message->command.in_url,ret,strlen(ret)+1);

  }else if(!strcmp(message->command.cmd, "publish")){
    ret = json_object_get_string_member (obj,"url");
    memcpy(message->command.pub_url,ret,strlen(ret)+1);

    ret = json_object_get_string_member (obj,"stream_id");
    memcpy(message->command.stream_id,ret,strlen(ret)+1);
     
    JsonObject *sub_obj = json_object_get_object_member (obj,"encoder_params");
    r = json_object_get_int_member (sub_obj,"bitrate");
    message->command.pub_bitrate = r;
    r = json_object_get_int_member (sub_obj,"fps");
    message->command.pub_fps = r;
    r = json_object_get_int_member (sub_obj,"width");
    message->command.pub_width = r;
    r = json_object_get_int_member (sub_obj,"height");
    message->command.pub_height = r;

  }else if(!strcmp(message->command.cmd, "switch")){
    ret = json_object_get_string_member (obj,"stream_id");
    memcpy(message->command.stream_id,ret,strlen(ret)+1);

    ret = json_object_get_string_member (obj,"video_id");
    memcpy(message->command.video_id,ret,strlen(ret)+1);

    ret = json_object_get_string_member (obj,"audio_id");
    memcpy(message->command.audio_id,ret,strlen(ret)+1);

    ret = json_object_get_string_member (obj,"effect");
    memcpy(message->command.effect,ret,strlen(ret)+1);

    r = json_object_get_int_member (obj,"duration");
    message->command.pub_height = r;
  }else if(!strcmp(message->command.cmd, "wraper")){

  }else if(!strcmp(message->command.cmd, "delay")){
    ret = json_object_get_string_member (obj,"stream_id");
    memcpy(message->command.stream_id,ret,strlen(ret)+1);

    //ret = json_object_get_string_member (obj,"video_id");
    //memcpy(message->command.video_id,ret,strlen(ret)+1);

    //ret = json_object_get_string_member (obj,"audio_id");
    //memcpy(message->command.audio_id,ret,strlen(ret)+1);

    ret = json_object_get_string_member (obj,"type");
    memcpy(message->command.delay_type,ret,strlen(ret)+1);
    if(!strcmp(ret,"video")) {
    memcpy(message->command.video_id , message->command.stream_id, strlen(message->command.stream_id)+1); 
    memcpy(message->command.audio_id , "-1",2);
    }else if (!strcmp(ret,"audio")){ 
    memcpy(message->command.audio_id , message->command.stream_id, strlen(message->command.stream_id)+1); 
    memcpy(message->command.video_id , "-1",2);
    }else{
    memcpy(message->command.audio_id , message->command.stream_id, strlen(message->command.stream_id)+1); 
    memcpy(message->command.video_id , message->command.stream_id, strlen(message->command.stream_id)+1); 
    }

    r = json_object_get_int_member (obj,"time");
    message->command.delay_time = r;
  }else if(!strcmp(message->command.cmd, "refresh")){

  }else if(!strcmp(message->command.cmd, "delete")){
    ret = json_object_get_string_member (obj,"stream_id");
    memcpy(message->command.stream_id,ret,strlen(ret)+1);

  }else if(!strcmp(message->command.cmd, "logo")){
    ret = json_object_get_string_member (obj,"stream_id");
    memcpy(message->command.stream_id,ret, strlen(ret)+1);
    ret = json_object_get_int_member (obj,"render_id");
    message->command.render_id = ret;
    ret = json_object_get_string_member (obj,"action");
    memcpy(message->command.action,ret,strlen(ret)+1);
    JsonArray  *array = json_object_get_array_member(obj, "logo_params");
    if(strcmp(ret,"delete")) {
      JsonObject *sub_obj = json_array_get_object_element(array, 0);
      ret =  json_object_get_string_member (sub_obj,"pathname");
      memcpy(message->command.logo_path, ret, strlen(ret)+1);
      JsonObject *sub_obj1 = json_object_get_object_member (sub_obj,"rect");
      ret =  json_object_get_int_member (sub_obj1,"left");
      message->command.left = ret;
      ret =  json_object_get_int_member (sub_obj1,"top");
      message->command.top = ret;
      ret =  json_object_get_int_member (sub_obj1,"width");
      message->command.width = ret;
      ret =  json_object_get_int_member (sub_obj1,"height");
      message->command.height = ret;
      ret =  json_object_get_int_member (sub_obj,"duration");
      message->command.duration = ret;
   }

  }else if(!strcmp(message->command.cmd, "subtitle")){

  }else if(!strcmp(message->command.cmd, "pip")){

  }else if(!strcmp(message->command.cmd, "publish_stop")){

  }else if(!strcmp(message->command.cmd, "stop_all")){

  }else{

  }

  g_object_unref(parser);
  return message;
}

static void vrsmsz_run_command(gchar* command){
  message_t* msg = parse_json_msg(command);
  if(!msg){g_print("msg parse error \n"); return;}
  message_list = g_list_append(message_list,msg);
  //gchar argv[2][1024];
  //sscanf(command,"%s %s",argv[0],argv[1]);
  g_print("Command --> %s \n",msg->command.cmd);

  if(!strcmp(msg->command.cmd,"pull")){
    g_idle_add(vrsmsz_add_stream, msg);
  }else if(!strcmp(msg->command.cmd,"delete")){
    g_idle_add(vrsmsz_remove_stream, msg);
  }else if(!strcmp(msg->command.cmd,"switch")){
    g_idle_add(vrsmsz_switch_stream, msg);
  }else if(!strcmp(msg->command.cmd,"quit")){
    exit(0);
  }else if(!strcmp(msg->command.cmd,"refresh")){
    gst_element_set_state (vrsmsz->pipeline, GST_STATE_READY);
    //if(vrsmsz->director.pre_bin)
    //  gst_element_set_state (vrsmsz->director.pre_bin, GST_STATE_PLAYING);
    gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
    gst_debug_bin_to_dot_file (GST_BIN_CAST(vrsmsz->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz");
  }else if(!strcmp(msg->command.cmd,"publish")){
    g_idle_add(vrsmsz_publish_stream,msg);
  }else if(!strcmp(msg->command.cmd,"publish_stop")){
    g_idle_add(vrsmsz_publish_stop,msg);
  }else if(!strcmp(msg->command.cmd,"delay")){
    g_idle_add(vrsmsz_stream_delay, msg);
  }else if(!strcmp(msg->command.cmd,"logo")){
    g_idle_add(vrsmsz_stream_logo, msg);
  }else if(!strcmp(msg->command.cmd,"stop_all")){
    gst_element_set_state(vrsmsz->pipeline,GST_STATE_NULL);
    g_print("exit \n");
    exit(0);
    vrsmsz_remove_all();
  }
  g_free(command);
}

static void build_response_message(message_t* msg){
   JsonBuilder *builder = json_builder_new();
   json_builder_begin_object(builder);
   g_print("Start building resp ---> %s\n", msg->command.cmd);
   if(!strcmp(msg->command.cmd,"pull")){
     if(!msg->vc){
	     g_print("vc = null\n");
             g_object_unref( builder);
	     return ;
     }
     g_print("build pull response-->\n");
     json_builder_set_member_name(builder, "cmd");
     json_builder_add_string_value(builder, "pull");

     json_builder_set_member_name(builder, "id");
     json_builder_add_int_value(builder, msg->command.id);
		   
     json_builder_set_member_name(builder, "stream_id");
     gchar tmp[1024];
     sprintf(tmp,"%d",msg->vc->stream_id);
     json_builder_add_string_value(builder, tmp);

     json_builder_set_member_name(builder, "url");
     json_builder_add_string_value(builder,msg->vc->preview_url);

     json_builder_set_member_name(builder, "encoder_params");
       json_builder_begin_object(builder);
       json_builder_set_member_name(builder, "bitrate");
       json_builder_add_int_value(builder, msg->command.in_bitrate);

       gint x=0,y=0;
       if(vrsmsz->mode == 4 ){
	     x=3840,y=1920;
       }else if(vrsmsz->mode == 8 ){
	     x=7680,y=3840;
       }else{
	     x=3840,y=1920;
       }
       json_builder_set_member_name(builder, "width");
       json_builder_add_int_value(builder, x);

       json_builder_set_member_name(builder, "height");
       json_builder_add_int_value(builder, y);

       json_builder_set_member_name(builder, "fps");
       json_builder_add_int_value(builder, msg->command.in_fps);

       json_builder_end_object(builder);

     json_builder_set_member_name(builder, "result");
     json_builder_add_string_value(builder, "OK");

     json_builder_set_member_name(builder, "errno");
     json_builder_add_int_value(builder, msg->errcode);
   }else if(!strcmp(msg->command.cmd,"publish")){
     g_print("build publish response-->\n");
     json_builder_set_member_name(builder, "cmd");
     json_builder_add_string_value(builder, "publish");

     json_builder_set_member_name(builder, "id");
     json_builder_add_int_value(builder, msg->command.id);
		   
     json_builder_set_member_name(builder, "stream_id");
     json_builder_add_string_value(builder, msg->command.stream_id);

     json_builder_set_member_name(builder, "url");
     json_builder_add_string_value(builder, vrsmsz->director.publish_url);

     json_builder_set_member_name(builder, "result");
     json_builder_add_string_value(builder, "OK");

     json_builder_set_member_name(builder, "errno");
     json_builder_add_int_value(builder, 0);

  
   }else if(!strcmp(msg->command.cmd,"switch")){
     g_print("build switch response-->\n");
     json_builder_set_member_name(builder, "cmd");
     json_builder_add_string_value(builder, "switch");

     json_builder_set_member_name(builder, "id");
     json_builder_add_int_value(builder, msg->command.id);
		   
     json_builder_set_member_name(builder, "stream_id");
     json_builder_add_string_value(builder, msg->command.stream_id);

     json_builder_set_member_name(builder, "video_id");
     json_builder_add_string_value(builder, msg->command.video_id);

     json_builder_set_member_name(builder, "audio_id");
     json_builder_add_string_value(builder, msg->command.audio_id);

     json_builder_set_member_name(builder, "url");
     json_builder_add_string_value(builder, vrsmsz->director.preview_url);

     json_builder_set_member_name(builder, "result");
     json_builder_add_string_value(builder, "OK");

     json_builder_set_member_name(builder, "errno");
     json_builder_add_int_value(builder, 0);

   }else if(!strcmp(msg->command.cmd,"delete")){
     g_print("build delete response-->\n");
     json_builder_set_member_name(builder, "cmd");
     json_builder_add_string_value(builder, "delete");

     json_builder_set_member_name(builder, "id");
     json_builder_add_int_value(builder, msg->command.id);
		   
     json_builder_set_member_name(builder, "stream_id");
     json_builder_add_string_value(builder, msg->command.stream_id);

     json_builder_set_member_name(builder, "result");
     json_builder_add_string_value(builder, "OK");

     json_builder_set_member_name(builder, "errno");
     json_builder_add_int_value(builder, 0);

   }else if(!strcmp(msg->command.cmd,"delay")){
     g_print("build delay response-->\n");
     json_builder_set_member_name(builder, "cmd");
     json_builder_add_string_value(builder, "delay");

     json_builder_set_member_name(builder, "id");
     json_builder_add_int_value(builder, msg->command.id);
		   
     json_builder_set_member_name(builder, "stream_id");
     json_builder_add_string_value(builder, msg->command.stream_id);

     json_builder_set_member_name(builder, "video_id");
     json_builder_add_string_value(builder, msg->command.video_id);

     json_builder_set_member_name(builder, "audio_id");
     json_builder_add_string_value(builder, msg->command.audio_id);

     json_builder_set_member_name(builder, "result");
     json_builder_add_string_value(builder, "OK");

     json_builder_set_member_name(builder, "errno");
     json_builder_add_int_value(builder, 0);

   }else if(!strcmp(msg->command.cmd,"refresh")){
     g_print("build refresh response-->\n");
     json_builder_set_member_name(builder, "cmd");
     json_builder_add_string_value(builder, "refresh");
		   
     json_builder_set_member_name(builder, "result");
     json_builder_add_string_value(builder, "OK");

     json_builder_set_member_name(builder, "errno");
     json_builder_add_int_value(builder, 0);

   }else if(!strcmp(msg->command.cmd,"logo")){
     g_print("build logo response-->\n");
     if(msg->command.render_id == -1){
	     g_print("render = -1\n");
             g_object_unref( builder);
	     return ;
     }
     json_builder_set_member_name(builder, "cmd");
     json_builder_add_string_value(builder, "logo");

     json_builder_set_member_name(builder, "action");
     json_builder_add_string_value(builder, msg->command.action);

     json_builder_set_member_name(builder, "id");
     json_builder_add_int_value(builder, msg->command.id);
		   
     json_builder_set_member_name(builder, "stream_id");
     json_builder_add_string_value(builder, msg->command.stream_id);

     json_builder_set_member_name(builder, "render_id");
     json_builder_add_int_value(builder, msg->command.render_id);
		   
     json_builder_set_member_name(builder, "result");
     json_builder_add_string_value(builder, "OK");

     json_builder_set_member_name(builder, "errno");
     json_builder_add_int_value(builder, 0);
   }else if(!strcmp(msg->command.cmd,"subtitle")){
     g_print("build subtitle response-->");
     json_builder_set_member_name(builder, "cmd");
     json_builder_add_string_value(builder, "subtitle");

     json_builder_set_member_name(builder, "id");
     json_builder_add_int_value(builder, msg->command.id);
		   
     json_builder_set_member_name(builder, "stream_id");
     json_builder_add_string_value(builder, msg->command.stream_id);

     json_builder_set_member_name(builder, "result");
     json_builder_add_string_value(builder, "OK");

     json_builder_set_member_name(builder, "errno");
     json_builder_add_int_value(builder, 0);
   }else if(!strcmp(msg->command.cmd,"pip")){
     g_print("build pip response-->");
     json_builder_set_member_name(builder, "cmd");
     json_builder_add_string_value(builder, "pip");

     json_builder_set_member_name(builder, "id");
     json_builder_add_int_value(builder, msg->command.id);
		   
     json_builder_set_member_name(builder, "stream_id");
     json_builder_add_string_value(builder, msg->command.stream_id);

     json_builder_set_member_name(builder, "result");
     json_builder_add_string_value(builder, "OK");

     json_builder_set_member_name(builder, "errno");
     json_builder_add_int_value(builder, 0);

   }else if(!strcmp(msg->command.cmd,"wraper")){
   }else if(!strcmp(msg->command.cmd,"publish_stop")){
     g_print("build publish_stop response-->");
     json_builder_set_member_name(builder, "cmd");
     json_builder_add_string_value(builder, "publish_stop");
		   
     json_builder_set_member_name(builder, "result");
     json_builder_add_string_value(builder, "OK");

     json_builder_set_member_name(builder, "errno");
     json_builder_add_int_value(builder, 0);
   }else if(!strcmp(msg->command.cmd,"stop_all")){
     g_print("build stop_all response-->");
     json_builder_set_member_name(builder, "cmd");
     json_builder_add_string_value(builder, "stop_all");
		   
     json_builder_set_member_name(builder, "result");
     json_builder_add_string_value(builder, "OK");

     json_builder_set_member_name(builder, "errno");
     json_builder_add_int_value(builder, 0);
     json_builder_end_object(builder);
   }
   json_builder_end_object(builder);

#if 1
   JsonNode *node = json_builder_get_root(builder);
   JsonGenerator *generator = json_generator_new();
   json_generator_set_root(generator, node);
   gchar *data = json_generator_to_data(generator, NULL);
   printf("json data --> %s\n", data);
   json_node_free(node);
   g_object_unref(generator);

   g_async_queue_push(vrsmsz->rep_queue, data);
#endif
   g_object_unref( builder);
   msg->is_responsed = TRUE;
}

static gboolean message_process(){
   gpointer msg;
   message_t* p_msg=NULL;
   //g_print("what is ...\n");
   msg = g_async_queue_try_pop( vrsmsz->req_queue );
   if( msg ) 
     vrsmsz_run_command( msg );
   GList* it = message_list;
   for(; it; it=it->next){
     p_msg = it->data;
     if( !p_msg->is_responsed ){
       build_response_message(p_msg);
       break;
     }
   }
   if(p_msg && p_msg->is_responsed ){
     message_list = g_list_remove(message_list,p_msg);
     g_free(p_msg);
   }

   return TRUE;
}

void chan_stream_init(vrstream_t* vs){
  vs->uridecodebin = NULL;  
  vs->video_src_pad = NULL;
  vs->audio_src_pad = NULL;
  vs->video_time = 0;
  vs->audio_time = 0;
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
  vs->venc_tee= NULL;
  vs->video_encoder_queue = NULL;
  vs->video_encoder_parser= NULL;
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
  ds->pre_render1 = NULL;
  ds->pre_render2 = NULL;
  ds->pre_video_scale = NULL;
  ds->pre_capsfilter = NULL;
  ds->pre_video_encoder = NULL;
  ds->pre_video_encoder_queue = NULL;
  ds->pre_video_encoder_parser = NULL;
  ds->pre_aenc_tee_queue = NULL;
  ds->pre_aenc_tee_queue_sinkpad= NULL;
  ds->pre_muxer= NULL;
  ds->pre_outer= NULL;
  ds->pub_vdec_tee_queue= NULL;
  ds->pub_vdec_tee_queue_sinkpad= NULL;
  ds->pub_fade= NULL;
  ds->pub_render1 = NULL;
  ds->pub_render2 = NULL;
  ds->pub_video_encoder= NULL;
  ds->pub_video_encoder_queue= NULL;
  ds->pub_video_encoder_parser= NULL;
  ds->pub_aenc_tee_queue= NULL;
  ds->pub_aenc_tee_queue_sinkpad= NULL;
  ds->pub_muxer= NULL;
  ds->pub_outer= NULL;
  ds->mixer= NULL;
  //vrsmsz->comp= NULL;
}


static void
on_negotiation_needed (GstElement * element, gpointer user_data)
{
  app_state = PEER_CALL_NEGOTIATING;

  gchar *msg = g_strdup_printf ("OFFER_REQUEST");
  soup_websocket_connection_send_text (ws_conn, msg);
  g_free (msg);
}

static void
send_ice_candidate_message (GstElement * webrtc G_GNUC_UNUSED, guint mlineindex,
    gchar * candidate, gpointer user_data G_GNUC_UNUSED)
{
  gchar *text;
  JsonObject *ice, *msg;

  if (app_state < PEER_CALL_NEGOTIATING) {
    cleanup_and_quit_loop ("Can't send ICE, not in call", APP_STATE_ERROR);
    return;
  }

  ice = json_object_new ();
  json_object_set_string_member (ice, "candidate", candidate);
  json_object_set_int_member (ice, "sdpMLineIndex", mlineindex);
  msg = json_object_new ();
  json_object_set_object_member (msg, "ice", ice);
  text = get_string_from_json_object (msg);
  json_object_unref (msg);

  soup_websocket_connection_send_text (ws_conn, text);
  g_free (text);
}


static void
on_ice_gathering_state_notify (GstElement * webrtcbin, GParamSpec * pspec,
    gpointer user_data)
{
  GstWebRTCICEGatheringState ice_gather_state;
  const gchar *new_state = "unknown";

  g_object_get (webrtcbin, "ice-gathering-state", &ice_gather_state, NULL);
  switch (ice_gather_state) {
    case GST_WEBRTC_ICE_GATHERING_STATE_NEW:
      new_state = "new";
      break;
    case GST_WEBRTC_ICE_GATHERING_STATE_GATHERING:
      new_state = "gathering";
      break;
    case GST_WEBRTC_ICE_GATHERING_STATE_COMPLETE:
      new_state = "complete";
      break;
  }
  gst_print ("ICE gathering state changed to %s\n", new_state);
}
/*
static void
on_incoming_stream (GstElement * webrtc, GstPad * pad, GstElement * pipe)
{
  GstElement *fakesink;
  GstPad *sinkpad;

  if (GST_PAD_DIRECTION (pad) != GST_PAD_SRC)
    return;

  fakesink = gst_element_factory_make ("fakesink", NULL);
  gst_bin_add (GST_BIN (pipe), fakesink);
  gst_element_sync_state_with_parent (fakesink);

}
*/

static void
handle_media_stream (GstPad * pad, GstElement * pipe, const char *convert_name,
    const char *sink_name)
{
  GstPad *qpad;
  GstElement *q, *conv, *resample, *sink;
  GstPadLinkReturn ret;

  gst_println ("Trying to handle stream with %s ! %s", convert_name, sink_name);

  q = gst_element_factory_make ("queue", NULL);
  g_assert_nonnull (q);
  conv = gst_element_factory_make (convert_name, NULL);
  g_assert_nonnull (conv);
  sink = gst_element_factory_make (sink_name, NULL);
  g_assert_nonnull (sink);

  if (g_strcmp0 (convert_name, "audioconvert") == 0) {
    /* Might also need to resample, so add it just in case.
     * Will be a no-op if it's not required. */
    resample = gst_element_factory_make ("audioresample", NULL);
    g_assert_nonnull (resample);
    gst_bin_add_many (GST_BIN (pipe), q, conv, resample, sink, NULL);
    gst_element_sync_state_with_parent (q);
    gst_element_sync_state_with_parent (conv);
    gst_element_sync_state_with_parent (resample);
    gst_element_sync_state_with_parent (sink);
    gst_element_link_many (q, conv, resample, sink, NULL);
  } else {
    gst_bin_add_many (GST_BIN (pipe), q, conv, sink, NULL);
    gst_element_sync_state_with_parent (q);
    gst_element_sync_state_with_parent (conv);
    gst_element_sync_state_with_parent (sink);
    gst_element_link_many (q, conv, sink, NULL);
  }

  qpad = gst_element_get_static_pad (q, "sink");

  ret = gst_pad_link (pad, qpad);
  g_assert_cmphex (ret, ==, GST_PAD_LINK_OK);
}
static void
on_incoming_decodebin_stream (GstElement * decodebin, GstPad * pad,
    GstElement * pipe)
{
  GstCaps *caps;
  const gchar *name;

  if (!gst_pad_has_current_caps (pad)) {
    gst_printerr ("Pad '%s' has no caps, can't do anything, ignoring\n",
        GST_PAD_NAME (pad));
    return;
  }

  caps = gst_pad_get_current_caps (pad);
  name = gst_structure_get_name (gst_caps_get_structure (caps, 0));

  if (g_str_has_prefix (name, "video")) {
    handle_media_stream (pad, pipe, "videoconvert", "autovideosink");
  } else if (g_str_has_prefix (name, "audio")) {
    handle_media_stream (pad, pipe, "audioconvert", "autoaudiosink");
  } else {
    gst_printerr ("Unknown pad %s, ignoring", GST_PAD_NAME (pad));
  }
  gst_debug_bin_to_dot_file (GST_BIN_CAST(pipe), GST_DEBUG_GRAPH_SHOW_ALL, "webrtc1");
}

static void
on_incoming_stream (GstElement * webrtc, GstPad * pad, GstElement * pipe)
{
  GstElement *decodebin;
  GstPad *sinkpad;

  if (GST_PAD_DIRECTION (pad) != GST_PAD_SRC)
    return;

  decodebin = gst_element_factory_make ("decodebin", NULL);
  g_signal_connect (decodebin, "pad-added",
      G_CALLBACK (on_incoming_decodebin_stream), pipe);
  gst_bin_add (GST_BIN (pipe), decodebin);
  gst_element_sync_state_with_parent (decodebin);

  sinkpad = gst_element_get_static_pad (decodebin, "sink");
  gst_pad_link (pad, sinkpad);
  gst_object_unref (sinkpad);
}


static gboolean
start_pipeline (gint our_id){
   gchar name[1024];
   vrchan_t* vc = vrsmsz->streams+our_id;
   vrstream_t* vs = &vc->vs;
   PeerStruct* ps = &vc->ps;
   GstPadLinkReturn ret;
   g_print("our_id %d\n",our_id);
   
   sprintf(name,"webrtcbin-%d",our_id);
   ps->webrtc = gst_element_factory_make("webrtcbin", name);
   if( !ps->webrtc ){
     g_print("error make\n");
     return FALSE;
   }
   gst_util_set_object_arg (G_OBJECT (ps->webrtc), "stun-server","stun://stun.l.google.com:19302");
   gst_util_set_object_arg (G_OBJECT (ps->webrtc), "bundle-policy","max-bundle");
   //***************************************************************/
   sprintf(name,"audioenc_queue-%d",our_id);
   ps->audioenc_queue = gst_element_factory_make("queue", name);
   if( !ps->audioenc_queue ){
     g_print("error make\n");
     return FALSE;
   }
   sprintf(name,"audiortppay-%d",our_id);
#if 0
   ps->audiortppay = gst_element_factory_make("rtpmp4apay", name);
#else
   ps->audiortppay = gst_element_factory_make("rtpopuspay", name);
#endif
   if( !ps->audiortppay ){
     g_print("error make\n");
     return FALSE;
   }
   sprintf(name,"audioqueue-%d",our_id);
   ps->audioqueue = gst_element_factory_make("queue", name);
   if( !ps->audioqueue ){
     g_print("error make\n");
     return FALSE;
   }
   sprintf(name,"audiocaps-%d",our_id);
   ps->audiocaps = gst_element_factory_make("capsfilter", name);
   if( !ps->audiocaps){
     g_print("error make\n");
     return FALSE;
   }
   gst_util_set_object_arg (G_OBJECT (ps->audiocaps), "caps",
#if 0
      "application/x-rtp,media=audio,encoding-name=MP4A-LATM,payload=111");
#else
      "application/x-rtp,media=audio,encoding-name=OPUS,payload=111"
#endif
   );
   //***************************************************************/
   sprintf(name,"videoenc_queue-%d",our_id);
   ps->videoenc_queue = gst_element_factory_make("queue", name);
   if( !ps->videoenc_queue ){
     g_print("error make\n");
     return FALSE;
   }
   sprintf(name,"videortppay-%d",our_id);
   ps->videortppay = gst_element_factory_make("rtph264pay", name);
   if( !ps->videortppay ){
     g_print("error make\n");
     return FALSE;
   }
   sprintf(name,"videoqueue-%d",our_id);
   ps->videoqueue = gst_element_factory_make("queue", name);
   if( !ps->videoqueue ){
     g_print("error make\n");
     return FALSE;
   }
   sprintf(name,"videocaps-%d",our_id);
   ps->videocaps = gst_element_factory_make("capsfilter", name);
   if( !ps->videocaps ){
     g_print("error make\n");
     return FALSE;
   }
   gst_util_set_object_arg (G_OBJECT (ps->videocaps), "caps",
      "application/x-rtp,media=video,encoding-name=H264,payload=102");

   sprintf(name,"peerbin-%d",our_id);
   ps->bin = gst_bin_new(name);
   gst_bin_add_many(GST_BIN(ps->bin), 
     ps->audioenc_queue, ps->audiortppay, ps->audioqueue,ps->audiocaps,
     ps->videoenc_queue, ps->videortppay, ps->videoqueue,ps->videocaps,ps->webrtc,NULL
   );
   gst_bin_add(GST_BIN(vrsmsz->pipeline),ps->bin);

   if(!gst_element_link_many(
     ps->audioenc_queue, ps->audiortppay, ps->audioqueue,ps->audiocaps,ps->webrtc, NULL)){
	g_print ("webrtc link 1  failed. %x %x %x %x %x \n",ps->audioenc_queue, ps->audiortppay, ps->audioqueue,ps->audiocaps,ps->webrtc);
   }
   if(!gst_element_link_many(
     ps->videoenc_queue, ps->videortppay, ps->videoqueue,ps->videocaps,ps->webrtc, NULL)){
	g_print ("webrtc link 2 failed. \n");
   }

   ps->audio_srcpad = gst_element_get_request_pad (vs->aenc_tee, "src_%u");
   if(!ps->audio_srcpad)
	g_print ("webrtc arnc failed. \n");
   ps->audio_ghost_srcpad = gst_ghost_pad_new ("audio-ghost-srcpad", ps->audio_srcpad);
   gst_element_add_pad (vs->bin, ps->audio_ghost_srcpad);

   ps->audio_sinkpad = gst_element_get_static_pad (ps->audioenc_queue, "sink");
   if(!ps->audio_sinkpad)
	g_print ("webrtc asnc failed. \n");
   ps->audio_ghost_sinkpad = gst_ghost_pad_new ("audio-ghost-sinkpad", ps->audio_sinkpad);
   if(!gst_element_add_pad (ps->bin, ps->audio_ghost_sinkpad)){
	g_print ("webrtc asnc failed. \n");
   }

   ret = gst_pad_link (ps->audio_ghost_srcpad,ps->audio_ghost_sinkpad);
   if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("webrtc link 3 failed %d\n",ret);
   }

   ps->video_srcpad = gst_element_get_request_pad (vs->venc_tee, "src_%u");
   if(!ps->video_srcpad)
      g_print ("webrtc vrnc failed. \n");
   ps->video_ghost_srcpad = gst_ghost_pad_new ("video-ghost-srcpad", ps->video_srcpad);
   gst_element_add_pad (vs->bin, ps->video_ghost_srcpad);

   ps->video_sinkpad = gst_element_get_static_pad (ps->videoenc_queue, "sink");
   if(!ps->video_sinkpad)
      g_print ("webrtc vsnc failed. \n");
   ps->video_ghost_sinkpad = gst_ghost_pad_new ("video-ghost-sinkpad", ps->video_sinkpad);
   gst_element_add_pad (ps->bin, ps->video_ghost_sinkpad);

   ret = gst_pad_link (ps->video_ghost_srcpad,ps->video_ghost_sinkpad);
   if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("webrtc link 4 failed\n");
   }

  g_signal_connect (ps->webrtc, "on-negotiation-needed",
      G_CALLBACK (on_negotiation_needed), NULL);
  g_signal_connect (ps->webrtc, "on-ice-candidate",
      G_CALLBACK (send_ice_candidate_message), NULL);
  g_signal_connect (ps->webrtc, "notify::ice-gathering-state",
      G_CALLBACK (on_ice_gathering_state_notify), NULL);
  g_signal_connect (ps->webrtc, "pad-added", 
      G_CALLBACK (on_incoming_stream), ps->bin);

  gst_element_set_state (ps->bin, GST_STATE_READY);

  g_signal_emit_by_name (ps->webrtc, "create-data-channel", "channel", NULL,
      &ps->send_channel);
  if (ps->send_channel) {
    gst_print ("Created data channel\n");
    connect_data_channel_signals (ps->send_channel);
  } else {
    gst_print ("Could not create data channel, is usrsctp available?\n");
  }

  g_signal_connect (ps->webrtc, "on-data-channel", G_CALLBACK (on_data_channel), ps);

  vrsmsz_set_play(ps->bin);
  vrsmsz_play();

  gst_debug_bin_to_dot_file (GST_BIN_CAST(vrsmsz->pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "vrsmsz-webrtc");
  return FALSE;
}

static gboolean
_start_pipeline (gpointer data){
   vrchan_t* vc = vrsmsz->streams+0;
   //vrstream_t* vs = &vc->vs;
   PeerStruct* ps = &vc->ps;
   gint our_id = *(gint*)data;
   if (start_pipeline (our_id)) {
     cleanup_and_quit_loop ("ERROR: failed to start pipeline",
     PEER_CALL_ERROR);
   }
   on_offer_received (ps->sdp, ps->webrtc);
   return FALSE;
}


static void
on_server_message (SoupWebsocketConnection * conn, SoupWebsocketDataType type,
    GBytes * message, gpointer user_data)
{
  gchar *text;

  switch (type) {
    case SOUP_WEBSOCKET_DATA_BINARY:
      gst_printerr ("Received unknown binary message, ignoring\n");
      return;
    case SOUP_WEBSOCKET_DATA_TEXT:{
      gsize size;
      const gchar *data = g_bytes_get_data (message, &size);
      /* Convert to NULL-terminated string */
      text = g_strndup (data, size);
      break;
    }
    default:
      g_assert_not_reached ();
  }

  if (g_strcmp0 (text, "HELLO") == 0) {
    /* Server has accepted our registration, we are ready to send commands */
    if (app_state != SERVER_REGISTERING) {
      cleanup_and_quit_loop ("ERROR: Received HELLO when not registering",
          APP_STATE_ERROR);
      goto out;
    }
    app_state = SERVER_REGISTERED;
    gst_print ("Registered with server\n");

  } else if (g_strcmp0 (text, "SESSION_OK") == 0) {
#if 0
    /* The call initiated by us has been setup by the server; now we can start
     * negotiation */
    if (app_state != PEER_CONNECTING) {
      cleanup_and_quit_loop ("ERROR: Received SESSION_OK when not calling",
          PEER_CONNECTION_ERROR);
      goto out;
    }

    app_state = PEER_CONNECTED;
    /* Start negotiation (exchange SDP and ICE candidates) */
    if (!start_pipeline (TRUE))
      cleanup_and_quit_loop ("ERROR: failed to start pipeline",
          PEER_CALL_ERROR);
  } else if (g_strcmp0 (text, "OFFER_REQUEST") == 0) {
    if (app_state != SERVER_REGISTERED) {
      gst_printerr ("Received OFFER_REQUEST at a strange time, ignoring\n");
      goto out;
    }
    gst_print ("Received OFFER_REQUEST, sending offer\n");
    /* Peer wants us to start negotiation (exchange SDP and ICE candidates) */
    if (!start_pipeline (TRUE))
      cleanup_and_quit_loop ("ERROR: failed to start pipeline",
          PEER_CALL_ERROR);
#endif
  } else if (g_str_has_prefix (text, "ERROR")) {
    /* Handle errors */
    switch (app_state) {
      case SERVER_CONNECTING:
        app_state = SERVER_CONNECTION_ERROR;
        break;
      case SERVER_REGISTERING:
        app_state = SERVER_REGISTRATION_ERROR;
        break;
      case PEER_CONNECTING:
        app_state = PEER_CONNECTION_ERROR;
        break;
      case PEER_CONNECTED:
      case PEER_CALL_NEGOTIATING:
        app_state = PEER_CALL_ERROR;
        break;
      default:
        app_state = APP_STATE_ERROR;
    }
    cleanup_and_quit_loop (text, 0);
  } else {
    /* Look for JSON messages containing SDP and ICE candidates */
    JsonNode *root;
    JsonObject *object, *child;
    JsonParser *parser = json_parser_new ();
    if (!json_parser_load_from_data (parser, text, -1, NULL)) {
      gst_printerr ("Unknown message '%s', ignoring\n", text);
      g_object_unref (parser);
      goto out;
    }

    root = json_parser_get_root (parser);
    if (!JSON_NODE_HOLDS_OBJECT (root)) {
      gst_printerr ("Unknown json message '%s', ignoring\n", text);
      g_object_unref (parser);
      goto out;
    }

    /* If peer connection wasn't made yet and we are expecting peer will
     * connect to us, launch pipeline at this moment */
    //if (!webrtc1 && our_id) {
   vrchan_t* vc = vrsmsz->streams+0;
   //vrstream_t* vs = &vc->vs;
   PeerStruct* ps = &vc->ps;
    /*  
       if (!start_pipeline (0)) {
        cleanup_and_quit_loop ("ERROR: failed to start pipeline",
            PEER_CALL_ERROR);
      }
    */

      app_state = PEER_CALL_NEGOTIATING;
    //}

    object = json_node_get_object (root);
    /* Check type of JSON message */
    if (json_object_has_member (object, "sdp")) {
      int ret;
      GstSDPMessage *sdp;
      const gchar *text, *sdptype;
      GstWebRTCSessionDescription *answer;

      g_assert_cmphex (app_state, ==, PEER_CALL_NEGOTIATING);

      child = json_object_get_object_member (object, "sdp");

      if (!json_object_has_member (child, "type")) {
        cleanup_and_quit_loop ("ERROR: received SDP without 'type'",
            PEER_CALL_ERROR);
        goto out;
      }

      sdptype = json_object_get_string_member (child, "type");
      /* In this example, we create the offer and receive one answer by default,
       * but it's possible to comment out the offer creation and wait for an offer
       * instead, so we handle either here.
       *
       * See tests/examples/webrtcbidirectional.c in gst-plugins-bad for another
       * example how to handle offers from peers and reply with answers using webrtcbin. */
      text = json_object_get_string_member (child, "sdp");
      ret = gst_sdp_message_new (&sdp);
      g_assert_cmphex (ret, ==, GST_SDP_OK);
      ret = gst_sdp_message_parse_buffer ((guint8 *) text, strlen (text), sdp);
      g_assert_cmphex (ret, ==, GST_SDP_OK);

      if (g_str_equal (sdptype, "answer")) {
#if 0
        gst_print ("Received answer:\n%s\n", text);
        answer = gst_webrtc_session_description_new (GST_WEBRTC_SDP_TYPE_ANSWER,
            sdp);
        g_assert_nonnull (answer);

        /* Set remote description on our pipeline */
        {
          GstPromise *promise = gst_promise_new ();
          g_signal_emit_by_name (webrtc1, "set-remote-description", answer,
              promise);
          gst_promise_interrupt (promise);
          gst_promise_unref (promise);
        }
        app_state = PEER_CALL_STARTED;
#endif
      } else {
        gst_print ("Received offer:\n%s\n", text);
        g_idle_add(_start_pipeline, &ps->src_id);
	ps->sdp = sdp;
        //on_offer_received (sdp, ps->webrtc);
      }

    } else if (json_object_has_member (object, "ice")) {
      const gchar *candidate;
      gint sdpmlineindex;

      child = json_object_get_object_member (object, "ice");
      candidate = json_object_get_string_member (child, "candidate");
      sdpmlineindex = json_object_get_int_member (child, "sdpMLineIndex");

      /* Add ice candidate sent by remote peer */
      g_print("--> 1\n");
      g_signal_emit_by_name (ps->webrtc, "add-ice-candidate", sdpmlineindex,
          candidate);
    } else {
      gst_printerr ("Ignoring unknown JSON message:\n%s\n", text);
    }
    g_object_unref (parser);
  }

out:
  g_free (text);
}


static void
on_server_closed (SoupWebsocketConnection * conn G_GNUC_UNUSED,
    gpointer user_data G_GNUC_UNUSED)
{
  app_state = SERVER_CLOSED;
  cleanup_and_quit_loop ("Server connection closed", 0);
}

static void
on_server_connected (SoupSession * session, GAsyncResult * res,
    SoupMessage * msg)
{
  GError *error = NULL;

  ws_conn = soup_session_websocket_connect_finish (session, res, &error);
  if (error) {
    cleanup_and_quit_loop (error->message, SERVER_CONNECTION_ERROR);
    g_error_free (error);
    return;
  }

  g_assert_nonnull (ws_conn);

  app_state = SERVER_CONNECTED;
  gst_print ("Connected to signalling server\n");

  g_signal_connect (ws_conn, "closed", G_CALLBACK (on_server_closed), NULL);
  g_signal_connect (ws_conn, "message", G_CALLBACK (on_server_message), NULL);

  /* Register with the server so it knows about us and can accept commands */
  //register_with_server ();
}

static void
connect_to_websocket_server_async (void)
{
  SoupLogger *logger;
  SoupMessage *message;
  SoupSession *session;
  const char *https_aliases[] = { "wss", NULL };

  session =
      soup_session_new_with_options (SOUP_SESSION_SSL_STRICT, FALSE,
      SOUP_SESSION_SSL_USE_SYSTEM_CA_FILE, TRUE,
      //SOUP_SESSION_SSL_CA_FILE, "/etc/ssl/certs/ca-bundle.crt",
      SOUP_SESSION_HTTPS_ALIASES, https_aliases, NULL);

  logger = soup_logger_new (SOUP_LOGGER_LOG_BODY, -1);
  soup_session_add_feature (session, SOUP_SESSION_FEATURE (logger));
  g_object_unref (logger);

  message = soup_message_new (SOUP_METHOD_GET, server_url);

  gst_print ("Connecting to server...\n");

  /* Once connected, we will register */
  soup_session_websocket_connect_async (session, message, NULL, NULL, NULL,
      (GAsyncReadyCallback) on_server_connected, message);
  app_state = SERVER_CONNECTING;
}

void vrsmsz_init(int argc, char **argv){

  if(!sync_time()){
    g_error("sync failed\n");
    return 1;
  }
  vrsmsz =  g_malloc0 ( sizeof(vrsmsz_t) );
  if( !vrsmsz ){
    g_error("vrsmsz init failed\n");
  }

  vrsmsz->req_queue = g_async_queue_new ();
  vrsmsz->rep_queue = g_async_queue_new ();
  if( !vrsmsz->req_queue || !vrsmsz->rep_queue )
	  g_print("queue is wrong\n");
   
  create_http_server(8888,NULL,NULL,vrsmsz);
  
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
  vrsmsz->isPreSwitched= FALSE;
  vrsmsz->isPubSwitched= FALSE;
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
    vrsmsz->streams[i].status = 0;

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
  vrsmsz->director.num_render = 0;
  for(int i=0;i<2;i++){
    vrsmsz->director.render[i].left = 0;
    vrsmsz->director.render[i].top = 0;
    vrsmsz->director.render[i].width = 0;
    vrsmsz->director.render[i].height = 0;
    vrsmsz->director.render[i].duration = 0;
    vrsmsz->director.render[i].type = 0;
    vrsmsz->director.render[i].id = -1;
    memset(vrsmsz->director.render[i].url,0,URL_LEN);
  }

  director_stream_init(&vrsmsz->director.ds);

  vrsmsz_play(); 
  vrsmsz->theclock = gst_element_get_clock (vrsmsz->pipeline);
  if(!vrsmsz->theclock) g_print("the clock false\n");
  g_timeout_add(10, message_process,NULL);
  //vrsmsz_null_channel();
  connect_to_websocket_server_async ();
}
