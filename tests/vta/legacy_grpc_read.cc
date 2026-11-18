/*
 * Copyright (C) 2026 LTBH Vedam project
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * A changed copy of Vedam is not called Vedam.
 * An application that only calls Vedam keeps its own license. See LICENSE.
 *
 * Speaks gRPC unary framing on HTTP/1.1: a 5-byte length prefix and protobuf bytes.
 */
extern "C" {
#include "util/attr.h"
}

#include "core/node.h"
#include "vedam/system/v1/system.pb.h"
#include "vedam/v1/types.pb.h"
#include "vi/class/system.h"
#include "vi/vedam.h"
#include "vii/class/system.h"
#include "vii/meta.h"
#include "vti/codec.h"
#include "vti/grpc.h"

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
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
 * @brief POST one Message body to /<service>/<method> and parse the Message reply.
 *
 * @param[in]  port      Bound TCP port
 * @param[in]  service   Service name, such as vedam.system.v1.SystemService
 * @param[in]  method    Method name, such as Get
 * @param[in]  payload   Encoded Message bytes
 * @param[out] resp      Parsed reply Message
 * @return 0 on success, -1 on failure.
 */
static int http_call(uint16_t port, const std::string &service, const char *method,
                     const std::string &payload, vedam::v1::Message *resp)
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
                       "POST /%s/%s HTTP/1.1\r\n"
                       "Content-Type: application/grpc+proto\r\n"
                       "Content-Length: %zu\r\n"
                       "\r\n",
                       service.c_str(), method, body.size());
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
        content_len =
            std::strtoul(got.c_str() + cl_pos + std::strlen("Content-Length:"), &cl_end, 10);
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
    vedam::v1::Message req;
    vedam::v1::Message resp;
    std::string payload;
    const char *bound;
    unsigned long port;
    char *end;
    v_status_t st;

    st = vedam_init();
    if (st != V_STATUS_SUCCESS) {
        return 1;
    }
#if VEDAM_ENABLE_GRPC
    bound = vti_grpc_bound(vedam_node()->rx[VTI_LINK_REMOTE]);
#else
    bound = nullptr;
#endif
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

    req.add_attrs()->set_id(VI_SYSTEM_ATTR_NODE_CLASS);
    req.add_attrs()->set_id(VI_SYSTEM_ATTR_PROTOCOL_VERSION);
    req.SerializeToString(&payload);
    if (http_call(static_cast<uint16_t>(port), "vedam.system.v1.SystemService", "Get", payload,
                  &resp) != 0) {
        vedam_deinit();
        return 1;
    }
    vedam_deinit();

    if (resp.status().code() != vedam::v1::STATUS_OK) {
        return 1;
    }
    if (resp.attrs_size() < 2) {
        return 1;
    }
    return 0;
}

TEST(VtaHttp, LegacySystemGet)
{
#if !VEDAM_ENABLE_GRPC
    GTEST_SKIP() << "grpc disabled in this defconfig";
#else
    EXPECT_EQ(vedam_legacy_grpc(), 0);
#endif
}

TEST(VtaHttp, EmptyCommitWithRealGet)
{
#if !VEDAM_ENABLE_GRPC
    GTEST_SKIP() << "grpc disabled in this defconfig";
#endif
    vedam::v1::Message req;
    vedam::v1::Message resp;
    std::string payload;
    const char *bound;
    const char *colon;
    unsigned long port;
    int rc;

    ASSERT_EQ(vedam_init(), V_STATUS_SUCCESS);
#if VEDAM_ENABLE_GRPC
    bound = vti_grpc_bound(vedam_node()->rx[VTI_LINK_REMOTE]);
#else
    bound = nullptr;
#endif
    colon = bound != nullptr ? std::strrchr(bound, ':') : nullptr;
    port = colon != nullptr ? std::strtoul(colon + 1, nullptr, 10) : 0;
    if (port == 0 || port > 65535ul || wait_tcp(static_cast<uint16_t>(port)) != 0) {
        vedam_deinit();
        FAIL() << "bound port";
    }
    req.add_attrs()->set_id(VI_SYSTEM_ATTR_RELEASE_COMMIT);
    req.SerializeToString(&payload);
    rc = http_call(static_cast<uint16_t>(port), "vedam.system.v1.SystemService", "Get", payload,
                   &resp);
    vedam_deinit();

    ASSERT_EQ(rc, 0);
    EXPECT_EQ(resp.status().vedam_status(), V_STATUS_SUCCESS);
    ASSERT_EQ(resp.attrs_size(), 1);
    EXPECT_EQ(resp.attrs(0).status().vedam_status(), V_STATUS_SUCCESS);
    EXPECT_EQ(resp.attrs(0).value().text().size(), 0u);
}

namespace
{

constexpr char kCommit[] = "0123456789abcdef0123456789abcdef01234567";
constexpr vuint32_t kCommitLen = 40;

/* A copy of the class table whose System get body fills values. */
struct vii_class g_test_classes[V_CLASS_ID_MAX + 1];

/**
 * @brief Test-only System get body: fills the state and the commit text.
 *
 * @param[in]     major Vertical major
 * @param[in]     minor Vertical minor
 * @param[in,out] msg   Call message
 * @return V_STATUS_SUCCESS, or the first attribute failure.
 */
v_status_t test_system_get(vuint32_t major, vuint32_t minor, vi_msg_t *msg)
{
    v_status_t first = V_STATUS_SUCCESS;

    (void)major;
    (void)minor;
    for (vuint32_t i = 0; i < msg->attr_count; i++) {
        v_attr_t *attr = &msg->attr_list[i];
        v_status_t st = V_STATUS_SUCCESS;

        if (attr->id == VII_SYSTEM_ATTR_SYSTEM_STATE) {
            static_cast<vi_system_state_t *>(attr->value.ptr)->operational = VI_SYSTEM_STATE_READY;
        } else if (attr->id == VII_SYSTEM_ATTR_RELEASE_COMMIT) {
            st = vattr_put_text(attr, kCommit, kCommitLen);
        }
        msg->status_list[i] = st;
        if (first == V_STATUS_SUCCESS) {
            first = st;
        }
    }
    return first;
}

/**
 * @brief Start the node with the test get body and return its HTTP port.
 *
 * @param[out] port Bound TCP port
 * @return 0 on success, -1 on failure (the node is stopped again).
 */
int start_http(uint16_t *port)
{
    const char *bound;
    const char *colon;
    unsigned long n;

    if (vedam_init() != V_STATUS_SUCCESS) {
        return -1;
    }
    std::memcpy(g_test_classes, vii_classes, sizeof(g_test_classes));
    g_test_classes[V_CLASS_SYSTEM].body[V_VERB_GET] = test_system_get;
    vedam_node()->classes = g_test_classes;
#if VEDAM_ENABLE_GRPC
    bound = vti_grpc_bound(vedam_node()->rx[VTI_LINK_REMOTE]);
#else
    bound = nullptr;
#endif
    colon = bound != nullptr ? std::strrchr(bound, ':') : nullptr;
    n = colon != nullptr ? std::strtoul(colon + 1, nullptr, 10) : 0;
    if (n == 0 || n > 65535ul || wait_tcp(static_cast<uint16_t>(n)) != 0) {
        vedam_node()->classes = vii_classes;
        vedam_deinit();
        return -1;
    }
    *port = static_cast<uint16_t>(n);
    return 0;
}

/**
 * @brief Restore the generated class table and stop the node.
 */
void stop_http()
{
    vedam_node()->classes = vii_classes;
    vedam_deinit();
}

/**
 * @brief Encode a System request for the state and the commit.
 *
 * @return Encoded Message bytes.
 */
std::string state_and_commit_request()
{
    vedam::v1::Message req;
    std::string payload;

    req.add_attrs()->set_id(VI_SYSTEM_ATTR_SYSTEM_STATE);
    req.add_attrs()->set_id(VI_SYSTEM_ATTR_RELEASE_COMMIT);
    req.SerializeToString(&payload);
    return payload;
}

} // namespace

TEST(VtaHttp, BasicStateAndCommitMatchLocalGet)
{
#if !VEDAM_ENABLE_GRPC
    GTEST_SKIP() << "grpc disabled in this defconfig";
#endif
    vedam::system::v1::SystemState state;
    vi_system_state_t local_state = {};
    char local_buf[64];
    v_attr_t attrs[2] = {};
    v_status_t statuses[2];
    vi_msg_t vim = {};
    vedam::v1::Message resp;
    uint16_t port;
    int rc;

    ASSERT_EQ(start_http(&port), 0);
    attrs[0].id = VI_SYSTEM_ATTR_SYSTEM_STATE;
    attrs[0].value.ptr = &local_state;
    attrs[1].id = VI_SYSTEM_ATTR_RELEASE_COMMIT;
    attrs[1].value.text.buf = local_buf;
    attrs[1].value.text.len = sizeof(local_buf);
    vim.attr_count = 2;
    vim.attr_list = attrs;
    vim.status_list = statuses;
    EXPECT_EQ(vi_system_get(&vim), V_STATUS_SUCCESS);
    rc = http_call(port, "vedam.system.v1.SystemService", "Get", state_and_commit_request(), &resp);
    stop_http();

    ASSERT_EQ(rc, 0);
    ASSERT_EQ(resp.status().code(), vedam::v1::STATUS_OK);
    ASSERT_EQ(resp.attrs_size(), 2);
    ASSERT_TRUE(resp.attrs(0).value().has_message());
    ASSERT_TRUE(resp.attrs(0).value().message().UnpackTo(&state));
    EXPECT_EQ(state.operational(), vedam::system::v1::SYSTEM_STATE_READY);
    EXPECT_EQ(static_cast<int>(state.operational()), static_cast<int>(local_state.operational));
    ASSERT_EQ(attrs[1].value.text.len, kCommitLen);
    EXPECT_EQ(resp.attrs(1).value().text(), std::string(local_buf, kCommitLen));
}

TEST(VtaHttp, NegativeOversizeTextIsRefusedAndTheNodeKeepsServing)
{
#if !VEDAM_ENABLE_GRPC
    GTEST_SKIP() << "grpc disabled in this defconfig";
#endif
    vedam::v1::Message req;
    vedam::v1::Message resp;
    vedam::v1::Attr *attr;
    std::string payload;
    uint16_t port;
    int first;
    int second;

    ASSERT_EQ(start_http(&port), 0);
    attr = req.add_attrs();
    attr->set_id(VI_SYSTEM_ATTR_RELEASE_COMMIT);
    attr->mutable_value()->set_text(std::string(VTI_CODEC_VALUE_MAX + 1, 'a'));
    req.SerializeToString(&payload);
    first = http_call(port, "vedam.system.v1.SystemService", "Set", payload, &resp);
    EXPECT_EQ(first, 0);
    ASSERT_EQ(resp.attrs_size(), 1);
    EXPECT_EQ(resp.attrs(0).status().vedam_status(), V_STATUS_BUFFER_OVERFLOW);
    EXPECT_EQ(resp.status().vedam_status(), V_STATUS_BUFFER_OVERFLOW);
    EXPECT_FALSE(resp.attrs(0).has_value());

    resp.Clear();
    second =
        http_call(port, "vedam.system.v1.SystemService", "Get", state_and_commit_request(), &resp);
    stop_http();
    ASSERT_EQ(second, 0);
    EXPECT_EQ(resp.status().code(), vedam::v1::STATUS_OK);
    ASSERT_EQ(resp.attrs_size(), 2);
    EXPECT_EQ(resp.attrs(1).value().text(), std::string(kCommit, kCommitLen));
}
