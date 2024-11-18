// Copyright (C) 2013 - Will Glozer.  All rights reserved.

#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "net.h"

#include "machnet.h"  // Add this at the top of net.c


// status sock_connect(connection *c, char *host) {
//     return OK;
// }


// status machnet_connect_handler(connection *c, char *remote_ip) {
//     MachnetFlow_t flow;
//     // int machnet_listen(void *channel_ctx, const char *local_ip, uint16_t local_port)
//     //channel_ctx is a pointer to the channel context (MachnetChannelCtx_t)
//     //holds connection-related information like the control plane connection and any shared resources
//     int connect_status = machnet_connect(c->channel_ctx, NULL, remote_ip, 0, &flow);
    
//     // Check the result of machnet_connect.
//     if (connect_status == 0) {
//         c->machnet_flow = flow;  // Store the flow information
//         return OK;
//     } else {
//         // machnet_connect already logs detailed error info, so no additional log here.
//         return ERROR;
//     }
// }

status sock_connect(connection *c, char *host) {
    MachnetFlow_t flow;

    if (!host || strlen(host) == 0) {
        fprintf(stderr, "[ERROR] Invalid host in sock_connect.\n");
        return ERROR;
    }
    printf("[DEBUG] Entering sock_connect with host: %s (net.c).\n", host);

    c->channel_ctx = machnet_attach();
    if (c->channel_ctx == NULL) {
        fprintf(stderr, "[ERROR] Failed to attach Machnet channel (net.c).\n");
        return ERROR;
    }
    printf("[DEBUG] Machnet channel attached successfully (net.c).\n");

    int connect_status = machnet_connect(c->channel_ctx, NULL, host, 0, &flow);
    if (connect_status == 0) {
        c->machnet_flow = flow;
        printf("[DEBUG] Machnet connected successfully to %s (net.c).\n", host);
        return OK;
    } else {
        fprintf(stderr, "[ERROR] Machnet connection failed to %s: %s (net.c).\n", host, strerror(errno));
        machnet_detach(c->channel_ctx);
        return ERROR;
    }
}





// status sock_close(connection *c) {
//     return OK;
// }

// status machnet_close_handler(connection *c) {
//     machnet_detach(c->channel_ctx);  // Use machnet_detach to clean up
//     return OK;
// }

status sock_close(connection *c) {
    machnet_detach(c->channel_ctx);  // Use machnet_detach to clean up
    return OK;
}


// status sock_read(connection *c, size_t *n) {
//     ssize_t r = read(c->fd, c->buf, sizeof(c->buf));
//     *n = (size_t) r;
//     return r >= 0 ? OK : ERROR;
// }

// status machnet_read_handler(connection *c, size_t *n) {
//     MachnetFlow_t flow_info;
//     ssize_t bytes_received;

//     // Use machnet_recv to receive data from Machnet channel
//     bytes_received = machnet_recv(c->channel_ctx, c->buf, sizeof(c->buf), &flow_info);

//     if (bytes_received > 0) {
//         *n = (size_t) bytes_received;  // Set the number of bytes received
//         return OK;
//     } else if (bytes_received == 0) {
//         return RETRY;  // No message available, retry later
//     } else {
//         perror("machnet_recv failed");  // Add error handling here
//         return ERROR;  // Error occurred during receive
//     }
// }

status sock_read(connection *c, size_t *n) {
    MachnetFlow_t flow_info;
    ssize_t bytes_received;

    // Use machnet_recv to receive data from the Machnet channel
    bytes_received = machnet_recv(c->channel_ctx, c->buf, sizeof(c->buf), &flow_info);

    if (bytes_received > 0) {
        *n = (size_t) bytes_received;  // Set the number of bytes received
        return OK;
    } else if (bytes_received == 0) {
        return RETRY;  // No message available, retry later
    } else {
        perror("machnet_recv failed");  // Add error handling here
        return ERROR;  // Error occurred during receive
    }
}



// status sock_write(connection *c, char *buf, size_t len, size_t *n) {
//     ssize_t r;
//     if ((r = write(c->fd, buf, len)) == -1) {
//         switch (errno) {
//             case EAGAIN: return RETRY;
//             default:     return ERROR;
//         }
//     }
//     *n = (size_t) r;
//     return OK;
// }

// status machnet_write_handler(connection *c, char *buf, size_t len, size_t *n) {
//     int result = machnet_send(c->channel_ctx, c->machnet_flow, buf, len);

//     if (result >= 0) {  // Check for non-negative result
//         *n = len;  // Set the number of bytes sent
//         return OK;
//     } else {
//         perror("machnet_send failed");  // Add error handling here
//         return ERROR;
//     }
// }

status sock_write(connection *c, char *buf, size_t len, size_t *n) {
    int result = machnet_send(c->channel_ctx, c->machnet_flow, buf, len);

    if (result >= 0) {  // Check for non-negative result indicating success
        *n = len;  // Set the number of bytes sent
        return OK;
    } else if (errno == EAGAIN) {
        return RETRY;  // Retry if non-blocking and resource temporarily unavailable
    } else {
        perror("machnet_send failed");  // Add error handling for other errors
        return ERROR;
    }
}


// size_t sock_readable(connection *c) {
//     int n, rc;
//     rc = ioctl(c->fd, FIONREAD, &n);
//     return rc == -1 ? 0 : n;
// }

// // Placeholder for non-blocking readiness
// size_t machnet_readable(connection *c) {
//     return 1;  // Assume non-blocking behavior, replace with specific logic if needed
// }

size_t sock_readable(connection *c) {
    return 1;  // Assume non-blocking behavior, replace with specific logic if needed
}