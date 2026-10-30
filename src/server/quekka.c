#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/epoll.h>

#include "quekka/Quekka_log.h"
#include "internal/libsocket.h"
#include "internal/libepoll.h"
#include "internal/connection.h"

static void callback_fn(void *ptr, uint32_t events, void *user_data);

static char	*quekkaBindingIp	= NULL;
static int	quekkaBindingPort	= 9999;

static int processArgs(int argc, char **argv);
static int gServerFd = -1;
static epoll_handler_t *gHandler = NULL;

int main(int argc, char *argv[]) {
	if (processArgs(argc, argv) == -1) return EXIT_FAILURE;

	logger_init("quekka.log", LOG_INFO);
	log_info("########## quekka server start ##########");

	int server_fd = socket_init();
	if (server_fd < 0) return EXIT_FAILURE;

	socket_setsockopt_reuseaddr(server_fd);
	socket_bind_address(quekkaBindingPort, server_fd);
	socket_listen(server_fd);

	gServerFd = server_fd;

	gHandler = epoll_handler_create();
	if (!gHandler) {
		log_error("epoll_handler_create failed");
		return EXIT_FAILURE;
	}

	connection_t *listener = (connection_t *)malloc(sizeof(connection_t));
	if (!listener) {
		log_error("listener allocation failed");
		return EXIT_FAILURE;
	}
	listener->fd = server_fd;
	listener->phase = READING_LENTH;

	if (epoll_handler_add(gHandler, server_fd, listener, EPOLLIN) == -1) {
		log_error("epoll_handler_add failed");
		return EXIT_FAILURE;
	}

	while (epoll_handler_wait(gHandler, -1, callback_fn, NULL) != -1) {
		//
	}

	log_error("epoll_handler_wait fatal - 서버 종료");
	return EXIT_FAILURE;
}

static int processArgs(int argc, char **argv) {
	int opt;
	while ((opt = getopt(argc, argv, "i:p:")) != -1) {
		switch (opt) {
			case 'i': quekkaBindingIp	= optarg;		break;
			case 'p': quekkaBindingPort	= atoi(optarg);	break;
			default : return -1;
		}
	}
	return 0;
}

static void callback_fn(void *ptr, uint32_t events, void *user_data) {
	(void) user_data;
	connection_t *connection = ptr;

	if (connection == NULL) {
		return;
	}

	/* 이번에는 리스닝 소켓의 접속 요청만 처리합니다. */
	if (connection->fd != gServerFd || !(events & EPOLLIN)) {
		return;
	}

	struct sockaddr_in client_addr;
	int client_fd = tcp_socket_accept(connection->fd, &client_addr);

	if (client_fd < 0) {
		perror("accept");
		return;
	}

	log_info("Client connected: fd=%d", client_fd);

	// 현재는 접속 확인 후 연결을 닫습니다.
	close(client_fd);

}
