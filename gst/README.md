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

GST_PLUGIN_FEATURE_RANK=vah264dec:259 gst-play-1.0 ~/video.mp4
