#!/bin/bash

import sys,os
import time
import thread

xml_file = open('example.xml','r')
string=xml_file.read()

counter=0
time_duration=0
def clientHttp(cmd):
	global counter
	while 1:
		time.sleep(0.01)
		start_time = time.time()
		if cmd == 1:
			os.system("wget --post-data=\'%s\' http://192.168.61.28:8091 -O /dev/null > /dev/null 2>&1 "% string)
		else:
			os.system("wget http://192.168.61.28:8091 -O /dev/null > /dev/null 2>&1 ")
		end_time = time.time()
		if end_time - start_time > 10.0:
			print '%f secs' % (end_time - start_time)
		counter += 1
argv = sys.argv
if len(argv) < 3:
	exit()
x=0
while x < int(argv[1]):
	thread.start_new_thread(   clientHttp,( int(argv[2]), )   )
	x += 1

while 1:
	#os.system('rm -f index.html*')
	time.sleep(1)
	time_duration +=1
	print str(counter/time_duration)+'\r'
