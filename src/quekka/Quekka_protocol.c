#include "../../Quekka_protocol.h"

int Quekka_serialize(const Quekka_message *message, uint8_t *buf, size_t size) {

	// 길이 검증
	if (len(message) != size) {
		return -1;
	}
	
}

