#include <gst/gst.h>
#define MAX_CHANNEL 16
#define MAX_VIDEO_TRACK  16
#define MAX_AUDIO_TRACK  16
#define DESCRIBE_LEN 2048
#define URL_LEN 2048

typedef struct{
  gchar cmd[256];	
  gint id;
  gchar in_url[1024];
  gchar pre_url[1024];
  gchar pub_url[1024];
  gchar stream_id[16];
  gchar video_id[16];
  gchar audio_id[16];

  gchar effect[16];

  gint in_bitrate;
  gint in_width;
  gint in_height;
  gint in_fps;

  gint pub_bitrate;
  gint pub_width;
  gint pub_height;
  gint pub_fps;

  gchar logo_path[1024];
  gchar pip_path[1024];
  gchar text_string[1024];
  gint  left;
  gint  top;
  gint width;
  gint height;
  gchar action[1024];
  gint duration;
  gint render_id;

  gchar font[16];
  gint  font_size;

  gchar delay_type[16];
  gint64 delay_time;
  gdouble volume;

}command_t;

typedef struct{
  GstElement * uridecodebin; 
  GstPad* video_src_pad;
  GstPad* audio_src_pad;
  gint video_time;
  gint audio_time;
  GstElement * vdec_tee;
  GstElement * vdec_tee_queue;
  GstPad* pre_vdec_tee_srcpad;
  GstPad* pre_vdec_tee_ghost_srcpad;
  GstPad* pub_vdec_tee_srcpad;
  GstPad* pub_vdec_tee_ghost_srcpad;

  GstPad* vdec_tee_filter_srcpad;
  GstPad* vdec_filter_tee_queue_sinkpad;
  GstElement* vdec_filter_tee_queue;
  GstElement* vdec_filter_tee_convert;

  GstElement * video_scale;
  GstElement * video_capsfilter;
  GstElement * video_encoder;
  GstElement * video_encoder_queue;
  GstElement * video_encoder_parser;
  GstElement * audio_convert;
  GstElement * audio_volume;
  GstElement * audio_encoder;
  GstElement * aenc_tee;
  GstPad* pre_aenc_tee_srcpad;
  GstPad* pre_aenc_tee_ghost_srcpad;
  GstPad* pub_aenc_tee_srcpad;
  GstPad* pub_aenc_tee_ghost_srcpad;
  GstElement * aenc_tee_queue;
  GstElement * muxer;  
  GstElement * outer;  
  GstElement * bin;
} vrstream_t;

typedef struct {
  gint    tracks;
  gint    stream_id;
  gint    video_id;
  gint    audio_id;
  gint    subs_id;
  gchar   in_url[URL_LEN];
  gchar   output_url[URL_LEN];
  gchar   preview_url[URL_LEN];
  gchar   push_url[URL_LEN];
  guint   resolution;
  vrstream_t vs;

  gint     status; // 0:null, 1:pending , 2:running, 3: error 
  gint     audio_status; // 0:null, 1:ready, 2:running, 3: error
  gint     video_status; // 0:null, 1:ready, 2:running, 3: error

  gchar   raw_url[URL_LEN];
  GPid    pid;
}vrchan_t;

typedef struct {
  gchar req[4096];
  gchar rep[4096];
  gint  errcode;
  gint  timeout;
  gboolean is_responsed;
  command_t command;
  vrchan_t* vc;
}message_t;

typedef struct{
  //compositor for fade;
  GstPad* video_filter_queue_sinkpad;
  GstElement * video_filter_queue;  
  GstElement * videoconvert;  
  GstElement * video_filter_chain;  
  GstElement * video_filter_tee;  

  GstPad* vfilter_queue_vdec_tee_srcpd;  
  GstElement * vfilter_vdec_tee_queue;  
  GstElement * video_convert;  
  GstElement * wrapper ;  
  GstElement * vfilter_tee;  

  GstPad* pre_video_filter_tee_srcpad;
  GstElement * pre_vdec_tee_queue;  
  GstPad *     pre_vdec_tee_queue_sinkpad;
  GstPad *     pre_vdec_tee_queue_ghost_sinkpad;
  GstElement * pre_fade;
  GstElement * pre_render1;
  GstElement * pre_render2;
  GstElement * pre_render1_queue;
  GstElement * pre_render2_queue;
  GstElement * pre_video_scale;
  GstElement * pre_capsfilter;
  GstElement * pre_video_encoder;
  GstElement * pre_video_encoder_queue;
  GstElement * pre_video_encoder_parser;
  GstElement * pre_aenc_tee_queue;
  GstPad *     pre_aenc_tee_queue_sinkpad;
  GstPad *     pre_aenc_tee_queue_ghost_sinkpad;
  GstElement * pre_muxer;
  GstElement * pre_outer;  

  GstPad* pub_video_filter_tee_srcpad;
  GstElement * pub_vdec_tee_queue;  
  GstPad *     pub_vdec_tee_queue_sinkpad;
  GstPad *     pub_vdec_tee_queue_ghost_sinkpad;
  GstElement * pub_fade;
  GstElement * pub_render1;
  GstElement * pub_render2;
  GstElement * pub_render1_queue;
  GstElement * pub_render2_queue;
  GstElement * pub_video_encoder;
  GstElement * pub_video_encoder_queue;
  GstElement * pub_video_encoder_parser;
  GstElement * pub_aenc_tee_queue;
  GstPad *     pub_aenc_tee_queue_sinkpad;
  GstPad *     pub_aenc_tee_queue_ghost_sinkpad;
  GstElement * pub_muxer;
  GstElement * pub_outer; 
  GstElement * mixer; 

} drstream_t;

typedef struct{
  gchar url[URL_LEN];
  gint    left;
  gint    top;
  gint    width;
  gint    height;
  gint    duration;
  gint    type;
  gint    id;
}vrrender_t;

typedef struct{
  GstElement  *pre_bin;
  GstElement  *pub_bin;
  GstElement  *swt_bin;
  GstElement  *eff_bin;

  gint    stream_id;
  gint    video_id;
  gint    audio_id;
  gchar   preview_url[URL_LEN];
  gchar   push_url[URL_LEN];
  gchar   publish_url[URL_LEN];
  drstream_t ds;

  vrrender_t render[2];
  int  num_render;

} drchan_t;

typedef struct{
  GstElement* pipeline;
  GMainLoop *loop;
  GstClock *theclock;
  GstBus *bus;

  guint mode; 

  vrchan_t streams[MAX_CHANNEL*100];  
  gint  stream_nbs;  
  guint streams_id[MAX_CHANNEL*100];

  drchan_t director;

  GAsyncQueue * req_queue;
  GAsyncQueue * rep_queue;
  gboolean isPreSwitched;
  gboolean isPubSwitched;
  
} vrsmsz_t;

extern vrsmsz_t* vrsmsz;

GST_API vrsmsz_t* vrsmsz_init(int argc, char **argv);
GST_API void vrsmsz_start();
void vrsmsz_play();
GST_API void vrsmsz_stop();
void vrsmsz_quit();
gboolean vrsmsz_clean();
gboolean vrsmsz_recover();
GST_API void vrsmsz_deinit();

gboolean vrsmsz_add_stream(gpointer data);
gboolean vrsmsz_remove_stream(gpointer data);
gboolean vrsmsz_switch_stream(gpointer data);
void vrsmsz_add_track();
void vrsmsz_switch_track();
void vrsmsz_switch_stream_crossfade();
void vrsmsz_add_text();
void vrsmsz_add_pic();
void vrsmsz_add_clip();

