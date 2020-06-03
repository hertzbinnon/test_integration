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
  gint type; // 1:video or 2:audio or 0:video&audio
  gint streamid;   // global id 
  gchar spec_desc[DESCRIBE_LEN]; // info about source
  gchar data[1024];
} describer_t;

typedef struct{
  describer_t *video_src[MAX_CHANNEL]; // 0 for null channel with videotestsrc
  describer_t *audio_src[MAX_CHANNEL]; // 0 for null channel with audiotestsrc
  describer_t *video_filter[MAX_CHANNEL]; // 
  describer_t *audio_filter[MAX_CHANNEL]; // 
  describer_t *video_encoder[MAX_CHANNEL];
  describer_t *audio_encoder[MAX_CHANNEL];
  describer_t *video_out[MAX_CHANNEL];
  describer_t *audio_out[MAX_CHANNEL];
  describer_t *comp;
  describer_t *mixer;
  guint v_director;
  guint a_director;

  describer_t *director_path; // director output path

  guint stream_ids; // how much streams
  describer_t* bins[MAX_CHANNEL]; // 0 is for test src
  GstElement* pipeline;
  GMainLoop *loop;
  GstClock *theclock;
  GstBus *bus;

} vrsmsz_t;

extern vrsmsz_t* vrsmsz;
void vrsmsz_init(int argc, char **argv);
void vrsmsz_start();
void vrsmsz_play();
void vrsmsz_stop();
void vrsmsz_quit();
void vrsmsz_deinit();

void vrsmsz_add_stream();
void vrsmsz_remove_stream();
void vrsmsz_add_track();
void vrsmsz_switch_stream();
void vrsmsz_switch_track();
void vrsmsz_switch_stream_crossfade();
void vrsmsz_add_text();
void vrsmsz_add_pic();
void vrsmsz_add_clip();


