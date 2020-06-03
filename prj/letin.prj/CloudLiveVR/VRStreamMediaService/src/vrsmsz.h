#include <gst/gst.h>
#define MAX_CHANNEL 16
#define MAX_VIDEO_TRACK  16
#define MAX_AUDIO_TRACK  16
#define DESCRIBE_LEN 2048


typedef struct{
  gchar descr[DESCRIBE_LEN];
  GstElement *bin;
  GstPad *src_pad;
  GstPad *sink_pad;
  gint type; // video or audio
  gint id;   // global id 
  gchar channel_desc[DESCRIBE_LEN]; // info about source
  gvoid data[1024];
} describer_t;

typedef struct{
  describer_t *video_src[MAX_CHANNEL];
  describer_t *audio_src[MAX_CHANNEL];
  describer_t *filter;
  describer_t *comp;
  guint cur_video_index;
  guint cur_audio_index;
  describer_t *encoder;
} vrsmsz_t;
