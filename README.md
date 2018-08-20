# test_integration
It is a recording for my job

Project
	+ngx						
   	-url 				http://nginx.org
		-version		1.14.0 
		-shell	 		ngx/nginx_configure.sh
		-plugins 																										
   					+ nginx-rtmp-module	
								-url 			https://github.com/arut/nginx-rtmp-module.git	 
								-patchs 	ngx/nginx-rtmp-module/hls/hls.patch														
		-examples
								ngx/conf/*.conf
	+ats				
		-url				http://trafficserver.apache.org/downloads	
		-version 		6.6.2	
		-shell			ats/ats_configure.sh		
		-plugins
						+ carp								
								-url 			https://github.com/apache/trafficserver/pull/843.patch 
								-patch		843.patch
		-examples
							ats/etc/*.config
							tests.hertz

