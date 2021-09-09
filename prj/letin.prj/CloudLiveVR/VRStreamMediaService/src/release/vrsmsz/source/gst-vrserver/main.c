#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "httpd.h"
#include "gstvrsmsz.h"

int main (int argc, char *argv[])
{
  if(argc != 5){
    g_error("USage: %s <ip> <port> <mode> <external_interface>",argv[0]);
    return -1;
  }
  vrsmsz_t* vrsmsz = vrsmsz_init(argc,argv);
  create_http_server(8888,NULL,NULL,vrsmsz);
  vrsmsz_start();
  vrsmsz_stop();
  vrsmsz_deinit();
  return 0;
}
