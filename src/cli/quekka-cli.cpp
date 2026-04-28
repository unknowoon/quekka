//
// Created by leejaeseong on 2025. 12. 31..
//
#include "quekka-cli.h"
#include "quekka/Quekka_producer.h"
#include <getopt.h>
#include <string>
#include <sys/epoll.h>

#include "internal/libepoll.h"

static std::string quekkaBindingIp = "127.0.0.1";
static int quekkaBindingPort = 9999;

static epoll_handler_t *gpQuekkaHandler;

static int processArgs(int argc, char **argv);
static void callback_fn(void *ptr, uint32_t events, void *user_data);
static int processQuekkaData(const char *data);

int main(int argc, char **argv) {
    if (processArgs(argc, argv) == -1) {
        exit(EXIT_FAILURE);
    }

    // 우선 소켓을 하나 얻는다.
    int fd = socket_init();
    if (fd == -1) {
        log_error("socket init failed");
        exit(EXIT_FAILURE);
    }

    // quekka 접속을 위해서, quekka ip, port 셋팅
    sockaddr_in clientAddr;
    memset(&clientAddr, 0x00, sizeof(clientAddr));
    socket_set_server_address(&clientAddr, quekkaBindingIp.c_str(), quekkaBindingPort);

    gpQuekkaHandler = epoll_handler_create();
    if (!gpQuekkaHandler) {
        log_error("failed to epoll_handler_create");
        exit(EXIT_FAILURE);
    }

    // quekka에 접속 예정된 소켓을 이벤트 핸들러에 미리 등록함
    epoll_handler_add(gpQuekkaHandler, fd, NULL/*TODO 나중에 할게여...*/, EPOLLIN);
    epoll_event_callback_fn callback = callback_fn;

    // quekka 접속 시도함.
    socket_connect(&clientAddr, fd);

    while (1) {
        int ret = epoll_handler_wait(gpQuekkaHandler, -1, callback, NULL);
        if (ret == -1)
            break;
    }

    log_error("epoll_handler_wait error, system bug");
    return 0;
}

static void callback_fn(void *ptr, uint32_t events, [[maybe_unused]]void *user_data) {
	int fd = 0; // TODO 우선 초기화 함
    log_info("quekka-cli, FD [%d]에 이벤트가 발생 하였습니다. events[%d]", fd, events);

    // 데이터 수신처리영역.
    char headerBuffer[sizeof(Quekka_header) + 1];
    memset(headerBuffer, 0x00, sizeof(headerBuffer));

    Quekka_header *pHeader = nullptr;
    while (true) {
        //////////////////////////
        // 우선 헤더 먼저 수신
        //////////////////////////
        int ret = recv(fd, headerBuffer, sizeof(Quekka_header) - 1, MSG_DONTWAIT);
        if (ret == 0) {
            // quekka에서 접속종료함.`
            log_info("disconnected[%d]", ret);
            epoll_handler_remove(gpQuekkaHandler, fd);
            close(fd);
            return;
        } else if (ret < 0) {
            // 상대측 또는 우리가 예상치 못한 소켓 에러 발생시. 디버깅
            log_fatal("fatal[%d]", ret);
            exit(EXIT_FAILURE);
        } else {
            pHeader = (Quekka_header *)headerBuffer;
            log_info("Quekka's Body Payload Len [%d]", pHeader->payload_len);
        }
        //////////////////////////
        // 우선 헤더 먼저 수신
        //////////////////////////
        char *pBodyBuffer = nullptr;
        pBodyBuffer = (char*)malloc(pHeader->payload_len + 1);
        if (pBodyBuffer == nullptr) {
            log_fatal("malloc failed");
            exit(EXIT_FAILURE);
        }

        memset(pBodyBuffer, 0x00, sizeof(pBodyBuffer)+1);
        ret = recv(fd, pBodyBuffer, pHeader->payload_len - 1, MSG_WAITALL);
        if (ret == 0) {
            // quekka에서 접속종료함.`
            log_info("disconnected[%d]", ret);
            epoll_handler_remove(gpQuekkaHandler, fd);
            close(fd);
            return;
        } else if (ret < 0) {
            // 상대측 또는 우리가 예상치 못한 소켓 에러 발생시. 디버깅
            log_fatal("fatal[%d]", ret);
            exit(EXIT_FAILURE);
        } else {
            processQuekkaData(pBodyBuffer);
            free(pBodyBuffer);
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

int processQuekkaData(const char *data) {
    log_info("Quekka data: %s", data);
    return 0;
}
