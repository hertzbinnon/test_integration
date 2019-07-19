./bin/vlc-static -v /home/hebin/CHC/芭蕾舞1803D_4k50M.mp4  --no-avcodec-hurry-up --avcodec-skip-frame -1 --avcodec-threads 8  --network-caching 300  --sout="#duplicate{dst='transcode{vcodec=h264,width=0,height=0,threads=24,vb=40000,venc=x264{vbv-maxrate=40000,vbv-bufsize=20000,tune=zerolentacy,keyint=25,min-keyint=25,qpmax=51,scenecut=0,bframes=3,},no-deinterlace,acodec=mp4a,ab=128,channels=2,samplerate=48000,aenc=ffmpeg,}:std{access=avio,mux=flv,dst=rtmp://192.168.61.22:1935/live/test}'}" --loop

./bin/vlc-static -v /home/hebin/CHC/芭蕾舞1803D_4k50M.mp4  --no-avcodec-hurry-up --avcodec-skip-frame -1 --avcodec-threads 8  --network-caching 300  --sout="#duplicate{dst='transcode{vcodec=h264,width=0,height=0,threads=24,vb=40000,venc=x264{vbv-maxrate=40000,vbv-bufsize=20000,tune=zerolentacy,keyint=25,min-keyint=25,qpmax=51,scenecut=0,bframes=3,},no-deinterlace,acodec=mp4a,ab=128,channels=2,samplerate=48000,aenc=ffmpeg,}:std{access=livehttp{seglen=1,delsegs=true,numsegs=5,index=cdn.test/html/mystream.m3u8,index-url=http://192.168.61.22:58082/mystream-#######.ts},mux=ts{use-key-frames},dst=cdn.test/html/mystream-#######.ts}'}" 

./bin/vlc-static -vvv /home/hebin/xinyule-1204_17_00.ts --sout-all --program 171 --sout-udp-ratecontrol  --sout="#duplicate{dst='transcode{vcodec=h264,width=720,height=576,threads=24,vb=4000,venc=x264{vbv-maxrate=4000,vbv-bufsize=2000,keyint=25,min-keyint=25,qpmax=51,scenecut=0,bframes=3,},no-deinterlace,acodec=mp4a,ab=128,channels=2,samplerate=48000,aenc=ffmpeg,}:std{access=udp,mux=ts,dst=192.168.8.62:12346}',select={program=171}}"

./bin/vlc-static -vvv /home/hebin/xinyule-1204_17_00.ts --sout-all --sout-udp-ratecontrol  --sout="#duplicate{dst='transcode{vcodec=h264,width=720,height=576,threads=24,vb=4000,venc=x264{vbv-maxrate=4000,vbv-bufsize=2000,keyint=25,min-keyint=25,qpmax=51,scenecut=0,bframes=3,},no-deinterlace,acodec=mp4a,ab=128,channels=2,samplerate=48000,aenc=ffmpeg,}:std{access=udp,mux=ts,dst=192.168.8.62:12346}',select={program=171}}"

./bin/vlc-static -vvv /home/hebin/xinyule-1204_17_00.ts --sout-all --sout-udp-ratecontrol  --sout="#duplicate{dst='transcode{vcodec=h264,width=720,height=576,threads=24,vb=4000,venc=x264{vbv-maxrate=4000,vbv-bufsize=2000,keyint=25,min-keyint=25,qpmax=51,scenecut=0,bframes=3,},no-deinterlace,acodec=mp4a,ab=128,channels=2,samplerate=48000,aenc=ffmpeg,}:std{access=udp,mux=ts,dst=192.168.8.62:12346}',select={es=1741,es=1713}}"

ffmpeg -v quiet -re  -i /home/hebin/klg.ts -c copy -f mpegts - | vlc - -vvv --sout="#std{access=udp,mux=ts,dst=192.168.8.62:12346}"
ffmpeg -v quiet -re  -i /home/hebin/klg.ts -c copy -f mpegts - | vlc - -vvv --sout="#std{access=udp,mux=ts,dst=192.168.8.62:12346}"
ffmpeg -v quiet -re  -i /home/hebin/klg.ts -c copy -f mpegts - | vlc fd://0  -vvv --sout="#std{access=udp,mux=ts,dst=192.168.8.62:12346}"

#		 -->	
# 	---> -->
#==>	 
#	---> 
#		 -->
#		 -->
./vlc-static  -vvv udp://@192.168.61.26:51234 --program 1357  --sout="#duplicate{dst='transcode{vcodec=h265,fps=25,width=1280,height=720,threads=24,vb=1500,venc=x265{vbv-maxrate=1500,vbv-bufsize=1000,keyint=25,min-keyint=25,qpmax=51,scenecut=0,bframes=1,},no-deinterlace,acodec=mp4a,ab=128,channels=2,samplerate=48000,aenc=ffmpeg,sfilter=logo{file=file:///home/image001.png,x=20,y=20},soverlay}:duplicate{dst=std{access=udp,,mux=ts,dst=192.168.8.62:12348},{dst=std{access=udp,,mux=ts,dst=192.168.8.62:12347}}',select={program=1357,es=803,es=1131},{dst='transcode{vcodec=h265,fps=25,width=1280,height=720,threads=24,vb=1500,venc=x265{vbv-maxrate=1500,vbv-bufsize=1000,keyint=25,min-keyint=25,qpmax=51,scenecut=0,bframes=1,},no-deinterlace,acodec=mp4a,ab=128,channels=2,samplerate=48000,aenc=ffmpeg,sfilter=logo{file=file:///home/image001.png,x=20,y=20},soverlay}:duplicate{dst=std{access=udp,,mux=ts,dst=192.168.8.62:12349},{dst=std{access=udp,,mux=ts,dst=192.168.8.62:12346}}',select={program=1357,es=804,es=1131}}"

vlc.exe --demux rawvideo --rawvid-fps 20 --rawvid-width 1280 --rawvid-height 720 --rawvid-chroma I420 ..\SlideShow_1280x720_20_qp22.yuv
