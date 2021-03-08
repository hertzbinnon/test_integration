#include <gst/gst.h>
#include <gst/sdp/sdp.h>
#include "httpd.h"
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
  gint left;
  gint top;
  gint width;
  gint height;
  gchar action[1024];
  gint duration;
  gint render_id;

  gchar font[16];
  gint  font_size;

  gchar delay_type[16];
  gint delay_time;

}command_t;

typedef struct{
  GstElement * uridecodebin; // 
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
  GstElement * video_enccapsfilter;
  GstElement * venc_tee;
  GstElement * video_encoder_queue;
  GstElement * video_encoder_parser;
  GstElement * audio_convert;
  GstElement * audio_encoder;
  GstElement * aenc_tee;
  GstElement * audio_encoder_parser;
  GstPad* pre_aenc_tee_srcpad;
  GstPad* pre_aenc_tee_ghost_srcpad;
  GstPad* pub_aenc_tee_srcpad;
  GstPad* pub_aenc_tee_ghost_srcpad;
  GstElement * aenc_tee_queue;
  GstElement * muxer; // 
  GstElement * outer; // 
  GstElement * bin;
} vrstream_t;

typedef struct{
  gchar our_id[256];
  gchar peer_id[256];
  gint  src_id;

  GstSDPMessage *sdp;

  GstPad *    audio_srcpad;
  GstPad *    audio_ghost_srcpad;
  GstPad *    audio_sinkpad;
  GstPad *    audio_ghost_sinkpad;
  GstElement* audioenc_queue;
  GstElement* audiortppay;
  GstElement* audioqueue;
  GstElement* audiocaps;

  GstPad *    video_srcpad;
  GstPad *    video_ghost_srcpad;
  GstPad *    video_sinkpad;
  GstPad *    video_ghost_sinkpad;
  GstElement* videoenc_queue;
  GstElement* videortppay;
  GstElement* videoqueue;
  GstElement* videocaps;
  GstElement* webrtc;
  GstElement *bin;

  GObject *send_channel;
  GObject *receive_channel;
} PeerStruct;

typedef struct {
  gint    tracks;
  gint    stream_id;
  gint    video_id;
  gint    audio_id;
  gint    subs_id;
  gchar   in_url[URL_LEN];
  gchar   output_url[URL_LEN];
  gchar   preview_url[URL_LEN];
  guint   resolution; // 4k or 8K
  vrstream_t vs;

  gint     status; // 0:null, 1:pending , 2:running, 3: error 
  gint     audio_status; // 0:null, 1:ready, 2:running, 3: error
  gint     video_status; // 0:null, 1:ready, 2:running, 3: error

  PeerStruct ps;
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
  GstElement * pre_video_scale;
  GstElement * pre_capsfilter;
  GstElement * pre_video_encoder;
  GstElement * pre_video_encoder_queue;
  GstElement * pre_video_encoder_parser;
  GstElement * pre_aenc_tee_queue;
  GstPad *     pre_aenc_tee_queue_sinkpad;
  GstPad *     pre_aenc_tee_queue_ghost_sinkpad;
  GstElement * pre_muxer;
  GstElement * pre_outer; // 

  GstPad* pub_video_filter_tee_srcpad;
  GstElement * pub_vdec_tee_queue; // points
  GstPad *     pub_vdec_tee_queue_sinkpad;
  GstPad *     pub_vdec_tee_queue_ghost_sinkpad;
  GstElement * pub_fade;
  GstElement * pub_render1;
  GstElement * pub_render2;
  GstElement * pub_video_encoder;
  GstElement * pub_video_encoder_queue;
  GstElement * pub_video_encoder_parser;
  GstElement * pub_aenc_tee_queue;
  GstPad *     pub_aenc_tee_queue_sinkpad;
  GstPad *     pub_aenc_tee_queue_ghost_sinkpad;
  GstElement * pub_muxer;
  GstElement * pub_outer; // 
  GstElement * mixer; // audio mix

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

  guint mode; // 8K or 4K or 2K

  vrchan_t streams[MAX_CHANNEL*100];  
  gint  stream_nbs;  
  guint streams_id[MAX_CHANNEL*100];

  drchan_t director;

  GAsyncQueue * req_queue;
  GAsyncQueue * rep_queue;
  gboolean isPreSwitched;
  gboolean isPubSwitched;
  
} vrsmsz_t;

enum AppState
{
  APP_STATE_UNKNOWN = 0,
  APP_STATE_ERROR = 1,          /* generic error */
  SERVER_CONNECTING = 1000,
  SERVER_CONNECTION_ERROR,
  SERVER_CONNECTED,             /* Ready to register */
  SERVER_REGISTERING = 2000,
  SERVER_REGISTRATION_ERROR,
  SERVER_REGISTERED,            /* Ready to call a peer */
  SERVER_CLOSED,                /* server connection closed by us or the server */
  PEER_CONNECTING = 3000,
  PEER_CONNECTION_ERROR,
  PEER_CONNECTED,
  PEER_CALL_NEGOTIATING = 4000,
  PEER_CALL_STARTED,
  PEER_CALL_STOPPING,
  PEER_CALL_STOPPED,
  PEER_CALL_ERROR,
};

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

