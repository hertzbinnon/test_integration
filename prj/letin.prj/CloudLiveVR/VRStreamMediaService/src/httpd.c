#include <nanohttp/nanohttp-server.h>
#include <nanohttp/nanohttp-logging.h>

#include <stdio.h>
#include <glib/gstdio.h>
#include <errno.h>
#include "vrsmsz.h"
/*
	Please ignore this file. This is just for me to test 
	some features and new functions. In the future this
	file will also be good start to learn nanohttp
*/
/*
static 
void _print_binary_ascii(int n)
{
  int i,c=0;
  char ascii[36];

  for (i=0;i<32;i++) {
    ascii[34-i-c] = (n & (1<<i))?'1':'0';
    if ((i+1)%8 == 0) {
        c++;
        ascii[i+c] = ' ';
    }
  }

  ascii[35]='\0';

  log_verbose2("%s", ascii);
}


static 
void _print_binary_ascii2(unsigned char n)
{
  int i,c=0;
  char ascii[9];

  for (i=0;i<8;i++) {
    ascii[7-i] = (n & (1<<i))?'1':'0';
  }

  ascii[8]='\0';

  log_verbose2("%s", ascii);
}


void echomime_service(httpd_conn_t *conn, hrequest_t *req)
{
  part_t    *part;
  int status;
  char   *content_type , *transfer_encoding, 
         *content_id,  *root_content_type;

	if (!req->attachments || !req->content_type) 
	{
		httpd_send_header(conn, 200, "OK");
		http_output_stream_write_string(conn->out, "<html><body><h1>");
		http_output_stream_write_string(conn->out, 
		  "You do not posted a MIME message!<hr></h1></body></html>");
		return;
	}
	else
	{

	  root_content_type = hpairnode_get(req->content_type->params, "type");

	  status = 
	     httpd_mime_send_header(conn, req->attachments->root_part->id, "",
	      root_content_type, 200, "OK");

	  if (status != H_OK)
	   return;

	  part = req->attachments->parts;
	  while (part)
	  {
	     content_id = hpairnode_get(part->header, HEADER_CONTENT_ID);
	     content_type = hpairnode_get(part->header, HEADER_CONTENT_TYPE);
	     transfer_encoding = hpairnode_get(part->header, HEADER_TRANSFER_ENCODING);
	     status = httpd_mime_send_file(conn, content_id, 
	           content_type, transfer_encoding, part->filename);
	     if (status != H_OK)
	       return;
	     part = part->next;
	  }	  

	  httpd_mime_end(conn);
	}
		
}
*/
static GAsyncQueue *req_queue = NULL;
static GAsyncQueue *rep_queue = NULL;
/*
SERVICE: http://host:port/postserver
*/
void post_service(httpd_conn_t *conn, hrequest_t *req)
{
  unsigned char *postdata;
  long received, total=0;
	//unsigned int tmp;
  char buffer[15];
  hpair_t *pair;

  if (req->method == HTTP_REQUEST_POST) {
    char *content_length_str;
    long content_length = 0;
    unsigned char *postdata = NULL;
    content_length_str =
                  hpairnode_get_ignore_case(req->header, HEADER_CONTENT_LENGTH);

    if (content_length_str != NULL)
      content_length = atol(content_length_str);
    if (content_length == 0){
      printf("content is null");
      if (!(postdata = (char *) malloc(1))){
       	log_error2("malloc failed (%s)", strerror(errno));
      }
      postdata[0] = '\0';
    }else{
      if (!(postdata = (unsigned char *) malloc(content_length + 1))){
        log_error2("malloc failed (%)", strerror(errno));
      }
      if (http_input_stream_read(req->in, postdata, (int) content_length) > 0){
        postdata[content_length] = '\0';
      }
    }
    printf("post data --> %s\n",postdata);
    g_async_queue_push(req_queue, postdata);

    printf("Waiting process response -->");
    gchar* msg = g_async_queue_pop(rep_queue);
    printf("sent data --> %s\n",msg);

    httpd_send_header(conn, 200, "OK");
    http_output_stream_write_string(conn->out, msg);
    free(msg);
    /*
    http_output_stream_write_string(conn->out, "<html><body>\n");
    http_output_stream_write_string(conn->out, "<h3>You Posted:</h3><hr>\n");
    while (http_input_stream_is_ready(req->in)){
	received = http_input_stream_read(req->in, buffer, 10);
	http_output_stream_write(conn->out, buffer, received);
	total += received;
    }
    http_output_stream_write_string(conn->out, "<h3>Received size</h3><hr>\n");
	sprintf(buffer, "%d", total);
	http_output_stream_write_string(conn->out, buffer);
   */
    //command = g_strdup(postdata);

	/*	free(postdata);*/

   } else {

     httpd_send_header(conn, 200, "OK");
     http_output_stream_write_string(conn->out, "<html><body>");
     http_output_stream_write_string(conn->out, "<form action=\"/postserver\" method=\"POST\">");
     http_output_stream_write_string(conn->out, "Enter Postdata: <input name=\"field\" type=\"text\">");
     http_output_stream_write_string(conn->out, "<input type=\"submit\">");
		
  }
/*
  http_output_stream_write_string(conn->out, "<hr><p><small>Content-Type:");
  if (req->content_type){
    http_output_stream_write_string(conn->out, req->content_type->type);
    http_output_stream_write_string(conn->out, "<br>");
    pair = req->content_type->params;
    while (pair){
      http_output_stream_write_string(conn->out, pair->key);
      http_output_stream_write_string(conn->out, "=");
      http_output_stream_write_string(conn->out, pair->value);
      http_output_stream_write_string(conn->out, "<br>");
      pair = pair->next;
    }
  }
  else
  {
     http_output_stream_write_string(conn->out, "Not available");
  }

  http_output_stream_write_string(conn->out, "</body></html>");
*/
}
// -NHTTPport 8888

int httpd_thread(void* data)
{
  req_queue = ((vrsmsz_t*)data)->req_queue;
  rep_queue = ((vrsmsz_t*)data)->rep_queue;

  if (!httpd_register("/postserver", post_service)) {
    fprintf(stderr, "Can not register service");
    return 1;
  }

  if (httpd_run()) {
    fprintf(stderr, "can not run httpd");
    return 1;
  }

  httpd_destroy();
#ifdef MEM_DEBUG
  _mem_report();
#endif
	return 0;
}


int create_http_server(int port , char* tls_cert_file, char* tls_key_file, gpointer data){
  int argc = 2;
  char* argv[2];
  char p[10];

  sprintf(p,"%d",port);
  argv[0] = g_strdup("-NHTTPport");
  argv[1] = g_strdup(p);
	//hlog_set_level(HLOG_VERBOSE);
  if (httpd_init(argc, argv)) {
    fprintf(stderr, "can not init httpd");
    return 1;
  }
  g_free(argv[0]);
  g_free(argv[1]);
  pthread_t thread;
  pthread_create (&thread, NULL, httpd_thread, data);
  return 0;
}
