#ifndef WRK_H
#define WRK_H

#include "types.h"
#include "config.h"
#include <pthread.h>
#include <inttypes.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <lua.h>
#include "stats.h"
#include "ae.h"
#include "http_parser.h"
#include "hdr_histogram.h"
#include "machnet.h"  // Include Machnet

#define VERSION  "4.0.0"
// #define RECVBUF  4096 // was 8192
// #define SAMPLES  10000 // was 100000000

// #define SOCKET_TIMEOUT_MS   1000 // was 2000
// #define CALIBRATE_DELAY_MS  2000 //was 10000
// #define TIMEOUT_INTERVAL_MS 250 // was 2000

#define RECVBUF  8192 // was 8192
#define SAMPLES  100000000 // was 100000000

#define SOCKET_TIMEOUT_MS   500 // was 2000
#define CALIBRATE_DELAY_MS  10000 //was 10000
#define TIMEOUT_INTERVAL_MS 100 // was 2000

extern char *local_ip; // Declare global local_ip in net.h

// typedef enum {
//     OK,
//     ERROR,
//     RETRY
// } status;

typedef struct {
    pthread_t thread;
    aeEventLoop *loop;
    struct addrinfo *addr;
    uint64_t connections;
    int interval;
    uint64_t stop_at;
    uint64_t complete;
    uint64_t requests;
    uint64_t bytes;
    uint64_t start;
    double throughput;
    uint64_t mean;
    struct hdr_histogram *latency_histogram;
    struct hdr_histogram *u_latency_histogram;
    tinymt64_t rand;
    lua_State *L;
    errors errors;
    struct connection *cs;
#ifdef MACHNET
    void *channel_ctx;  // Added this for Machnet context
#endif
} thread;

typedef struct {
    char  *buffer;
    size_t length;
    char  *cursor;
} buffer;

typedef struct connection {
    thread *thread;
    http_parser parser;
    enum {
        FIELD, VALUE
    } state;
    int fd;
    SSL *ssl;
    double throughput;
    double catch_up_throughput;
    uint64_t complete;
    uint64_t complete_at_last_batch_start;
    uint64_t catch_up_start_time;
    uint64_t complete_at_catch_up_start;
    uint64_t thread_start;
    uint64_t start;
    char *request;
    size_t length;
    size_t written;
    uint64_t pending;
    buffer headers;
    buffer body;
    char buf[RECVBUF];
    uint64_t actual_latency_start;
    bool has_pending;
    bool caught_up;
    // Internal tracking numbers (used purely for debugging):
    uint64_t latest_should_send_time;
    uint64_t latest_expected_start;
    uint64_t latest_connect;
    uint64_t latest_write;
    uint64_t latest_read;  // Timestamp of the last read operation

#ifdef MACHNET
    void *channel_ctx;  // loops back to thread connection ctx
    MachnetFlow_t machnet_flow; // Added this for Machnet flow information
#endif

} connection;

// Include `net.h` after defining `connection`
#include "net.h"

// Added struct config below existing definitions
typedef struct config {
    uint64_t threads;
    uint64_t connections;
    uint64_t duration;
    uint64_t timeout;
    uint64_t pipeline;
    uint64_t rate;
    uint64_t delay_ms;
    bool     latency;
    bool     u_latency;
    bool     dynamic;
    bool     record_all_responses;
    char    *host;
    char    *port;   // Port number (new addition)
    char    *script;
    SSL_CTX *ctx;
} config;

// make cfg accessible globally??
extern config cfg;


// Declare new functions
void poll_machnet_connections(thread *thread);
status sock_connect_wrapper(connection *c, char *host);



#endif /* WRK_H */
