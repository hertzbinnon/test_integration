# test_integration
It is a recording for my job

Project
	+cdn
		+nginx
		+varnish
		+GeoIP
		+bind
	+ngx						
   		-url 			http://nginx.org
		-version		1.14.0 
		-shell	 		ngx/nginx_configure.sh
		-plugins 																										
   					+ nginx-rtmp-module	
							-url 		https://github.com/arut/nginx-rtmp-module.git	 
							-patchs 	ngx/nginx-rtmp-module/hls/hls.patch														
		-examples
						ngx/conf/*.conf
	+ats				
		-url			http://trafficserver.apache.org/downloads	
		-version 		6.6.2	
		-shell			ats/ats_configure.sh		
		-plugins
						+ carp								
							-url 		https://github.com/apache/trafficserver/pull/843.patch 
							-patch		843.patch
		-examples
						ats/etc/*.config
						tests.hertz
	+gst
		+pygobject
			-url			https://git.gnome.org/browse/pygobject
		+gst-python
			-url			https://anongit.freedesktop.org/git/gstreamer/gst-python.git
		+pycairo
			-url			https://github.com/pygobject/pycairo.git
		+cerbero
			-url			git://anongit.freedesktop.org/gstreamer/cerbero
		-examples
							gst/examples
							+webrtc
								-url https://github.com/centricular/gstwebrtc-demos.git
								-patch (SOUP_SESSION_SSL_STRICT, FALSE)
		-plugins
					+ gst-plugin-base
							-patchs gst/gstdecodebin2.c.patch
	+kms-???
		-url
	+ai--???
		objectDetect
			-url 			https://github.com/jimfcarroll	
		voiceDectect
			-url			https://github.com/alumae/kaldi-gstreamer-server
