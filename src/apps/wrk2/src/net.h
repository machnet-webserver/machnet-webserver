#ifndef NET_H
#define NET_H

#include "config.h"
#include <stdint.h>
#include <sys/time.h> 
#include <openssl/ssl.h>
#include "machnet.h"  // For Machnet API

// Forward declaration of `connection`
struct connection;

// Define the `status` type
typedef enum {
    OK,
    ERROR,
    RETRY
} status;

// Define the `sock` struct
struct sock {
    status (*connect)(struct connection *, char *);
    status (*close)(struct connection *);
    status (*read)(struct connection *, size_t *);
    status (*write)(struct connection *, char *, size_t, size_t *);
    size_t (*readable)(struct connection *);
};

// Declare global variable
extern char *local_ip;

// Function prototypes
status sock_connect(struct connection *c, char *local_ip, char *remote_ip, uint16_t remote_port);
status sock_close(struct connection *);
status sock_read(struct connection *, size_t *);
status sock_write(struct connection *, char *, size_t, size_t *);
size_t sock_readable(struct connection *);

#endif /* NET_H */
