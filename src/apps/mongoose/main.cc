#include "mongoose.h"   // To build, run: cc main.c mongoose.c

static volatile int g_keep_running = 1;

DEFINE_string(local_ip, "127.0.0.1", "IP of the local Machnet interface");
DEFINE_string(local_port, "8000", "Remote port to connect to.");

void SigIntHandler([[maybe_unused]] int signal) { g_keep_running = 0; }

// HTTP server event handler function
void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_POLL) {
    // printf("Got POLL Event\n");
  }
  if (ev == MG_EV_HTTP_MSG) {    
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    struct mg_http_serve_opts opts = { .root_dir = "./web_root/" };

/*  
    printf("\n===\nMsg: \n");
    for(size_t i = 0; i < hm->message.len; i++) {
      printf("%c", hm->message.buf[i]);  
    }
    printf("\n===\n");
*/
  
    mg_http_serve_dir(c, hm, &opts);
  }
  if (ev == MG_EV_READ) {
    //printf("Message read event\n");  
  }
}

int main(int argc, char *argv[]) {
  ::google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  gflags::SetUsageMessage("Simple Mongoose Server.");

  struct mg_mgr mgr;  // Declare event manager

  std::string url = "";
  url += FLAGS_local_ip;
  url += ":";
  url += FLAGS_local_port;

  printf("Starting Webserver at %s\n", url.c_str());

  //signal(SIGINT, SigIntHandler);

  mg_mgr_init(&mgr);  // Initialise event manager
  mg_http_listen(&mgr, url.c_str(), ev_handler, NULL);  // Setup listener

  // Run an infinite event loop (threaded)
  for(;;) {
    mg_mgr_poll(&mgr, 0);
  }

  return 0;
}