#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "httpd.h"

static GstElement *pipeline;
static GstClock *theclock;
static GMainLoop *loop;
static GstElement *bin1, *bin2, *bin3, *bin4, *bin5, *bin6, *bin7, *bin8,*comp;
static GstElement *src[16];
static gint Index=0;

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
      break;
  }

  return TRUE;
}



/* start a bin with the given description */
static GstElement *create_stream (const gchar * descr)
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

static void pause_play_stream (GstElement * bin, gint seconds)
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
      punch_in = FALSE;
      break;
    case GST_STATE_CHANGE_SUCCESS:
      /* success, no async state changes, same as async, timestamps start
       * from 0 */
    case GST_STATE_CHANGE_ASYNC:
      /* no live source, bin will preroll. We have to punch it in because in
       * this situation timestamps start from 0.  */
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

static void message_received (GstBus * bus, GstMessage * message, GstPipeline * pipeline)
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

static void eos_message_received (GstBus * bus, GstMessage * message,
    GstPipeline * pipeline)
{
  message_received (bus, message, pipeline);
  //g_main_loop_quit (loop);
}

static gboolean perform_step (gpointer pstep)
{
  gint step = GPOINTER_TO_INT (pstep);
  static gint numbs = 0;
  gchar pattern_str[1025];

  switch (step) {
    case 0:
      /* live stream locks on to running_time, pipeline configures latency. */
      sprintf(pattern_str,"( videotestsrc name=bin0 is-live=true ! compositor  name=comp ! videoconvert ! timeoverlay ! queue ! xvimagesink name=v4llive%d )",  numbs++);
      bin1 =create_stream(pattern_str);
      gst_element_set_state (bin1, GST_STATE_PLAYING);
      g_print ("creating bin0\n");
      //pause_play_stream (bin1, 0);
      g_timeout_add_seconds (3, (GSourceFunc) perform_step,
          GINT_TO_POINTER (6));
      break;
    case 1:
      /* live stream locks on to running_time, pipeline reconfigures latency
       * together with the previously added bin so that they run synchronized. */
      g_print("creating bin2\n");
      sprintf(pattern_str,"( uridecodebin uri=%s ! videoconvert ! timeoverlay ! queue ! xvimagesink name=v4llive%d )", "http://192.168.0.134:8080/daobo5.ts", numbs++);
      bin2 =create_stream(pattern_str);
      gst_element_set_state (bin2, GST_STATE_PLAYING);
     // pause_play_stream (bin2, 0);
     // g_timeout_add_seconds (1, (GSourceFunc) perform_step,
     //    GINT_TO_POINTER (2));
      break;
    case 2:
      /* non-live stream, need base_time to align with current running live sources. */
      g_print ("creating bin3\n");
      bin3 = create_stream ("( audiotestsrc ! alsasink name=atnonlive )");
      pause_play_stream (bin3, 0);
      g_timeout_add_seconds (1, (GSourceFunc) perform_step,
          GINT_TO_POINTER (3));
      break;
    case 3:
      g_print ("creating bin4\n");
      bin4 =create_stream("( videotestsrc ! timeoverlay ! videoconvert ! ximagesink name=vtnonlive )");
      pause_play_stream(bin4, 0);
      g_timeout_add_seconds(1, (GSourceFunc) perform_step,
          GINT_TO_POINTER (4));
      break;
    case 4:
      /* live stream locks on to running_time */
      g_print ("creating bin5\n");
      bin5 =
          create_stream
          ("( videotestsrc is-live=1 ! timeoverlay ! videoconvert ! ximagesink name=vtlive )");
      pause_play_stream (bin5, 0);
      g_timeout_add_seconds (1, (GSourceFunc) perform_step,
          GINT_TO_POINTER (5));
      break;
    case 5:
      /* pause the fist live stream for 2 seconds */
      pause_play_stream (bin1, 2);
      /* pause the non-live stream for 2 seconds */
      pause_play_stream (bin4, 2);
      /* pause the pseudo live stream for 2 seconds */
      pause_play_stream (bin5, 2);
      //g_print ("Waiting 5 seconds\n");
      //g_timeout_add_seconds (5, (GSourceFunc) perform_step,
      //    GINT_TO_POINTER (6));
      break;
    case 6:
       comp = gst_bin_get_by_name(GST_BIN(pipeline),"comp");
       src[Index] = gst_bin_get_by_name(GST_BIN(pipeline),"bin0");
       gst_element_unlink(src[Index], comp);
       gst_bin_remove(GST_BIN(pipeline),src[Index]);
       gst_element_set_state(src[Index], GST_STATE_NULL);

       Index = ((Index+1) % 16);
       sprintf(pattern_str,"bin%d",Index);
       g_print ("creating binx %d %s\n",Index,pattern_str);
       src[Index] = gst_element_factory_make("videotestsrc",pattern_str);
       //g_object_set(G_OBJECT(src[Index]),"uri", "http://192.168.0.134:8080/daobo5.ts", NULL);
       g_object_set(G_OBJECT(src[Index]),"is-live", TRUE, NULL);
       gst_bin_add(GST_BIN(bin1), src[Index]);
       gst_element_link(src[Index], comp);
       //gst_element_sync_state_with_parent(src[Index]);
       int ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
         switch (ret) {
    case GST_STATE_CHANGE_NO_PREROLL:
      /* live source, timestamps are running_time of the pipeline clock. */
      break;
    case GST_STATE_CHANGE_SUCCESS:
      /* success, no async state changes, same as async, timestamps start
       * from 0 */
      break;
    case GST_STATE_CHANGE_ASYNC:
      /* no live source, bin will preroll. We have to punch it in because in
       * this situation timestamps start from 0.  */
      break;
    case GST_STATE_CHANGE_FAILURE:
      /* fall through to return */
    default:
      return;
  }
      g_timeout_add_seconds (1, (GSourceFunc) perform_step,
          GINT_TO_POINTER (7));
      break;
    case 7:
       gst_element_set_state (pipeline, GST_STATE_PLAYING);
      g_print ("quitting\n");
      //g_main_loop_quit (loop);
      break;
    default:
      break;
  }
  return FALSE;
}

int main (int argc, char *argv[])
{
  GstBus *bus;

  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, TRUE);

  pipeline = gst_pipeline_new ("pipeline");

  /* setup message handling */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_signal_watch(bus);
  /*g_signal_connect (bus, "message::error", (GCallback) message_received,
      pipeline);
  g_signal_connect (bus, "message::warning", (GCallback) message_received,
      pipeline);
  g_signal_connect (bus, "message::eos", (GCallback) eos_message_received,
      pipeline);
*/
  g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (message_cb), NULL);
  /* we set the pipeline to PLAYING, this will distribute a default clock and
   * start running. no preroll is needed */
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* get the clock now. Since we never set the pipeline to PAUSED again, the
   * clock will not change, even when we add new clock providers later.  */
  theclock = gst_element_get_clock (pipeline);

#if 1
  /* start our actions while we are in the mainloop so that we can catch errors
   * and other messages. */
  g_idle_add ((GSourceFunc) perform_step, GINT_TO_POINTER (0));
#else
  /* create http server */
  create_http_server(8888,NULL,NULL,(GSourceFunc)perform_step);
 #endif
  /* go to main loop */
  g_main_loop_run (loop);

  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (bus);
  gst_object_unref (pipeline);
  gst_object_unref (theclock);

  return 0;
}