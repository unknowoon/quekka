//
// Created by 우지범 on 2025. 12. 10..
//

#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "internal/libsocket.h"

#include "quekka/Quekka_message.h"
#include "quekka/Quekka_producer.h"
#include "quekka/Quekka_config.h"
#include "quekka/Quekka_log.h"
#include "quekka/Quekka_quecode.h"
#include "quekka/Quekka_protocol.h"


static _Thread_local Quekka_producer *g_producer = NULL;

static void header_assemble(Quekka_header *header, const char *topic);
static void message_assemble(Quekka_header *header, Quekka_message *message, const char *payload, uint16_t seq, uint8_t flag);

/**
 *
 *
 * @return
 */
Quekka_producer *Quekka_producer_init(const Quekka_config *config) {
    // 프로듀서 객체 초기화
    // g_producer 널 체크
    if (g_producer != NULL) {
        return g_producer;
    }

    // 서버와 tcp 소켓연결
    const int socket_fd = socket_init();
    if (!socket_fd) {
        log_error("failed to create socket.");
        Quecode_set_code(QUECODE_FAILURE);
        return NULL;
    }

    // 접속 서버 정보 세팅
    struct sockaddr_in server_address;
    socket_set_server_address(&server_address, config->_ip, config->_port);

    // 서버 접속
    socket_connect(&server_address, socket_fd);

    // 메모리 초기화
    g_producer = calloc(1, sizeof(Quekka_producer));
    if (!g_producer) {
        Quecode_set_code(QUECODE_FAILURE);
        return NULL;
    }

    memcpy(g_producer, config, sizeof(Quekka_producer));
    g_producer->_fd = socket_fd;
    g_producer->Quekka_publish = Quekka_publish;
    return g_producer;
}

/**
 *
 * @param topic
 * @param payload
 * @param payload_size
 * @return
 */
int Quekka_publish(const char *topic, const char *payload, const size_t payload_size) {

	// TODO: payload_size -> payload_size로 변경하는게 어떨지? 
	// UNINT16_MAX로 topic 길이 체크를 하기 때문에.

	const size_t topic_length = strlen(topic);
    const size_t payload_length = strlen(payload);

    if (topic_length == 0 || topic_length >= UINT16_MAX) {
        Quecode_set_code(QUECODE_FAILURE);
		return -1;
    }

    if (payload_length != payload_size) {
        Quecode_set_code(QUECODE_FAILURE);
        return -1;
    }

	// 메시지 길이 초과시 분할 
    size_t remaining = payload_length;
    uint16_t seq = 0;

    Quekka_header header = {0};

	// 헤더 조립
    header_assemble(&header, topic);
    while (remaining > 0) {

        const size_t chunk_size = (remaining > QUEKKA_PAYLOAD_MAX) ? QUEKKA_PAYLOAD_MAX : remaining;
        const uint8_t flag = (remaining >= QUEKKA_PAYLOAD_MAX) ? QUEKKA_FLAG_MORE : QUEKKA_FLAG_LAST;

		Quekka_message message = {0};

        message_assemble(&header, &message, payload + (seq * QUEKKA_PAYLOAD_MAX), seq, flag);

		// NOTE: server 를 보고 flag 추가했다. 
		// epoll handler가 필요하며
        send(g_producer->_fd, &message, sizeof(message), MSG_DONTWAIT);

        remaining -= chunk_size;
        seq++;
    }

    return 0;
}

/**
 *
 * @param header
 * @param topic
 * @param flag
 * @return
 */
static void header_assemble(Quekka_header *header, const char *topic) {
    // TODO: 시간 유틸 함수 생성 필요
    // 메시지 아이디 조립: 현재시간으로 포멧맞춰 문자열 생성
    const time_t now = time(NULL);
    const struct tm *t = localtime(&now);
    char msg_id[QUEKKA_MSG_ID_MAX];
    strftime(msg_id, sizeof(msg_id), QUEKKA_MSG_ID_FORMAT, t);
	
	strncpy(header->topic, topic, sizeof(QUEKKA_TOPIC_MAX-1));
	header->topic[QUEKKA_TOPIC_MAX-1] = '\0';

	strncpy(header->message_id, msg_id, sizeof(QUEKKA_MSG_ID_MAX-1));
	header->message_id[QUEKKA_MSG_ID_MAX-1] = '\0';

	header->topic_len = (uint16_t) strlen(topic);
}

/**
 *
 * @param header
 * @param message
 * @param payload
 * @param seq
 * @param flag
 */
static void message_assemble(Quekka_header *header, Quekka_message *message, const char *payload, const uint16_t seq, const uint8_t flag) {

	header->payload_len = (uint16_t) strlen(payload);
    header->seq = seq;
    header->flags = flag;

    memcpy(&message->header, header, QUEKKA_HEADER_SIZE-1);

    strncpy(message->payload, payload, QUEKKA_PAYLOAD_MAX-1);
	message->payload[QUEKKA_PAYLOAD_MAX-1] = '\0';
}
