/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * Speaks gRPC unary framing on HTTP/1.1: a 5-byte length prefix and protobuf bytes.
 */
#include "core/core.h"
#include "vi/class/system.h"
#include "vi/vedam.h"
#include "vti/class_uuid.h"
#include "vti/codec.h"
#include "vti/grpc.h"
#include "vedam/types.pb.h"

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

#include <string>

/**
 * @brief Wait until a TCP accept is possible on @p port.
 *
 * @param[in] port Local TCP port
 * @return 0 when connect succeeds, -1 after retries fail.
 */
static int wait_tcp(uint16_t port)
{
    int tries;

    for (tries = 0; tries < 50; tries++) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in addr;

        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0) {
            close(fd);
            return 0;
        }
        close(fd);
        usleep(20000);
    }
    return -1;
}

/**
 * @brief POST one Message body to /vedam.<uuid>/Get and parse the Message reply.
 *
 * @param[in]  port      Bound TCP port
 * @param[in]  path_uuid Hyphenated UUID
 * @param[in]  payload   Encoded Message bytes
 * @param[out] resp      Parsed reply Message
 * @return 0 on success, -1 on failure.
 */
static int http_read(uint16_t port, const std::string &path_uuid, const std::string &payload,
                     vedam::Message *resp)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    uint32_t n;
    std::string body(5, '\0');
    char hdr[320];
    int hn;
    std::string got;
    char tmp[2048];
    size_t pos;
    std::string rbody;

    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    n = htonl(static_cast<uint32_t>(payload.size()));
    body[0] = 0;
    std::memcpy(&body[1], &n, 4);
    body += payload;

    hn = std::snprintf(hdr, sizeof(hdr),
                       "POST /vedam.%s/Get HTTP/1.1\r\n"
                       "Content-Type: application/grpc+proto\r\n"
                       "Content-Length: %zu\r\n"
                       "\r\n",
                       path_uuid.c_str(), body.size());
    send(fd, hdr, static_cast<size_t>(hn), 0);
    send(fd, body.data(), body.size(), 0);

    for (;;) {
        ssize_t r = recv(fd, tmp, sizeof(tmp), 0);
        size_t cl_pos;
        unsigned long content_len;
        char *cl_end;

        if (r <= 0) {
            close(fd);
            return -1;
        }
        got.append(tmp, static_cast<size_t>(r));
        pos = got.find("\r\n\r\n");
        if (pos == std::string::npos) {
            continue;
        }
        cl_pos = got.find("Content-Length:");
        if (cl_pos == std::string::npos || cl_pos > pos) {
            close(fd);
            return -1;
        }
        content_len = std::strtoul(got.c_str() + cl_pos + std::strlen("Content-Length:"), &cl_end, 10);
        if (cl_end == got.c_str() + cl_pos + std::strlen("Content-Length:")) {
            close(fd);
            return -1;
        }
        if (got.size() >= pos + 4 + content_len) {
            rbody = got.substr(pos + 4, content_len);
            break;
        }
    }
    close(fd);
    if (rbody.size() >= 5 && rbody[0] == 0) {
        rbody = rbody.substr(5);
    }
    return resp->ParseFromString(rbody) ? 0 : -1;
}

/**
 * @brief Exercise a System Get through the temporary HTTP path.
 *
 * @return 0 on success, 1 on failure.
 */
int vedam_legacy_grpc(void)
{
    vedam::Message req;
    vedam::Message resp;
    std::string payload;
    const char *bound;
    unsigned long port;
    char *end;
    char uuid_str[40];
    v_status_t st;

    st = vedam_init();
    if (st != V_STATUS_SUCCESS) {
        return 1;
    }
    bound = vta_grpc_bound(vedam_node()->rx[VTA_LINK_REMOTE]);
    if (bound == NULL) {
        vedam_deinit();
        return 1;
    }
    port = std::strtoul(std::strrchr(bound, ':') != NULL ? std::strrchr(bound, ':') + 1 : "", &end,
                        10);
    if (port == 0 || port > 65535ul) {
        vedam_deinit();
        return 1;
    }
    if (wait_tcp(static_cast<uint16_t>(port)) != 0) {
        vedam_deinit();
        return 1;
    }

    st = vta_class_uuid_string(V_CLASS_SYSTEM, uuid_str, sizeof(uuid_str));
    if (st != V_STATUS_SUCCESS) {
        vedam_deinit();
        return 1;
    }

    req.add_attrs()->set_id(VI_SYSTEM_ATTR_NODE_CLASS);
    req.add_attrs()->set_id(VI_SYSTEM_ATTR_PROTOCOL_VERSION);
    req.SerializeToString(&payload);
    if (http_read(static_cast<uint16_t>(port), uuid_str, payload, &resp) != 0) {
        vedam_deinit();
        return 1;
    }
    vedam_deinit();

    if (resp.status().code() != vedam::STATUS_OK) {
        return 1;
    }
    if (resp.attrs_size() < 2) {
        return 1;
    }
    return 0;
}

TEST(VtaHttp, LegacySystemGet)
{
    EXPECT_EQ(vedam_legacy_grpc(), 0);
}
