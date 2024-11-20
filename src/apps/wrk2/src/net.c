// Copyright (C) 2013 - Will Glozer.  All rights reserved.

#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "net.h"

#include "machnet.h"  

char *local_ip;  // Declare `local_ip` as a global variable

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

// Enhanced sock_connect with debug statements
status sock_connect(connection *c, char *local_ip, char *remote_ip, uint16_t remote_port) {
    MachnetFlow_t flow;

    // Always set local_ip to "10.10.1.1"
    local_ip = "10.10.1.1";
    remote_port = 8000;

    // Validate remote_ip
    if (!remote_ip || strlen(remote_ip) == 0) {
        fprintf(stderr, "[ERROR] Invalid remote IP in sock_connect.\n");
        return ERROR;
    }

    // Debug remote_ip before further processing
    printf("[DEBUG] Entering sock_connect with local IP: %s, remote IP: %s, port: %u\n", local_ip, remote_ip, remote_port);

    // Attach a Machnet channel
    c->channel_ctx = machnet_attach();
    if (!c->channel_ctx) {
        fprintf(stderr, "[ERROR] Failed to attach Machnet channel (net.c): %s\n", strerror(errno));
        return ERROR;
    }
    printf("[DEBUG] Machnet channel attached successfully (net.c).\n");

    // Debug remote_ip and local_ip before machnet_connect
    printf("[DEBUG] Preparing to call machnet_connect with local_ip: %s, remote_ip: %s, port: %u\n",
           local_ip, remote_ip, remote_port);

    // Call machnet_connect
    int connect_status = machnet_connect(c->channel_ctx, local_ip, remote_ip, remote_port, &flow);

    if (connect_status == 0) {
        c->machnet_flow = flow; // Store flow context
        printf("[DEBUG] Machnet connected successfully to %s:%u (net.c).\n", remote_ip, remote_port);

        // Attempt to read data after connection
        size_t received_size = 0;
        int retries = 5;  // Retry up to 5 times
        status read_status = RETRY; // Initialize with RETRY
        printf("[DEBUG] Attempting to read from port: %u\n", c->machnet_flow.src_port);

        for (int attempt = 0; attempt < retries; ++attempt) {
            printf("[DEBUG] Entering sock_read for port: %u (Attempt %d/%d)\n", c->machnet_flow.src_port, attempt + 1, retries);
            read_status = sock_read(c, &received_size);
            if (read_status == OK) {
                printf("[DEBUG] Successfully read %zu bytes from port: %u\n", received_size, c->machnet_flow.src_port);
                return OK; // Exit immediately on success
            } else if (read_status == ERROR) {
                printf("[ERROR] sock_read failed on port: %u. Exiting read loop.\n", c->machnet_flow.src_port);
                return ERROR; // Exit immediately on error
            }
            usleep(100000);  // Wait 100ms before retrying
        }

        // If all retries are exhausted and no data received
        if (read_status == RETRY) {
            printf("[ERROR] Exhausted retries. No data received from port: %u (net.c).\n", c->machnet_flow.src_port);
        }

        return ERROR;
    } else {
        fprintf(stderr, "[ERROR] Machnet connection failed to %s:%u: %s\n", remote_ip, remote_port, strerror(errno));
        machnet_detach(c->channel_ctx); // Cleanup on failure
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
    if (c->channel_ctx) {
        machnet_detach(c->channel_ctx);
        printf("[DEBUG] Machnet channel detached successfully (net.c).\n");
    }
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
    ssize_t bytes_received = machnet_recv(c->channel_ctx, c->buf, sizeof(c->buf), &flow_info);

    if (bytes_received > 0) {
        *n = (size_t)bytes_received;
        printf("[DEBUG] Received %ld bytes (net.c).\n", bytes_received);
        return OK;
    } else if (bytes_received == 0) {
        printf("[DEBUG] No data available to read (net.c).\n");
        return RETRY;
    } else {
        fprintf(stderr, "[ERROR] machnet_recv failed: %s\n", strerror(errno));
        return ERROR;
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

    if (result >= 0) {
        *n = len;
        printf("[DEBUG] Sent %zu bytes (net.c).\n", len);
        return OK;
    } else {
        fprintf(stderr, "[ERROR] machnet_send failed: %s\n", strerror(errno));
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