#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "httpd.h"
#include "vrsmsz.h"

int main (int argc, char *argv[])
{
  if(argc != 4){
    g_error("USage: %s <ip> <port> <mode>",argv[0]);
    return -1;
  }
  vrsmsz_init(argc,argv);
  connect_to_websocket_server_async ();
  vrsmsz_start();// loop
  vrsmsz_stop();// null
  vrsmsz_deinit();
  return 0;
}
