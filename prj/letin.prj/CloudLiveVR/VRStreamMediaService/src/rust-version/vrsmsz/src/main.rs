extern crate gstreamer as gst;
use gst::prelude::*;
use glib_sys;
use std::env;
const MAX_CHANNEL: usize =8;

struct Command{
  cmd: String,	
  id: i32,
  in_url: String,
  pre_url: String,
  pub_url: String,
  stream_id: String,

  effect:String,
  duration:i32,

  in_bitrate:i32,
  in_width:i32,
  in_height:i32,
  in_fps:i32,

  pub_bitrate:i32,
  pub_width:i32,
  pub_height:i32,
  pub_fps:i32,

  logo_path:String,
  pip_path: String,
  text_string:String,
  left:i32,
  top: i32,
  width:i32,
  height:i32,

  font:String,
  font_size:i32,

  delay_type:String,
  delay_time:i32,

}

struct VRStream{
  uridecodebin:gst::Element,  // 
  vdec_tee:gst::Element, 
  vdec_tee_queue:gst::Element ,
  pre_vdec_tee_srcpad:gst::Pad,
  pre_vdec_tee_ghost_srcpad:gst::Pad,
  pub_vdec_tee_srcpad:gst::Pad,
  pub_vdec_tee_ghost_srcpad:gst::Pad,
  vdec_tee_filter_srcpad:gst::Pad,
  vdec_filter_tee_queue_sinkpad:gst::Pad,
  vdec_filter_tee_queue:gst::Element,
  vdec_filter_tee_convert:gst::Element,
  video_scale:gst::Element ,
  video_capsfilter:gst::Element ,
  video_encoder:gst::Element ,
  video_encoder_queue:gst::Element ,
  video_encoder_parser:gst::Element ,
  audio_convert:gst::Element, 
  audio_encoder:gst::Element, 
  aenc_tee:gst::Element ,
  pre_aenc_tee_srcpad:gst::Pad,
  pre_aenc_tee_ghost_srcpad:gst::Pad,
  pub_aenc_tee_srcpad:gst::Pad,
  pub_aenc_tee_ghost_srcpad:gst::Pad,
  aenc_tee_queue:gst::Element ,
  muxer: gst::Element, // 
  outer: gst::Element, // 
  bin: gst::Element,
}

struct VRChan{
  tracks:i32,
  stream_id:i32,
  video_id: i32,
  audio_id: i32,
  subs_id: i32,
  in_url: String,
  output_url:String,
  preview_url:String,
  resolution:i32, 
  vs: VRStream ,

  status:i32, 
  audio_status:i32, 
  video_status:i32,
}

struct Message{
  req: String,
  rep: String,
  errcode:i32,
  timeout:i32,
  is_responsed: bool,
  command: Command,
  vc: VRChan,
}

struct DRStream{
  //compositor for fade;
  video_filter_queue_sinkpad: gst::Pad,  
  video_filter_queue: gst::Element,  
  videoconvert: gst::Element,  
  video_filter_chain: gst::Element,  
  video_filter_tee: gst::Element,  

  vfilter_queue_vdec_tee_srcpd: gst::Pad, 
  vfilter_vdec_tee_queue: gst::Element,
  video_convert: gst::Element, 
  wrapper : gst::Element, 
  vfilter_tee: gst::Element,  

  pre_video_filter_tee_srcpad:gst::Pad,
  pre_vdec_tee_queue:gst::Element,
  pre_vdec_tee_queue_sinkpad:gst::Pad,
  pre_vdec_tee_queue_ghost_sinkpad:gst::Pad,

  pre_video_scale:gst::Element,
  pre_capsfilter:gst::Element,
  pre_video_encoder:gst::Element,
  pre_video_encoder_queue:gst::Element,
  pre_video_encoder_parser:gst::Element,
  pre_aenc_tee_queue:gst::Element,
  pre_aenc_tee_queue_sinkpad:gst::Pad,
  pre_aenc_tee_queue_ghost_sinkpad:gst::Pad,
  pre_muxer:gst::Element,
  pre_outer:gst::Element,  

  pub_video_filter_tee_srcpad:gst::Pad,
  pub_vdec_tee_queue:gst::Element,  
  pub_vdec_tee_queue_sinkpad:gst::Pad,
  pub_vdec_tee_queue_ghost_sinkpad:gst::Pad,
  pub_video_encoder:gst::Element,
  pub_video_encoder_queue:gst::Element,
  pub_video_encoder_parser:gst::Element,
  pub_aenc_tee_queue:gst::Element,
  pub_aenc_tee_queue_sinkpad:gst::Pad,
  pub_aenc_tee_queue_ghost_sinkpad:gst::Pad,
  pub_muxer:gst::Element,
  pub_outer:gst::Element, 
  mixer:gst::Element, 

}

struct DRChan{
  pre_bin: gst::Element,
  pub_bin: gst::Element,
  swt_bin: gst::Element,
  eff_bin: gst::Element,

  stream_id: i32,
  video_id: i32,
  audio_id: i32,
  preview_url: String,
  publish_url: String,

  ds: DRStream,
}

impl DRChan{
  fn new()->DRChan{
    
  }
}

struct VRSmsz{
  pipeline: gst::Pipeline,
  looper: glib::MainLoop,
  theclock: Option<gst::Clock>,
  bus: gst::Bus ,

  mode: i32,

  streams: [VRChan; MAX_CHANNEL],  
  stream_nbs: i32,
  streams_id: [usize; MAX_CHANNEL],

  director: DRChan,

  req_queue: glib_sys::GAsyncQueue,
  rep_queue: glib_sys::GAsyncQueue,
  is_switched:bool,
  
}

impl VRSmsz{
  fn new(mode:i32) -> VRSmsz{
      let pipeline = gst::Pipeline::new(Some("vrsmsz-pipeline"));
      let looper   = glib::MainLoop::new(None, false);
      let theclock = pipeline.get_clock();
      let bus      = pipeline.get_bus().unwrap();
      let streams:[VRChan; MAX_CHANNEL] ; 
      let streams_id:[usize; MAX_CHANNEL];
      let director = ();//DRChan::new();
      let req_queue = ();
      let rep_queue = ();
      let stream_nbs = 0;
      let is_switched = false;
      let mut i = 0;
      loop{
        streams[i] = ();//VRChan::new(i);
        streams_id[i] = 0;
        i = i+1;
        if i == MAX_CHANNEL {
           break;
        }
      }

      VRSmsz{
        pipeline,
        looper,
        theclock,
        bus,
        mode,
        streams,
        stream_nbs,
        streams_id,
        director,
        req_queue,
        rep_queue,
        is_switched,
      }
  }
}

fn main() {
    let args: Vec<_> = env::args().collect();
    if args.len() != 4 {
        println!("Usage: vrsmsz <host> <port> <mode>");
        std::process::exit(-1);
    }
    gst::init().unwrap();
}
