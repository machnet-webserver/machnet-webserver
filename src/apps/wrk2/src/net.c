#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "net.h"
#include "machnet.h"

status sock_connect(connection *c, char *local_ip, char *remote_ip, uint16_t remote_port) {
    MachnetFlow_t flow;

    if (!remote_ip || strlen(remote_ip) == 0) {
        fprintf(stderr, "[ERROR] Invalid remote IP in sock_connect.\n");
        return ERROR;
    }
    printf("[DEBUG] Entering sock_connect with local IP: %s, remote IP: %s, port: %u\n", local_ip, remote_ip, remote_port);

    c->channel_ctx = machnet_attach();
    if (!c->channel_ctx) {
        fprintf(stderr, "[ERROR] Failed to attach Machnet channel (net.c): %s\n", strerror(errno));
        return ERROR;
    }
    printf("[DEBUG] Machnet channel attached successfully (net.c).\n");

    int connect_status = machnet_connect(c->channel_ctx, local_ip, remote_ip, remote_port, &flow);
    if (connect_status == 0) {
        c->machnet_flow = flow;  // Store flow context
        printf("[DEBUG] Machnet connected successfully to %s:%u (net.c).\n", remote_ip, remote_port);
        return OK;
    } else {
        fprintf(stderr, "[ERROR] Machnet connection failed to %s:%u: %s\n", remote_ip, remote_port, strerror(errno));
        machnet_detach(c->channel_ctx);  // Cleanup on failure
        return ERROR;
    }
}

status sock_read(connection *c, size_t *n) {
    MachnetFlow_t flow_info;
    ssize_t bytes_received = machnet_recv(c->channel_ctx, c->buf, sizeof(c->buf), &flow_info);

    if (bytes_received > 0) {
        *n = (size_t) bytes_received;
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

status sock_close(connection *c) {
    if (c->channel_ctx) {
        machnet_detach(c->channel_ctx);
        printf("[DEBUG] Machnet channel detached successfully (net.c).\n");
    }
    return OK;
}
