#include <string.h>
#include <stdio.h>

#define CBC 1

#include "aes.h"
#include "crypto.h"

const uint8_t iv[]  = { 0x13, 0x22, 0x51, 0x78, 0x4f, 0x50, 0x23, 0x25, 0x65, 0x61, 0x5f, 0x71, 0x60, 0x61, 0x23, 0x7f };

// data is always 1024 bytes
// key is always 32 bytes
uint8_t * encrypt(char data[])
{
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, (uint8_t *)KEY, iv);
    AES_CBC_encrypt_buffer(&ctx, (uint8_t *)data, 1024);

    return (uint8_t *)data;
}

char * decrypt(uint8_t data[])
{
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, (uint8_t *)KEY, iv);
    AES_ctx_set_iv(&ctx, iv);
    AES_CBC_decrypt_buffer(&ctx, data, 1024);

    return (char *)data;
}
