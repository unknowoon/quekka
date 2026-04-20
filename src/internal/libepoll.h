#ifndef EPOLL_HANDLER_H
#define EPOLL_HANDLER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EPOLL_MAX_EVENTS 64

// 불투명 포인터
typedef struct epoll_handler_t epoll_handler_t;

// 이벤트 콜백 타입
typedef void (*epoll_event_callback_fn)(void *ptr, uint32_t events, void *user_data);

// epoll 핸들러 생성
epoll_handler_t* epoll_handler_create(void);

// epoll 핸들러 삭제
void epoll_handler_destroy(epoll_handler_t *handler);

// fd 추가 (Edge-triggered)
int epoll_handler_add(epoll_handler_t *handler, int fd, void *ptr, uint32_t events);

// fd 삭제
int epoll_handler_remove(epoll_handler_t *handler, int fd);

// 이벤트 대기 및 콜백 호출
int epoll_handler_wait(epoll_handler_t *handler, int timeout_ms,
                       epoll_event_callback_fn callback, void *user_data);

// epoll fd 얻기
int epoll_handler_get_fd(epoll_handler_t *handler);

#ifdef __cplusplus
}
#endif

#endif
