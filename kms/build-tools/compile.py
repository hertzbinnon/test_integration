#!/usr/bin/python
import os,sys

cwd = os.getcwd()
catalog = os.listdir(cwd)
#print catalog

dirs = []
for i in catalog:
	if os.path.isdir(i):
		dirs.append(i)
print dirs

debian_local=''
for i in dirs:
	os.system('cd ' + i + '&& git reset --hard')
	if i == 'armodule':
		debian_local = i + '/ar-markerdetector'
	else:
		debian_local = i
	os.system('cat ' + debian_local + '/debian/control | sed -e \"s/$/\\!\\!/g\" | tr -d \'\\n\' | sed "s/\\!\\! / /g" | sed \"s/\\!\\!/\\n/g\" | grep \"Build-Depends\" | sed \"s/Build-Depends: //g\" | sed "s/Build-Depends-Indep: //g" | sed \"s/([^)]*)//g\" | sed \"s/, */ /g\"  | sed \"s/\\[linux-any\\]/ /g\" | sed \"s/\\[!hurd-any\\]/ /g\" | sed \"s/|/ /g\" > ' + i + '.deps')
	#os.system('cat ' + debian_local + '/debian/control | sed -e \"s/$/\\!\\!/g\" | tr -d \'\\n\' | sed "s/\\!\\! / /g" | sed \"s/\\!\\!/\\n/g\" | grep \"Build-Depends\" | sed \"s/Build-Depends: //g\" | sed "s/Build-Depends-Indep: //g" | sed \"s/([^)]*)//g\" | sed \"s/, */ /g\"  | sed \"s/\\[linux-any\\]/ /g\" | sed \"s/\\[!hurd-any\\]/ /g\" | sed \"s/|/ /g\" >> all.deps')

