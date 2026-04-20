#ifndef EPOLL_HANDLER_H
#define EPOLL_HANDLER_H

#include <stdint.h>
#include "libepoll.h"

// handler 구조체 
struct epoll_handler_t {
    int epoll_fd;
    struct epoll_event events[EPOLL_MAX_EVENTS];
};

// epoll 핸들러 생성
epoll_handler_t* epoll_handler_create(void) {
    epoll_handler_t *handler = calloc(1, sizeof(epoll_handler_t));
    if (!handler) return NULL;

    handler->epoll_fd = epoll_create1(0);
    if (handler->epoll_fd == -1) {
        perror("epoll_create1");
        free(handler);
        return NULL;
    }

    return handler;
}

// epoll 핸들러 삭제
void epoll_handler_destroy(epoll_handler_t *handler) {
    if (!handler) return;

    if (handler->epoll_fd != -1) {
        close(handler->epoll_fd);
    }
    free(handler);
}

// fd 추가 (Edge-triggered)
int epoll_handler_add(epoll_handler_t *handler, int fd, void *ptr, uint32_t events) {
    if (!handler) return -1;

    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = ptr;

    if (epoll_ctl(handler->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl: add");
        return -1;
    }
    return 0;
}

// fd 삭제
int epoll_handler_remove(epoll_handler_t *handler, const int fd) {
    if (!handler) return -1;

    if (epoll_ctl(handler->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        perror("epoll_ctl: del");
        return -1;
    }
    return 0;
}

// 이벤트 대기 및 콜백 호출
int epoll_handler_wait(epoll_handler_t *handler, const int timeout_ms, const epoll_event_callback_fn callback, void *user_data) {
    if (!handler || !callback) return -1;

    const int nfds = epoll_wait(handler->epoll_fd, handler->events, EPOLL_MAX_EVENTS, timeout_ms);

    if (nfds == -1) {
        perror("epoll_wait");
        return -1;
    }

    for (int i = 0; i < nfds; i++) {
        callback(handler->events[i].data.ptr, handler->events[i].events, user_data);
    }

    return nfds;
}

// epoll fd 얻기
int epoll_handler_get_fd(epoll_handler_t *handler) {
    if (!handler) return -1;
    return handler->epoll_fd;
}
