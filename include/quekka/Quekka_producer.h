//
// Created by jaeseong on 2025. 12. 10..
//

#ifndef QUEKKA_QUEKKA_PRODUCER_H
#define QUEKKA_QUEKKA_PRODUCER_H

#include <stdint.h>

#include "Quekka_config.h"
#include "Quekka_message.h"

typedef struct Quekka_producer {
    int _fd;
    Quekka_config *_config;
    int (*Quekka_publish)(const char *topic, const char *payload, size_t size);
} Quekka_producer;

/**
 *
 * @param config
 * @return
 */
Quekka_producer *Quekka_producer_init(const Quekka_config *config);

/**
 *
 * @param topic
 * @param payload
 * @param size
 * @return
 */
int Quekka_publish(const char *topic, const char *payload, size_t size);

#endif //QUEKKA_QUEKKA_PRODUCER_H
