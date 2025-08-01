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
#define GET_TAG(s)(s->broadcast ? 0x40 : 0) | s->security | s->suite

static unsigned char AUTHENTICATION_KEY_SIZE(DLMS_AES aes)
{
    return aes == DLMS_AES_128 ? 16 : 32;
}

static int gxgcm_init(
    const DLMS_AES aes,
    unsigned char* cipherKey,
    unsigned char* roundKeys)
{
    gxaes_keyExpansion(aes, cipherKey, roundKeys);
    return 0;
}

// Increase the block counter.
static void gxgcm_increaseBlockCounter(gxByteBuffer* nonceAndCounter,
    uint32_t counter)
{
    nonceAndCounter->size = 12;
    bb_setUInt32(nonceAndCounter, counter);
}




static int gxgcm_transformBlock(
    const DLMS_AES aes,
    const unsigned char* systemTitle,
    const uint32_t counter,
    unsigned char* data,
    const uint32_t length,
    uint32_t algorithmInitialblockCounter,
    unsigned char* roundKeys)
{
    gxByteBuffer nonceAndCounter;
    unsigned char NONSE[16];
    bb_attach(&nonceAndCounter, NONSE, 0, 16);
    bb_set(&nonceAndCounter, systemTitle, 8);
    bb_setUInt32(&nonceAndCounter, counter);
    bb_setUInt32(&nonceAndCounter, algorithmInitialblockCounter);

    uint16_t inputOffset = 0;
    uint32_t pos, pos2, count;
    unsigned char counterModeBlock[16] = { 0 };
    for (pos = 0; pos < length; pos += 16)
    {
#ifndef DLMS_USE_AES_HARDWARE_SECURITY_MODULE
        gxaes_cipher(aes, 1, NONSE, roundKeys, counterModeBlock);
#else
        gxByteBuffer cbm, secret;
        BB_ATTACH(cbm, counterModeBlock, 0);
        bb_attach(&secret, roundKeys, aes == DLMS_AES_128 ? 16 : 32, 
            aes == DLMS_AES_128 ? 16 : 32);
        gx_hsmAesEncrypt(aes, &nonceAndCounter, &secret, &cbm);
#endif //DLMS_USE_AES_HARDWARE_SECURITY_MODULE
        gxgcm_increaseBlockCounter(&nonceAndCounter, ++algorithmInitialblockCounter);
        count = length - pos < 16 ? length - pos : 16;
        for (pos2 = 0; pos2 != count; ++pos2)
        {
            data[inputOffset] ^= counterModeBlock[pos2];
            ++inputOffset;
        }
    }
    return 0;
}

// Make Xor for 128 bits.
static void gxgcm_xor(unsigned char* block, unsigned char* value)
{
    unsigned char pos;
    for (pos = 0; pos != 16; ++pos)
    {
        block[pos] ^= value[pos];
    }
}

// Shift block to right.
static void gxgcm_shiftRight(unsigned char* block)
{
    uint32_t val = GETU32(block + 12);
    val >>= 1;
    if (block[11] & 0x01)
    {
        val |= 0x80000000;
    }
    PUT32(block + 12, val);

    val = GETU32(block + 8);
    val >>= 1;
    if (block[7] & 0x01)
        val |= 0x80000000;
    PUT32(block + 8, val);

    val = GETU32(block + 4);
    val >>= 1;
    if (block[3] & 0x01)
        val |= 0x80000000;
    PUT32(block + 4, val);

    val = GETU32(block);
    val >>= 1;
    PUT32(block, val);
}

static void gxgcm_multiplyH(unsigned char* y, unsigned char* h)
{
    unsigned char i, j;
    unsigned char tmp[16];
    unsigned char z[16] = { 0 };
    memcpy(tmp, h, 16);
    //Loop every byte.
    for (i = 0; i != 16; ++i)
    {
        //Loop every bit.
        for (j = 0; j != 8; j++)
        {
            if ((y[i] & (1 << (7 - j))) != 0)
            {
                gxgcm_xor(z, &tmp[0]);
            }
            //If last bit.
            if ((tmp[15] & 0x01) != 0)
            {
                gxgcm_shiftRight(&tmp[0]);
                tmp[0] ^= 0xe1;
            }
            else
            {
                gxgcm_shiftRight(&tmp[0]);
            }
        }
    }
    memcpy(y, z, 16);
}

// Count GHASH.
static void gxgcm_getGHash(
    const DLMS_SECURITY security,
    const DLMS_AES aes,
    unsigned char tag,
    unsigned char* authenticationKey,
    gxByteBuffer* value,
    uint32_t lenA,
    uint32_t lenC,
    unsigned char* Y, 
    unsigned char* key)
{
    unsigned char EMPTY[16] = { 0 };
    unsigned char H[32] = { 0 };
#ifndef DLMS_USE_AES_HARDWARE_SECURITY_MODULE
    gxaes_cipher(aes, 1, EMPTY, key, H);
#else
    gxByteBuffer e, h, secret;
    BB_ATTACH(e, EMPTY, sizeof(EMPTY));
    BB_ATTACH(h, H, 0);
    bb_attach(&secret, key, (aes == DLMS_AES_128) ? 16 : 32, (aes == DLMS_AES_128) ? 16 : 32);
    gx_hsmAesEncrypt(aes, &e, &secret, &h);
#endif //DLMS_USE_AES_HARDWARE_SECURITY_MODULE
    uint32_t cnt, pos;
    unsigned char X[16];
    //Handle tag and authentication key.
    memset(X, 0, 16);
    memset(Y, 0, 16);
    X[0] = tag;
    memcpy(X + 1, authenticationKey, 15);
    gxgcm_xor(Y, X);
    gxgcm_multiplyH(Y, H);
    uint32_t available = AUTHENTICATION_KEY_SIZE(aes);
    if (available > 15)
    {
        available -= 15;
        if (available > 16)
        {
            //If key size is 32 bytes.
            available = 16;
            memcpy(&X[0], authenticationKey + 15, available);
            gxgcm_xor(Y, X);
            gxgcm_multiplyH(Y, H);
            available = AUTHENTICATION_KEY_SIZE(aes) - 31;
            //Add the authentication key remaining.
            memcpy(&X[0], authenticationKey + 31, available);
        }
        else
        {
            //Add the authentication key remaining.
            memcpy(&X[0], authenticationKey + 15, available);
        }
    }

    //Plain text.
    memset(X + available, 0, 16 - available);
    available = 0;
    gxgcm_xor(Y, X);
    gxgcm_multiplyH(Y, H);
 
    for (pos = 0; pos < bb_available(value); pos += 16)
    {
        memset(X + available, 0, 16 - available);
        cnt = bb_available(value);
        cnt -= pos;
        if (cnt > 16 - available)
        {
            cnt = 16 - available;
        }
        memcpy(X + available, value->data + value->position + pos, cnt);
        gxgcm_xor(Y, X);
        gxgcm_multiplyH(Y, H);
    }
    if (bb_available(value) + available > pos)
    {
        memset(X + available, 0, 16 - available);
        memcpy(X, value->data + value->position + pos - available, available);
        gxgcm_xor(Y, X);
        gxgcm_multiplyH(Y, H);
    }
    PUT32(X, 0L);
    PUT32(X + 4, lenA);
    PUT32(X + 8, 0L);
    PUT32(X + 12, lenC);
    gxgcm_xor(Y, X);
    gxgcm_multiplyH(Y, H);
}

static int gxgcm_getTag(
    const DLMS_SECURITY security,
    const DLMS_AES aes,
    DLMS_COUNT_TYPE type,
    const unsigned char* systemTitle,
    const uint32_t frameCounter,
    unsigned char* authenticationKey,
    const unsigned char tag,
    gxByteBuffer* value,
    gxByteBuffer* aTag,
    unsigned char* key)
{
    int ret;
    //Length of the crypted data.
    uint32_t lenC = 0;
    //Length of the authenticated data.
    uint32_t lenA = (1 + AUTHENTICATION_KEY_SIZE(aes)) * 8;


    lenC = 8 * bb_available(value);

    unsigned char hash[16];
    gxgcm_getGHash(security, aes, tag, authenticationKey,
        value, lenA, lenC, &hash[0], key);
    ret = gxgcm_transformBlock(aes, systemTitle, frameCounter,
        hash, sizeof(hash), 1,
        key);
    if (ret == 0)
    {
        if (type == DLMS_COUNT_TYPE_TAG)
        {
            aTag->size = 0;
        }
        ret = bb_set(aTag, &hash[0], 12);
    }
    return ret;
}


#ifndef DLMS_IGNORE_MALLOC
int cip_crypt(
    ciphering* settings,
    const DLMS_SECURITY security,
    DLMS_COUNT_TYPE type,
    uint32_t frameCounter,
    unsigned char tag,
    const unsigned char* systemTitle,
    gxByteBuffer* key,
    gxByteBuffer* input,
    unsigned char encrypt)
#else
int cip_crypt(
    ciphering* settings,
    const DLMS_SECURITY security,
    DLMS_COUNT_TYPE type,
    uint32_t frameCounter,
    unsigned char tag,
    const unsigned char* systemTitle,
    unsigned char* key,
    gxByteBuffer* input,
    unsigned char encrypt)
#endif //DLMS_IGNORE_MALLOC
{
#ifndef DLMS_USE_AES_HARDWARE_SECURITY_MODULE
    //AES 128 uses 176 round key(11 * 16) and 256 uses 240 (15 * 16).
#ifndef GX_DLMS_MICROCONTROLLER
    unsigned char gx_roundKeys[240];
#else
    static unsigned char gx_roundKeys[240];
#endif //DLMS_USE_AES_HARDWARE_SECURITY_MODULE
#endif //GX_DLMS_MICROCONTROLLER
    int ret = 0;
    DLMS_AES aes = settings->suite == DLMS_SECURITY_SUITE_V2 ? DLMS_AES_256 : DLMS_AES_128;
    if (ret == 0)
    {
        switch (security)
        {
        case DLMS_SECURITY_AUTHENTICATION_ENCRYPTION:
        #ifndef DLMS_USE_AES_HARDWARE_SECURITY_MODULE
        #ifndef DLMS_IGNORE_MALLOC
            gxgcm_init(aes, key->data, gx_roundKeys);
        #else
            gxgcm_init(aes, key, gx_roundKeys);
            #endif //DLMS_IGNORE_MALLOC  
            #endif //DLMS_USE_AES_HARDWARE_SECURITY_MODULE

            if (ret == 0 &&
                (ret = gxgcm_transformBlock(aes, systemTitle, frameCounter,
                    input->data + input->position, bb_available(input), 2,
            #ifndef DLMS_USE_AES_HARDWARE_SECURITY_MODULE
                                gx_roundKeys
            #else
            #ifndef DLMS_IGNORE_MALLOC
                                key->data
            #else
                                key
            #endif //DLMS_IGNORE_MALLOC  
            #endif //DLMS_USE_AES_HARDWARE_SECURITY_MODULE
                )) == 0)
            {
                if (encrypt)
                {
                    //The authentication tag is validated in a prior step.
                    ret = gxgcm_getTag(
                        security, aes, type, systemTitle, frameCounter,
                #ifdef DLMS_IGNORE_MALLOC
                            settings->authenticationKey,
                #else
                            settings->authenticationKey.data,
                #endif //DLMS_IGNORE_MALLOC
                            GET_TAG(settings), input, input,
                #ifndef DLMS_USE_AES_HARDWARE_SECURITY_MODULE
                            gx_roundKeys
                #else
                #ifndef DLMS_IGNORE_MALLOC
                            key->data
                #else
                            key
                #endif //DLMS_IGNORE_MALLOC
                #endif //DLMS_USE_AES_HARDWARE_SECURITY_MODULE
                    );
                }
            }
            break;
        default:
            ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
        if (ret == 0 && encrypt)
        {
            ++settings->invocationCounter;
        }
        if (ret == 0 && encrypt && type == DLMS_COUNT_TYPE_PACKET)
        {
            //Nonse must be 20 bytes because it's used later.
#ifdef GX_DLMS_MICROCONTROLLER
            static unsigned char NONSE[20] = { 0 };
#else
            unsigned char NONSE[20] = { 0 };
#endif //GX_DLMS_MICROCONTROLLER
            gxByteBuffer nonse;
            bb_attach(&nonse, NONSE, 0, sizeof(NONSE));
            if ((ret = bb_setUInt8(&nonse, tag)) == 0)
            {
                
                tag = (settings->broadcast ? 0x40 : 0) | settings->security | settings->suite;
                if ((ret = hlp_setObjectCount(5 + input->size, &nonse)) == 0 &&
                    (ret = bb_setUInt8(&nonse, tag)) == 0 &&
                    (ret = bb_setUInt32(&nonse, frameCounter)) == 0 &&
                    (ret = bb_insert(nonse.data, nonse.size, input, 0)) == 0)
                {
                }
            }
        }
    }
    return ret;
}


#ifndef DLMS_IGNORE_MALLOC
int cip_encrypt(
    ciphering* settings,
    DLMS_SECURITY security,
    DLMS_COUNT_TYPE type,
    uint32_t frameCounter,
    unsigned char tag,
    const unsigned char* systemTitle,
    gxByteBuffer* key,
    gxByteBuffer* input)
#else
int cip_encrypt(
    ciphering* settings,
    DLMS_SECURITY security,
    DLMS_COUNT_TYPE type,
    uint32_t frameCounter,
    unsigned char tag,
    const unsigned char* systemTitle,
    unsigned char* key,
    gxByteBuffer* input)
#endif //DLMS_IGNORE_MALLOC
{
#ifdef DLMS_DEBUG
    svr_notifyTrace5(GET_STR_FROM_EEPROM("System title: "), systemTitle, 8);
#ifndef DLMS_IGNORE_MALLOC
    svr_notifyTrace5(GET_STR_FROM_EEPROM("Block cipher key: "), key->data, key->size);
    svr_notifyTrace5(GET_STR_FROM_EEPROM("Authentication key: "), settings->authenticationKey.data, settings->authenticationKey.size);
#else
    svr_notifyTrace5(GET_STR_FROM_EEPROM("Block cipher key: "), key, 16);
    svr_notifyTrace5(GET_STR_FROM_EEPROM("Authentication key: "), settings->authenticationKey, 16);
#endif //DLMS_IGNORE_MALLOC

#endif //DLMS_DEBUG
#ifndef DLMS_IGNORE_MALLOC
    unsigned char keySize = settings->suite == DLMS_SECURITY_SUITE_V2 ? 32 : 16;
    if (settings->security == DLMS_SECURITY_NONE ||
        bb_available(&settings->authenticationKey) != keySize)
    {
        //Invalid system title.
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
#endif //DLMS_IGNORE_MALLOC
    return cip_crypt(
        settings,
        security,
        type,
        frameCounter,
        tag,
        systemTitle,
        key,
        input,
        1);
}


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