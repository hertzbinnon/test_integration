#include <gst/gst.h>
#include <gst/controller/gstdirectcontrolbinding.h>
#include <gst/controller/gstinterpolationcontrolsource.h>

static GstElement *pipeline;
static GstClock *theclock;
static GMainLoop *loop;
static GstElement *bin1, *bin2, *bin3, *bin4, *bin5;
static GstElement *src1, *src2, *src3=NULL, *queue1, *comp, *conv, *sink;
static GstPad *blockpad;
static GstControlSource *control_source;

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
      //g_print("what is %s\n",gst_message_type_get_name (GST_MESSAGE_TYPE (message)));
      break;
  }

  return TRUE;
}

/* Functions below print the Capabilities in a human-friendly format */
static gboolean print_field (GQuark field, const GValue * value, gpointer pfx) {
  gchar *str = gst_value_serialize (value);

  g_print ("%s  %15s: %s\n", (gchar *) pfx, g_quark_to_string (field), str);
  g_free (str);
  return TRUE;
}

static void print_caps (const GstCaps * caps, const gchar * pfx) {
  guint i;

  g_return_if_fail (caps != NULL);

  if (gst_caps_is_any (caps)) {
    g_print ("%sANY\n", pfx);
    return;
  }
  if (gst_caps_is_empty (caps)) {
    g_print ("%sEMPTY\n", pfx);
    return;
  }

  for (i = 0; i < gst_caps_get_size (caps); i++) {
    GstStructure *structure = gst_caps_get_structure (caps, i);

    g_print ("%s%s\n", pfx, gst_structure_get_name (structure));
    gst_structure_foreach (structure, print_field, (gpointer) pfx);
  }
}

/* Shows the CURRENT capabilities of the requested pad in the given element */
static void print_pad_capabilities (GstElement *element, gchar *pad_name, gdouble alpha) {
  GstPad *pad = NULL;
  GstCaps *caps = NULL;

  /* Retrieve pad */
  pad = gst_element_get_static_pad (element, pad_name);
  //pad = gst_element_get_request_pad (element, pad_name);
  if (!pad) {
    g_printerr ("Could not retrieve pad '%s'\n", pad_name);
    return;
  }

  g_object_set(pad, "alpha", alpha);
  /* Retrieve negotiated caps (or acceptable caps if negotiation is not finished yet) */
  caps = gst_pad_get_current_caps (pad);
  if (!caps)
    caps = gst_pad_query_caps (pad, NULL);

  /* Print and free */
  g_print ("Caps for the %s pad:\n", pad_name);
  print_caps (caps, "      ");
  gst_caps_unref (caps);
  gst_object_unref (pad);
}

static gboolean
perform_step (gpointer pstep)
{
  gint step = GPOINTER_TO_INT (pstep);
  GstPad *pad ;

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
	gst_element_link_pads(src2,"src", comp, "sink_0");
        //gst_element_link_many(src2, comp,NULL);
        //print_pad_capabilities(comp,"sink_0",1);
        //gst_element_link_many(src2, sink,NULL);
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
        //gst_element_link_many(src1, conv,NULL);
	gst_element_link_pads(src1,"src", comp, "sink_0");
        //gst_element_link_many(src1, comp,NULL);
        //print_pad_capabilities(comp,"sink_0",1);
        //pause_play_stream (pipeline, 0);
  	//gst_element_sync_state_with_parent(src2);
        gst_element_set_state (pipeline, GST_STATE_PLAYING);
        g_timeout_add_seconds (3, (GSourceFunc) perform_step,
          GINT_TO_POINTER (2));
      break;
    case 2:
	if( !src2 ){
  		src2 = gst_element_factory_make ("videotestsrc", NULL);
  		g_object_set (src2, "is-live", TRUE, NULL);
  		g_object_set (src2, "pattern", 1, NULL);
	}
        g_print("newpad is add\n");
        gst_bin_add (GST_BIN (pipeline), src2);
	gst_element_link_pads(src2,"src", comp, "sink_1");
        //gst_element_link_many(src2, comp,NULL);
        //print_pad_capabilities(comp,"sink_0",0);
        print_pad_capabilities(comp,"sink_1",1);
	//pad = gst_element_get_static_pad(comp,"sink_%u");
        gst_element_set_state (pipeline, GST_STATE_PLAYING);
        g_timeout_add_seconds (3, (GSourceFunc) perform_step,
          GINT_TO_POINTER (3));
      break;
   case 3:
	if( !src3 ){
  		src3 = gst_element_factory_make ("videotestsrc", NULL);
  		g_object_set (src3, "is-live", TRUE, NULL);
  		g_object_set (src3, "pattern", 20, NULL);
	}
        g_print("newpad is add ..... \n");
        gst_bin_add (GST_BIN (pipeline), src3);
        //gst_element_link_many(src3, comp,NULL);
	gst_element_link_pads(src3,"src", comp, "sink_2");
        print_pad_capabilities(comp,"sink_0", 0.9);
        print_pad_capabilities(comp,"sink_1", 0.0);
        print_pad_capabilities(comp,"sink_2", 0.0);
	//pad = gst_element_get_static_pad(comp,"sink_%u");
        gst_element_set_state (pipeline, GST_STATE_PLAYING);
        g_timeout_add_seconds (3, (GSourceFunc) perform_step,
          GINT_TO_POINTER (4));
      break;
   case 4:
        g_print("switch ..... \n");
  	control_source = gst_interpolation_control_source_new ();
  	pad = gst_element_get_static_pad (comp, "sink_0");
  	gst_util_set_object_arg (G_OBJECT (pad), "operator", "source" );
  	gst_object_add_control_binding (GST_OBJECT (pad),
      		gst_direct_control_binding_new_absolute (GST_OBJECT (pad), "alpha",control_source));
        g_object_set (control_source, "mode", GST_INTERPOLATION_MODE_LINEAR, NULL);
      gst_timed_value_control_source_set (GST_TIMED_VALUE_CONTROL_SOURCE
      (control_source), 0, 1.0);
      gst_timed_value_control_source_set (GST_TIMED_VALUE_CONTROL_SOURCE
      (control_source), 10 * GST_SECOND, 0.0);
      g_object_set (pad, "zorder", 0, NULL);

  	control_source = gst_interpolation_control_source_new ();
  	pad = gst_element_get_static_pad (comp, "sink_1");
  	gst_util_set_object_arg (G_OBJECT (pad), "operator", "add" );
  	gst_object_add_control_binding (GST_OBJECT (pad),
      		gst_direct_control_binding_new_absolute (GST_OBJECT (pad), "alpha",control_source));
        g_object_set (control_source, "mode", GST_INTERPOLATION_MODE_LINEAR, NULL);
      gst_timed_value_control_source_set (GST_TIMED_VALUE_CONTROL_SOURCE
      (control_source), 0, 0.0);
      gst_timed_value_control_source_set (GST_TIMED_VALUE_CONTROL_SOURCE
      (control_source), 10 * GST_SECOND, 1.0);
      g_object_set (pad, "zorder", 1, NULL);
/*
  	control_source = gst_interpolation_control_source_new ();
  	pad = gst_element_get_static_pad (comp, "sink_2");
  	gst_util_set_object_arg (G_OBJECT (pad), "operator", "overlay" );
  	gst_object_add_control_binding (GST_OBJECT (pad),
      		gst_direct_control_binding_new_absolute (GST_OBJECT (pad), "alpha",control_source));
        g_object_set (control_source, "mode", GST_INTERPOLATION_MODE_LINEAR, NULL);
      gst_timed_value_control_source_set (GST_TIMED_VALUE_CONTROL_SOURCE
      (control_source), 0, 0.0);
      gst_timed_value_control_source_set (GST_TIMED_VALUE_CONTROL_SOURCE
      (control_source), 10 * GST_SECOND, 1.0);
      g_object_set (pad, "zorder", 1, NULL);
*/
      break;
    default:
      break;
  }
  return FALSE;
}


int
main (int argc, char *argv[])
{
  GstBus *bus;

  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, TRUE);

  pipeline = gst_pipeline_new ("pipeline");
  comp = gst_element_factory_make ("compositor", "comp");

  src2 = gst_element_factory_make ("videotestsrc", NULL);
  g_object_set (src2, "is-live", TRUE, NULL);
  g_object_set (src2, "pattern", 18, NULL);
  src1 = gst_element_factory_make ("videotestsrc", NULL);
  g_object_set (src1, "is-live", TRUE, NULL);
  //queue1 = gst_element_factory_make ("queue", NULL);
  //blockpad = gst_element_get_static_pad (queue1, "src");
  //blockpad = gst_element_get_static_pad (src1, "src");
  conv = gst_element_factory_make ("videoconvert", NULL);
  sink = gst_element_factory_make ("ximagesink", NULL);
  //gst_bin_add_many (GST_BIN (pipeline), src1, conv, sink, NULL);
  gst_bin_add_many (GST_BIN (pipeline), src1, comp, conv, sink, NULL);
  //gst_element_link_many (src1, conv, sink, NULL);
  gst_element_link_many (src1, comp, conv, sink, NULL);

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
