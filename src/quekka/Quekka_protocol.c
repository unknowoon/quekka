#include "quekka/Quekka_protocol.h"

int Quekka_serialize(const Quekka_message *message, uint8_t *buf, size_t size) {

	// 길이 검증
	size_t total_len = QUEKKA_HEADER_SIZE + message->header->payload_len;
	if (size < total_len) {
		return -1;
	}

	memcpy(buf, message->header, QUEKKA_HEADER_SIZE);
	memcpy(buf + QUEKKA_HEADER_SIZE, message->payload, message->header->payload_len);

	return (int) total_len;
}

int Quekka_deserialize(const uint8_t *buf, Quekka_message *message, size_t size) {

	// 길이 검증
	if (size < QUEKKA_HEADER_SIZE) {
		return -1;
	}

	memcpy(message->header, buf, QUEKKA_HEADER_SIZE);

	size_t paylen = message->header->payload_len;
	if (size != paylen + QUEKKA_HEADER_SIZE) {
		return -1;
	}

	memcpy(message->payload, buf + QUEKKA_HEADER_SIZE, paylen);

	return (int) size;
}

