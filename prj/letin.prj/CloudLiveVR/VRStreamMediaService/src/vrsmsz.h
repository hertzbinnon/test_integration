#include <gst/gst.h>
#include "httpd.h"
#define MAX_CHANNEL 16
#define MAX_VIDEO_TRACK  16
#define MAX_AUDIO_TRACK  16
#define DESCRIBE_LEN 2048
#define URL_LEN 2048

typedef struct{
  //GstElement * video_dec; // 0 for null channel with videotestsrc
  //GstElement * audio_dec; // 0 for null channel with audiotestsrc
  guint video_id;
  guint audio_id;
  guint stream_id;
  gchar src_url[URL_LEN];
  gchar pre_sink_url[URL_LEN];
  guint dis; // 4k or 8K
  GstElement * uridecodebin; // 
  GstElement * vdec_tee;
  GstElement * vdec_tee_queue;
  GstPad* pre_vdec_tee_srcpad;
  GstPad* pub_vdec_tee_srcpad;
  GstElement * video_scale;
  GstElement * video_capsfilter;
  GstElement * video_encoder;
  GstElement * audio_convert;
  GstElement * audio_encoder;
  GstElement * aenc_tee;
  GstPad* pre_aenc_tee_srcpad;
  GstPad* pub_aenc_tee_srcpad;
  GstElement * aenc_tee_queue;
  GstElement * muxer; // 
  GstElement * outer; // 
} vrstream_t;

typedef struct{
  vrstream_t streams[MAX_CHANNEL]; // 0 is for null stream;
  guint streams_id[MAX_CHANNEL];
  //GstElement * videoconverter;
  GstElement * queue_video_filter; // 
  GstElement * videoconvert; // 
  GstElement * video_chain; // 
  GstElement * tee_filter; // 

  //GstElement * audio_filter; // 
  GstElement * pre_vdec_tee_queue;
  GstPad *     pre_vdec_tee_queue_sinkpad;
  GstElement * pre_video_scale;
  GstElement * pre_capsfilter;
  GstElement * pre_video_encoder;
  GstElement * pre_aenc_tee_queue;
  GstPad *     pre_aenc_tee_queue_sinkpad;
  GstElement * pre_muxer;
  GstElement * pre_outer; // 

  GstElement * pub_vdec_tee_queue;
  GstPad *     pub_vdec_tee_queue_sinkpad;
  //GstElement * pub_capfilter;
  GstElement * pub_video_encoder;
  GstElement * pub_aenc_tee_queue;
  GstPad *     pub_aenc_tee_queue_sinkpad;
  GstElement * pub_muxer;
  GstElement * pub_outer; // 
  //GstElement * audio_encoder;
  GstElement * mixer; // audio mix
  GstElement * comp;  // video mix

  gchar comp_sink0_pad_name[16];
  gchar comp_sink1_pad_name[16];
  //GstElement * tee;
  guint v_director;
  guint a_director;

  gchar director_stream_preview_url[URL_LEN];
  gchar director_stream_publish_url[URL_LEN];
  guint mode;

  gint stream_nbs; // how much streams
  GstElement* pipeline;
  GMainLoop *loop;
  GstClock *theclock;
  GstBus *bus;
  
  GAsyncQueue * queue;
  gboolean canSwitch;
} vrsmsz_t;

extern vrsmsz_t* vrsmsz;
void vrsmsz_init(int argc, char **argv);
void vrsmsz_start();
void vrsmsz_play();
void vrsmsz_stop();
void vrsmsz_quit();
void vrsmsz_deinit();

gboolean vrsmsz_add_stream(gpointer data);
gboolean vrsmsz_remove_stream(gpointer data);
gboolean vrsmsz_switch_stream(gpointer data);
void vrsmsz_add_track();
void vrsmsz_switch_track();
void vrsmsz_switch_stream_crossfade();
void vrsmsz_add_text();
void vrsmsz_add_pic();
void vrsmsz_add_clip();


