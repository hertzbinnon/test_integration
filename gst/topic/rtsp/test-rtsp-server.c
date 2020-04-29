/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
 * Copyright (C) 2015 Centricular Ltd
 *     Author: Sebastian Dröge <sebastian@centricular.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <gst/gst.h>

#include <gst/rtsp-server/rtsp-server.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#define DEFAULT_RTSP_PORT "8554"

static char *port = (char *) DEFAULT_RTSP_PORT;

static GOptionEntry entries[] = {
  {"port", 'p', 0, G_OPTION_ARG_STRING, &port,
      "Port to listen on (default: " DEFAULT_RTSP_PORT ")", "PORT"},
  {NULL}
};

typedef struct
{
  GMainLoop *loop;
  GstElement *source;
  GstElement *sink;
  GAsyncQueue* queue;
} ProgramData;

/* called when we need to give data to appsrc */
static void
need_data (GstElement * appsrc, guint need_size, ProgramData* data)
{
  GstBuffer *buffer;
  guint size;
  GstFlowReturn ret;
//  GstEvent *eos_event;

  //g_print ("++++++++++++++++++++++++++++++++++++++++++need=%d+++++++++++ \n",need_size);
  //buffer= g_async_queue_try_pop(data->queue);
  buffer= g_async_queue_pop(data->queue);
/*
  size = g_async_queue_length (data->queue);
  if( buffer == NULL ){
	g_print("----------------- no streams\n");
    eos_event = gst_event_new_eos ();
	gst_element_send_event (appsrc, eos_event);
    return ;
  }
*/
  //gst_buffer_unref(buffer);
  /* this makes the image black/white */

  g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
  g_print ("++++++++++++++++++++++++++++++++++++++++++pop  done+++++++++++ %d\r",g_async_queue_length (data->queue));
}

/* called when the appsink notifies us that there is a new buffer ready for
 * processing */
static GstFlowReturn
on_new_sample_from_sink (GstElement * elt, ProgramData * data)
{
  GstSample *sample;
  GstBuffer *app_buffer, *buffer;
  GstElement *source;
  GstFlowReturn ret;
  GAsyncQueue* queue;

  if( !data->source ) {
    //gst_buffer_unref(buffer);
    return GST_FLOW_OK;
  }

  /* get the sample from appsink */
  sample = gst_app_sink_pull_sample (GST_APP_SINK (elt));
  buffer = gst_sample_get_buffer (sample);

  //gst_buffer_ref(buffer);
  //g_print ("++++++++++++++++++++++ %d\n",gst_buffer_get_size(buffer));
  //gst_buffer_unref(buffer);
  /* make a copy */
  //app_buffer = gst_buffer_copy (buffer);
  
  /* we don't need the appsink sample anymore */
  gst_sample_unref (sample);

  /* get source an push new buffer */
  //source = gst_bin_get_by_name (GST_BIN (sink), "testsource");
  //g_print ("+++++++++++push+++++++++++ %d\n",gst_buffer_get_size(buffer));
  //ret = gst_app_src_push_buffer (GST_APP_SRC (source), app_buffer);
  g_async_queue_push(data->queue, gst_buffer_ref(buffer));
  g_print ("+++++++++++push done+++++++++++ %d\r",g_async_queue_length (data->queue));
  //gst_object_unref (source);

  return GST_FLOW_OK;
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
static void
media_out_configure (GstRTSPMediaFactory * factory, GstRTSPMedia * media,
    ProgramData* data)
{
  GstElement *element, *appsrc;
  guint size;
  GstEvent *eos_event;

  g_print ("<------------------------------------------------------------%s\n",__func__);
  /* get the element used for providing the streams of the media */
  element = gst_rtsp_media_get_element (media);
  size = g_async_queue_length (data->queue);
  if( size == 0 && !data->sink){
	g_print("----------------- no streams\n");
    eos_event = gst_event_new_eos ();
	gst_element_send_event (element, eos_event);
    return ;
  }

  /* get our appsrc, we named it 'mysrc' with the name property */
  appsrc = gst_bin_get_by_name_recurse_up (GST_BIN (element), "sink_play");

  /* configure for time-based format */
  g_object_set (appsrc, "format", GST_FORMAT_TIME, NULL);
  /*g_object_set (G_OBJECT (appsrc), "caps",
      gst_caps_new_simple ("application/x-rtp",
          "media", G_TYPE_STRING, "video",
          "clock-rate", G_TYPE_INT, 90000, NULL), NULL);
  */

  data->source = appsrc;
  /* uncomment the next line to block when appsrc has buffered enough */
  /* g_object_set (appsrc, "block", TRUE, NULL); */
  g_signal_connect (appsrc, "need-data", (GCallback) need_data, data);
  gst_object_unref (appsrc);
  gst_object_unref (element);
  g_print ("------------------------------------------------------------%s>\n",__func__);
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appint */
static void
media_in_configure (GstRTSPMediaFactory * factory, GstRTSPMedia * media,
    ProgramData* data)
{
  GstElement *element, *appsink;

  g_print ("++++++++++++++++++++++%s\n",__func__);
  /* get the element used for providing the streams of the media */
  element = gst_rtsp_media_get_element (media);

  /* get our appsink, we named it 'mysrc' with the name property */
  appsink = gst_bin_get_by_name_recurse_up (GST_BIN (element), "source_record");
  g_print ("++++++++++++++++++++++%s\n",__func__);
  g_object_set (G_OBJECT (appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
  data->sink = appsink;
  g_print ("++++++++++++++++++++++%s\n",__func__);
  g_signal_connect (appsink,"new-sample",G_CALLBACK (on_new_sample_from_sink), data);
  //gst_object_unref (appsink);
}

static void
free_buffer(gpointer item)
{
  GstBuffer * buffer = item;
  gst_buffer_unref(buffer);
}


int
main (int argc, char *argv[])
{
  GMainLoop *loop;
  GstRTSPServer *server;
  GstRTSPMountPoints *mounts;
  GstRTSPMediaFactory *factory_record,*factory_play,*factory;
  GOptionContext *optctx;
  GError *error = NULL;

  ProgramData *data = NULL;
  gst_init (&argc, &argv);
/*
  optctx = g_option_context_new ("<launch line> - Test RTSP Server, Launch\n\n"
      "Example: \"( decodebin name=depay0 ! autovideosink )\"");
  g_option_context_add_main_entries (optctx, entries, NULL);
  g_option_context_add_group (optctx, gst_init_get_option_group ());
  if (!g_option_context_parse (optctx, &argc, &argv, &error)) {
    g_printerr ("Error parsing options: %s\n", error->message);
    g_option_context_free (optctx);
    g_clear_error (&error);
    return -1;
  }

  if (argc < 2) {
    g_print ("%s\n", g_option_context_get_help (optctx, TRUE, NULL));
    return 1;
  }
  g_option_context_free (optctx);
*/

  data = g_new0 (ProgramData, 1);

  data->loop = g_main_loop_new (NULL, FALSE);

  data->queue = g_async_queue_new_full(free_buffer);

  /* create a server instance */
  server = gst_rtsp_server_new ();

  g_object_set (server, "service", port, NULL);

  /* get the mount points for this server, every server has a default object
   * that be used to map uri mount points to media factories */
  mounts = gst_rtsp_server_get_mount_points (server);

/**************************************record*****************************************/
  factory_record = gst_rtsp_media_factory_new ();
  gst_rtsp_media_factory_set_transport_mode (factory_record,GST_RTSP_TRANSPORT_MODE_RECORD);
  gst_rtsp_media_factory_set_launch (factory_record, "tee name=depay0 ! appsink name=source_record caps=\"application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264\""  /*teer. ! decodebin name=depay-1 ! autovideosink*/);
  //gst_rtsp_media_factory_set_launch (factory_record, " decodebin name=depay0 ! autovideosink");
  gst_rtsp_media_factory_set_latency (factory_record, 2000);

  g_signal_connect (factory_record, "media-configure", (GCallback) media_in_configure,data);

  gst_rtsp_mount_points_add_factory (mounts, "/test_record", factory_record);

/************************************** play *****************************************/
  factory_play = gst_rtsp_media_factory_new ();
  gst_rtsp_media_factory_set_launch (factory_play, "appsrc name=sink_play is-live=true caps=\"application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264\" ! rtph264depay ! h264parse ! rtph264pay name=pay0 pt=96");
  gst_rtsp_media_factory_set_shared (factory_play, TRUE);

  g_signal_connect (factory_play, "media-configure", (GCallback) media_out_configure,data);

  gst_rtsp_mount_points_add_factory (mounts, "/test_play", factory_play);

/*************************************************************************************/
  factory = gst_rtsp_media_factory_new ();
  gst_rtsp_media_factory_set_launch (factory, "( videotestsrc ! x264enc ! rtph264pay name=pay0 pt=96 )");
  gst_rtsp_media_factory_set_shared (factory, TRUE);
  gst_rtsp_mount_points_add_factory (mounts, "/test", factory);

/*************************************************************************************/
  /* don't need the ref to the mapper anymore */
  g_object_unref (mounts);

  /* attach the server to the default maincontext */
  gst_rtsp_server_attach (server, NULL);

  /* start serving */
  g_print ("stream ready at rtsp://127.0.0.1:%s/test\n", port);
  g_print ("On the sender, send a stream with rtspclientsink:\n"
      "  gst-launch-1.0 videotestsrc ! x264enc ! rtspclientsink location=rtsp://127.0.0.1:%s/test\n",
      port);
  g_main_loop_run (data->loop);

  return 0;
}
