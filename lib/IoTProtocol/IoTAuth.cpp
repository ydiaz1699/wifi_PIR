/**
 * IoTAuth V4.3 — Implementación HMAC-SHA256 truncada
 *
 * Usa BearSSL (integrado en ESP8266 Arduino core) para HMAC-SHA256.
 * El HMAC se calcula sobre: version + type + src + dst + bootId + seq + flags + payload (sin auth TLV).
 * Se trunca a 4 bytes y se agrega como último TLV del payload.
 */

#include "IoTAuth.h"
#include <bearssl/bearssl_hmac.h>
#include <bearssl/bearssl_hash.h>
#include <string.h>

// ============================================================
// Constructor
// ============================================================

IoTAuth::IoTAuth(const uint8_t* key, uint8_t keyLen)
    : _keyLen(keyLen), _required(false)
{
    if (keyLen > IOT_AUTH_KEY_MAX_SIZE) keyLen = IOT_AUTH_KEY_MAX_SIZE;
    _keyLen = keyLen;
    memcpy(_key, key, keyLen);
}

// ============================================================
// Cálculo del HMAC
// ============================================================

void IoTAuth::_computeHmac(const IoTPacket &pkt, uint8_t payloadLenForHash, uint8_t out[IOT_HMAC_TRUNC_SIZE]) {
    // Preparar los datos a firmar:
    // version(1) + type(1) + src(1) + dst(1) + bootId(2) + seq(4) + flags(1) + payload(N)
    // Total header fields: 11 bytes + payload

    uint8_t headerData[11];
    headerData[0] = pkt.version;
    headerData[1] = static_cast<uint8_t>(pkt.type);
    headerData[2] = pkt.src;
    headerData[3] = pkt.dst;
    headerData[4] = (pkt.bootId >> 8) & 0xFF;
    headerData[5] = pkt.bootId & 0xFF;
    headerData[6] = (pkt.seq >> 24) & 0xFF;
    headerData[7] = (pkt.seq >> 16) & 0xFF;
    headerData[8] = (pkt.seq >> 8) & 0xFF;
    headerData[9] = pkt.seq & 0xFF;
    headerData[10] = pkt.flags & ~IOT_FLAG_AUTHENTICATED;  // Sin el flag auth para el cálculo

    // BearSSL HMAC-SHA256
    br_hmac_key_context kc;
    br_hmac_context ctx;

    br_hmac_key_init(&kc, &br_sha256_vtable, _key, _keyLen);
    br_hmac_init(&ctx, &kc, 0);  // 0 = full output, truncaremos manualmente

    // Alimentar: header fields + payload (sin el TLV de auth)
    br_hmac_update(&ctx, headerData, 11);
    if (payloadLenForHash > 0) {
        br_hmac_update(&ctx, pkt.payload, payloadLenForHash);
    }

    // Obtener resultado completo (32 bytes) y truncar
    uint8_t fullHmac[32];
    br_hmac_out(&ctx, fullHmac);

    // Truncar a los primeros 4 bytes
    memcpy(out, fullHmac, IOT_HMAC_TRUNC_SIZE);
}

// ============================================================
// Firmar paquete
// ============================================================

bool IoTAuth::signPacket(IoTPacket &pkt) {
    // Verificar que hay espacio para el TLV de auth (tag:1 + len:1 + value:4 = 6 bytes)
    if ((uint16_t)pkt.payloadLen + 2 + IOT_HMAC_TRUNC_SIZE > IOT_MAX_PAYLOAD) {
        return false;  // No hay espacio
    }

    // Calcular HMAC sobre el payload actual (sin el auth TLV)
    uint8_t hmac[IOT_HMAC_TRUNC_SIZE];
    _computeHmac(pkt, pkt.payloadLen, hmac);

    // Agregar TLV AUTH_HMAC4
    pkt.payload[pkt.payloadLen++] = IOT_TLV_AUTH_HMAC4;
    pkt.payload[pkt.payloadLen++] = IOT_HMAC_TRUNC_SIZE;
    memcpy(&pkt.payload[pkt.payloadLen], hmac, IOT_HMAC_TRUNC_SIZE);
    pkt.payloadLen += IOT_HMAC_TRUNC_SIZE;

    // Setear flag
    pkt.flags |= IOT_FLAG_AUTHENTICATED;

    return true;
}

// ============================================================
// Verificar paquete
// ============================================================

bool IoTAuth::verifyPacket(const IoTPacket &pkt) {
    // Si no tiene el flag de auth
    if (!isAuthenticated(pkt)) {
        return !_required;  // Si auth es requerido → false; si opcional → true
    }

    // Buscar el TLV AUTH_HMAC4 en el payload
    // Debe ser el último TLV (lo agregamos al final en signPacket)
    uint8_t offset = 0;
    uint8_t authOffset = 0;
    bool found = false;

    while (offset + 2 <= pkt.payloadLen) {
        uint8_t tag = pkt.payload[offset];
        uint8_t len = pkt.payload[offset + 1];
        if ((uint16_t)offset + 2 + len > pkt.payloadLen) break;

        if (tag == IOT_TLV_AUTH_HMAC4 && len == IOT_HMAC_TRUNC_SIZE) {
            authOffset = offset;
            found = true;
        }
        offset += 2 + len;
    }

    if (!found) return false;  // Flag dice auth pero no hay TLV

    // Extraer el HMAC recibido
    uint8_t receivedHmac[IOT_HMAC_TRUNC_SIZE];
    memcpy(receivedHmac, &pkt.payload[authOffset + 2], IOT_HMAC_TRUNC_SIZE);

    // Calcular HMAC sobre el payload SIN el TLV de auth
    // (todo antes del authOffset)
    uint8_t hmac[IOT_HMAC_TRUNC_SIZE];
    _computeHmac(pkt, authOffset, hmac);

    // Comparación en tiempo constante (previene timing attacks)
    uint8_t diff = 0;
    for (int i = 0; i < IOT_HMAC_TRUNC_SIZE; i++) {
        diff |= hmac[i] ^ receivedHmac[i];
    }

    return (diff == 0);
}
