/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2004,2005 Wim Taymans <wim@fluendo.com>
 *
 * gstpipeline.c: Overall pipeline management element
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
 * SECTION:gstpipeline
 * @title: GstPipeline
 * @short_description: Top-level bin with clocking and bus management
                       functionality.
 * @see_also: #GstElement, #GstBin, #GstClock, #GstBus
 *
 * A #GstPipeline is a special #GstBin used as the toplevel container for
 * the filter graph. The #GstPipeline will manage the selection and
 * distribution of a global #GstClock as well as provide a #GstBus to the
 * application.
 *
 * gst_pipeline_new() is used to create a pipeline. when you are done with
 * the pipeline, use gst_object_unref() to free its resources including all
 * added #GstElement objects (if not otherwise referenced).
 *
 * Elements are added and removed from the pipeline using the #GstBin
 * methods like gst_bin_add() and gst_bin_remove() (see #GstBin).
 *
 * Before changing the state of the #GstPipeline (see #GstElement) a #GstBus
 * can be retrieved with gst_pipeline_get_bus(). This bus can then be
 * used to receive #GstMessage from the elements in the pipeline.
 *
 * By default, a #GstPipeline will automatically flush the pending #GstBus
 * messages when going to the NULL state to ensure that no circular
 * references exist when no messages are read from the #GstBus. This
 * behaviour can be changed with gst_pipeline_set_auto_flush_bus().
 *
 * When the #GstPipeline performs the PAUSED to PLAYING state change it will
 * select a clock for the elements. The clock selection algorithm will by
 * default select a clock provided by an element that is most upstream
 * (closest to the source). For live pipelines (ones that return
 * #GST_STATE_CHANGE_NO_PREROLL from the gst_element_set_state() call) this
 * will select the clock provided by the live source. For normal pipelines
 * this will select a clock provided by the sinks (most likely the audio
 * sink). If no element provides a clock, a default #GstSystemClock is used.
 *
 * The clock selection can be controlled with the gst_pipeline_use_clock()
 * method, which will enforce a given clock on the pipeline. With
 * gst_pipeline_auto_clock() the default clock selection algorithm can be
 * restored.
 *
 * A #GstPipeline maintains a running time for the elements. The running
 * time is defined as the difference between the current clock time and
 * the base time. When the pipeline goes to READY or a flushing seek is
 * performed on it, the running time is reset to 0. When the pipeline is
 * set from PLAYING to PAUSED, the current clock time is sampled and used to
 * configure the base time for the elements when the pipeline is set
 * to PLAYING again. The effect is that the running time (as the difference
 * between the clock time and the base time) will count how much time was spent
 * in the PLAYING state. This default behaviour can be changed with the
 * gst_element_set_start_time() method.
 */

#include "gst_private.h"
#include "gsterror.h"
#include "gst-i18n-lib.h"

#include "gstpipeline.h"
#include "gstinfo.h"
#include "gstsystemclock.h"
#include "gstutils.h"

#include <stdio.h>
#include "gstvrsmsz.h"
#include <json-glib/json-glib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <stdlib.h>
 
#define TEST_RENDER
//#define TEST_FADE
#define MEDIA_PATH "/tmp"
//#define X264_ENC 


GST_DEBUG_CATEGORY_STATIC (pipeline_debug);
#define GST_CAT_DEFAULT pipeline_debug

/* Pipeline signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

#define DEFAULT_DELAY           0
#define DEFAULT_AUTO_FLUSH_BUS  TRUE
#define DEFAULT_LATENCY         GST_CLOCK_TIME_NONE

enum
{
  PROP_0,
  PROP_DELAY,
  PROP_AUTO_FLUSH_BUS,
  PROP_LATENCY
};

struct _GstPipelinePrivate
{
  /* with LOCK */
  gboolean auto_flush_bus;
  gboolean is_live;

  /* when we need to update stream_time or clock when going back to
   * PLAYING*/
  GstClockTime last_start_time;
  gboolean update_clock;

  GstClockTime latency;

  /* seqnum of the most recent instant-rate-request, %GST_SEQNUM_INVALID if none */
  guint32 instant_rate_seqnum;
  gdouble active_instant_rate;
  GstClockTime instant_rate_upstream_anchor;
  GstClockTime instant_rate_clock_anchor;
};


static void gst_pipeline_dispose (GObject * object);
static void gst_pipeline_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_pipeline_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstClock *gst_pipeline_provide_clock_func (GstElement * element);
static GstStateChangeReturn gst_pipeline_change_state (GstElement * element,
    GstStateChange transition);

static void gst_pipeline_handle_message (GstBin * bin, GstMessage * message);
static gboolean gst_pipeline_do_latency (GstBin * bin);
static gboolean gst_pipeline_handle_instant_rate (GstPipeline * pipeline,
    gdouble rate, guint32 seqnum);

/* static guint gst_pipeline_signals[LAST_SIGNAL] = { 0 }; */

#define _do_init \
{ \
  GST_DEBUG_CATEGORY_INIT (pipeline_debug, "pipeline", GST_DEBUG_BOLD, \
      "debugging info for the 'pipeline' container element"); \
}

#define gst_pipeline_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstPipeline, gst_pipeline, GST_TYPE_BIN,
    G_ADD_PRIVATE (GstPipeline) _do_init);

static void
gst_pipeline_class_init (GstPipelineClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
  GstBinClass *gstbin_class = GST_BIN_CLASS (klass);

  gobject_class->set_property = gst_pipeline_set_property;
  gobject_class->get_property = gst_pipeline_get_property;

  /**
   * GstPipeline:delay:
   *
   * The expected delay needed for elements to spin up to the
   * PLAYING state expressed in nanoseconds.
   * see gst_pipeline_set_delay() for more information on this option.
   **/
  g_object_class_install_property (gobject_class, PROP_DELAY,
      g_param_spec_uint64 ("delay", "Delay",
          "Expected delay needed for elements "
          "to spin up to PLAYING in nanoseconds", 0, G_MAXUINT64, DEFAULT_DELAY,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstPipeline:auto-flush-bus:
   *
   * Whether or not to automatically flush all messages on the
   * pipeline's bus when going from READY to NULL state. Please see
   * gst_pipeline_set_auto_flush_bus() for more information on this option.
   **/
  g_object_class_install_property (gobject_class, PROP_AUTO_FLUSH_BUS,
      g_param_spec_boolean ("auto-flush-bus", "Auto Flush Bus",
          "Whether to automatically flush the pipeline's bus when going "
          "from READY into NULL state", DEFAULT_AUTO_FLUSH_BUS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstPipeline:latency:
   *
   * Latency to configure on the pipeline. See gst_pipeline_set_latency().
   *
   * Since: 1.6
   **/
  g_object_class_install_property (gobject_class, PROP_LATENCY,
      g_param_spec_uint64 ("latency", "Latency",
          "Latency to configure on the pipeline", 0, G_MAXUINT64,
          DEFAULT_LATENCY, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gobject_class->dispose = gst_pipeline_dispose;

  gst_element_class_set_static_metadata (gstelement_class, "Pipeline object",
      "Generic/Bin",
      "Complete pipeline object",
      "Erik Walthinsen <omega@cse.ogi.edu>, Wim Taymans <wim@fluendo.com>");

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_pipeline_change_state);
  gstelement_class->provide_clock =
      GST_DEBUG_FUNCPTR (gst_pipeline_provide_clock_func);
  gstbin_class->handle_message =
      GST_DEBUG_FUNCPTR (gst_pipeline_handle_message);
  gstbin_class->do_latency = GST_DEBUG_FUNCPTR (gst_pipeline_do_latency);
}

static void
gst_pipeline_init (GstPipeline * pipeline)
{
  GstBus *bus;

  pipeline->priv = gst_pipeline_get_instance_private (pipeline);

  /* set default property values */
  pipeline->priv->auto_flush_bus = DEFAULT_AUTO_FLUSH_BUS;
  pipeline->delay = DEFAULT_DELAY;
  pipeline->priv->latency = DEFAULT_LATENCY;

  pipeline->priv->is_live = FALSE;

  /* create and set a default bus */
  bus = gst_bus_new ();
#if 0
  /* FIXME, disabled for 0.10.5 release as it caused to many regressions */
  /* Start our bus in flushing if appropriate */
  if (pipeline->priv->auto_flush_bus)
    gst_bus_set_flushing (bus, TRUE);
#endif

  gst_element_set_bus (GST_ELEMENT_CAST (pipeline), bus);
  GST_DEBUG_OBJECT (pipeline, "set bus %" GST_PTR_FORMAT " on pipeline", bus);
  gst_object_unref (bus);
}

static void
gst_pipeline_dispose (GObject * object)
{
  GstPipeline *pipeline = GST_PIPELINE (object);
  GstClock **clock_p = &pipeline->fixed_clock;

  GST_CAT_DEBUG_OBJECT (GST_CAT_REFCOUNTING, pipeline, "%p dispose", pipeline);

  /* clear and unref any fixed clock */
  gst_object_replace ((GstObject **) clock_p, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_pipeline_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstPipeline *pipeline = GST_PIPELINE (object);

  switch (prop_id) {
    case PROP_DELAY:
      gst_pipeline_set_delay (pipeline, g_value_get_uint64 (value));
      break;
    case PROP_AUTO_FLUSH_BUS:
      gst_pipeline_set_auto_flush_bus (pipeline, g_value_get_boolean (value));
      break;
    case PROP_LATENCY:
      gst_pipeline_set_latency (pipeline, g_value_get_uint64 (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_pipeline_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstPipeline *pipeline = GST_PIPELINE (object);

  switch (prop_id) {
    case PROP_DELAY:
      g_value_set_uint64 (value, gst_pipeline_get_delay (pipeline));
      break;
    case PROP_AUTO_FLUSH_BUS:
      g_value_set_boolean (value, gst_pipeline_get_auto_flush_bus (pipeline));
      break;
    case PROP_LATENCY:
      g_value_set_uint64 (value, gst_pipeline_get_latency (pipeline));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* set the start_time to 0, this will cause us to select a new base_time and
 * make the running_time start from 0 again. */
static void
reset_start_time (GstPipeline * pipeline, GstClockTime start_time)
{
  GST_OBJECT_LOCK (pipeline);
  if (GST_ELEMENT_START_TIME (pipeline) != GST_CLOCK_TIME_NONE) {
    GST_DEBUG_OBJECT (pipeline, "reset start_time to 0");
    GST_ELEMENT_START_TIME (pipeline) = start_time;
    pipeline->priv->last_start_time = -1;

    /* Reset instant rate multiplier because we flushed / reset time.
     * Old anchor's don't make sense */
    pipeline->priv->instant_rate_seqnum = GST_SEQNUM_INVALID;
    pipeline->priv->instant_rate_upstream_anchor =
        pipeline->priv->instant_rate_clock_anchor = GST_CLOCK_TIME_NONE;
    pipeline->priv->active_instant_rate = 1.0;
    GST_DEBUG_OBJECT (pipeline, "Reset start time to %" GST_TIME_FORMAT,
        GST_TIME_ARGS (start_time));
  } else {
    GST_DEBUG_OBJECT (pipeline, "application asked to not reset stream_time");
  }
  GST_OBJECT_UNLOCK (pipeline);
}

/**
 * gst_pipeline_new:
 * @name: (allow-none): name of new pipeline
 *
 * Create a new pipeline with the given name.
 *
 * Returns: (transfer floating): newly created GstPipeline
 *
 * MT safe.
 */
GstElement *
gst_pipeline_new (const gchar * name)
{
  return gst_element_factory_make ("pipeline", name);
}

/* takes a snapshot of the running_time of the pipeline and store this as the
 * element start_time. This is the time we will set as the running_time of the
 * pipeline when we go to PLAYING next. */
static void
pipeline_update_start_time (GstElement * element)
{
  GstPipeline *pipeline = GST_PIPELINE_CAST (element);
  GstClock *clock;

  GST_OBJECT_LOCK (element);
  if ((clock = element->clock)) {
    GstClockTime now;

    gst_object_ref (clock);
    GST_OBJECT_UNLOCK (element);

    /* calculate the time when we stopped */
    now = gst_clock_get_time (clock);
    gst_object_unref (clock);

    GST_OBJECT_LOCK (element);
    /* store the current running time */
    if (GST_ELEMENT_START_TIME (pipeline) != GST_CLOCK_TIME_NONE) {
      if (now != GST_CLOCK_TIME_NONE)
        GST_ELEMENT_START_TIME (pipeline) = now - element->base_time;
      else
        GST_WARNING_OBJECT (element,
            "Clock %s returned invalid time, can't calculate "
            "running_time when going to the PAUSED state",
            GST_OBJECT_NAME (clock));

      /* we went to PAUSED, when going to PLAYING select clock and new
       * base_time */
      pipeline->priv->update_clock = TRUE;
    }
    GST_DEBUG_OBJECT (element,
        "start_time=%" GST_TIME_FORMAT ", now=%" GST_TIME_FORMAT
        ", base_time %" GST_TIME_FORMAT,
        GST_TIME_ARGS (GST_ELEMENT_START_TIME (pipeline)),
        GST_TIME_ARGS (now), GST_TIME_ARGS (element->base_time));
  }
  GST_OBJECT_UNLOCK (element);
}

/* MT safe */
static GstStateChangeReturn
gst_pipeline_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn result = GST_STATE_CHANGE_SUCCESS;
  GstPipeline *pipeline = GST_PIPELINE_CAST (element);
  GstClock *clock;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_NULL:
      break;
    case GST_STATE_CHANGE_READY_TO_READY:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PLAYING:
      break;
    case GST_STATE_CHANGE_NULL_TO_READY:
      GST_OBJECT_LOCK (element);
      if (element->bus)
        gst_bus_set_flushing (element->bus, FALSE);
      GST_OBJECT_UNLOCK (element);
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      GST_OBJECT_LOCK (element);
      pipeline->priv->update_clock = TRUE;
      GST_OBJECT_UNLOCK (element);

      /* READY to PAUSED starts running_time from 0 */
      reset_start_time (pipeline, 0);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
    {
      GstClockTime now, start_time, last_start_time, delay;
      gboolean update_clock;
      GstClock *cur_clock;

      GST_DEBUG_OBJECT (element, "selecting clock and base_time");

      GST_OBJECT_LOCK (element);
      cur_clock = element->clock;
      if (cur_clock)
        gst_object_ref (cur_clock);
      /* get the desired running_time of the first buffer aka the start_time */
      start_time = GST_ELEMENT_START_TIME (pipeline);
      last_start_time = pipeline->priv->last_start_time;
      pipeline->priv->last_start_time = start_time;
      /* see if we need to update the clock */
      update_clock = pipeline->priv->update_clock;
      pipeline->priv->update_clock = FALSE;
      delay = pipeline->delay;
      GST_OBJECT_UNLOCK (element);

      /* running time changed, either with a PAUSED or a flush, we need to check
       * if there is a new clock & update the base time */
      /* only do this for top-level, however */
      if (GST_OBJECT_PARENT (element) == NULL &&
          (update_clock || last_start_time != start_time)) {
        GST_DEBUG_OBJECT (pipeline, "Need to update start_time");

        /* when going to PLAYING, select a clock when needed. If we just got
         * flushed, we don't reselect the clock. */
        if (update_clock) {
          GST_DEBUG_OBJECT (pipeline, "Need to update clock.");
          clock = gst_element_provide_clock (element);
        } else {
          GST_DEBUG_OBJECT (pipeline,
              "Don't need to update clock, using old clock.");
          /* only try to ref if cur_clock is not NULL */
          if (cur_clock)
            gst_object_ref (cur_clock);
          clock = cur_clock;
        }

        if (clock) {
          now = gst_clock_get_time (clock);
        } else {
          GST_DEBUG_OBJECT (pipeline, "no clock, using base time of NONE");
          now = GST_CLOCK_TIME_NONE;
        }

        if (clock != cur_clock) {
          /* now distribute the clock (which could be NULL). If some
           * element refuses the clock, this will return FALSE and
           * we effectively fail the state change. */
          if (!gst_element_set_clock (element, clock))
            goto invalid_clock;

          /* if we selected and distributed a new clock, let the app
           * know about it */
          gst_element_post_message (element,
              gst_message_new_new_clock (GST_OBJECT_CAST (element), clock));
        }

        if (clock)
          gst_object_unref (clock);

        if (start_time != GST_CLOCK_TIME_NONE && now != GST_CLOCK_TIME_NONE) {
          GstClockTime new_base_time = now - start_time + delay;
          GST_DEBUG_OBJECT (element,
              "start_time=%" GST_TIME_FORMAT ", now=%" GST_TIME_FORMAT
              ", base_time %" GST_TIME_FORMAT,
              GST_TIME_ARGS (start_time), GST_TIME_ARGS (now),
              GST_TIME_ARGS (new_base_time));

          gst_element_set_base_time (element, new_base_time);
        } else {
          GST_DEBUG_OBJECT (pipeline,
              "NOT adjusting base_time because start_time is NONE");
        }
      } else {
        GST_DEBUG_OBJECT (pipeline,
            "NOT adjusting base_time because we selected one before");
      }

      if (cur_clock)
        gst_object_unref (cur_clock);
      break;
    }
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
    {
      /* we take a start_time snapshot before calling the children state changes
       * so that they know about when the pipeline PAUSED. */
      pipeline_update_start_time (element);
      break;
    }
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      pipeline->priv->is_live = FALSE;
      reset_start_time (pipeline, 0);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
  }

  result = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  if (GST_STATE_TRANSITION_NEXT (transition) == GST_STATE_PAUSED) {
    pipeline->priv->is_live = result == GST_STATE_CHANGE_NO_PREROLL;
    GST_INFO_OBJECT (pipeline, "pipeline is%slive",
        pipeline->priv->is_live ? " " : " not ");
  }

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_NULL:
      break;
    case GST_STATE_CHANGE_READY_TO_READY:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PLAYING:
      break;
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
    {
      /* Take a new snapshot of the start_time after calling the state change on
       * all children. This will be the running_time of the pipeline when we go
       * back to PLAYING */
      pipeline_update_start_time (element);
      break;
    }
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
    {
      GstBus *bus;
      gboolean auto_flush;

      /* grab some stuff before we release the lock to flush out the bus */
      GST_OBJECT_LOCK (element);
      if ((bus = element->bus))
        gst_object_ref (bus);
      auto_flush = pipeline->priv->auto_flush_bus;
      GST_OBJECT_UNLOCK (element);

      if (bus) {
        if (auto_flush) {
          gst_bus_set_flushing (bus, TRUE);
        } else {
          GST_INFO_OBJECT (element, "not flushing bus, auto-flushing disabled");
        }
        gst_object_unref (bus);
      }
      break;
    }
  }
  return result;

  /* ERRORS */
invalid_clock:
  {
    /* we generate this error when the selected clock was not
     * accepted by some element */
    GST_ELEMENT_ERROR (pipeline, CORE, CLOCK,
        (_("Selected clock cannot be used in pipeline.")),
        ("Pipeline cannot operate with selected clock"));
    GST_DEBUG_OBJECT (pipeline,
        "Pipeline cannot operate with selected clock %p", clock);
    if (clock)
      gst_object_unref (clock);
    return GST_STATE_CHANGE_FAILURE;
  }
}

/* intercept the bus messages from our children. We watch for the ASYNC_START
 * message with is posted by the elements (sinks) that require a reset of the
 * running_time after a flush. ASYNC_START also brings the pipeline back into
 * the PAUSED, pending PAUSED state. When the ASYNC_DONE message is received the
 * pipeline will redistribute the new base_time and will bring the elements back
 * to the desired state of the pipeline. */
/* GST_MESSAGE_INSTANT_RATE_REQUEST: This message is only posted by sinks and
 *     bins containing sinks (which are also considered sinks). Once all sinks
 *     have posted this message it is posted to the parent bin, or if this is
 *     a top-level bin (e.g. pipeline), a instant-rate-sync-time event with
 *     the current running time is sent to the whole pipeline.
 */
static void
gst_pipeline_handle_message (GstBin * bin, GstMessage * message)
{
  GstPipeline *pipeline = GST_PIPELINE_CAST (bin);

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_RESET_TIME:
    {
      GstClockTime running_time;

      gst_message_parse_reset_time (message, &running_time);

      /* reset our running time if we need to distribute a new base_time to the
       * children. */
      reset_start_time (pipeline, running_time);

      /* If we are live, sample a new base_time immediately */
      if (pipeline->priv->is_live
          && GST_STATE_TARGET (pipeline) == GST_STATE_PLAYING) {
        gst_pipeline_change_state (GST_ELEMENT (pipeline),
            GST_STATE_CHANGE_PAUSED_TO_PLAYING);
      }

      break;
    }
    case GST_MESSAGE_CLOCK_LOST:
    {
      GstClock *clock;

      gst_message_parse_clock_lost (message, &clock);

      GST_OBJECT_LOCK (bin);
      if (clock == GST_ELEMENT_CAST (bin)->clock) {
        GST_DEBUG_OBJECT (bin, "Used clock '%s' got lost",
            GST_OBJECT_NAME (clock));
        pipeline->priv->update_clock = TRUE;
      }
      GST_OBJECT_UNLOCK (bin);
    }
      break;

    case GST_MESSAGE_INSTANT_RATE_REQUEST:{
      guint32 seqnum = gst_message_get_seqnum (message);
      gdouble rate_multiplier;

      gst_message_parse_instant_rate_request (message, &rate_multiplier);

      gst_pipeline_handle_instant_rate (pipeline, rate_multiplier, seqnum);

      break;
    }
    default:
      break;
  }
  GST_BIN_CLASS (parent_class)->handle_message (bin, message);
}

static gboolean
gst_pipeline_do_latency (GstBin * bin)
{
  GstPipeline *pipeline = GST_PIPELINE (bin);
  GstQuery *query;
  GstClockTime latency;
  GstClockTime min_latency, max_latency;
  gboolean res;

  GST_OBJECT_LOCK (pipeline);
  latency = pipeline->priv->latency;
  GST_OBJECT_UNLOCK (pipeline);

  if (latency == GST_CLOCK_TIME_NONE)
    return GST_BIN_CLASS (parent_class)->do_latency (bin);

  GST_DEBUG_OBJECT (pipeline, "querying latency");

  query = gst_query_new_latency ();
  if ((res = gst_element_query (GST_ELEMENT_CAST (pipeline), query))) {
    gboolean live;

    gst_query_parse_latency (query, &live, &min_latency, &max_latency);

    GST_DEBUG_OBJECT (pipeline,
        "got min latency %" GST_TIME_FORMAT ", max latency %"
        GST_TIME_FORMAT ", live %d", GST_TIME_ARGS (min_latency),
        GST_TIME_ARGS (max_latency), live);

    if (max_latency < min_latency) {
      /* this is an impossible situation, some parts of the pipeline might not
       * work correctly. We post a warning for now. */
      GST_ELEMENT_WARNING (pipeline, CORE, CLOCK, (NULL),
          ("Impossible to configure latency: max %" GST_TIME_FORMAT " < min %"
              GST_TIME_FORMAT ". Add queues or other buffering elements.",
              GST_TIME_ARGS (max_latency), GST_TIME_ARGS (min_latency)));
    }

    if (latency < min_latency) {
      /* This is a problematic situation as we will most likely drop lots of
       * data if we configure a too low latency */
      GST_ELEMENT_WARNING (pipeline, CORE, CLOCK, (NULL),
          ("Configured latency is lower than detected minimum latency: configured %"
              GST_TIME_FORMAT " < min %" GST_TIME_FORMAT,
              GST_TIME_ARGS (latency), GST_TIME_ARGS (min_latency)));
    }
  } else {
    /* this is not a real problem, we just don't configure any latency. */
    GST_WARNING_OBJECT (pipeline, "failed to query latency");
  }
  gst_query_unref (query);


  /* configure latency on elements */
  res =
      gst_element_send_event (GST_ELEMENT_CAST (pipeline),
      gst_event_new_latency (latency));
  if (res) {
    GST_INFO_OBJECT (pipeline, "configured latency of %" GST_TIME_FORMAT,
        GST_TIME_ARGS (latency));
  } else {
    GST_WARNING_OBJECT (pipeline,
        "did not really configure latency of %" GST_TIME_FORMAT,
        GST_TIME_ARGS (latency));
  }

  return res;
}

/**
 * gst_pipeline_get_bus:
 * @pipeline: a #GstPipeline
 *
 * Gets the #GstBus of @pipeline. The bus allows applications to receive
 * #GstMessage packets.
 *
 * Returns: (transfer full): a #GstBus, unref after usage.
 *
 * MT safe.
 */
GstBus *
gst_pipeline_get_bus (GstPipeline * pipeline)
{
  return gst_element_get_bus (GST_ELEMENT_CAST (pipeline));
}

static GstClock *
gst_pipeline_provide_clock_func (GstElement * element)
{
  GstClock *clock = NULL;
  GstPipeline *pipeline = GST_PIPELINE (element);

  /* if we have a fixed clock, use that one */
  GST_OBJECT_LOCK (pipeline);
  if (GST_OBJECT_FLAG_IS_SET (pipeline, GST_PIPELINE_FLAG_FIXED_CLOCK)) {
    clock = pipeline->fixed_clock;
    if (clock)
      gst_object_ref (clock);
    GST_OBJECT_UNLOCK (pipeline);

    GST_CAT_DEBUG (GST_CAT_CLOCK, "pipeline using fixed clock %p (%s)",
        clock, clock ? GST_STR_NULL (GST_OBJECT_NAME (clock)) : "-");
  } else {
    GST_OBJECT_UNLOCK (pipeline);
    /* let the parent bin select a clock */
    clock =
        GST_ELEMENT_CLASS (parent_class)->provide_clock (GST_ELEMENT
        (pipeline));
    /* no clock, use a system clock */
    if (!clock) {
      clock = gst_system_clock_obtain ();

      GST_CAT_DEBUG (GST_CAT_CLOCK, "pipeline obtained system clock: %p (%s)",
          clock, clock ? GST_STR_NULL (GST_OBJECT_NAME (clock)) : "-");
    } else {
      GST_CAT_DEBUG (GST_CAT_CLOCK, "pipeline obtained clock: %p (%s)",
          clock, clock ? GST_STR_NULL (GST_OBJECT_NAME (clock)) : "-");
    }
  }
  return clock;
}

/**
 * gst_pipeline_get_clock: (skip)
 * @pipeline: a #GstPipeline
 *
 * Gets the current clock used by @pipeline. Users of object
 * oriented languages should use gst_pipeline_get_pipeline_clock()
 * to avoid confusion with gst_element_get_clock() which has a different behavior.
 *
 * Unlike gst_element_get_clock(), this function will always return a
 * clock, even if the pipeline is not in the PLAYING state.
 *
 * Returns: (transfer full): a #GstClock, unref after usage.
 */
GstClock *
gst_pipeline_get_clock (GstPipeline * pipeline)
{
  return gst_pipeline_get_pipeline_clock (pipeline);
}

/**
 * gst_pipeline_get_pipeline_clock:
 * @pipeline: a #GstPipeline
 *
 * Gets the current clock used by @pipeline.
 *
 * Unlike gst_element_get_clock(), this function will always return a
 * clock, even if the pipeline is not in the PLAYING state.
 *
 * Returns: (transfer full): a #GstClock, unref after usage.
 *
 * Since: 1.6
 */
GstClock *
gst_pipeline_get_pipeline_clock (GstPipeline * pipeline)
{
  g_return_val_if_fail (GST_IS_PIPELINE (pipeline), NULL);

  return gst_pipeline_provide_clock_func (GST_ELEMENT_CAST (pipeline));
}


/**
 * gst_pipeline_use_clock:
 * @pipeline: a #GstPipeline
 * @clock: (transfer none) (allow-none): the clock to use
 *
 * Force @pipeline to use the given @clock. The pipeline will
 * always use the given clock even if new clock providers are added
 * to this pipeline.
 *
 * If @clock is %NULL all clocking will be disabled which will make
 * the pipeline run as fast as possible.
 *
 * MT safe.
 */
void
gst_pipeline_use_clock (GstPipeline * pipeline, GstClock * clock)
{
  GstClock **clock_p;

  g_return_if_fail (GST_IS_PIPELINE (pipeline));

  GST_OBJECT_LOCK (pipeline);
  GST_OBJECT_FLAG_SET (pipeline, GST_PIPELINE_FLAG_FIXED_CLOCK);

  clock_p = &pipeline->fixed_clock;
  gst_object_replace ((GstObject **) clock_p, (GstObject *) clock);
  GST_OBJECT_UNLOCK (pipeline);

  GST_CAT_DEBUG (GST_CAT_CLOCK, "pipeline using fixed clock %p (%s)", clock,
      (clock ? GST_OBJECT_NAME (clock) : "nil"));
}

/**
 * gst_pipeline_set_clock: (skip)
 * @pipeline: a #GstPipeline
 * @clock: (transfer none): the clock to set
 *
 * Set the clock for @pipeline. The clock will be distributed
 * to all the elements managed by the pipeline.
 *
 * Returns: %TRUE if the clock could be set on the pipeline. %FALSE if
 *   some element did not accept the clock.
 *
 * MT safe.
 */
gboolean
gst_pipeline_set_clock (GstPipeline * pipeline, GstClock * clock)
{
  g_return_val_if_fail (pipeline != NULL, FALSE);
  g_return_val_if_fail (GST_IS_PIPELINE (pipeline), FALSE);

  return
      GST_ELEMENT_CLASS (parent_class)->set_clock (GST_ELEMENT_CAST (pipeline),
      clock);
}

/**
 * gst_pipeline_auto_clock:
 * @pipeline: a #GstPipeline
 *
 * Let @pipeline select a clock automatically. This is the default
 * behaviour.
 *
 * Use this function if you previous forced a fixed clock with
 * gst_pipeline_use_clock() and want to restore the default
 * pipeline clock selection algorithm.
 *
 * MT safe.
 */
void
gst_pipeline_auto_clock (GstPipeline * pipeline)
{
  GstClock **clock_p;

  g_return_if_fail (pipeline != NULL);
  g_return_if_fail (GST_IS_PIPELINE (pipeline));

  GST_OBJECT_LOCK (pipeline);
  GST_OBJECT_FLAG_UNSET (pipeline, GST_PIPELINE_FLAG_FIXED_CLOCK);

  clock_p = &pipeline->fixed_clock;
  gst_object_replace ((GstObject **) clock_p, NULL);
  GST_OBJECT_UNLOCK (pipeline);

  GST_CAT_DEBUG (GST_CAT_CLOCK, "pipeline using automatic clock");
}

/**
 * gst_pipeline_set_delay:
 * @pipeline: a #GstPipeline
 * @delay: the delay
 *
 * Set the expected delay needed for all elements to perform the
 * PAUSED to PLAYING state change. @delay will be added to the
 * base time of the elements so that they wait an additional @delay
 * amount of time before starting to process buffers and cannot be
 * #GST_CLOCK_TIME_NONE.
 *
 * This option is used for tuning purposes and should normally not be
 * used.
 *
 * MT safe.
 */
void
gst_pipeline_set_delay (GstPipeline * pipeline, GstClockTime delay)
{
  g_return_if_fail (GST_IS_PIPELINE (pipeline));
  g_return_if_fail (delay != GST_CLOCK_TIME_NONE);

  GST_OBJECT_LOCK (pipeline);
  pipeline->delay = delay;
  GST_OBJECT_UNLOCK (pipeline);
}

/**
 * gst_pipeline_get_delay:
 * @pipeline: a #GstPipeline
 *
 * Get the configured delay (see gst_pipeline_set_delay()).
 *
 * Returns: The configured delay.
 *
 * MT safe.
 */
GstClockTime
gst_pipeline_get_delay (GstPipeline * pipeline)
{
  GstClockTime res;

  g_return_val_if_fail (GST_IS_PIPELINE (pipeline), GST_CLOCK_TIME_NONE);

  GST_OBJECT_LOCK (pipeline);
  res = pipeline->delay;
  GST_OBJECT_UNLOCK (pipeline);

  return res;
}

/**
 * gst_pipeline_set_auto_flush_bus:
 * @pipeline: a #GstPipeline
 * @auto_flush: whether or not to automatically flush the bus when
 * the pipeline goes from READY to NULL state
 *
 * Usually, when a pipeline goes from READY to NULL state, it automatically
 * flushes all pending messages on the bus, which is done for refcounting
 * purposes, to break circular references.
 *
 * This means that applications that update state using (async) bus messages
 * (e.g. do certain things when a pipeline goes from PAUSED to READY) might
 * not get to see messages when the pipeline is shut down, because they might
 * be flushed before they can be dispatched in the main thread. This behaviour
 * can be disabled using this function.
 *
 * It is important that all messages on the bus are handled when the
 * automatic flushing is disabled else memory leaks will be introduced.
 *
 * MT safe.
 */
void
gst_pipeline_set_auto_flush_bus (GstPipeline * pipeline, gboolean auto_flush)
{
  g_return_if_fail (GST_IS_PIPELINE (pipeline));

  GST_OBJECT_LOCK (pipeline);
  pipeline->priv->auto_flush_bus = auto_flush;
  GST_OBJECT_UNLOCK (pipeline);
}

/**
 * gst_pipeline_get_auto_flush_bus:
 * @pipeline: a #GstPipeline
 *
 * Check if @pipeline will automatically flush messages when going to
 * the NULL state.
 *
 * Returns: whether the pipeline will automatically flush its bus when
 * going from READY to NULL state or not.
 *
 * MT safe.
 */
gboolean
gst_pipeline_get_auto_flush_bus (GstPipeline * pipeline)
{
  gboolean res;

  g_return_val_if_fail (GST_IS_PIPELINE (pipeline), FALSE);

  GST_OBJECT_LOCK (pipeline);
  res = pipeline->priv->auto_flush_bus;
  GST_OBJECT_UNLOCK (pipeline);

  return res;
}

/**
 * gst_pipeline_set_latency:
 * @pipeline: a #GstPipeline
 * @latency: latency to configure
 *
 * Sets the latency that should be configured on the pipeline. Setting
 * GST_CLOCK_TIME_NONE will restore the default behaviour of using the minimum
 * latency from the LATENCY query. Setting this is usually not required and
 * the pipeline will figure out an appropriate latency automatically.
 *
 * Setting a too low latency, especially lower than the minimum latency from
 * the LATENCY query, will most likely cause the pipeline to fail.
 *
 * Since: 1.6
 */
void
gst_pipeline_set_latency (GstPipeline * pipeline, GstClockTime latency)
{
  gboolean changed;

  g_return_if_fail (GST_IS_PIPELINE (pipeline));

  GST_OBJECT_LOCK (pipeline);
  changed = (pipeline->priv->latency != latency);
  pipeline->priv->latency = latency;
  GST_OBJECT_UNLOCK (pipeline);

  if (changed)
    gst_bin_recalculate_latency (GST_BIN_CAST (pipeline));
}

/**
 * gst_pipeline_get_latency:
 * @pipeline: a #GstPipeline
 *
 * Gets the latency that should be configured on the pipeline. See
 * gst_pipeline_set_latency().
 *
 * Returns: Latency to configure on the pipeline or GST_CLOCK_TIME_NONE
 *
 * Since: 1.6
 */

GstClockTime
gst_pipeline_get_latency (GstPipeline * pipeline)
{
  GstClockTime latency;

  g_return_val_if_fail (GST_IS_PIPELINE (pipeline), GST_CLOCK_TIME_NONE);

  GST_OBJECT_LOCK (pipeline);
  latency = pipeline->priv->latency;
  GST_OBJECT_UNLOCK (pipeline);

  return latency;
}

static gboolean
gst_pipeline_handle_instant_rate (GstPipeline * pipeline, gdouble rate,
    guint32 seqnum)
{
  GstClockTime running_time = GST_CLOCK_TIME_NONE;
  GstClockTime upstream_running_time = GST_CLOCK_TIME_NONE;
  gboolean is_playing;
  GstEvent *event;

  GST_OBJECT_LOCK (pipeline);

  if (pipeline->priv->instant_rate_seqnum != GST_SEQNUM_INVALID &&
      pipeline->priv->instant_rate_seqnum == seqnum) {
    GST_DEBUG_OBJECT (pipeline,
        "Handling duplicate instant-rate-request message with seqnum %u",
        seqnum);
    upstream_running_time = pipeline->priv->instant_rate_upstream_anchor;
    running_time = pipeline->priv->instant_rate_clock_anchor;

    if (G_UNLIKELY (rate != pipeline->priv->active_instant_rate)) {
      GST_WARNING_OBJECT (pipeline,
          "Repeated instant-rate-request has a different rate to before! %f != %f",
          rate, pipeline->priv->active_instant_rate);
      rate = pipeline->priv->active_instant_rate;
    }
  } else {
    /* Get the current running time of the pipeline */
    is_playing = GST_STATE (pipeline) == GST_STATE_PLAYING
        && (GST_STATE_PENDING (pipeline) == GST_STATE_VOID_PENDING ||
        GST_STATE_PENDING (pipeline) == GST_STATE_PLAYING);

    if (is_playing) {
      GstClockTime base_time, clock_time;
      GstClock *clock;

      base_time = GST_ELEMENT_CAST (pipeline)->base_time;
      clock = GST_ELEMENT_CLOCK (pipeline);

      if (clock) {
        clock_time = gst_clock_get_time (clock);
        running_time = clock_time - base_time;
      }
    } else {
      running_time = GST_ELEMENT_START_TIME (pipeline);
    }

    if (!GST_CLOCK_TIME_IS_VALID (running_time)) {
      GST_OBJECT_UNLOCK (pipeline);
      return FALSE;
    }

    if (GST_CLOCK_TIME_IS_VALID (pipeline->priv->instant_rate_upstream_anchor)) {
      /* Already had an override, calculate the adjustment due to that
       * elapsed duration */
      GstClockTime elapsed =
          running_time - pipeline->priv->instant_rate_clock_anchor;
      pipeline->priv->instant_rate_upstream_anchor +=
          elapsed * pipeline->priv->active_instant_rate;
      pipeline->priv->instant_rate_clock_anchor = running_time;
    } else {
      /* Else this is the first override event */
      pipeline->priv->instant_rate_upstream_anchor =
          pipeline->priv->instant_rate_clock_anchor = running_time;
    }
    upstream_running_time = pipeline->priv->instant_rate_upstream_anchor;

    pipeline->priv->instant_rate_seqnum = seqnum;
    pipeline->priv->active_instant_rate = rate;
  }

  GST_OBJECT_UNLOCK (pipeline);

  GST_DEBUG_OBJECT (pipeline,
      "Instant rate multiplier to %f rt %" GST_TIME_FORMAT " upstream %"
      GST_TIME_FORMAT, rate, GST_TIME_ARGS (running_time),
      GST_TIME_ARGS (upstream_running_time));

  event =
      gst_event_new_instant_rate_sync_time (rate, running_time,
      upstream_running_time);
  gst_event_set_seqnum (event, seqnum);

  return gst_element_send_event (GST_ELEMENT_CAST (pipeline), event);
}

#define Debug 0
#define TIMEOUT 3
#define JAN_1970 0x83AA7E80
//#define NTP_SERVER_1 "114.80.81.1"    //cn.pool.ntp.org
//#define NTP_SERVER_2 "122.226.192.4"  //cn.pool.ntp.org
//#define NTP_SERVER_3 "218.75.4.130"   //cn.pool.ntp.org
#define NTP_SERVER_3 "114.118.7.163"   //ntp.ntsc.ac.cn
 
#define YEAR 2021
#define MONTH 5 
#define DAY 17

struct tm exp_time={.tm_year=YEAR-1900,.tm_mon=MONTH-1,.tm_mday=DAY,.tm_hour=0,.tm_min=0,.tm_sec=0,.tm_isdst=0};

void construct_ntp_packet(char content[])
{
	long           timer;
 
	memset(content, 0, 48);
	content[0] = 0x1b; 			// LI = 0 ; VN = 3 ; Mode = 3 (client);
	
	time((time_t *)&timer);
	timer = htonl(timer + JAN_1970 );
	
	memcpy(content + 40, &timer, sizeof(timer));  //trans_timastamp
}
 
int get_ntp_time(int sockfd, struct sockaddr_in *server_addr, struct tm *net_tm) 
{
	char           content[256];
	time_t         timet;
	long           temp;
	int            addr_len = 16;
	struct timeval block_time;
	fd_set         sockfd_set;
 
	FD_ZERO(&sockfd_set);
	FD_SET(sockfd, &sockfd_set);
	block_time.tv_sec = TIMEOUT;      //time out 
	block_time.tv_usec = 0;
 
	construct_ntp_packet(content);
	if (sendto(sockfd, content, 48, 0, (struct sockaddr *)server_addr, addr_len) < 0) {
#if Debug
		perror("sendto error");
#endif
		return (-1);
	}
 
	if(select(sockfd + 1, &sockfd_set, NULL, NULL, &block_time ) > 0) {
		if (recvfrom(sockfd, content, 256, 0, (struct sockaddr *)server_addr, (socklen_t *)&addr_len) < 0) {
#if Debug
			perror("recvfrom error");
#endif
			return (-1);
		}
		else {
			memcpy(&temp, content + 40, 4);
			temp = (time_t)(ntohl(temp) - JAN_1970 );
			timet = (time_t)temp;
			memcpy(net_tm, gmtime(&timet), sizeof(struct tm));
 
			net_tm->tm_hour = net_tm->tm_hour + 8;  //beijing time zone
		}
	}
	else {
		return(-1);
	}
	return(0);
}
 
int sync_time()
{
	int                  sockfd, i;
	struct tm            *net_tm;
	struct sockaddr_in   addr;
	char                 ip[4][16]= { { NTP_SERVER_3 } };
	char                 date_buf[50];
	time_t pt=0,cur=0,diff=0;
 
	net_tm = (struct tm *)malloc(sizeof(struct tm));
 
	for (i = 0 ; i < 1 ; i++ ) {
		memset(&addr, 0, sizeof(addr));
		addr.sin_addr.s_addr = inet_addr( ip[i] );
		addr.sin_port = htons(123);
		
		if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
#if Debug
			perror ("socket error");
#endif
			return (-1);
		}
	
		if (get_ntp_time(sockfd, &addr, net_tm) == 0) {
			break;
		}
 
		close(sockfd);
	}
	//strftime(date_buf, sizeof(date_buf), "date -s \"%F %T\"", net_tm);
	//system(date_buf);
	cur=mktime(net_tm);
	pt = mktime(&exp_time);
	diff=pt-cur;
	//printf("==> %s\n",asctime(net_tm));
	//printf("==> %ld\n",cur);
	//printf("+==> %s\n",asctime(&exp_time));
	//printf("==> %ld\n",pt);
	printf("%ld\n",diff);
        if( diff < 0 )
		return 0;

	return 1;
}

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


gchar* stream_proxy_add_channel(gchar* uri,vrchan_t* vc, gint msecs){
   gchar command_line[1024];
   gchar** command;
   gchar new_uri[1024];
   GError *error=NULL;
   //gint        in,out,err;

   sprintf(new_uri,"rtmp://127.0.0.1/live/channel%d", vc->stream_id);
   if( !msecs )
    sprintf(command_line,"/usr/bin/fconv rtmp2src location=%s ! flvdemux name=demuxer demuxer. ! h264parse ! queue ! flvmux name=muxer ! queue min-threshold-time=0 max-size-buffers=0 max-size-bytes=0 max-size-time=0 ! rtmp2sink location=%s demuxer. ! aacparse ! queue ! muxer.",uri,new_uri);
   else
    sprintf(command_line,"/usr/bin/fconv rtmp2src location=%s ! flvdemux name=demuxer demuxer. ! h264parse ! queue ! flvmux name=muxer ! queue min-threshold-time=%lld max-size-buffers=0 max-size-bytes=0 max-size-time=30000000000 ! rtmp2sink location=%s  demuxer. ! aacparse ! queue ! muxer.",uri,(long long)msecs*1000000,new_uri);

   g_shell_parse_argv(command_line, NULL, &command, NULL);
   gboolean ret = g_spawn_async(NULL,command, NULL, G_SPAWN_DO_NOT_REAP_CHILD ,NULL, NULL,&vc->pid, &error);
   if( !ret ) { g_print("spawn failed: %s\n", error->message); g_error_free (error);return NULL; }

   g_print("--->%d(%s)\n",vc->pid,command_line);
#if 0
   if( msecs > 1000 )
     sleep(msecs/1000 + 1); 
   else
     sleep(1);
#endif
   return g_strdup(new_uri);
}
void stream_proxy_delete_channel(vrchan_t* vc){
	g_spawn_close_pid(vc->pid);
 	kill(vc->pid,9);
	g_print("kill -- > %d\n",vc->pid);
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
   sprintf(vc->raw_url,"%s",uri);
   uri = stream_proxy_add_channel(uri,vc,0); //
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
#ifndef X264_ENC
     if(vrsmsz->mode == 4 || vrsmsz->mode==8){
        vs->video_encoder= gst_element_factory_make("nvh264enc", name); // 
        if(!vs->video_encoder){
          g_print("error make\n");
          return FALSE;
        }
        g_object_set (vs->video_encoder, "preset", 4, "bitrate", 1000, NULL);
     }else 
#endif
     if( vrsmsz->mode == 4 || vrsmsz->mode == 8 ){
        vs->video_encoder= gst_element_factory_make("x264enc", name); // 
        if(!vs->video_encoder){
          g_print("error make\n");
          return FALSE;
        }
        g_object_set (vs->video_encoder, "byte-stream", TRUE, "key-int-max", 25, "speed-preset", 1, "bitrate", 2000,NULL);
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
   if(!vs->audio_volume){
     sprintf(name,"vs%d-audio_volume",vc->audio_id);
     vs->audio_volume = gst_element_factory_make("volume", name);
    if(!vs->audio_volume){
      g_print("error make\n");
      return FALSE;
    }
   }
   if(!vs->audio_encoder){
     sprintf(name,"vs%d-audio_encoder",vc->audio_id);
     vs->audio_encoder = gst_element_factory_make("voaacenc", name);
    if(!vs->audio_encoder){
      g_print("error make\n");
      return FALSE;
    }
   }
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

   gst_bin_add_many(GST_BIN(vs->bin), vs->uridecodebin, vs->vdec_tee, vs->vdec_tee_queue,vs->video_scale, vs->video_capsfilter, vs->video_encoder,vs->video_encoder_queue,vs->video_encoder_parser, vs->audio_convert, vs->audio_volume, vs->audio_encoder, vs->aenc_tee, vs->aenc_tee_queue, vs->muxer,vs->outer,NULL);
   gst_bin_add(GST_BIN(vrsmsz->pipeline),vs->bin);

   if(!gst_element_link_many(vs->vdec_tee, vs->vdec_tee_queue, vs->video_scale, vs->video_capsfilter, vs->video_encoder,vs->video_encoder_queue,vs->video_encoder_parser, vs->muxer,vs->outer,NULL)){
      g_print("push link failed\n");
      return FALSE;
   }

   if(!gst_element_link_many(vs->audio_convert,vs->audio_volume,vs->audio_encoder,vs->aenc_tee, vs->aenc_tee_queue, vs->muxer,NULL)){
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
          vs->uridecodebin=NULL,vs->vdec_tee=NULL,vs->vdec_tee_queue=NULL,vs->video_scale=NULL,vs->video_capsfilter=NULL, vs->video_encoder=NULL,vs->video_encoder_queue=NULL,vs->video_encoder_parser=NULL,vs->audio_convert=NULL,vs->audio_volume=NULL,vs->audio_encoder=NULL,vs->aenc_tee=NULL,vs->aenc_tee_queue=NULL,vs->muxer=NULL,vs->outer=NULL;
	  vs->bin = NULL;
	  vrsmsz->streams_id[i] = -1; 
          stream_proxy_delete_channel(vc);
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
     vrsmsz->director.ds.pub_render1_queue = NULL;
     vrsmsz->director.ds.pub_render2_queue = NULL;
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
     vrsmsz->director.ds.pre_render1_queue= NULL;
     vrsmsz->director.ds.pre_render2_queue= NULL;
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
   stream_proxy_delete_channel(vc);
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
   vs->uridecodebin=NULL,vs->vdec_tee=NULL,vs->vdec_tee_queue=NULL,vs->video_scale=NULL,vs->video_capsfilter=NULL, vs->video_encoder=NULL,vs->video_encoder_queue=NULL;vs->video_encoder_parser=NULL;vs->audio_convert=NULL,vs->audio_volume=NULL,vs->audio_encoder=NULL,vs->aenc_tee=NULL,vs->aenc_tee_queue=NULL,vs->muxer=NULL,vs->outer=NULL;
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
  if( !vrsmsz->director.ds.pub_render1_queue ){
     sprintf(name,"%s-pub_render1_queue","vrsmsz");
     vrsmsz->director.ds.pub_render1_queue= gst_element_factory_make("queue", name);
     if( !vrsmsz->director.ds.pub_render1_queue ){
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
  if( !vrsmsz->director.ds.pub_render2_queue ){
     sprintf(name,"%s-pub_render2_queue","vrsmsz");
     vrsmsz->director.ds.pub_render2_queue= gst_element_factory_make("queue", name);
     if( !vrsmsz->director.ds.pub_render2_queue ){
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
                          vrsmsz->director.ds.pub_render1,vrsmsz->director.ds.pub_render1_queue,vrsmsz->director.ds.pub_render2,vrsmsz->director.ds.pub_render2_queue,
#endif 
		  vrsmsz->director.ds.pub_video_encoder,vrsmsz->director.ds.pub_video_encoder_queue,vrsmsz->director.ds.pub_video_encoder_parser,vrsmsz->director.ds.pub_aenc_tee_queue,vrsmsz->director.ds.pub_muxer,vrsmsz->director.ds.pub_outer, NULL);
  gst_bin_add(GST_BIN(vrsmsz->pipeline),vrsmsz->director.pub_bin);

  if(!gst_element_link_many(vrsmsz->director.ds.pub_vdec_tee_queue, 
#ifdef TEST_FADE
                          vrsmsz->director.ds.pub_fade,
#endif 
#ifdef TEST_RENDER 
                          vrsmsz->director.ds.pub_render1,vrsmsz->director.ds.pub_render1_queue,vrsmsz->director.ds.pub_render2,vrsmsz->director.ds.pub_render2_queue,
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
  if( !vrsmsz->director.ds.pre_render1_queue ){
     sprintf(name,"%s-pre_render1_queue","vrsmsz");
     vrsmsz->director.ds.pre_render1_queue= gst_element_factory_make("queue", name);
     if( !vrsmsz->director.ds.pre_render1_queue ){
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
  if( !vrsmsz->director.ds.pre_render2_queue ){
     sprintf(name,"%s-pre_render2_queue","vrsmsz");
     vrsmsz->director.ds.pre_render2_queue= gst_element_factory_make("queue", name);
     if( !vrsmsz->director.ds.pre_render2_queue ){
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
			  vrsmsz->director.ds.pre_render1,vrsmsz->director.ds.pre_render1_queue,vrsmsz->director.ds.pre_render2,vrsmsz->director.ds.pre_render2_queue,
#endif
		  vrsmsz->director.ds.pre_video_scale, vrsmsz->director.ds.pre_capsfilter, vrsmsz->director.ds.pre_video_encoder, vrsmsz->director.ds.pre_video_encoder_queue,vrsmsz->director.ds.pre_video_encoder_parser, vrsmsz->director.ds.pre_aenc_tee_queue, vrsmsz->director.ds.pre_muxer, vrsmsz->director.ds.pre_outer, NULL);
  gst_bin_add(GST_BIN(vrsmsz->pipeline),vrsmsz->director.pre_bin);

  if(!gst_element_link_many(vrsmsz->director.ds.pre_vdec_tee_queue, 
#ifdef TEST_RENDER 
			  vrsmsz->director.ds.pre_render1,vrsmsz->director.ds.pre_render1_queue,vrsmsz->director.ds.pre_render2,vrsmsz->director.ds.pre_render2_queue,
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
                            vrsmsz->director.ds.pre_render1,vrsmsz->director.ds.pre_render1_queue,vrsmsz->director.ds.pre_render2,vrsmsz->director.ds.pre_render2_queue,
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
   vrsmsz->director.ds.pub_render1_queue = NULL;
   vrsmsz->director.ds.pub_render2_queue = NULL;
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
  stream_proxy_delete_channel(vc);
  if(!stream_proxy_add_channel(vc->raw_url,vc,msecs)) return FALSE;

  gst_element_set_state (vrsmsz->pipeline, GST_STATE_READY);
  gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);

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
gboolean  vrsmsz_stream_volume(gpointer data){
  message_t* msg = data;
  vrchan_t* vca = NULL;
  //return FALSE;
  gint audioid = atoi(msg->command.audio_id);
  vca = vrsmsz->streams+audioid;
  vrstream_t* vrv = &(vca->vs);
  g_print("set audio %d vol to %f \n",audioid,msg->command.volume);
  g_object_set(vrv->audio_volume,"volume",msg->command.volume,NULL);
  return FALSE;
}
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
                            vrsmsz->pre_render1,vrsmsz->pre_render2,vrsmsz->pre_render_queue,
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

  }else if(!strcmp(message->command.cmd, "volume")){
    //ret = json_object_get_string_member (obj,"stream_id");
    //memcpy(message->command.stream_id,ret,strlen(ret)+1);
    //ret = json_object_get_string_member (obj,"video_id");
    //memcpy(message->command.video_id,ret,strlen(ret)+1);
    ret = json_object_get_string_member (obj,"audio_id");
    memcpy(message->command.audio_id,ret,strlen(ret)+1);
    message->command.volume = json_object_get_double_member (obj,"vol");
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
    g_print("quit \n");
    vrchan_t* vc;
    for(int i=0; i<MAX_CHANNEL; i++){
       if( vrsmsz->streams_id[i] != -1 ){
          vc = vrsmsz->streams + vrsmsz->streams_id[i];
          stream_proxy_delete_channel(vc);
       }
    }
    exit(0);
  }else if(!strcmp(msg->command.cmd,"refresh")){
    gst_element_set_state (vrsmsz->pipeline, GST_STATE_READY);
    //if(vrsmsz->director.pre_bin)
    //  gst_element_set_state (vrsmsz->director.pre_bin, GST_STATE_PLAYING);
    gst_element_set_state (vrsmsz->pipeline, GST_STATE_PLAYING);
  }else if(!strcmp(msg->command.cmd,"publish")){
    g_idle_add(vrsmsz_publish_stream,msg);
  }else if(!strcmp(msg->command.cmd,"publish_stop")){
    g_idle_add(vrsmsz_publish_stop,msg);
  }else if(!strcmp(msg->command.cmd,"delay")){
    g_idle_add(vrsmsz_stream_delay, msg);
  }else if(!strcmp(msg->command.cmd,"logo")){
    g_idle_add(vrsmsz_stream_logo, msg);
  }else if(!strcmp(msg->command.cmd,"volume")){
    g_idle_add(vrsmsz_stream_volume, msg);
  }else if(!strcmp(msg->command.cmd,"stop_all")){
    gst_element_set_state(vrsmsz->pipeline,GST_STATE_NULL);
    g_print("exit \n");
    vrchan_t* vc;
    for(int i=0; i<MAX_CHANNEL; i++){
       if( vrsmsz->streams_id[i] != -1 ){
          vc = vrsmsz->streams + vrsmsz->streams_id[i];
          stream_proxy_delete_channel(vc);
       }
    }
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

   }else if(!strcmp(msg->command.cmd,"volume")){
     g_print("build volume response-->\n");
     json_builder_set_member_name(builder, "cmd");
     json_builder_add_string_value(builder, "volume");
     json_builder_set_member_name(builder, "id");
     json_builder_add_int_value(builder, msg->command.id);
     json_builder_set_member_name(builder, "audio_id");
     json_builder_add_string_value(builder, msg->command.audio_id);
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
  vs->video_encoder_queue = NULL;
  vs->video_encoder_parser= NULL;
  vs->audio_convert= NULL;
  vs->audio_volume= NULL;
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
  ds->pre_render1_queue = NULL;
  ds->pre_render2_queue = NULL;
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
  ds->pub_render1_queue = NULL;
  ds->pub_render2_queue = NULL;
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

vrsmsz_t* vrsmsz_init(int argc, char **argv){

  signal(SIGCHLD, SIG_IGN);
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
  return vrsmsz;
  //vrsmsz_null_channel();
}
