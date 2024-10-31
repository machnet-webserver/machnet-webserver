#ifndef NET_H
#define NET_H

#include "config.h"
#include <stdint.h>
#include <openssl/ssl.h>
#include "wrk.h"
#include "machnet.h"  // for machnet API


typedef enum {
    OK,
    ERROR,
    RETRY
} status;

struct sock {
    status ( *connect)(connection *, char *);
    status (   *close)(connection *);
    status (    *read)(connection *, size_t *);
    status (   *write)(connection *, char *, size_t, size_t *);
};

// status sock_connect(connection *, char *);
// status sock_close(connection *);
// status sock_read(connection *, size_t *);
// status sock_write(connection *, char *, size_t, size_t *);
// size_t sock_readable(connection *);

// Machnet changes
status machnet_connect_handler(connection *, char *);       // Machnet connection handler
status machnet_close_handler(connection *);                 // Machnet close handler
status machnet_read_handler(connection *, size_t *);        // Machnet read handler
status machnet_write_handler(connection *c, char *buf, size_t len, size_t *n);

#endif /* NET_H */
