https://github.com/Kurento
https://www.kurento.org/blog/kurento-6140-june-2020
https://doc-kurento.readthedocs.io/en/latest/dev/dev_guide.html
http://www.kurento.org/blog/rtp-ii-streaming-ffmpeg
kurento
├── gst-libav
├── gst-plugins-bad
├── gst-plugins-base
├── gst-plugins-good
├── gst-plugins-ugly
├── gstreamer
├── jsoncpp
├── kms-chroma
├── kms-cmake-utils
├── kms-core
├── kms-crowddetector
├── kms-datachannelexample
├── kms-elements
├── kms-filters
├── kms-jsonrpc
├── kms-markerdetector
├── kms-opencv-plugin-sample
├── kms-platedetector
├── kms-plugin-sample
├── kms-pointerdetector
├── kurento-media-server
├── kurento-module-creator
├── libnice
├── libs3
├── openwebrtc-gst-plugins
└── usrsctp

# Question
1.sfu config
https://stackoverflow.com/questions/36564536/how-to-disable-video-encoding-in-kurento-media-server  #sfu config
kurento/modules/kurento/SdpEndpoint.conf.json  
"videoCodecs" : [
//    {
//      "name" : "VP8/90000"
//    },
    {
      "name" : "H264/90000"
    }
]
#mediasoup-demo
npm install
npm start
