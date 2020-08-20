import os
import json
from django.http import HttpResponse
from django.views.decorators.csrf import csrf_exempt
UPLOAD_FILE_PATH = '/tmp/'
@csrf_exempt
def upload(request):
	request_params = request.POST
	print(request_params)
	file_name = request_params['file_name']
	file_content_type = request_params['file_content_type']
	file_md5 = request_params['file_md5']
	file_path = request_params['file_path']
	file_size = request_params['file_size']
	ip_address = request.META.get('HTTP_X_REAL_IP') or request.META.get('HTTP_REMOTE_ADD')
	# save file to tmp
	new_file_name = file_name#'%s_%s' % (file_md5, ip_address)
	new_file_path = ''.join([UPLOAD_FILE_PATH, new_file_name])
	#new_file_path = ''.join([UPLOAD_FILE_PATH, new_file_name, os.path.splitext(file_name)[-1]])
	with open(new_file_path, 'ab') as new_file:
		with open(file_path, 'rb') as f:
			new_file.write(f.read())
	content = json.dumps({
		'name': file_name,
		'content_type': file_content_type,
		'md5': file_md5,
		'path': file_path,
		'size': file_size,
		'ip': ip_address,
	})
	response = HttpResponse(content, content_type='application/json; charset=utf-8')
	return response
