#include <gst/gst.h>
#include "httpd.h"
#define MAX_CHANNEL 16
#define MAX_VIDEO_TRACK  16
#define MAX_AUDIO_TRACK  16
#define DESCRIBE_LEN 2048
#define URL_LEN 2048

typedef struct{
  GstElement * uridecodebin; // 
  GstElement * vdec_tee;
  GstElement * vdec_tee_queue;
  GstPad* pre_vdec_tee_srcpad;
  GstPad* pub_vdec_tee_srcpad;
  GstPad* vdec_tee_filter_srcpad;
  GstPad* vdec_filter_tee_queue_sinkpad;
  GstElement* vdec_filter_tee_queue;
  GstElement* vdec_filter_tee_convert;
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


typedef struct {
  GstElement  *bin;
  gint    tracks;
  gint   stream_id;
  gint   video_id;
  gint   audio_id;
  gint   subs_id;
  gchar   in_url[URL_LEN];
  gchar   output_url[URL_LEN];
  gchar   preview_url[URL_LEN];
  guint   resolution; // 4k or 8K
  vrstream_t vs;
}vrchan_t;

typedef struct{
  //GstElement * videoconverter;
  GstPad* video_filter_queue_sinkpad; // 
  GstElement * video_filter_queue; // 
  GstElement * videoconvert; // 
  GstElement * video_filter_chain; // 
  GstElement * video_filter_tee; // 

  //GstElement * audio_filter; // 
  GstPad* pre_video_filter_tee_srcpad;
  GstElement * pre_vdec_tee_queue; // points
  GstPad *     pre_vdec_tee_queue_sinkpad;
  GstElement * pre_video_scale;
  GstElement * pre_capsfilter;
  GstElement * pre_video_encoder;
  GstElement * pre_aenc_tee_queue;
  GstPad *     pre_aenc_tee_queue_sinkpad;
  GstElement * pre_muxer;
  GstElement * pre_outer; // 

  GstPad* pub_video_filter_tee_srcpad;
  GstElement * pub_vdec_tee_queue; // points
  GstPad *     pub_vdec_tee_queue_sinkpad;
  //GstElement * pub_capfilter;
  GstElement * pub_video_encoder;
  GstElement * pub_aenc_tee_queue;
  GstPad *     pub_aenc_tee_queue_sinkpad;
  GstElement * pub_muxer;
  GstElement * pub_outer; // 
  //GstElement * audio_encoder;
  GstElement * mixer; // audio mix
  //GstElement * comp;  // video mix

} drstream_t;

typedef struct{
  GstElement  *pre_bin;
  GstElement  *pub_bin;
  GstElement  *swt_bin;
  GstElement  *eff_bin;

  gint   stream_id;
  gint   video_id;
  gint   audio_id;
  gchar   preview_url[URL_LEN];
  gchar   publish_url[URL_LEN];

  drstream_t ds;
} drchan_t;

typedef struct{
  GstElement* pipeline;
  GMainLoop *loop;
  GstClock *theclock;
  GstBus *bus;

  guint mode; // 8K or 4K or 2K

  vrchan_t streams[MAX_CHANNEL*100];  
  gint stream_nbs;  
  guint streams_id[MAX_CHANNEL*100];

  drchan_t director;

  GAsyncQueue * queue;
  gboolean canSwitch;
  
  vrchan_t *remove_chan;
} vrsmsz_t;

extern vrsmsz_t* vrsmsz;

void vrsmsz_init(int argc, char **argv);
void vrsmsz_play();
void vrsmsz_stop();
void vrsmsz_quit();
gboolean vrsmsz_clean();
gboolean vrsmsz_recover();
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


