/** @file

  A brief file description

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

#include "P_Net.h"
#include <netdb.h>

#include "diags.i"

/*
 * Choose a net test application
 */
#include "NetTest-http-server.c"
//#include "NetTest-simple-proxy.c"

#include "ts/I_Layout.h" 
#include "RecordsConfig.h"

static void
check_config_directories(void)
{
  ats_scoped_str rundir(RecConfigReadRuntimeDir());
  ats_scoped_str sysconfdir(RecConfigReadConfigDir());

  if (access(sysconfdir, R_OK) == -1) {
    fprintf(stderr, "unable to access() config dir '%s': %d, %s\n", (const char *)sysconfdir, errno, strerror(errno));
    fprintf(stderr, "please set the 'TS_ROOT' environment variable\n");
    _exit(1);
  }

  if (access(rundir, R_OK | W_OK) == -1) {
    fprintf(stderr, "unable to access() local state dir '%s': %d, %s\n", (const char *)rundir, errno, strerror(errno));
    fprintf(stderr, "please set 'proxy.config.local_state_dir'\n");
    _exit(1);
  }
}

static void
initialize_process_manager()
{
  mgmt_use_syslog();

  // Temporary Hack to Enable Communication with LocalManager
  if (getenv("PROXY_REMOTE_MGMT")) {
    remote_management_flag = true;
  }

  if (remote_management_flag) {
    // We are being managed by traffic_manager, TERM ourselves if it goes away.
    EnableDeathSignal(SIGTERM);
  }

  RecProcessInit(remote_management_flag ? RECM_CLIENT : RECM_STAND_ALONE, diags);
  LibRecordsConfigInit();

  // Start up manager
  pmgmt = new ProcessManager(remote_management_flag);

  pmgmt->start();
  RecProcessInitMessage(remote_management_flag ? RECM_CLIENT : RECM_STAND_ALONE);
  pmgmt->reconfigure();
  check_config_directories();

  //
  // Define version info records
  //
  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.short", appVersionInfo.VersionStr, RECP_NON_PERSISTENT);
  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.long", appVersionInfo.FullVersionInfoStr, RECP_NON_PERSISTENT);
  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.build_number", appVersionInfo.BldNumStr, RECP_NON_PERSISTENT);
  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.build_time", appVersionInfo.BldTimeStr, RECP_NON_PERSISTENT);
  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.build_date", appVersionInfo.BldDateStr, RECP_NON_PERSISTENT);
  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.build_machine", appVersionInfo.BldMachineStr,
                        RECP_NON_PERSISTENT);
  RecRegisterStatString(RECT_PROCESS, "proxy.process.version.server.build_person", appVersionInfo.BldPersonStr,
                        RECP_NON_PERSISTENT);
}


static void
chdir_root()
{
  const char *prefix = Layout::get()->prefix;

  if (chdir(prefix) < 0) {
    fprintf(stderr, "%s: unable to change to root directory \"%s\" [%d '%s']\n", appVersionInfo.AppStr, prefix, errno,
            strerror(errno));
    fprintf(stderr, "%s: please correct the path or set the TS_ROOT environment variable\n", appVersionInfo.AppStr);
    _exit(1);
  } else {
    printf("%s: using root directory '%s'\n", appVersionInfo.AppStr, prefix);
  }
}

struct ShowStats : public Continuation {
#ifdef ENABLE_TIME_TRACE
  FILE *fp;
#endif
  int cycle;
  int64_t last_cc;
  int64_t last_rb;
  int64_t last_w;
  int64_t last_r;
  int64_t last_wb;
  int64_t last_nrb;
  int64_t last_nw;
  int64_t last_nr;
  int64_t last_nwb;
  int64_t last_p;
  int64_t last_o;
  int
  mainEvent(int event, Event *e)
  {
    (void)event;
    (void)e;
    if (!(cycle++ % 24))
      printf("r:rr w:ww r:rbs w:wbs open polls\n");
    int64_t sval, cval;

    NET_READ_DYN_SUM(net_calls_to_readfromnet_stat, sval);
    int64_t d_rb = sval - last_rb;
    last_rb += d_rb;
    NET_READ_DYN_SUM(net_calls_to_readfromnet_afterpoll_stat, sval);
    int64_t d_r = sval - last_r;
    last_r += d_r;

    NET_READ_DYN_SUM(net_calls_to_writetonet_stat, sval);
    int64_t d_wb = sval - last_wb;
    last_wb += d_wb;
    NET_READ_DYN_SUM(net_calls_to_writetonet_afterpoll_stat, sval);
    int64_t d_w = sval - last_w;
    last_w += d_w;

    NET_READ_DYN_STAT(net_read_bytes_stat, sval, cval);
    int64_t d_nrb = sval - last_nrb;
    last_nrb += d_nrb;
    int64_t d_nr = cval - last_nr;
    last_nr += d_nr;

    NET_READ_DYN_STAT(net_write_bytes_stat, sval, cval);
    int64_t d_nwb = sval - last_nwb;
    last_nwb += d_nwb;
    int64_t d_nw = cval - last_nw;
    last_nw += d_nw;

    NET_READ_GLOBAL_DYN_SUM(net_connections_currently_open_stat, sval);
    int64_t d_o = sval;

    NET_READ_DYN_STAT(net_handler_run_stat, sval, cval);
    int64_t d_p = cval - last_p;
    last_p += d_p;
    printf("%" PRId64 ":%" PRId64 " %" PRId64 ":%" PRId64 " %" PRId64 ":%" PRId64 " %" PRId64 ":%" PRId64 " %" PRId64 " %" PRId64
           "\n",
           d_rb, d_r, d_wb, d_w, d_nrb, d_nr, d_nwb, d_nw, d_o, d_p);
#ifdef ENABLE_TIME_TRACE
    int i;
    fprintf(fp, "immediate_events_time_dist\n");
    for (i = 0; i < TIME_DIST_BUCKETS_SIZE; i++) {
      if ((i % 10) == 0)
        fprintf(fp, "\n");
      fprintf(fp, "%5d ", immediate_events_time_dist[i]);
    }
    fprintf(fp, "\ncnt_immediate_events=%d\n", cnt_immediate_events);

    fprintf(fp, "cdb_callback_time_dist\n");
    for (i = 0; i < TIME_DIST_BUCKETS_SIZE; i++) {
      if ((i % 10) == 0)
        fprintf(fp, "\n");
      fprintf(fp, "%5d ", cdb_callback_time_dist[i]);
    }
    fprintf(fp, "\ncdb_cache_callbacks=%d\n", cdb_cache_callbacks);

    fprintf(fp, "callback_time_dist\n");
    for (i = 0; i < TIME_DIST_BUCKETS_SIZE; i++) {
      if ((i % 10) == 0)
        printf("\n");
      fprintf(fp, "%5d ", callback_time_dist[i]);
    }
    fprintf(fp, "\ncache_callbacks=%d\n", cache_callbacks);

    fprintf(fp, "rmt_callback_time_dist\n");
    for (i = 0; i < TIME_DIST_BUCKETS_SIZE; i++) {
      if ((i % 10) == 0)
        fprintf(fp, "\n");
      fprintf(fp, "%5d ", rmt_callback_time_dist[i]);
    }
    fprintf(fp, "\nrmt_cache_callbacks=%d\n", rmt_cache_callbacks);

    fprintf(fp, "inmsg_time_dist\n");
    for (i = 0; i < TIME_DIST_BUCKETS_SIZE; i++) {
      if ((i % 10) == 0)
        fprintf(fp, "\n");
      fprintf(fp, "%5d ", inmsg_time_dist[i]);
    }
    fprintf(fp, "\ninmsg_events=%d\n", inmsg_events);

    fprintf(fp, "open_delay_time_dist\n");
    for (i = 0; i < TIME_DIST_BUCKETS_SIZE; i++) {
      if ((i % 10) == 0)
        fprintf(fp, "\n");
      fprintf(fp, "%5d ", open_delay_time_dist[i]);
    }
    fprintf(fp, "\nopen_delay_events=%d\n", open_delay_events);

    fprintf(fp, "cluster_send_time_dist\n");
    for (i = 0; i < TIME_DIST_BUCKETS_SIZE; i++) {
      if ((i % 10) == 0)
        fprintf(fp, "\n");
      fprintf(fp, "%5d ", cluster_send_time_dist[i]);
    }
    fprintf(fp, "\ncluster_send_events=%d\n", cluster_send_events);
    fflush(fp);
#endif
    return EVENT_CONT;
  }
  ShowStats()
    : Continuation(NULL),
      cycle(0),
      last_cc(0),
      last_rb(0),
      last_w(0),
      last_r(0),
      last_wb(0),
      last_nrb(0),
      last_nw(0),
      last_nr(0),
      last_nwb(0),
      last_p(0),
      last_o(0)
  {
    SET_HANDLER(&ShowStats::mainEvent);
#ifdef ENABLE_TIME_TRACE
    fp = fopen("./time_trace.out", "a");
#endif
  }
};

struct alarm_printer : public Continuation {
  alarm_printer(ProxyMutex *m) : Continuation(m) { SET_HANDLER(&alarm_printer::dummy_function); }
  int
  dummy_function(int /* event ATS_UNUSED */, Event * /* e ATS_UNUSED */)
  {
    printf("alarm  thread=0x%llx \n",(unsigned long long)pthread_self());
    return 0;
  }
};

int
main()
{
  // do not buffer stdout
  setbuf(stdout, NULL);
  int nproc = ink_number_of_processors();

  RecModeT mode_type = RECM_STAND_ALONE;

  Layout::create(); 
  chdir_root();
  size_t stacksize = 1024*1024;
  //REC_ReadConfigInteger(stacksize, "proxy.config.thread.default.stacksize");

  init_diags("net_test", NULL);
   // Local process manager
  initialize_process_manager();
  
  RecProcessInit(mode_type);
  ink_event_system_init(EVENT_SYSTEM_MODULE_VERSION);
  ink_net_init(NET_SYSTEM_MODULE_VERSION);

  /*
   * ignore broken pipe
   */
  signal(SIGPIPE, SIG_IGN);

  /*
   * start processors
   */

  eventProcessor.start(nproc);
  eventProcessor.schedule_every(new alarm_printer(new_ProxyMutex()), HRTIME_SECONDS(1));
  eventProcessor.schedule_every(new ShowStats(), HRTIME_SECONDS(1));
  RecProcessStart();

/*
 *  Reset necessary config variables
 */

#ifdef USE_SOCKS
  net_config_socks_server_host = "209.131.52.54";
  net_config_socks_server_port = 1080;
  net_config_socks_needed      = 1;
#endif

  netProcessor.start(0,stacksize);
  //sslNetProcessor.start(1,1024*1024);

  /*
   * Call the tests main function
   */
  test_main();
  this_thread()->execute();
  return 0;
}
