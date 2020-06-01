#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

static GstElement *pipeline;
static GstClock *theclock;
static GMainLoop *loop;
static GstElement *bin1, *bin2, *bin3, *bin4, *bin5;
static GstElement *src1, *src2, *queue1, *comp, *conv, *sink;
static GstPad *blockpad;

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

      g_main_loop_quit (loop);
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
      g_main_loop_quit (loop);
      break;
    default:
      g_print("what is %s\n",gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
      break;
  }

  return TRUE;
}
/* start a bin with the given description */
static GstElement *
create_stream (const gchar * descr)
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
  gst_bin_add (GST_BIN_CAST (pipeline), bin);

  return bin;
}

static void
pause_play_stream (GstElement * bin, gint seconds)
{
  gboolean punch_in;
  GstStateChangeReturn ret;
  GstClockTime now, base_time, running_time;

  /* get current running time, we need this value to continue playback of
   * non-live pipelines. */
  now = gst_clock_get_time (theclock);
  base_time = gst_element_get_base_time (bin);

  running_time = now - base_time;

  /* set the new bin to PAUSED, the parent bin will notice (because of the ASYNC
   * message and will perform latency calculations again when going to PLAYING
   * later. */
  ret = gst_element_set_state (bin, GST_STATE_PAUSED);

  switch (ret) {
    case GST_STATE_CHANGE_NO_PREROLL:
      /* live source, timestamps are running_time of the pipeline clock. */
	    g_print("is live\n");
      punch_in = FALSE;
      break;
    case GST_STATE_CHANGE_SUCCESS:
      /* success, no async state changes, same as async, timestamps start
       * from 0 */
    case GST_STATE_CHANGE_ASYNC:
      /* no live source, bin will preroll. We have to punch it in because in
       * this situation timestamps start from 0.  */
	 g_print("is not live\n");
      punch_in = TRUE;
      break;
    case GST_STATE_CHANGE_FAILURE:
      /* fall through to return */
    default:
      return;
  }

  if (seconds)
    g_usleep (seconds * G_USEC_PER_SEC);

  if (punch_in) {
    /* new bin has to be aligned with previous running_time. We do this by taking
     * the current absolute clock time and calculating the base time that would
     * give the previous running_time. We set this base_time on the bin before
     * setting it to PLAYING. */
    now = gst_clock_get_time (theclock);
    base_time = now - running_time;

    gst_element_set_base_time (bin, base_time);
  }

  /* now set the pipeline to PLAYING */
  gst_element_set_state (bin, GST_STATE_PLAYING);
}

static void
message_received (GstBus * bus, GstMessage * message, GstPipeline * pipeline)
{
  const GstStructure *s;

  s = gst_message_get_structure (message);
  g_print ("message from \"%s\" (%s): ",
      GST_STR_NULL (GST_ELEMENT_NAME (GST_MESSAGE_SRC (message))),
      gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
  if (s) {
    gchar *sstr;

    sstr = gst_structure_to_string (s);
    g_print ("%s\n", sstr);
    g_free (sstr);
  } else {
    g_print ("no message details\n");
  }
}

static void
eos_message_received (GstBus * bus, GstMessage * message,
    GstPipeline * pipeline)
{
  message_received (bus, message, pipeline);
  g_main_loop_quit (loop);
}

static gboolean
perform_step (gpointer pstep)
{
  gint step = GPOINTER_TO_INT (pstep);

  switch (step) {
    case 0:
	if( !src2 ){
	
  		src2 = gst_element_factory_make ("videotestsrc", NULL);
  		g_object_set (src2, "is-live", TRUE, NULL);
  		g_object_set (src2, "pattern", 20, NULL);
	} 
  	g_print("pad is remove\n");
  	gst_element_set_state (src1, GST_STATE_NULL);
        gst_bin_remove (GST_BIN (pipeline), src1);
	src1= NULL;

        g_print("newpad is add\n");
        gst_bin_add (GST_BIN (pipeline), src2);
        gst_element_link_many(src2, conv,NULL);
        //pause_play_stream (pipeline, 0);
  	//gst_element_sync_state_with_parent(src2);
        gst_element_set_state (pipeline, GST_STATE_PLAYING);
      /* live stream locks on to running_time, pipeline configures latency. */
        g_timeout_add_seconds (3, (GSourceFunc) perform_step,
          GINT_TO_POINTER (1));
      break;
    case 1:
        if( !src1 ){
	
  		src1 = gst_element_factory_make ("videotestsrc", NULL);
  		g_object_set (src1, "is-live", TRUE, NULL);
  		//g_object_set (src2, "pattern", 18, NULL);
	}
	g_print("pad is remove\n");
  	gst_element_set_state (src2, GST_STATE_NULL);
        gst_bin_remove (GST_BIN (pipeline), src2);
	src2=NULL;

        g_print("newpad is add\n");
        gst_bin_add (GST_BIN (pipeline), src1);
        gst_element_link_many(src1, conv,NULL);
        //pause_play_stream (pipeline, 0);
  	//gst_element_sync_state_with_parent(src2);
        gst_element_set_state (pipeline, GST_STATE_PLAYING);
        g_timeout_add_seconds (3, (GSourceFunc) perform_step,
          GINT_TO_POINTER (0));
      break;
    default:
      break;
  }
  return FALSE;
}

#if 0
static GstPadProbeReturn
pad_probe_cb (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstPad *srcpad, *sinkpad;
  static test_flags = 1;

  g_print("pad is blocked now\n");
  if( test_flags == 0 ) 
  	return GST_PAD_PROBE_REMOVE;
  test_flags = 0;

  /* remove the probe first */
  gst_pad_remove_probe (pad, GST_PAD_PROBE_INFO_ID (info));

  /* install new probe for EOS */
  //srcpad = gst_element_get_static_pad (cur_effect, "src");
  //gst_pad_add_probe (srcpad, GST_PAD_PROBE_TYPE_BLOCK |
  //    GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, event_probe_cb, user_data, NULL);
  //gst_object_unref (srcpad);

  /* push EOS into the element, the probe will be fired when the
   * EOS leaves the effect and it has thus drained all of its data */
  //sinkpad = gst_element_get_static_pad (cur_effect, "sink");
  //gst_pad_send_event (sinkpad, gst_event_new_eos ());
  //gst_object_unref (sinkpad);

  gst_element_set_state (pipeline, GST_STATE_PAUSED);
  src2 = gst_element_factory_make ("videotestsrc", NULL);
  g_object_set (src2, "is-live", TRUE, NULL);
  g_object_set (src2, "pattern", 1, NULL);

  //srcpad = gst_element_get_static_pad (src1, "src");

  //sinkpad = gst_element_get_static_pad (conv, "sink");

  g_print("pad is remove\n");
  gst_element_set_state (src1, GST_STATE_NULL);
  gst_bin_remove (GST_BIN (pipeline), src1);

  g_print("newpad is add\n");
  if(gst_bin_add (GST_BIN (pipeline), src2) != TRUE){
  	g_print("newpad is add error\n");
  }
  if(gst_element_link(src2, conv) != TRUE){
  	g_print("newpad is link error\n");
  }
  gst_element_sync_state_with_parent(src2);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  return GST_PAD_PROBE_OK;
}

static gboolean
timeout_cb (gpointer user_data)
{
  //gst_pad_add_probe (blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
  //    pad_probe_cb, user_data, NULL);
static int flags = 0;
  if(flags  == 1) return TRUE;
  flags = 1;
  g_idle_add ((GSourceFunc) perform_step, GINT_TO_POINTER (0));

  return TRUE;
}

#endif
int
main (int argc, char *argv[])
{
  GstBus *bus;

  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, TRUE);

  pipeline = gst_pipeline_new ("pipeline");
  src2 = gst_element_factory_make ("videotestsrc", NULL);
  g_object_set (src2, "is-live", TRUE, NULL);
  g_object_set (src2, "pattern", 18, NULL);
  src1 = gst_element_factory_make ("videotestsrc", NULL);
  g_object_set (src1, "is-live", TRUE, NULL);
  //queue1 = gst_element_factory_make ("queue", NULL);
  //blockpad = gst_element_get_static_pad (queue1, "src");
  blockpad = gst_element_get_static_pad (src1, "src");
  conv = gst_element_factory_make ("videoconvert", NULL);
  sink = gst_element_factory_make ("ximagesink", NULL);
  gst_bin_add_many (GST_BIN (pipeline), src1, conv, sink, NULL);
  gst_element_link_many (src1, conv, sink, NULL);

  /* setup message handling */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_signal_watch (bus);
  g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (message_cb), NULL);
  gst_object_unref (GST_OBJECT (bus));

  gst_element_set_state (pipeline, GST_STATE_PLAYING);
  theclock = gst_element_get_clock (pipeline);

#if 0
  g_timeout_add_seconds (1, timeout_cb, loop);
#endif
  g_timeout_add_seconds (3, (GSourceFunc) perform_step,
          GINT_TO_POINTER (0));
  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (bus);
  gst_object_unref (pipeline);
  gst_object_unref (theclock);

  return 0;
}
