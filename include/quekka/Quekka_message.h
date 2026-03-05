//
// Created by jaeseong on 2025. 12. 10..
//

#ifndef QUEKKA_QUEKKA_MESSAGE_H
#define QUEKKA_QUEKKA_MESSAGE_H

#include "Quekka_config.h"
#include <stdint.h>

#define QUEKKA_MSG_SIZE       4096
#define QUEKKA_TOPIC_MAX      (127 + 1)   // 토픽 최대 127자 + null
#define QUEKKA_MSG_ID_MAX     (17 + 1)    // yyyymmddhhMMsssss + null
#define QUEKKA_HEADER_SIZE    153         // 128 + 18 + 2 + 2 + 2 + 1
#define QUEKKA_PAYLOAD_MAX    (QUEKKA_MSG_SIZE - QUEKKA_HEADER_SIZE)  // 3943 + 8176 * 1024
#define QUEKKA_MSG_ID_FORMAT  "%Y%m%d%H%M%S"

// 메시지 분할 플래그
#define QUEKKA_FLAG_MORE      0x00  // 후속 조각 있음
#define QUEKKA_FLAG_LAST      0x01  // 마지막 조각

#pragma pack(push, 1)
typedef struct Quekka_header {
    char topic[QUEKKA_TOPIC_MAX];       // 128 bytes
    char message_id[QUEKKA_MSG_ID_MAX]; // 18 bytes (yyyymmddhhMMsssss)
    uint16_t topic_len;                 // 2 bytes
    uint16_t payload_len;               // 2 bytes
    uint16_t seq;                       // 2 bytes - 분할 조각 인덱스
    uint8_t flags;                      // 1 byte  - 0x01: 마지막 조각
} Quekka_header;

typedef struct Quekka_message {
    Quekka_header *header;
    char payload[QUEKKA_PAYLOAD_MAX];
} Quekka_message;
#pragma pack(pop)

#endif //QUEKKA_QUEKKA_MESSAGE_H
