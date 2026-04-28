//
// Created by Claude on 2025. 12. 12.
// epoll 관련 유닛테스트
//

#include <gtest/gtest.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

extern "C" {
#include "internal/libepoll.h"
#include "internal/client.h"
#include "internal/libsocket.h"
}

// =============================================================================
// epoll_handler 테스트
// =============================================================================
class EpollHandlerTest : public ::testing::Test {
protected:
    epoll_handler_t *handler = nullptr;

    void SetUp() override {
        handler = epoll_handler_create();
    }

    void TearDown() override {
        if (handler) {
            epoll_handler_destroy(handler);
        }
    }
};

TEST_F(EpollHandlerTest, CreateAndDestroy) {
    ASSERT_NE(handler, nullptr);
    EXPECT_GE(epoll_handler_get_fd(handler), 0);
}

TEST_F(EpollHandlerTest, AddAndRemoveFd) {
    // 테스트용 파이프 생성
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    // epoll에 추가
    int ret = epoll_handler_add(handler, pipefd[0], NULL, /* TODO */EPOLLIN);
    EXPECT_EQ(ret, 0);

    // epoll에서 제거
    ret = epoll_handler_remove(handler, pipefd[0]);
    EXPECT_EQ(ret, 0);

    close(pipefd[0]);
    close(pipefd[1]);
}

// =============================================================================
// client_manager 테스트
// =============================================================================
class ClientManagerTest : public ::testing::Test {
protected:
    client_manager_t *mgr = nullptr;

    void SetUp() override {
        mgr = client_manager_create();
    }

    void TearDown() override {
        if (mgr) {
            client_manager_destroy(mgr);
        }
    }
};

TEST_F(ClientManagerTest, CreateAndDestroy) {
    ASSERT_NE(mgr, nullptr);
    EXPECT_EQ(client_manager_count(mgr), 0);
}

TEST_F(ClientManagerTest, AddClient) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    client_info_t *client = client_manager_add(mgr, 100, addr);
    ASSERT_NE(client, nullptr);
    EXPECT_EQ(client->fd, 100);
    EXPECT_EQ(client_manager_count(mgr), 1);
}

TEST_F(ClientManagerTest, FindClient) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    client_manager_add(mgr, 100, addr);
    client_manager_add(mgr, 200, addr);
    client_manager_add(mgr, 300, addr);

    client_info_t *found = client_manager_find(mgr, 200);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->fd, 200);

    // 존재하지 않는 fd
    found = client_manager_find(mgr, 999);
    EXPECT_EQ(found, nullptr);
}

TEST_F(ClientManagerTest, RemoveClient) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    client_manager_add(mgr, 100, addr);
    client_manager_add(mgr, 200, addr);
    EXPECT_EQ(client_manager_count(mgr), 2);

    client_manager_remove(mgr, 100);
    EXPECT_EQ(client_manager_count(mgr), 1);
    EXPECT_EQ(client_manager_find(mgr, 100), nullptr);
    EXPECT_NE(client_manager_find(mgr, 200), nullptr);
}

// =============================================================================
// tcp_socket 테스트
// =============================================================================
TEST(TcpSocketTest, SetNonblocking) {
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);

    int ret = tcp_socket_set_nonblocking(pipefd[0]);
    EXPECT_EQ(ret, 0);

    // 논블로킹 확인
    int flags = fcntl(pipefd[0], F_GETFL, 0);
    EXPECT_TRUE(flags & O_NONBLOCK);

    close(pipefd[0]);
    close(pipefd[1]);
}

TEST(TcpSocketTest, CreateServerSocket) {
    struct sockaddr_in addr;
    int listen_fd = tcp_socket_create_server("127.0.0.1", 0, &addr);  // 포트 0 = 자동 할당

    // 포트 0은 실패할 수 있으므로 특정 포트로 테스트
    if (listen_fd < 0) {
        // 포트 충돌 시 다른 포트로 재시도
        listen_fd = tcp_socket_create_server("127.0.0.1", 19999, &addr);
    }

    ASSERT_GE(listen_fd, 0);

    // 소켓이 논블로킹인지 확인
    int flags = fcntl(listen_fd, F_GETFL, 0);
    EXPECT_TRUE(flags & O_NONBLOCK);

    close(listen_fd);
}
