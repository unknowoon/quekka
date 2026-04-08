//
// Created by 우지범 on 2025. 12. 12..
//
#include "internal/comm.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>

#include "internal/libsocket.h"
#include "internal/client.h"
#include "internal/libepoll.h"
#include "internal/server.h"

int socket_init(void) {
	int server_fd = 0;
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		const int err = errno;
		log_error("socket failed: %s", strerror(err));
		return -1;
	}
	return server_fd;
}

void socket_setsockopt_reuseaddr(const int server_fd) {
	const int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
		log_error("setsockopt failed: %s", strerror(errno));
		close(server_fd);
		exit(1);
	}
}

void socket_bind_address(const int port, const int socket_fd) {
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		log_error("bind failed: %s", strerror(errno));
		close(socket_fd);
		exit(1);
	}
}

void socket_listen(const int socket_fd) {
	if (listen(socket_fd, SOMAXCONN) < 0) {
		log_error("listen failed: %s", strerror(errno));
		close(socket_fd);
		exit(1);
	}
}

void socket_accept(const int socket_fd) {
	struct sockaddr_in address;
	socklen_t address_length = sizeof(address);
	const int client_fd = accept(socket_fd, (struct sockaddr *)&address, &address_length);
	if (client_fd < 0) {
		log_error("accept failed: %s", strerror(errno));
		close(socket_fd);
		exit(1);
	}
}

void socket_set_server_address(struct sockaddr_in *server_address, const char *server_ip, const int server_port) {
	memset(server_address, 0, sizeof(struct sockaddr_in));
	server_address->sin_family = AF_INET;
	server_address->sin_port = htons(server_port);
	inet_pton(AF_INET, server_ip, &server_address->sin_addr);
}

void socket_connect(struct sockaddr_in *server_address, const int socket_fd) {
	if (connect(socket_fd, (struct sockaddr *)server_address, sizeof(struct sockaddr_in)) < 0) {
		log_error("connect failed: %s", strerror(errno));
		close(socket_fd);
		exit(1);
	}
}

void socket_connect_with_client(const int server_fd) {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	while (1) {
		const int client_fd = accept(server_fd, (struct sockaddr*) &client_addr, &client_len);
		if (client_fd < 0) {
			log_error("accept failed: %s", strerror(errno));
			continue;
		}

		// Quekka 서버에서 사용할 예정
		log_info("Quekka: Client connected from %s:%d",
			   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		pid_t pid = fork();

		// fork 실패 했을 경우
		if (pid < 0) {
			log_error("fork failed: %s", strerror(errno));
			close(client_fd);

			// 본인일 경우
		} else if (pid == 0) {
			close(server_fd);

			char fd_str[32];
			snprintf(fd_str, sizeof(fd_str), "%d", client_fd);

			log_error("execl failed");
			exit(1);

			// fork 성공 자식프로세스의 경우
		} else {
			close(client_fd);
			log_info("Quekka: Created child process %d for client", pid);
		}
	}
}

int comm_init(const int socket_fd) {
    if (socket_fd < 0) {
        log_error("Comm: Invalid socket file descriptor");
        return -1;
    }

    log_info("Comm: Initialized with socket fd %d (PID: %d)", socket_fd, getpid());
    return 0;
}

int comm_handle_data(int socket_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received, bytes_sent;

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        bytes_received = read(socket_fd, buffer, BUFFER_SIZE - 1);

        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                log_info("Comm: Client disconnected");
            } else {
                log_error("Comm: read failed: %s", strerror(errno));
            }
            break;
        }

        buffer[bytes_received] = '\0';
        log_debug("Comm: Received %ld bytes: %s", bytes_received, buffer);

        if (strncmp(buffer, "quit", 4) == 0) {
            log_info("Comm: Client requested to quit");
            break;
        }

        char response[BUFFER_SIZE + 8];
        snprintf(response, sizeof(response), "Echo: %s", buffer);

        bytes_sent = write(socket_fd, response, strlen(response));
        if (bytes_sent < 0) {
            log_error("Comm: write failed: %s", strerror(errno));
            break;
        }

        log_debug("Comm: Sent %ld bytes: %s", bytes_sent, response);
    }

    return 0;
}

void comm_cleanup(const int socket_fd) {
    log_info("Comm: Socket closed");
    close(socket_fd);
}
typedef struct client_node_t {
    client_info_t info;
    struct client_node_t *next;
} client_node_t;

// 클라이언트 매니저 구조체
struct client_manager_t {
    client_node_t *clients;
    int client_count;
};

// 클라이언트 매니저 생성
client_manager_t* client_manager_create(void) {
    client_manager_t *mgr = calloc(1, sizeof(client_manager_t));
    if (!mgr) return NULL;

    mgr->clients = NULL;
    mgr->client_count = 0;

    return mgr;
}

// 클라이언트 매니저 삭제
void client_manager_destroy(client_manager_t *mgr) {
    if (!mgr) return;

    // 모든 클라이언트 노드 해제
    client_node_t *node = mgr->clients;
    while (node) {
        client_node_t *next = node->next;
        free(node);
        node = next;
    }

    free(mgr);
}

// 클라이언트 추가
client_info_t* client_manager_add(client_manager_t *mgr, const int fd, const struct sockaddr_in addr) {
    if (!mgr) return NULL;

    if (mgr->client_count >= CLIENT_MAX_CLIENTS) {
        return NULL;
    }

    client_node_t *node = malloc(sizeof(client_node_t));
    if (!node) return NULL;

    node->info.fd = fd;
    node->info.addr = addr;
    node->info.user_data = NULL;

    // 리스트 앞에 추가
    node->next = mgr->clients;
    mgr->clients = node;
    mgr->client_count++;

    return &node->info;
}

// 클라이언트 찾기
client_info_t* client_manager_find(client_manager_t *mgr, const int fd) {
    if (!mgr) return NULL;

    client_node_t *node = mgr->clients;
    while (node) {
        if (node->info.fd == fd) {
            return &node->info;
        }
        node = node->next;
    }
    return NULL;
}

// 클라이언트 삭제
void client_manager_remove(client_manager_t *mgr, const int fd) {
    if (!mgr) return;

    client_node_t **pp = &mgr->clients;

    while (*pp) {
        if ((*pp)->info.fd == fd) {
            client_node_t *to_delete = *pp;
            *pp = (*pp)->next;
            free(to_delete);
            mgr->client_count--;
            return;
        }
        pp = &(*pp)->next;
    }
}

// 클라이언트 수 반환
int client_manager_count(client_manager_t *mgr) {
    if (!mgr) return 0;
    return mgr->client_count;
}


// 내부 서버 구조체
struct socket_server_t {
	int listen_fd;
	epoll_handler_t *epoll;
	client_manager_t *client_mgr;
	struct sockaddr_in addr;
	server_callbacks_t callbacks;
	void *user_data;
	int running;
};

// === 이벤트 핸들러들 ===

// 새로운 연결 수락
static void handle_accept(const socket_server_t *server) {
	while (1) {
		struct sockaddr_in client_addr;
		const int client_fd = tcp_socket_accept(server->listen_fd, &client_addr);

		if (client_fd == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;  // 모든 대기 중인 연결 처리 완료
			} else {
				perror("accept");
				break;
			}
		}

		// 논블로킹 설정
		if (tcp_socket_set_nonblocking(client_fd) == -1) {
			close(client_fd);
			continue;
		}

		// 클라이언트 등록
		client_info_t *client = client_manager_add(server->client_mgr, client_fd, client_addr);
		if (!client) {
			fprintf(stderr, "Too Many Clients\n");
			close(client_fd);
			continue;
		}

		// epoll에 추가
		if (epoll_handler_add(server->epoll, client_fd, EPOLLIN | EPOLLET) == -1) {
			client_manager_remove(server->client_mgr, client_fd);
			close(client_fd);
			continue;
		}

		// on_connection 콜백 호출
		if (server->callbacks.on_connection) {
			server->callbacks.on_connection(client, server->user_data);
		}
	}
}

// 클라이언트 데이터 수신
static void handle_client(const socket_server_t *server, const int client_fd) {
	client_info_t *client = client_manager_find(server->client_mgr, client_fd);
	if (!client) return;

	char buffer[BUFFER_SIZE];

	while (1) {
		const ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);

		if (n > 0) {
			// 데이터 수신 성공 - on_received 콜백 호출
			if (server->callbacks.on_received) {
				server->callbacks.on_received(client, buffer, n, server->user_data);
			}

		} else if (n == 0) {
			// 연결 종료
			if (server->callbacks.on_disconnected) {
				server->callbacks.on_disconnected(client, server->user_data);
			}

			epoll_handler_remove(server->epoll, client_fd);
			close(client_fd);
			client_manager_remove(server->client_mgr, client_fd);
			break;

		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			} else {
				perror("recv");
				if (server->callbacks.on_disconnected) {
					server->callbacks.on_disconnected(client, server->user_data);
				}
				epoll_handler_remove(server->epoll, client_fd);
				close(client_fd);
				client_manager_remove(server->client_mgr, client_fd);
				break;
			}
		}
	}
}

// epoll 이벤트 콜백
static void epoll_event_callback(int fd, uint32_t events, void *user_data) {
	(void)events;  // 현재 미사용
	socket_server_t *server = (socket_server_t*)user_data;

	if (fd == server->listen_fd) {
		handle_accept(server);
	} else {
		handle_client(server, fd);
	}
}

// === 공개 API 함수들 ===

socket_server_t* socket_server_create(
		const char *ip,
		int port,
		server_callbacks_t *callbacks,
		void *user_data) {
	if (!callbacks) {
		fprintf(stderr, "Callbacks are required\n");
		return NULL;
	}

	socket_server_t *server = calloc(1, sizeof(socket_server_t));
	if (!server) return NULL;

	// TCP 소켓 생성
	server->listen_fd = socket_init();
	if (server->listen_fd < 0) {
		free(server);
		return NULL;
	}

	// 소켓 옵션 및 바인드
	socket_setsockopt_reuseaddr(server->listen_fd);

	// 주소 설정
	server->addr.sin_family = AF_INET;
	server->addr.sin_port = htons(port);
	if (ip) {
		inet_pton(AF_INET, ip, &server->addr.sin_addr);
	} else {
		server->addr.sin_addr.s_addr = INADDR_ANY;
	}

	if (bind(server->listen_fd, (struct sockaddr *)&server->addr, sizeof(server->addr)) < 0) {
		log_error("bind failed: %s", strerror(errno));
		close(server->listen_fd);
		free(server);
		return NULL;
	}

	socket_listen(server->listen_fd);

	// epoll 핸들러 생성
	server->epoll = epoll_handler_create();
	if (!server->epoll) {
		close(server->listen_fd);
		free(server);
		return NULL;
	}

	// 클라이언트 매니저 생성
	server->client_mgr = client_manager_create();
	if (!server->client_mgr) {
		epoll_handler_destroy(server->epoll);
		close(server->listen_fd);
		free(server);
		return NULL;
	}

	// listen_fd를 epoll에 등록
	if (epoll_handler_add(server->epoll, server->listen_fd, EPOLLIN | EPOLLET) == -1) {
		client_manager_destroy(server->client_mgr);
		epoll_handler_destroy(server->epoll);
		close(server->listen_fd);
		free(server);
		return NULL;
	}

	server->callbacks = *callbacks;
	server->user_data = user_data;
	server->running = 0;

	return server;
}

int socket_server_start(socket_server_t *server) {
	if (!server) return -1;

	server->running = 1;

	// on_start 콜백 호출
	if (server->callbacks.on_start) {
		server->callbacks.on_start(server->user_data);
	}

	// 이벤트 루프
	while (server->running) {
		int ret = epoll_handler_wait(server->epoll, -1, epoll_event_callback, server);
		if (ret == -1 && errno != EINTR) {
			break;
		}
	}

	// on_stop 콜백 호출
	if (server->callbacks.on_stop) {
		server->callbacks.on_stop(server->user_data);
	}

	return 0;
}

void socket_server_stop(socket_server_t *server) {
	if (!server) return;
	server->running = 0;
}

void socket_server_destroy(socket_server_t *server) {
	if (!server) return;

	if (server->listen_fd != -1) {
		close(server->listen_fd);
	}

	if (server->epoll) {
		epoll_handler_destroy(server->epoll);
	}

	if (server->client_mgr) {
		client_manager_destroy(server->client_mgr);
	}

	free(server);
}

int socket_server_send(client_info_t *client, const char *data, const size_t len) {
	if (!client || !data) return -1;
	return (int) send(client->fd, data, len, 0);
}

void socket_server_disconnect(client_info_t *client) {
	if (!client) return;
	close(client->fd);
}
// 논블로킹 모드 설정
int tcp_socket_set_nonblocking(const int fd) {
	const int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// TCP 서버 소켓 생성
int tcp_socket_create_server(const char *ip, int port, struct sockaddr_in *addr) {
	int fd = socket_init();
	if (fd < 0) return -1;

	socket_setsockopt_reuseaddr(fd);

	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	if (ip) {
		inet_pton(AF_INET, ip, &addr->sin_addr);
	} else {
		addr->sin_addr.s_addr = INADDR_ANY;
	}

	if (bind(fd, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
		close(fd);
		return -1;
	}

	if (listen(fd, SOMAXCONN) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

// 클라이언트 연결 수락
int tcp_socket_accept(int listen_fd, struct sockaddr_in *client_addr) {
	socklen_t client_len = sizeof(*client_addr);
	return accept(listen_fd, (struct sockaddr*)client_addr, &client_len);
}
