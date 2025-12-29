#include "quekka/Quekka_log.h"
#include "internal/comm.h"
#include "internal/libsocket.h"
#include "internal/epoll_handler.h"
#include <getopt.h>
#include <sys/epoll.h>

static char *quekkaBindingIp = NULL;
static int quekkaBindingPort = 9999;

static int gServerFd = -1;
static epoll_handler_t *gHandler = NULL;

static int processArgs(int argc, char **argv);

static void callback_fn(int fd, uint32_t events, void *user_data);

int main(int argc, char *argv[]) {
    if (processArgs(argc, argv) == -1) {
        exit(EXIT_FAILURE);
    }

    logger_init("quekka.log", LOG_INFO);

    log_info("##########################################");
    log_info("################# quekka ##################");
    log_info("##########################################");

    int port = quekkaBindingPort;

    // 서버 소켓 생성 및 설정
    int server_fd = socket_init();
    if (server_fd < 0) {
        exit(EXIT_FAILURE);
    }

    // 서버 side로써, 우선 socket 생성.
    socket_setsockopt_reuseaddr(server_fd);
    socket_bind_address(port, server_fd);
    socket_listen(server_fd);

    gHandler = epoll_handler_create();
    if (!gHandler) {
        log_error("failed to epoll_handler_create");
        exit(EXIT_FAILURE);
    }

    // connect 연결 요청 전용 fd를 등록함
    epoll_handler_add(gHandler, server_fd, EPOLLIN);

    gServerFd = server_fd;

    epoll_event_callback_fn callback = callback_fn;

    int ret = 0;
    while (1) {
        ret = epoll_handler_wait(gHandler, -1, callback, NULL);
        if (ret == -1)
            break;
    }
    log_error("epoll_handler_wait error[%d]), system bug", ret);
    return 0;
}

static void callback_fn(int fd, uint32_t events, void *user_data) {
    log_info("FD [%d]에 이벤트가 발생 하였습니다. events[%d]", fd, events);

    // 연결 요청이라면, accept 처리하여, 생성된 fd를 다시, epoll add 한다
    if (gServerFd == fd) {
        int accepted_fd = tcp_socket_accept(gServerFd, NULL);

        epoll_handler_add(gHandler, accepted_fd, EPOLLIN);
    } else {
        // 데이터 수신처리영역.
        char tmpBuffer[1024];
        memset(tmpBuffer, 0, sizeof(tmpBuffer));
        int ret = recv(fd, tmpBuffer, sizeof(tmpBuffer) - 1, MSG_DONTWAIT);
        if (ret == 0) {
            // 상대측에서 연결을 종료했다면, 소켓을 닫습니다.
            // 그리고 epoll 객체에서도, 관심fd로 제거.
            log_info("disconnected[%d]", ret);
            epoll_handler_remove(gHandler, fd);
            close( fd );
        }
        else if (ret < 0) {
            // 상대측 또는 우리가 예상치 못한 소켓 에러 발생시. 디버깅
            log_fatal("fatal[%d]", ret);
        }
        else {
            // 데이터 정상 수신
            log_info("user_data[%s]", tmpBuffer);
        }
    }
};


/**
 * using getopt
 * @param argc
 * @param argv
 * @return
 */
int processArgs(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "i:p:")) != -1) {
        switch (opt) {
            case 'i':
                quekkaBindingIp = optarg;
                log_info("IP address set to: %s", quekkaBindingIp);
                break;
            case 'p':
                quekkaBindingPort = atoi(optarg);
                log_info("Port set to: %d", quekkaBindingPort);
                break;
            default:
                log_error("Usage: %s [-i ip-address] [-p port]", argv[0]);
                return -1;
        }
    }
    return 0;
}
