https://github.com/topics/gstreamer-plugins
https://github.com/topics/gstreamer
https://github.com/wwmm/pulseeffects 
https://gitlab.gnome.org/GNOME/pitivi
https://github.com/gstreamer-java

gst have  registry.x86_64.bin 
> 1.14 local dirs  > ~/.cache/cerbero-sources


https://developer.ridgerun.com/wiki/index.php/How_to_generate_a_Gstreamer_pipeline_diagram_(graph)
export GST_DEBUG_DUMP_DOT_DIR=/tmp
sudo apt-get install graphviz
dot -Tpng 0.00.24.846778049-gst-launch.PLAYING_PAUSED.dot > pipeline.png
eog pipeline.png

GST_PLUGIN_FEATURE_RANK=nvh264dec:259 gst-play-1.0 ~/video.mp4

# vim /home/hertz/sdb1/cerbero/build/sources/linux_x86_64/gstreamer-1.0/subprojects/gst-plugins-base/_builddir/gst-libs/gst/gl/gstglconfig.h
#define GST_GL_HAVE_WINDOW_GBM 1
GST_DEBUG=gl*:4 GST_GL_WINDOW=gbm gst-launch-1.0  glvideomixer name=m ! gldownload ! videoconvert ! video/x-raw ! nvh264enc ! fakesink      videotestsrc is-live=true ! video/x-raw, format=YUY2 ,framerate=5/1 ! glupload ! glcolorconvert ! m.
