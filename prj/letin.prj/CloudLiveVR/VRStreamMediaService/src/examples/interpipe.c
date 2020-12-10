#include <gst/gst.h>
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
      break;
    default:
      g_print("what is %s\n",gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
      break;
  }

  return TRUE;
}


int flag = 1;
static gboolean
perform_step (gpointer pstep){
GstElement *src=pstep;
 if(flag == 1){
  g_object_set (src, "listen-to", "camera2", NULL);
  flag = 2;
 }else{
  g_object_set (src, "listen-to", "camera1", NULL);
  flag = 1;
 }
 // g_print("switch camera2\n");
  return TRUE;
}
int main(int argc , char** argv){
  GMainLoop *loop;
  GstBus *bus;
  loop = g_main_loop_new (NULL, TRUE);
  gst_init (&argc, &argv);
/* Create pipelines */
  //GstElement *pipe1 = gst_parse_launch ("videotestsrc pattern=ball is-live=true ! tee name=teer ! queue ! interpipesink name=camera1 forward-events=true sync=false forward-eos=true teer. ! queue ! ximagesink", NULL);
  //GstElement *pipe2 = gst_parse_launch ("videotestsrc pattern=snow is-live=true ! tee name=teer ! queue ! interpipesink name=camera2 forward-events=true sync=false forward-eos=true teer. ! queue ! ximagesink", NULL);
  GstElement *pipe3 = gst_parse_launch ("videotestsrc pattern=ball is-live=true ! queue ! videoconvert ! x264enc ! queue ! interpipesink name=encoder_h264 forward-events=true forward-eos=true sync=false async=false enable-last-sample=false drop=true", NULL);
  GstElement *pipe4 = gst_parse_launch ("interpipesrc name=src1 listen-to=encoder_h264 allow-renegotiation=false !  queue ! h264parse ! perf ! mpegtsmux ! filesink name=/tmp/1.ts", NULL);
  if(!pipe4 && !pipe3) exit(0);

  //GstElement *pipe3 = gst_parse_launch ("interpipesrc name=src listen-to=camera1 is-live=true ! videoconvert ! queue ! perf ! fpsdisplaysink", NULL);
  //GstElement *pipe3 = gst_parse_launch ("interpipesrc name=src listen-to=camera1 is-live=true ! queue ! videoconvert ! x264enc !  queue ! h264parse ! avdec_h264 ! videoconvert ! fpsdisplaysink", NULL);

/* Grab a reference to the interpipesrc */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipe3));
  gst_bus_add_signal_watch (bus);
  g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (message_cb), NULL);
  gst_object_unref (GST_OBJECT (bus));
  GstElement *src = gst_bin_get_by_name(GST_BIN(pipe3), "src");
  //g_timeout_add_seconds (3, (GSourceFunc) perform_step,src);
  //gst_element_set_state (pipe1, GST_STATE_PLAYING);
  //gst_element_set_state (pipe2, GST_STATE_PLAYING);
  gst_element_set_state (pipe3, GST_STATE_PLAYING);
  gst_element_set_state (pipe4, GST_STATE_PLAYING);

/* Perform the switch */
  GstElement *src1 = gst_bin_get_by_name(GST_BIN(pipe4), "src1");
  g_object_set (src1, "listen-to", "encoder", NULL);
  g_main_loop_run (loop);
  return 0;	
}
