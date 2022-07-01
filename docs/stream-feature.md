---------------------------------------------------------------
|Open source project worth reading the source code immediately|
|		ats(高可用(容灾),高并发,DNS,cluster)	      |
|		libwebrtc(HPF/AEC/AGC/NG/neteq FEC/JB BWE/RC  |
|			audio/video udp/tcp/rtp/rtcp drm)     |
|--------------------------------------------------------------
|Open source project worth using to work		      |
|		ats vlc opencv ffmpeg gstreamer (av of g)     |
|--------------------------------------------------------------
|Open source projects directly related to mathematics         |
|		x264/x265/xavs2/av1/h.266 fdk-aac/opus	      |
|-------------------------------------------------------------|

What knowledge is involved

1. Streaming media service:

Camera < capute[Progressive/Interlace] samples,SD/HD/UHD/4K/8K,SDR/HDR/HFR/WCG >
 |
 V
[RGB,YUV,PCM etc memory format]  (--> render --> blend --> display)
 |
 V
Filter < AR/VR,NS/AGC/AEC,SR/fi >
 |
 V
Encoder < x264,x265,vp8,vp9,AV1,avs+,aac,vorbis FOV>
 |
 V
mux    < TS/MP4/RTP >
 |
 V
streaming_out < RTSP/HTTP/RTMP/SRT/RIST/WEBRTC SIP/Signal >
 |
 V
transport < UDP/TCP/QUIC DRM  FEC/jitter/Bandwidth estimation >

2. Content Delivery Network

Loading banlance:
	HAproxy/LVS/Nginx,keeplive,DNS poll

Web proxy:
	Nginx,ATS

Web cache:
	ATS,Redis,Memcache

Web server:
	Nginx,lighttpd

Streaming server:
        nginx-rtmp,srs,janus,kurento,gstwebrtc,gstdaemon

3. Content edit (rendering)
pitivi ptgui stitcher blender 

4. Player 
Fov/VR/AR
Motion-to-Photon Latency (MTP) <= 20ms Simulator Sickness

5. key feature

高性能，高负载，分布式多级服务器网络架构 IO/并发。
webrtc 连麦
webrtc 浏览器兼容
webrtc 屏幕共享
hpf,aec,agc,ans,neteq,小波,傅立叶变换
fec,jitterbuffer。

Protocol：
1.TCP/IP
2.HTTP/RTMP/RTSP/RTP/RTCP/SDP HLS/DASH 	
3.WEBRTC/P2P ICE/TURN/STUN 	
4.FLV/MP4/TS			   		
5.H264/H265/AAC			   	

Tools:
1) Django/celery/MySQL/Redis	
2) ffmpeg/nginx/kurento/x264 	
3) LVS、DNS、Nginx		//调度
4) EPOLL/IO/THREADS/SOCKET   	//并发、IO模型

Algorithms
1) Codecs的一般原理，H.264/H.265的对比.
2) 负载均衡算法，DNS原理，高可用，分布式，集群。应用于CDN，分布式编码，MCU/SFU的云部署。
3) HPF/AEC/AGC/NG/neteq	// 客户端
4) FEC/JB		// 两端
5) BWE/BC		// 发送端
6) Video super-resolution 
7) Video frame interpolation 
8) 360 video fov encoder
