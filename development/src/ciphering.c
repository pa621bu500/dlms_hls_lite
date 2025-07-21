#include <string.h>
#include "../include/enums.h"
#include "../include/errorcodes.h"
#include "../include/ciphering.h"
#include "../include/helpers.h"

static const unsigned char DEFAULT_BROADCAST_BLOCK_CIPHER_KEY[] = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
static const unsigned char DEFAULT_BLOCK_CIPHER_KEY[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
static const unsigned char DEFAULT_SYSTEM_TITLE[] = { 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48 };
static const unsigned char DEFAULT_AUTHENTICATION_KEY[] = { 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
                                                            0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF
};


void cip_init(ciphering* target)
{
    target->invocationCounter = 0;
    target->suite = DLMS_SECURITY_SUITE_V0;
    target->security = DLMS_SECURITY_NONE;
    target->securityPolicy = DLMS_SECURITY_POLICY_NOTHING;
    target->encrypt = 0;
#ifndef DLMS_IGNORE_MALLOC
    BYTE_BUFFER_INIT(&target->blockCipherKey);
    bb_set(&target->blockCipherKey, DEFAULT_BLOCK_CIPHER_KEY, sizeof(DEFAULT_BLOCK_CIPHER_KEY));
    BYTE_BUFFER_INIT(&target->broadcastBlockCipherKey);
    bb_set(&target->broadcastBlockCipherKey, DEFAULT_BROADCAST_BLOCK_CIPHER_KEY, sizeof(DEFAULT_BROADCAST_BLOCK_CIPHER_KEY));
    BYTE_BUFFER_INIT(&target->systemTitle);
    bb_set(&target->systemTitle, DEFAULT_SYSTEM_TITLE, sizeof(DEFAULT_SYSTEM_TITLE));
    BYTE_BUFFER_INIT(&target->authenticationKey);
    bb_set(&target->authenticationKey, DEFAULT_AUTHENTICATION_KEY, sizeof(DEFAULT_AUTHENTICATION_KEY));
    target->dedicatedKey = NULL;
#else
    memcpy(target->blockCipherKey, DEFAULT_BLOCK_CIPHER_KEY, sizeof(DEFAULT_BLOCK_CIPHER_KEY));
    memcpy(target->broadcastBlockCipherKey, DEFAULT_BROADCAST_BLOCK_CIPHER_KEY, sizeof(DEFAULT_BROADCAST_BLOCK_CIPHER_KEY));
    memcpy(target->systemTitle, DEFAULT_SYSTEM_TITLE, sizeof(DEFAULT_SYSTEM_TITLE));
    memcpy(target->authenticationKey, DEFAULT_AUTHENTICATION_KEY, sizeof(DEFAULT_AUTHENTICATION_KEY));
    memset(target->dedicatedKey, 0, 16);
#endif //DLMS_IGNORE_MALLOC
    target->broadcast = 0;
}