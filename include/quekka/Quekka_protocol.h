#ifndef QUEKKA_QUEKKA_PROTOCOL_H
#define QUEKKA_QUEKKA_PROTOCOL_H

#include <stdint.h>

#include "Quekka_message.h"

int Quekka_serialize(const Quekka_message *message, uint8_t *buf, size_t size);
int Quekka_deserialize(const uint8_t *buf, Quekka_message *message, size_t size);

#endif 

