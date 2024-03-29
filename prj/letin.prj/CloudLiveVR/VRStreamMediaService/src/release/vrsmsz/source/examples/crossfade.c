/*
 * GStreamer
 * Copyright (C) 2017 Thibault Saunier <thibault.saunier@osg-samsung.com>
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

/**
 * Simple crossfade example using the compositor element.
 *
 * Takes two video files and crossfades them for 10 seconds and returns.
 */

#include <stdlib.h>
#include <gst/gst.h>
#include <gst/controller/gstdirectcontrolbinding.h>
#include <gst/controller/gstinterpolationcontrolsource.h>

typedef struct
{
  GstElement *compositor;
  guint z_order;
} VideoInfo;

static gchar *
ensure_uri (const gchar * location)
{
  if (gst_uri_is_valid (location))
    return g_strdup (location);
  else
    return gst_filename_to_uri (location, NULL);
}

static gint alpha = 0.0;
static void
_pad_added_cb (GstElement * decodebin, GstPad * pad, VideoInfo * info)
{
  GstPad *sinkpad =
      gst_element_get_request_pad (GST_ELEMENT (info->compositor), "sink_%u");
  /*
  GstControlSource *control_source;
  gboolean is_last = info->z_order == 1;

  control_source = gst_interpolation_control_source_new ();

  gst_util_set_object_arg (G_OBJECT (sinkpad), "operator",
      info->z_order == 0 ? "source" : "add");
  gst_object_add_control_binding (GST_OBJECT (sinkpad),
      gst_direct_control_binding_new_absolute (GST_OBJECT (sinkpad), "alpha",
          control_source));

  g_object_set (control_source, "mode", GST_INTERPOLATION_MODE_LINEAR, NULL);

  gst_timed_value_control_source_set (GST_TIMED_VALUE_CONTROL_SOURCE
      (control_source), 0, is_last ? 0.0 : 1.0);
  gst_timed_value_control_source_set (GST_TIMED_VALUE_CONTROL_SOURCE
      (control_source), 10 * GST_SECOND, is_last ? 1.0 : 0.0);
  g_object_set (sinkpad, "zorder", info->z_order, NULL);
  */

  g_object_set (sinkpad, "alpha", alpha + 0.1, NULL);
  gst_pad_link (pad, sinkpad);
  alpha += 0.1;

  g_free (info);
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

static gboolean
perform_step (gpointer pstep)
{
  gint step = GPOINTER_TO_INT (pstep);

  switch (step) {
    case 0:
    break;
}
int
main (int argc, char *argv[])
{
  gint i;
  GstMessage *message;
  GstElement *compositor, *sink, *pipeline;
  GstBus *bus;
  GMainLoop *loop;

  if (argc != 3) {
    //g_error ("Need to provide 2 input videos");
    //return -1;
  }

  gst_init (&argc, &argv);
  enable_factory("nvh264dec",TRUE);
  pipeline = gst_element_factory_make ("pipeline", NULL);
  compositor = gst_element_factory_make ("compositor", NULL);
  sink =
      gst_parse_bin_from_description ("videoconvert ! queue ! nvh264enc gop-size=30 preset=0 bitrate=2000 ! h264parse ! queue ! flvmux name=muxer ! rtmp2sink location=rtmp://127.0.0.1/live/compo", TRUE,
      //gst_parse_bin_from_description ("videoconvert ! autovideosink", TRUE,
      NULL);

  gst_util_set_object_arg (G_OBJECT (compositor), "background", "black");

  gst_bin_add_many (GST_BIN (pipeline), compositor, sink, NULL);
  g_assert (gst_element_link (compositor, sink));

  for (i = 1; i < 10; i++) {
    gchar *uri = ensure_uri (argv[i]);
    VideoInfo *info = g_malloc0 (sizeof (VideoInfo));
    GstElement *uridecodebin = gst_element_factory_make ("uridecodebin", NULL);

    g_object_set (uridecodebin, "uri", uri, "expose-all-streams", FALSE,
        "caps", gst_caps_from_string ("video/x-raw(ANY)"), NULL);

    info->compositor = compositor;
    info->z_order = i - 1;
    g_signal_connect (uridecodebin, "pad-added", (GCallback) _pad_added_cb,
        info);

    gst_bin_add (GST_BIN (pipeline), uridecodebin);
  }

  bus = gst_element_get_bus (pipeline);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  loop = g_main_loop_new (NULL, TRUE);
  g_idle_add ((GSourceFunc) perform_step, GINT_TO_POINTER (0));
  g_main_loop_run (loop);
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (pipeline);

  return 0;
}
