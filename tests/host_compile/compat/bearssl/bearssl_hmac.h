#pragma once

#ifndef OPENSSL_SUPPRESS_DEPRECATED
#define OPENSSL_SUPPRESS_DEPRECATED
#endif
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <cstddef>
#include <cstdint>

struct br_hmac_key_context {
    const uint8_t* key;
    std::size_t key_len;
};

struct br_hmac_context {
    HMAC_CTX* ctx;
};

inline void br_hmac_key_init(br_hmac_key_context* context,
                             const void*, const void* key,
                             std::size_t key_len) {
    context->key = static_cast<const uint8_t*>(key);
    context->key_len = key_len;
}

inline void br_hmac_init(br_hmac_context* context,
                         const br_hmac_key_context* key, int) {
    context->ctx = HMAC_CTX_new();
    HMAC_Init_ex(context->ctx, key->key, static_cast<int>(key->key_len),
                 EVP_sha256(), nullptr);
}

inline void br_hmac_update(br_hmac_context* context,
                           const void* data, std::size_t length) {
    HMAC_Update(context->ctx, static_cast<const unsigned char*>(data), length);
}

inline void br_hmac_out(br_hmac_context* context, void* out) {
    unsigned int length = 0;
    HMAC_Final(context->ctx, static_cast<unsigned char*>(out), &length);
    HMAC_CTX_free(context->ctx);
    context->ctx = nullptr;
}
