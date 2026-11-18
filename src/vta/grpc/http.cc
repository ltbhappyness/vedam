/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 */
/*
 * Speaks gRPC unary framing on HTTP/1.1: a 5-byte length prefix and protobuf bytes.
 * The gRPC C++ library is not linked.
 * One listen socket serves every class. The path is /<package>.<Service>/<Method>,
 * for example /vedam.system.v1.SystemService/Get, the path a stock gRPC or Connect
 * client generated from the proto uses.
 * A worker keeps the accepted socket open and reads the next request.
 */
#include "vti/grpc.h"

#if VEDAM_ENABLE_GRPC || VEDAM_ENABLE_GRPC_UDS

#include "vti/codec.h"

#include "platform/memory.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/**
 * @brief Case-insensitive match for an HTTP header name prefix.
 *
 * @param[in] p    Header line start
 * @param[in] name Lower-case header name including colon
 * @return Non-zero when @p p starts with @p name ignoring case on letters.
 */
static int hdr_match(const char *p, const char *name)
{
    size_t i;

    for (i = 0; name[i] != '\0'; i++) {
        char a;
        char b;

        a = p[i];
        b = name[i];
        if (a >= 'A' && a <= 'Z') {
            a = static_cast<char>(a - 'A' + 'a');
        }
        if (a != b) {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Find Content-Length in an HTTP header block.
 *
 * @param[in] hdr     Header bytes
 * @param[in] hdr_len Header length (before the blank line)
 * @return Content length, or -1 when missing or invalid.
 */
static int find_content_length(const uint8_t *hdr, size_t hdr_len)
{
    size_t i;
    char *end;
    int content_len;

    for (i = 0; i + 15 < hdr_len; i++) {
        if (hdr_match(reinterpret_cast<const char *>(hdr + i), "content-length:")) {
            content_len = static_cast<int>(
                std::strtoul(reinterpret_cast<const char *>(hdr + i + 15), &end, 10));
            if (end == reinterpret_cast<const char *>(hdr + i + 15) || content_len < 0) {
                return -1;
            }
            return content_len;
        }
    }
    return -1;
}

namespace
{

/**
 * @brief Read one HTTP/1.1 request into @p buf and split method, path, and body.
 *
 * Parses in place: null-terminates method and path inside @p buf. Body points
 * into the same buffer. All storage is connection-owned.
 *
 * @param[in]  fd       Socket descriptor
 * @param[out] buf      Connection-owned I/O buffer
 * @param[in]  cap      Capacity of @p buf
 * @param[out] method   Points at method inside @p buf
 * @param[out] path     Points at path inside @p buf
 * @param[out] body     Points at body inside @p buf
 * @param[out] body_len Body length
 * @return 0 on success, -1 on failure or peer close before a full request.
 */
int read_http(int fd, char *buf, size_t cap, char **method, char **path, uint8_t **body,
              size_t *body_len)
{
    size_t used;
    size_t hdr_end;
    size_t line_end;
    size_t i;
    size_t sp1;
    size_t sp2;
    int content_len;
    ssize_t n;
    char *p;
    char *end;

    used = 0;
    hdr_end = (size_t)-1;
    content_len = -1;
    while (used < cap) {
        n = recv(fd, buf + used, cap - used, 0);
        if (n < 0) {
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        used += static_cast<size_t>(n);
        for (i = 0; i + 3 < used; i++) {
            if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n') {
                hdr_end = i;
                break;
            }
        }
        if (hdr_end != (size_t)-1) {
            break;
        }
    }
    if (hdr_end == (size_t)-1) {
        return -1;
    }

    line_end = (size_t)-1;
    for (i = 0; i + 1 < hdr_end; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n') {
            line_end = i;
            break;
        }
    }
    if (line_end == (size_t)-1) {
        return -1;
    }

    sp1 = (size_t)-1;
    sp2 = (size_t)-1;
    for (i = 0; i < line_end; i++) {
        if (buf[i] == ' ') {
            if (sp1 == (size_t)-1) {
                sp1 = i;
            } else {
                sp2 = i;
                break;
            }
        }
    }
    if (sp1 == (size_t)-1 || sp2 == (size_t)-1 || sp1 == 0 || sp2 <= sp1 + 1) {
        return -1;
    }

    buf[sp1] = '\0';
    buf[sp2] = '\0';
    buf[line_end] = '\0';
    *method = buf;
    *path = buf + sp1 + 1;

    p = buf + line_end + 2;
    while (p < buf + hdr_end) {
        char *line;
        char *nl;

        line = p;
        nl = nullptr;
        for (char *q = p; q <= buf + hdr_end; q++) {
            if (q[0] == '\r' && q[1] == '\n') {
                nl = q;
                break;
            }
        }
        if (nl == nullptr) {
            break;
        }
        *nl = '\0';
        if (hdr_match(line, "content-length:")) {
            content_len = static_cast<int>(std::strtoul(line + 15, &end, 10));
            if (end == line + 15 || content_len < 0 || static_cast<size_t>(content_len) > cap) {
                return -1;
            }
        }
        p = nl + 2;
    }

    *body = reinterpret_cast<uint8_t *>(buf + hdr_end + 4);
    *body_len = used - (hdr_end + 4);
    if (content_len >= 0) {
        while (*body_len < static_cast<size_t>(content_len) && used < cap) {
            n = recv(fd, buf + used, cap - used, 0);
            if (n <= 0) {
                return -1;
            }
            used += static_cast<size_t>(n);
            *body_len = used - (hdr_end + 4);
        }
        if (*body_len < static_cast<size_t>(content_len)) {
            return -1;
        }
        *body_len = static_cast<size_t>(content_len);
    }
    return 0;
}

/**
 * @brief Write an HTTP/1.1 response with a grpc-status trailer.
 *
 * Keeps the connection open for another request on the same socket.
 *
 * @param[in] fd          Socket descriptor
 * @param[in] http_status HTTP status code
 * @param[in] grpc_status Grpc status digit string
 * @param[in] body        Response body
 * @param[in] body_len    Length of @p body
 */
void write_http(int fd, int http_status, const char *grpc_status, const uint8_t *body,
                size_t body_len)
{
    char hdr[512];
    int n;

    n = std::snprintf(hdr, sizeof(hdr),
                      "HTTP/1.1 %d OK\r\n"
                      "Content-Type: application/grpc+proto\r\n"
                      "Grpc-Status: %s\r\n"
                      "Content-Length: %zu\r\n"
                      "Connection: keep-alive\r\n"
                      "\r\n",
                      http_status, grpc_status != nullptr ? grpc_status : "12", body_len);
    if (n > 0) {
        send(fd, hdr, static_cast<size_t>(n), 0);
    }
    if (body != nullptr && body_len > 0) {
        send(fd, body, body_len, 0);
    }
}

/**
 * @brief Remove the five-byte grpc-web length prefix from @p body in place.
 *
 * @param[in,out] body     Body pointer
 * @param[in,out] body_len Body length
 * @return true when the prefix was valid or absent with matching length.
 */
bool strip_grpc_prefix(const uint8_t **body, size_t *body_len)
{
    uint32_t n;

    if (*body_len >= 5 && (*body)[0] == 0) {
        std::memcpy(&n, *body + 1, 4);
        n = ntohl(n);
        if (n + 5 == *body_len) {
            *body += 5;
            *body_len -= 5;
            return true;
        }
    }
    return *body_len == 0;
}

/**
 * @brief Serve requests on one accepted client socket until the peer closes.
 *
 * @param[in,out] slot Worker slot that owns the connection buffers
 */
void handle_client(struct vti_http_slot *slot)
{
    const char *grpc_status;
    char *method;
    char *path;
    uint8_t *body_raw;
    const uint8_t *body;
    size_t body_len;
    size_t out_len;
    char *slash;
    int fd;
    int rc;

    fd = slot->fd;

    for (;;) {
        method = nullptr;
        path = nullptr;
        body_raw = nullptr;
        body = nullptr;
        body_len = 0;
        out_len = 0;
        grpc_status = "12";

        rc = read_http(fd, reinterpret_cast<char *>(slot->http_buf), sizeof(slot->http_buf),
                       &method, &path, &body_raw, &body_len);
        if (rc != 0) {
            break;
        }

        body = body_raw;
        strip_grpc_prefix(&body, &body_len);
        if (std::strcmp(method, "POST") != 0 || path[0] != '/') {
            write_http(fd, 200, "12", nullptr, 0);
            continue;
        }

        slash = std::strrchr(path, '/');
        if (slash == path || slash[1] == '\0') {
            write_http(fd, 200, "12", nullptr, 0);
            continue;
        }
        *slash = '\0';
        rc = vti_codec_http(path + 1, slash + 1, body, body_len, slot->out_buf,
                            sizeof(slot->out_buf), &out_len, &grpc_status, &slot->codec);
        if (rc != 0) {
            write_http(fd, 200, "12", nullptr, 0);
            continue;
        }

        write_http(fd, 200, grpc_status, slot->out_buf, out_len);
    }

    close(fd);
}

/**
 * @brief Claim a free worker slot in @p pool.
 *
 * @param[in,out] pool HTTP worker pool
 * @return Slot index, or -1 when none are free.
 */
int claim_slot(struct vti_http_pool *pool)
{
    int i;

    for (i = 0; i < VTI_HTTP_WORKERS; i++) {
        int zero = 0;

        if (__atomic_compare_exchange_n(&pool->slot[i].busy, &zero, 1, 0, __ATOMIC_ACQ_REL,
                                        __ATOMIC_ACQUIRE)) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Return non-zero when @p pool has a free worker slot.
 *
 * @param[in] pool HTTP worker pool
 * @return Non-zero when a slot is free.
 */
int any_free(struct vti_http_pool *pool)
{
    int i;

    for (i = 0; i < VTI_HTTP_WORKERS; i++) {
        if (__atomic_load_n(&pool->slot[i].busy, __ATOMIC_ACQUIRE) == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Serve one HTTP connection slot until the pool stops.
 * @param[in] raw Argument raw
 * @return Result of http_worker.
 */
void *http_worker(void *raw)
{
    struct vti_http_slot *slot = static_cast<struct vti_http_slot *>(raw);
    struct vti_http_pool *pool = slot->pool;

    for (;;) {
        struct pollfd pfd;
        char bit;
        int pr;
        int stop;
        ssize_t n;

        pfd.fd = slot->kick_r;
        pfd.events = POLLIN;
        pr = poll(&pfd, 1, -1);
        stop = __atomic_load_n(&pool->stop, __ATOMIC_ACQUIRE);
        if (pr < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (pr > 0) {
            n = read(slot->kick_r, &bit, 1);
            (void)n;
        }
        if (slot->fd >= 0) {
            handle_client(slot);
            slot->fd = -1;
            __atomic_store_n(&slot->busy, 0, __ATOMIC_RELEASE);
            bit = 1;
            if (pool->poke_w >= 0) {
                (void)write(pool->poke_w, &bit, 1);
            }
        }
        if (stop) {
            break;
        }
    }
    return NULL;
}

/**
 * @brief Accept connections and hand them to free HTTP workers.
 * @param[in] raw Argument raw
 * @return Result of http_listen.
 */
void *http_listen(void *raw)
{
    struct vti_http_pool *pool = static_cast<struct vti_http_pool *>(raw);

    for (;;) {
        struct pollfd pfd[2];
        int nfd;
        int pr;
        int stop;
        int slot;
        int cfd;
        char bit;
        ssize_t nwrote;

        stop = __atomic_load_n(&pool->stop, __ATOMIC_ACQUIRE);
        if (stop) {
            break;
        }
        nfd = 0;
        pfd[nfd].fd = pool->poke_r;
        pfd[nfd].events = POLLIN;
        nfd++;
        if (any_free(pool)) {
            pfd[nfd].fd = pool->listen_fd;
            pfd[nfd].events = POLLIN;
            nfd++;
        }
        pr = poll(pfd, nfd, -1);
        stop = __atomic_load_n(&pool->stop, __ATOMIC_ACQUIRE);
        if (stop) {
            break;
        }
        if (pr < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if ((pfd[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0 && nfd == 1) {
            break;
        }
        if (pfd[0].revents & POLLIN) {
            while (read(pool->poke_r, &bit, 1) > 0) {
            }
        }
        if (nfd == 2 && (pfd[1].revents & POLLIN) != 0) {
            slot = claim_slot(pool);
            if (slot < 0) {
                continue;
            }
            cfd = accept(pool->listen_fd, NULL, NULL);
            if (cfd < 0) {
                __atomic_store_n(&pool->slot[slot].busy, 0, __ATOMIC_RELEASE);
                continue;
            }
            pool->slot[slot].fd = cfd;
            bit = 1;
            nwrote = write(pool->slot[slot].kick_w, &bit, 1);
            if (nwrote != 1) {
                close(cfd);
                pool->slot[slot].fd = -1;
                __atomic_store_n(&pool->slot[slot].busy, 0, __ATOMIC_RELEASE);
            }
        }
    }
    return NULL;
}

} // namespace

/**
 * @brief Bind a TCP listen socket for the temporary HTTP remote door.
 *
 * @param[in] bind_spec Host:port string
 * @param[out] bound Buffer for the bound address
 * @param[in] bound_cap Capacity of @p bound
 * @return 0 on success, -1 on failure.
 */
extern "C" int vti_grpc_bind_tcp(const char *bind_spec, char *bound, size_t bound_cap)
{
    int fd;
    int one;
    int rc;
    unsigned long parsed;
    uint16_t port;
    char *end;
    const char *colon;
    size_t host_len;
    char host[128];
    struct sockaddr_in addr;
    struct sockaddr_in got;
    socklen_t got_len;

    fd = -1;
    if (bind_spec == NULL || bind_spec[0] == '\0' || bound == NULL || bound_cap < 8) {
        return -1;
    }
    colon = std::strrchr(bind_spec, ':');
    if (colon == nullptr || colon == bind_spec || colon[1] == '\0') {
        return -1;
    }
    host_len = static_cast<size_t>(colon - bind_spec);
    if (host_len >= sizeof(host)) {
        return -1;
    }
    std::memcpy(host, bind_spec, host_len);
    host[host_len] = '\0';
    parsed = std::strtoul(colon + 1, &end, 10);
    if (end == colon + 1 || *end != '\0' || parsed > 65535ul) {
        return -1;
    }
    port = static_cast<uint16_t>(parsed);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    one = 1;
    rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if (rc != 0) {
        goto out_close;
    }

    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    rc = inet_pton(AF_INET, host, &addr.sin_addr);
    if (rc != 1) {
        goto out_close;
    }

    rc = bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    if (rc < 0) {
        goto out_close;
    }

    rc = listen(fd, 16);
    if (rc < 0) {
        goto out_close;
    }

    got_len = sizeof(got);
    rc = getsockname(fd, reinterpret_cast<sockaddr *>(&got), &got_len);
    if (rc != 0) {
        goto out_close;
    }
    rc = std::snprintf(bound, bound_cap, "%s:%u", host, ntohs(got.sin_port));
    if (rc < 0 || static_cast<size_t>(rc) >= bound_cap) {
        goto out_close;
    }
    return fd;

out_close:
    close(fd);
    return -1;
}

/**
 * @brief Start HTTP workers against an already-bound listen socket.
 *
 * @param[in,out] pool HTTP worker pool
 * @param[in] listen_fd Listening socket
 * @return 0 on success, -1 on failure.
 */
extern "C" int vti_http_pool_start(struct vti_http_pool *pool, int listen_fd)
{
    int i;
    int rc;
    int pipes[2];

    if (pool == NULL || listen_fd < 0) {
        return -1;
    }
    std::memset(pool, 0, sizeof(*pool));
    pool->listen_fd = listen_fd;
    pool->poke_r = -1;
    pool->poke_w = -1;
    for (i = 0; i < VTI_HTTP_WORKERS; i++) {
        pool->slot[i].pool = pool;
        pool->slot[i].index = i;
        pool->slot[i].fd = -1;
        pool->slot[i].kick_r = -1;
        pool->slot[i].kick_w = -1;
    }

    rc = pipe(pipes);
    if (rc != 0) {
        return -1;
    }
    rc = fcntl(pipes[0], F_SETFL, O_NONBLOCK);
    if (rc != 0) {
        close(pipes[0]);
        close(pipes[1]);
        return -1;
    }
    pool->poke_r = pipes[0];
    pool->poke_w = pipes[1];

    for (i = 0; i < VTI_HTTP_WORKERS; i++) {
        rc = pipe(pipes);
        if (rc != 0) {
            goto out_stop;
        }
        rc = fcntl(pipes[0], F_SETFL, O_NONBLOCK);
        if (rc != 0) {
            close(pipes[0]);
            close(pipes[1]);
            goto out_stop;
        }
        pool->slot[i].kick_r = pipes[0];
        pool->slot[i].kick_w = pipes[1];
        rc = vplatform_thread_create(&pool->slot[i].thread, http_worker, &pool->slot[i]);
        if (rc != 0) {
            goto out_stop;
        }
        pool->slot[i].started = 1;
    }

    rc = vplatform_thread_create(&pool->listen_thread, http_listen, pool);
    if (rc != 0) {
        goto out_stop;
    }
    pool->listen_started = 1;
    return 0;

out_stop:
    vti_http_pool_stop(pool);
    return -1;
}

/**
 * @brief Wake and join HTTP workers, then close the wake pipe.
 *
 * @param[in,out] pool HTTP worker pool
 */
extern "C" void vti_http_pool_stop(struct vti_http_pool *pool)
{
    int i;
    char bit;

    if (pool == NULL) {
        return;
    }
    __atomic_store_n(&pool->stop, 1, __ATOMIC_RELEASE);
    bit = 1;
    if (pool->poke_w >= 0) {
        (void)write(pool->poke_w, &bit, 1);
    }
    if (pool->listen_fd >= 0) {
        shutdown(pool->listen_fd, SHUT_RDWR);
    }
    if (pool->listen_started) {
        (void)vplatform_thread_join(&pool->listen_thread);
        pool->listen_started = 0;
    }
    for (i = 0; i < VTI_HTTP_WORKERS; i++) {
        if (pool->slot[i].fd >= 0) {
            shutdown(pool->slot[i].fd, SHUT_RDWR);
        }
        if (pool->slot[i].kick_w >= 0) {
            (void)write(pool->slot[i].kick_w, &bit, 1);
        }
        if (pool->slot[i].started) {
            (void)vplatform_thread_join(&pool->slot[i].thread);
            pool->slot[i].started = 0;
        }
        if (pool->slot[i].kick_r >= 0) {
            close(pool->slot[i].kick_r);
            pool->slot[i].kick_r = -1;
        }
        if (pool->slot[i].kick_w >= 0) {
            close(pool->slot[i].kick_w);
            pool->slot[i].kick_w = -1;
        }
    }
    if (pool->poke_r >= 0) {
        close(pool->poke_r);
        pool->poke_r = -1;
    }
    if (pool->poke_w >= 0) {
        close(pool->poke_w);
        pool->poke_w = -1;
    }
}

/**
 * @brief Close a listen socket and optionally unlink a Unix path.
 *
 * @param[in] fd Listen socket
 * @param[in] uds_path Unix path to unlink, or NULL
 */
extern "C" void vti_grpc_unbind(int fd, const char *uds_path)
{
    if (fd >= 0) {
        close(fd);
    }
    if (uds_path != NULL && uds_path[0] != '\0') {
        unlink(uds_path);
    }
}

/**
 * @brief Send one framed protobuf Message and read the framed reply.
 *
 * The path is /<package>.<Service>/<Method>. @p req already carries the
 * five-byte grpc-web header. The reply body is unframed in place inside
 * @p io_buf; *@p resp points at those bytes.
 *
 * Uses Content-Length and leaves the socket open for another call.
 *
 * @param[in]     fd         Connected TCP socket
 * @param[in]     service    Service name, such as vedam.system.v1.SystemService
 * @param[in]     verb_name  Method name
 * @param[in]     req        Framed request bytes (header plus Message)
 * @param[in]     req_len    Length of @p req
 * @param[out]    resp       Points at unframed Message bytes inside @p io_buf
 * @param[out]    resp_len   Length of the unframed Message
 * @param[in,out] io_buf     Connection-owned HTTP scratch buffer
 * @param[in]     io_cap     Capacity of @p io_buf
 * @return V_STATUS_SUCCESS, or a failure code.
 */
extern "C" v_status_t vti_grpc_http_exchange(int fd, const char *service, const char *verb_name,
                                             const uint8_t *req, size_t req_len,
                                             const uint8_t **resp, size_t *resp_len,
                                             uint8_t *io_buf, size_t io_cap)
{
    size_t used;
    size_t hdr_end;
    size_t i;
    size_t body_off;
    size_t body_have;
    const uint8_t *body_ptr;
    size_t body_len;
    char hdr[320];
    int hn;
    int content_len;
    ssize_t n;
    ssize_t w;

    if (fd < 0 || service == NULL || verb_name == NULL || resp == NULL || resp_len == NULL ||
        io_buf == NULL) {
        return V_STATUS_INVALID_PARAMETER;
    }
    if (req == NULL && req_len != 0) {
        return V_STATUS_INVALID_PARAMETER;
    }

    hn = std::snprintf(hdr, sizeof(hdr),
                       "POST /%s/%s HTTP/1.1\r\n"
                       "Content-Type: application/grpc+proto\r\n"
                       "Content-Length: %zu\r\n"
                       "Connection: keep-alive\r\n"
                       "\r\n",
                       service, verb_name, req_len);
    if (hn < 0 || (size_t)hn >= sizeof(hdr)) {
        return V_STATUS_FAILURE;
    }

    w = send(fd, hdr, static_cast<size_t>(hn), 0);
    if (w != hn) {
        return V_STATUS_FAILURE;
    }
    if (req_len > 0) {
        w = send(fd, req, req_len, 0);
        if (w < 0 || (size_t)w != req_len) {
            return V_STATUS_FAILURE;
        }
    }

    used = 0;
    hdr_end = (size_t)-1;
    content_len = -1;
    while (used < io_cap) {
        n = recv(fd, io_buf + used, io_cap - used, 0);
        if (n < 0) {
            return V_STATUS_FAILURE;
        }
        if (n == 0) {
            return V_STATUS_FAILURE;
        }
        used += static_cast<size_t>(n);
        for (i = 0; i + 3 < used; i++) {
            if (io_buf[i] == '\r' && io_buf[i + 1] == '\n' && io_buf[i + 2] == '\r' &&
                io_buf[i + 3] == '\n') {
                hdr_end = i;
                break;
            }
        }
        if (hdr_end != (size_t)-1) {
            break;
        }
    }
    if (hdr_end == (size_t)-1) {
        return V_STATUS_FAILURE;
    }

    content_len = find_content_length(io_buf, hdr_end);
    if (content_len < 0) {
        return V_STATUS_FAILURE;
    }

    body_off = hdr_end + 4;
    body_have = used - body_off;
    while (body_have < static_cast<size_t>(content_len) && used < io_cap) {
        n = recv(fd, io_buf + used, io_cap - used, 0);
        if (n <= 0) {
            return V_STATUS_FAILURE;
        }
        used += static_cast<size_t>(n);
        body_have = used - body_off;
    }
    if (body_have < static_cast<size_t>(content_len)) {
        return V_STATUS_BUFFER_OVERFLOW;
    }

    body_ptr = io_buf + body_off;
    body_len = static_cast<size_t>(content_len);
    vti_codec_grpc_unframe(&body_ptr, &body_len);
    *resp = body_ptr;
    *resp_len = body_len;
    return V_STATUS_SUCCESS;
}

#endif /* VEDAM_ENABLE_GRPC || VEDAM_ENABLE_GRPC_UDS */
