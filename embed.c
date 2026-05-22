#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define SOF_BYTE 0xAA
#define MAX_PAYLOAD_LENGTH 16

#define PARSER_OK 1
#define PARSER_IN_PROGRESS 0
#define PARSER_CHECKSUM_ERROR -1
#define PARSER_TIMEOUT -2

typedef enum {
    STATE_WAIT_FOR_SOF,
    STATE_RECEIVE_CMD,
    STATE_RECEIVE_LEN,
    STATE_RECEIVE_PAYLOAD,
    STATE_RECEIVE_CHECKSUM
} ParserState;

typedef struct {
    ParserState state;
    uint8_t cmd, len, payload[MAX_PAYLOAD_LENGTH];
    uint8_t checksum, calc;
    uint8_t index;
    uint32_t timeout, last_time;
} UARTParser;

void reset(UARTParser *p) {
    p->state = STATE_WAIT_FOR_SOF;
    p->cmd = p->len = p->checksum = p->calc = p->index = 0;
    memset(p->payload, 0, sizeof(p->payload));
}

void init(UARTParser *p, uint32_t timeout) {
    reset(p);
    p->timeout = timeout;
}

int feed(UARTParser *p, uint8_t byte, uint32_t t) {

    if (p->timeout && p->state != STATE_WAIT_FOR_SOF &&
        (t - p->last_time > p->timeout)) {
        reset(p);
        return PARSER_TIMEOUT;
    }

    switch (p->state) {

    case STATE_WAIT_FOR_SOF:
        if (byte == SOF_BYTE) {
            p->state = STATE_RECEIVE_CMD;
            p->last_time = t;
        }
        break;

    case STATE_RECEIVE_CMD:
        p->cmd = p->calc = byte;
        p->state = STATE_RECEIVE_LEN;
        p->last_time = t;
        break;

    case STATE_RECEIVE_LEN:
        p->len = byte;
        if (p->len > MAX_PAYLOAD_LENGTH) {
            reset(p);
            return PARSER_CHECKSUM_ERROR;
        }
        p->calc ^= byte;
        p->index = 0;
        p->state = (p->len == 0) ? STATE_RECEIVE_CHECKSUM : STATE_RECEIVE_PAYLOAD;
        p->last_time = t;
        break;

    case STATE_RECEIVE_PAYLOAD:
        p->payload[p->index++] = byte;
        p->calc ^= byte;
        if (p->index >= p->len)
            p->state = STATE_RECEIVE_CHECKSUM;
        p->last_time = t;
        break;

    case STATE_RECEIVE_CHECKSUM:
        p->checksum = byte;
        int result = (p->checksum == p->calc) ? PARSER_OK : PARSER_CHECKSUM_ERROR;
        reset(p);
        return result;
    }

    return PARSER_IN_PROGRESS;
}

void print_payload(uint8_t *p, uint8_t len) {
    printf("[");
    for (int i = 0; i < len; i++)
        printf("%02X%s", p[i], (i < len - 1) ? " " : "");
    printf("]");
}

void run(UARTParser *p, uint8_t *b, uint32_t *t, int n) {

    uint8_t cmd, len, payload[MAX_PAYLOAD_LENGTH];

    for (int i = 0; i < n; i++) {

        cmd = p->cmd;
        len = p->len;
        memcpy(payload, p->payload, MAX_PAYLOAD_LENGTH);

        int r = feed(p, b[i], t[i]);

        if (r == PARSER_IN_PROGRESS)
            printf("t=%3ums byte=0x%02X -> receiving...\n", t[i], b[i]);

        else if (r == PARSER_OK) {
            printf("t=%3ums byte=0x%02X -> FRAME OK CMD=0x%02X LEN=%u PAYLOAD=",
                   t[i], b[i], cmd, len);
            print_payload(payload, len);
            printf("\n");
        }

        else if (r == PARSER_CHECKSUM_ERROR)
            printf("t=%3ums byte=0x%02X -> CHECKSUM ERROR\n", t[i], b[i]);

        else if (r == PARSER_TIMEOUT) {
            printf("t=%3ums byte=0x%02X -> TIMEOUT -- parser reset\n", t[i], b[i]);

            if (feed(p, b[i], t[i]) == PARSER_IN_PROGRESS)
                printf("t=%3ums byte=0x%02X -> receiving... (re-fed after reset)\n",
                       t[i], b[i]);
        }
    }
}

int main() {

    UARTParser p;

    /* TEST 1 (FIXED CHECKSUM) */
    uint8_t b1[] = {0xAA,0x01,0x03,0x10,0x20,0x30,0x02};
    uint32_t t1[] = {0,5,10,15,20,25,30};

    printf("\n===== TEST 1 =====\n");
    init(&p, 50);
    run(&p, b1, t1, 7);

    /* TEST 2 */
    uint8_t b2[] = {0xAA,0x01,0x03,0x10,0xAA,0x05,0x01,0x7F,0x7B};
    uint32_t t2[] = {0,5,10,15,200,205,210,215,220};

    printf("\n===== TEST 2 =====\n");
    init(&p, 50);
    run(&p, b2, t2, 9);

    /* TEST 3 */
    uint8_t b3[] = {0xAA,0x03,0x01,0x55,0x57,0xAA,0x04,0x02,0xAA,0xBB,0x17};
    uint32_t t3[] = {0,5,10,15,20,25,30,35,40,45,50};

    printf("\n===== TEST 3 =====\n");
    init(&p, 50);
    run(&p, b3, t3, 11);

    /* TEST 4 */
    printf("\n===== TEST 4 =====\n");
    init(&p, 0);
    run(&p, b2, t2, 9);

    return 0;
}