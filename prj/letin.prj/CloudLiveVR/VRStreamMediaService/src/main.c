#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "httpd.h"
#include "vrsmsz.h"

int main (int argc, char *argv[])
{
  vrsmsz_init(argc,argv);
  //create_http_server(8888,NULL,NULL,(GSourceFunc)perform_step);
  vrsmsz_start();// playing
  vrsmsz_play();// loop
  vrsmsz_stop();// null
  vrsmsz_deinit();
  return 0;
}
