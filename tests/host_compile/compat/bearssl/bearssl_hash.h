#pragma once

// Test-only BearSSL compatibility marker. The production build uses the
// ESP8266 Arduino BearSSL implementation; host tests provide the equivalent
// SHA-256 selector through OpenSSL in bearssl_hmac.h.
static const int br_sha256_vtable = 0;
