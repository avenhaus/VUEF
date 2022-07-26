#ifndef VUEF_H
#define VUEF_H

#include <Arduino.h>
#include "VuefConfInt.h"

/* ============================================== *\
 * Constants
\* ============================================== */

extern const char EMPTY_STRING[];
extern const char NEW_LINE[];

extern const char HOSTNAME[] PROGMEM;
extern const char PROJECT_NAME[] PROGMEM;
extern const char PROJECT_VERSION[] PROGMEM;
extern const char COMPILE_DATE[] PROGMEM;
extern const char COMPILE_TIME[] PROGMEM;

#define FST (const char *)F
#define PM (const __FlashStringHelper *)


extern void vuefInit();
extern void vuefRun(uint32_t now=0);

/* ============================================== *\
 * Debug
\* ============================================== */

#if SERIAL_DEBUG < 1
#define DEBUG_println(...) 
#define DEBUG_print(...) 
#define DEBUG_printf(...) 
#else
#define DEBUG_println(...) if (debugStream) {debugStream->println(__VA_ARGS__);}
#define DEBUG_print(...) if (debugStream) {debugStream->print(__VA_ARGS__);}
#define DEBUG_printf(...) if (debugStream) {debugStream->printf(__VA_ARGS__);}
#endif

extern Print* debugStream;


/* ============================================== *\
 * Error Codes
\* ============================================== */

typedef enum ErrorCode {
    EC_OK,
    EC_ERROR=500,
    EC_UNKNOWN,
    EC_TIMEOUT,
    EC_BAD_ARGUMENT,
    EC_HTTP_OK=200,
    EC_BAD_REQUEST=400,
    EC_UNAUTHORIZED=401,
    EC_PAYMENT_REQUIRED=402,
    EC_FORBIDDEN=403,
    EC_NOT_FOUND=404,
    EC_METHOD_NOT_ALLOWED=405,
    EC_NOT_ACCEPTABLE=406,
    EC_PROXY_AUTHENTICATION_REQUIRED=407,
    EC_REQUEST_TIMEOUT=408,
    EC_CONFLICT=409,
    EC_GONE=410,
    EC_LENGTH_REQUIRED=411,
    EC_PRECONDITION_FAILED=412,
    EC_PAYLOAD_TOO_LARGE=413,
    EC_URI_TOO_LONG=414,
    EC_UNSUPPORTED_MEDIA=415,
    EC_RANGE_NOT_SATISFIABLE=416,
    EC_EXPECTATION_FAILED=417,
    EC_MISDIRECTED_REQUEST=421,
    EC_UNPROCESSABLE_ENTITY=422,
    EC_LOCKED=423,
    EC_FAILED_DEPENDENCY=424,
    EC_TOO_EARLY=425,
    EC_UPGRADE_REQUIRED=426,
    EC_PRECONDITION_REQUIRED=428,
    EC_TOO_MANY_REQUESTS=429,
    EC_REQUEST_HEADER_FIELDS_TOO_LARGE=431,
    EC_UNAVAILABLE_FOR_LEGAL_REASONS=451,
    EC_INTERNAL_SERVER_ERROR=500,
    EC_NOT_IMPLEMENTED=501,
    EC_BAD_GATEWAY=502,
    EC_SERVICE_UNAVAILABLE=503,
    EC_GATEWAY_TIMEOUT=504,
    EC_HTTP_COMPILE_NOT_SUPPORTED=505,
    EC_VARIANT_ALSO_NEGOTIATES=506,
    EC_INSUFFICIENT_STORAGE=507,
    EC_LOOP_DETECTED=508,
    EC_NOT_EXTENDED=510,
    EC_NETWORK_AUTHENTICATION_REQUIRED=511
} ErrorCode;


/* ============================================== *\
 * Includes
\* ============================================== */

#include "ConfigReg.h"
#include "StateReg.h"
#include "Helper.h"
#include "Network.h"

#endif // VUEF_H