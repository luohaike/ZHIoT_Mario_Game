#include "wifi_net.h"
#include "stm32l4xx_hal.h"
#include "usart_api.h"
#include <stdio.h>
#include <string.h>

#define WIFI_SSID       "MARIO-MP"
#define WIFI_PASS       "12345678"
#define WIFI_UDP_PORT   9000U

#define RX_RING_SIZE    512U
#define TX_RING_SIZE    256U
#define EV_QUEUE_SIZE   16U
#define PEER_TIMEOUT_MS 2500U

typedef struct __attribute__((packed)) {
    uint16_t magic;       /* 0x4D48 ('MH') */
    uint8_t  seq;
    uint8_t  room_state;
    uint8_t  level;
    uint8_t  lives;
    uint16_t coins;
    uint32_t score;
    uint32_t slot_uid[WIFI_MAX_PLAYERS];
    struct __attribute__((packed)) {
        int16_t x;
        int16_t y;
        uint8_t spr;
        uint8_t flags;
    } players[WIFI_MAX_PLAYERS];
    uint8_t  ev_seq;
    uint8_t  ev_type;
    uint8_t  ev_row;
    uint8_t  ev_col;
    uint8_t  ev_val;
    uint8_t  checksum;
} WifiHostSnap;

typedef struct __attribute__((packed)) {
    uint16_t magic;       /* 0x4D43 ('MC') */
    uint8_t  seq;
    uint8_t  reserved;
    uint32_t uid;
    int16_t  x;
    int16_t  y;
    uint8_t  spr;
    uint8_t  flags;
    uint8_t  ev_seq;
    uint8_t  ev_type;
    uint8_t  ev_row;
    uint8_t  ev_col;
    uint8_t  ev_val;
    uint8_t  checksum;
} WifiClientPkt;

_Static_assert(sizeof(WifiHostSnap) == 58, "WifiHostSnap size check");
_Static_assert(sizeof(WifiClientPkt) == 20, "WifiClientPkt size check");

typedef struct {
    uint8_t type;
    uint8_t row;
    uint8_t col;
    uint8_t val;
} NetEventItem;

static volatile uint8_t  rx_ring[RX_RING_SIZE];
static volatile uint16_t rx_head = 0U;
static volatile uint16_t rx_tail = 0U;

static volatile uint8_t  tx_ring[TX_RING_SIZE];
static volatile uint16_t tx_head = 0U;
static volatile uint16_t tx_tail = 0U;

static WifiStatus status;
static uint8_t    uart1_opened = 0U;
static uint32_t   local_uid = 0U;

/* AT / TX state */
static uint8_t  at_wait_ok = 0U;
static uint8_t  at_retry = 0U;
static uint32_t at_deadline_ms = 0U;
static uint8_t  at_got_ok = 0U;
static uint8_t  at_got_err = 0U;
static uint8_t  at_got_already = 0U;
static uint8_t  at_got_prompt = 0U;
static uint8_t  at_got_send_ok = 0U;

typedef enum {
    TX_IDLE = 0,
    TX_WAIT_PROMPT,
    TX_WAIT_SEND_OK
} TxStep;

static TxStep   tx_step = TX_IDLE;
static uint32_t tx_deadline_ms = 0U;
static uint32_t last_pkt_tx_ms = 0U;
static uint8_t  tx_seq = 0U;
static uint8_t  udp_send_use_1arg = 0U;
static uint8_t  pending_pkt_buf[64];
static uint16_t pending_pkt_len = 0U;

/* Local player & host room mirror to transmit */
static int16_t  loc_x = 48;
static int16_t  loc_y = 160;
static uint8_t  loc_spr = 0U;
static uint8_t  loc_flags = 0x06U;
static uint8_t  host_room_state = 0U;
static uint8_t  host_level = 1U;
static uint8_t  host_lives = 3U;
static uint16_t host_coins = 0U;
static uint32_t host_score = 0U;

/* Outgoing event repeat latch */
static uint8_t  out_ev_seq = 0U;
static uint8_t  out_ev_type = 0U;
static uint8_t  out_ev_row = 0U;
static uint8_t  out_ev_col = 0U;
static uint8_t  out_ev_val = 0U;
static uint8_t  out_ev_repeat = 0U;

/* Incoming event queue + deduplication */
static NetEventItem ev_queue[EV_QUEUE_SIZE];
static uint8_t      ev_q_head = 0U;
static uint8_t      ev_q_tail = 0U;
static uint8_t      last_host_ev_seq = 0U;
static uint8_t      last_client_ev_seq[WIFI_MAX_PLAYERS];
static uint32_t     slot_uids[WIFI_MAX_PLAYERS];

/* Stream parser (+IPD binary vs ASCII AT lines) */
typedef enum {
    PARSE_LINE = 0,
    PARSE_IPD_LEN,
    PARSE_IPD_DATA
} ParseState;

static ParseState parse_st = PARSE_LINE;
static char       line_buf[128];
static uint8_t    line_len = 0U;
static uint16_t   ipd_len = 0U;
static uint16_t   ipd_pos = 0U;
static uint8_t    ipd_buf[128];
static const char IPD_HDR[] = "+IPD,";
static uint8_t    ipd_hdr_match = 0U;

static uint8_t xor_checksum(const uint8_t *buf, uint16_t len_no_csum)
{
    uint8_t c = 0U;
    for (uint16_t i = 0U; i < len_no_csum; i++) {
        c ^= buf[i];
    }
    return c;
}

static uint32_t compute_uid32(void)
{
    uint32_t u0 = *(const volatile uint32_t *)(UID_BASE + 0x00U);
    uint32_t u1 = *(const volatile uint32_t *)(UID_BASE + 0x04U);
    uint32_t u2 = *(const volatile uint32_t *)(UID_BASE + 0x08U);
    uint32_t v = u0 ^ (u1 * 0x9E3779B9UL) ^ (u2 * 0x85EBCA6BUL);
    return (v == 0U) ? 0x12345678UL : v;
}

void WiFi_Net_USART1_IRQ(void)
{
    uint32_t isr = USART1->ISR;
    if ((isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE | USART_ISR_PE)) != 0U) {
        USART1->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NCF | USART_ICR_PECF;
    }
    if ((isr & USART_ISR_RXNE) != 0U) {
        uint8_t b = (uint8_t)(USART1->RDR & 0xFFU);
        uint16_t next = (uint16_t)((rx_head + 1U) % RX_RING_SIZE);
        if (next != rx_tail) {
            rx_ring[rx_head] = b;
            rx_head = next;
        }
    }
    if ((isr & USART_ISR_TXE) != 0U && (USART1->CR1 & USART_CR1_TXEIE) != 0U) {
        if (tx_tail != tx_head) {
            USART1->TDR = tx_ring[tx_tail];
            tx_tail = (uint16_t)((tx_tail + 1U) % TX_RING_SIZE);
        } else {
            CLEAR_BIT(USART1->CR1, USART_CR1_TXEIE);
        }
    }
}

static void usart1_send_bytes(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0U; i < len; i++) {
        uint16_t next = (uint16_t)((tx_head + 1U) % TX_RING_SIZE);
        if (next == tx_tail) {
            break;
        }
        tx_ring[tx_head] = data[i];
        tx_head = next;
    }
    SET_BIT(USART1->CR1, USART_CR1_TXEIE);
}

static void usart1_send_str(const char *s)
{
    usart1_send_bytes((const uint8_t *)s, (uint16_t)strlen(s));
}

static void push_ev_queue(uint8_t type, uint8_t row, uint8_t col, uint8_t val)
{
    uint8_t next = (uint8_t)((ev_q_head + 1U) % EV_QUEUE_SIZE);
    if (next == ev_q_tail) {
        return;
    }
    ev_queue[ev_q_head].type = type;
    ev_queue[ev_q_head].row = row;
    ev_queue[ev_q_head].col = col;
    ev_queue[ev_q_head].val = val;
    ev_q_head = next;
}

static void clear_at_flags(void)
{
    at_got_ok = 0U;
    at_got_err = 0U;
    at_got_already = 0U;
    at_got_prompt = 0U;
    at_got_send_ok = 0U;
}

static void send_at_cmd(const char *cmd, uint32_t timeout_ms)
{
    clear_at_flags();
    usart1_send_str(cmd);
    at_wait_ok = 1U;
    at_deadline_ms = HAL_GetTick() + timeout_ms;
}

static void handle_ipd_packet(const uint8_t *buf, uint16_t len, uint32_t now_ms)
{
    if (status.role == WIFI_MODE_HOST && len == sizeof(WifiClientPkt)) {
        const WifiClientPkt *cp = (const WifiClientPkt *)buf;
        if (cp->magic != 0x4D43U) {
            return;
        }
        if (xor_checksum(buf, (uint16_t)(sizeof(WifiClientPkt) - 1U)) != cp->checksum) {
            return;
        }
        if (cp->uid == 0U || cp->uid == local_uid) {
            return;
        }

        /* Find existing slot or assign free slot 1..3 */
        int8_t slot = -1;
        for (uint8_t s = 1U; s < WIFI_MAX_PLAYERS; s++) {
            if (slot_uids[s] == cp->uid) {
                slot = (int8_t)s;
                break;
            }
        }
        if (slot < 0) {
            for (uint8_t s = 1U; s < WIFI_MAX_PLAYERS; s++) {
                if (slot_uids[s] == 0U || !status.players[s].active) {
                    slot_uids[s] = cp->uid;
                    last_client_ev_seq[s] = cp->ev_seq;
                    slot = (int8_t)s;
                    break;
                }
            }
        }
        if (slot < 1) {
            return;
        }

        status.players[slot].x = cp->x;
        status.players[slot].y = cp->y;
        status.players[slot].spr = cp->spr;
        status.players[slot].flags = cp->flags;
        status.players[slot].active = 1U;
        status.players[slot].last_rx_ms = now_ms;
        status.rx_ok_count++;

        if (cp->ev_seq != 0U && cp->ev_seq != last_client_ev_seq[slot] && cp->ev_type != NET_EV_NONE) {
            last_client_ev_seq[slot] = cp->ev_seq;
            push_ev_queue(cp->ev_type, cp->ev_row, cp->ev_col, cp->ev_val);
            /* Re-broadcast client event to all peers as Host authoritative event */
            WiFi_Net_PushEvent(cp->ev_type, cp->ev_row, cp->ev_col, cp->ev_val);
        }
    } else if (status.role == WIFI_MODE_JOIN && len == sizeof(WifiHostSnap)) {
        const WifiHostSnap *hs = (const WifiHostSnap *)buf;
        if (hs->magic != 0x4D48U) {
            return;
        }
        if (xor_checksum(buf, (uint16_t)(sizeof(WifiHostSnap) - 1U)) != hs->checksum) {
            return;
        }

        status.remote_room_state = hs->room_state;
        status.remote_level = hs->level;
        status.remote_lives = hs->lives;
        status.remote_coins = hs->coins;
        status.remote_score = hs->score;
        status.rx_ok_count++;

        uint8_t cnt = 0U;
        for (uint8_t s = 0U; s < WIFI_MAX_PLAYERS; s++) {
            slot_uids[s] = hs->slot_uid[s];
            if (hs->slot_uid[s] == local_uid) {
                status.my_slot = s;
                status.stage = WIFI_ST_CONNECTED;
            }
            if (hs->slot_uid[s] != 0U) {
                cnt++;
                status.players[s].x = hs->players[s].x;
                status.players[s].y = hs->players[s].y;
                status.players[s].spr = hs->players[s].spr;
                status.players[s].flags = hs->players[s].flags;
                status.players[s].active = 1U;
                status.players[s].last_rx_ms = now_ms;
            } else {
                status.players[s].active = 0U;
            }
        }
        status.player_count = cnt;

        if (hs->ev_seq != 0U && hs->ev_seq != last_host_ev_seq && hs->ev_type != NET_EV_NONE) {
            last_host_ev_seq = hs->ev_seq;
            push_ev_queue(hs->ev_type, hs->ev_row, hs->ev_col, hs->ev_val);
        }
    }
}

static void process_at_line(const char *ln)
{
    if (strcmp(ln, "OK") == 0) {
        at_got_ok = 1U;
    } else if (strcmp(ln, "ERROR") == 0 || strcmp(ln, "FAIL") == 0) {
        at_got_err = 1U;
    } else if (strcmp(ln, "ALREADY CONNECTED") == 0) {
        at_got_already = 1U;
    } else if (strcmp(ln, "SEND OK") == 0) {
        at_got_send_ok = 1U;
    } else if (strcmp(ln, "SEND FAIL") == 0) {
        at_got_err = 1U;
    }
}

static void pump_rx_stream(uint32_t now_ms)
{
    while (rx_tail != rx_head) {
        uint8_t b = rx_ring[rx_tail];
        rx_tail = (uint16_t)((rx_tail + 1U) % RX_RING_SIZE);

        if (parse_st == PARSE_IPD_DATA) {
            if (ipd_pos < sizeof(ipd_buf)) {
                ipd_buf[ipd_pos] = b;
            }
            ipd_pos++;
            if (ipd_pos >= ipd_len) {
                if (ipd_len <= sizeof(ipd_buf)) {
                    handle_ipd_packet(ipd_buf, ipd_len, now_ms);
                }
                parse_st = PARSE_LINE;
                line_len = 0U;
                ipd_hdr_match = 0U;
            }
            continue;
        }

        if (parse_st == PARSE_IPD_LEN) {
            if (b >= '0' && b <= '9') {
                ipd_len = (uint16_t)(ipd_len * 10U + (uint16_t)(b - '0'));
                if (ipd_len > 256U) {
                    parse_st = PARSE_LINE;
                    line_len = 0U;
                }
            } else if (b == ':') {
                if (ipd_len > 0U) {
                    ipd_pos = 0U;
                    parse_st = PARSE_IPD_DATA;
                } else {
                    parse_st = PARSE_LINE;
                }
            } else {
                parse_st = PARSE_LINE;
                line_len = 0U;
            }
            continue;
        }

        /* PARSE_LINE */
        if (b == (uint8_t)IPD_HDR[ipd_hdr_match]) {
            ipd_hdr_match++;
            if (ipd_hdr_match == 5U) {
                ipd_hdr_match = 0U;
                ipd_len = 0U;
                line_len = 0U;
                parse_st = PARSE_IPD_LEN;
                continue;
            }
        } else {
            ipd_hdr_match = (b == '+') ? 1U : 0U;
        }

        if (b == '>') {
            at_got_prompt = 1U;
        }

        if (b == '\r' || b == '\n') {
            if (line_len > 0U) {
                line_buf[line_len] = '\0';
                process_at_line(line_buf);
                line_len = 0U;
            }
        } else if (line_len < sizeof(line_buf) - 1U) {
            line_buf[line_len++] = (char)b;
        }
    }
}

static void queue_udp_tx(const uint8_t *pkt, uint16_t len, const char *dst_ip, uint32_t now_ms)
{
    char cmd[64];
    if (len > sizeof(pending_pkt_buf)) {
        return;
    }
    memcpy(pending_pkt_buf, pkt, len);
    pending_pkt_len = len;
    clear_at_flags();
    if (udp_send_use_1arg) {
        snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u\r\n", (unsigned)len);
    } else {
        snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u,\"%s\",%u\r\n",
                 (unsigned)len, dst_ip, (unsigned)WIFI_UDP_PORT);
    }
    usart1_send_str(cmd);
    tx_step = TX_WAIT_PROMPT;
    tx_deadline_ms = now_ms + 80U;
}

static void pump_udp_tx(uint32_t now_ms)
{
    if (tx_step == TX_WAIT_PROMPT) {
        if (at_got_prompt) {
            at_got_prompt = 0U;
            usart1_send_bytes(pending_pkt_buf, pending_pkt_len);
            tx_step = TX_WAIT_SEND_OK;
            tx_deadline_ms = now_ms + 120U;
        } else if (at_got_err) {
            udp_send_use_1arg = 1U;
            tx_step = TX_IDLE;
        } else if ((int32_t)(now_ms - tx_deadline_ms) >= 0) {
            tx_step = TX_IDLE;
        }
        return;
    }

    if (tx_step == TX_WAIT_SEND_OK) {
        if (at_got_send_ok || at_got_err || (int32_t)(now_ms - tx_deadline_ms) >= 0) {
            tx_step = TX_IDLE;
        }
        return;
    }

    /* TX_IDLE: transmit at 25 Hz (every 40ms) */
    if ((now_ms - last_pkt_tx_ms) < 40U) {
        return;
    }
    last_pkt_tx_ms = now_ms;

    if (status.role == WIFI_MODE_HOST) {
        WifiHostSnap hs;
        memset(&hs, 0, sizeof(hs));
        hs.magic = 0x4D48U;
        hs.seq = tx_seq++;
        hs.room_state = host_room_state;
        hs.level = host_level;
        hs.lives = host_lives;
        hs.coins = host_coins;
        hs.score = host_score;

        slot_uids[0] = local_uid;
        status.players[0].x = loc_x;
        status.players[0].y = loc_y;
        status.players[0].spr = loc_spr;
        status.players[0].flags = loc_flags;
        status.players[0].active = 1U;

        uint8_t cnt = 0U;
        for (uint8_t s = 0U; s < WIFI_MAX_PLAYERS; s++) {
            hs.slot_uid[s] = slot_uids[s];
            if (slot_uids[s] != 0U && status.players[s].active) {
                hs.players[s].x = status.players[s].x;
                hs.players[s].y = status.players[s].y;
                hs.players[s].spr = status.players[s].spr;
                hs.players[s].flags = status.players[s].flags;
                cnt++;
            }
        }
        status.player_count = cnt;

        if (out_ev_repeat > 0U) {
            hs.ev_seq = out_ev_seq;
            hs.ev_type = out_ev_type;
            hs.ev_row = out_ev_row;
            hs.ev_col = out_ev_col;
            hs.ev_val = out_ev_val;
            out_ev_repeat--;
        }
        hs.checksum = xor_checksum((const uint8_t *)&hs, (uint16_t)(sizeof(hs) - 1U));
        queue_udp_tx((const uint8_t *)&hs, sizeof(hs), "192.168.4.255", now_ms);
    } else if (status.role == WIFI_MODE_JOIN) {
        WifiClientPkt cp;
        memset(&cp, 0, sizeof(cp));
        cp.magic = 0x4D43U;
        cp.seq = tx_seq++;
        cp.uid = local_uid;
        cp.x = loc_x;
        cp.y = loc_y;
        cp.spr = loc_spr;
        cp.flags = loc_flags;
        if (out_ev_repeat > 0U) {
            cp.ev_seq = out_ev_seq;
            cp.ev_type = out_ev_type;
            cp.ev_row = out_ev_row;
            cp.ev_col = out_ev_col;
            cp.ev_val = out_ev_val;
            out_ev_repeat--;
        }
        cp.checksum = xor_checksum((const uint8_t *)&cp, (uint16_t)(sizeof(cp) - 1U));
        queue_udp_tx((const uint8_t *)&cp, sizeof(cp), "192.168.4.1", now_ms);
    }
}

void WiFi_Net_Start(WifiRole role)
{
    local_uid = compute_uid32();
    memset(&status, 0, sizeof(status));
    memset(slot_uids, 0, sizeof(slot_uids));
    memset(last_client_ev_seq, 0, sizeof(last_client_ev_seq));
    last_host_ev_seq = 0U;
    out_ev_seq = 0U;
    out_ev_repeat = 0U;
    ev_q_head = 0U;
    ev_q_tail = 0U;
    tx_step = TX_IDLE;
    at_wait_ok = 0U;
    at_retry = 0U;
    udp_send_use_1arg = 0U;

    if (role == WIFI_MODE_SOLO) {
        status.role = WIFI_MODE_SOLO;
        status.stage = WIFI_ST_OFF;
        status.my_slot = 0U;
        status.player_count = 1U;
        return;
    }

    if (!uart1_opened) {
        uart_open(uart1, 115200);
        SET_BIT(USART1->CR1, USART_CR1_RXNEIE);
        uart1_opened = 1U;
    }

    rx_head = 0U;
    rx_tail = 0U;
    tx_head = 0U;
    tx_tail = 0U;
    parse_st = PARSE_LINE;
    line_len = 0U;
    ipd_hdr_match = 0U;

    status.role = role;
    status.stage = WIFI_ST_PROBE;
    status.my_slot = (role == WIFI_MODE_HOST) ? 0U : 1U;
    status.player_count = 1U;
    if (role == WIFI_MODE_HOST) {
        slot_uids[0] = local_uid;
        status.players[0].active = 1U;
    }
}

void WiFi_Net_Stop(void)
{
    if (uart1_opened && status.role != WIFI_MODE_SOLO) {
        usart1_send_str("AT+CIPCLOSE\r\n");
    }
    status.role = WIFI_MODE_SOLO;
    status.stage = WIFI_ST_OFF;
    status.my_slot = 0U;
    status.player_count = 1U;
    at_wait_ok = 0U;
    tx_step = TX_IDLE;
}

void WiFi_Net_Tick(uint32_t now_ms)
{
    if (status.role == WIFI_MODE_SOLO) {
        return;
    }

    pump_rx_stream(now_ms);

    /* Timeout disconnected remote peers on Host */
    if (status.role == WIFI_MODE_HOST && (status.stage == WIFI_ST_READY || status.stage == WIFI_ST_CONNECTED)) {
        for (uint8_t s = 1U; s < WIFI_MAX_PLAYERS; s++) {
            if (status.players[s].active && (now_ms - status.players[s].last_rx_ms) > PEER_TIMEOUT_MS) {
                status.players[s].active = 0U;
                slot_uids[s] = 0U;
            }
        }
    }

    if (status.stage == WIFI_ST_READY || status.stage == WIFI_ST_CONNECTED) {
        pump_udp_tx(now_ms);
        return;
    }

    if (status.stage == WIFI_ST_ERROR) {
        return;
    }

    /* Non-blocking AT setup sequence */
    if (!at_wait_ok) {
        char cmd[80];
        switch (status.stage) {
            case WIFI_ST_PROBE:
                send_at_cmd("ATE0\r\n", 400U);
                break;
            case WIFI_ST_CWMODE:
                snprintf(cmd, sizeof(cmd), "AT+CWMODE=%u\r\n",
                         (status.role == WIFI_MODE_HOST) ? 2U : 1U);
                send_at_cmd(cmd, 800U);
                break;
            case WIFI_ST_AP_OR_JOIN:
                if (status.role == WIFI_MODE_HOST) {
                    snprintf(cmd, sizeof(cmd), "AT+CWSAP=\"%s\",\"%s\",6,3\r\n",
                             WIFI_SSID, WIFI_PASS);
                    send_at_cmd(cmd, 2000U);
                } else {
                    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n",
                             WIFI_SSID, WIFI_PASS);
                    send_at_cmd(cmd, 10000U);
                }
                break;
            case WIFI_ST_CIPMUX:
                send_at_cmd("AT+CIPMUX=0\r\n", 600U);
                break;
            case WIFI_ST_UDP_OPEN:
                if (status.role == WIFI_MODE_HOST) {
                    snprintf(cmd, sizeof(cmd),
                             "AT+CIPSTART=\"UDP\",\"192.168.4.255\",%u,%u,2\r\n",
                             (unsigned)WIFI_UDP_PORT, (unsigned)WIFI_UDP_PORT);
                } else {
                    snprintf(cmd, sizeof(cmd),
                             "AT+CIPSTART=\"UDP\",\"192.168.4.1\",%u,%u,0\r\n",
                             (unsigned)WIFI_UDP_PORT, (unsigned)WIFI_UDP_PORT);
                }
                send_at_cmd(cmd, 2000U);
                break;
            default:
                break;
        }
        return;
    }

    /* Check AT reply */
    if (at_got_ok || (status.stage == WIFI_ST_UDP_OPEN && at_got_already)) {
        at_wait_ok = 0U;
        at_retry = 0U;
        if (status.stage == WIFI_ST_PROBE) status.stage = WIFI_ST_CWMODE;
        else if (status.stage == WIFI_ST_CWMODE) status.stage = WIFI_ST_AP_OR_JOIN;
        else if (status.stage == WIFI_ST_AP_OR_JOIN) status.stage = WIFI_ST_CIPMUX;
        else if (status.stage == WIFI_ST_CIPMUX) status.stage = WIFI_ST_UDP_OPEN;
        else if (status.stage == WIFI_ST_UDP_OPEN) {
            status.stage = WIFI_ST_READY;
            last_pkt_tx_ms = 0U;
        }
    } else if (at_got_err || (int32_t)(now_ms - at_deadline_ms) >= 0) {
        at_wait_ok = 0U;
        at_retry++;
        if (status.stage == WIFI_ST_UDP_OPEN && at_retry == 1U) {
            usart1_send_str("AT+CIPCLOSE\r\n");
        } else if (at_retry >= 5U) {
            status.stage = WIFI_ST_ERROR;
        }
    }
}

void WiFi_Net_SetLocalPlayer(int16_t x, int16_t y, uint8_t spr, uint8_t flags)
{
    loc_x = x;
    loc_y = y;
    loc_spr = spr;
    loc_flags = flags;
}

void WiFi_Net_SetHostRoom(uint8_t room_state, uint8_t level, uint8_t lives, uint16_t coins, uint32_t score)
{
    host_room_state = room_state;
    host_level = level;
    host_lives = lives;
    host_coins = coins;
    host_score = score;
}

void WiFi_Net_PushEvent(uint8_t ev_type, uint8_t ev_row, uint8_t ev_col, uint8_t ev_val)
{
    out_ev_seq++;
    if (out_ev_seq == 0U) {
        out_ev_seq = 1U;
    }
    out_ev_type = ev_type;
    out_ev_row = ev_row;
    out_ev_col = ev_col;
    out_ev_val = ev_val;
    out_ev_repeat = 8U;
}

uint8_t WiFi_Net_PopEvent(uint8_t *ev_type, uint8_t *ev_row, uint8_t *ev_col, uint8_t *ev_val)
{
    if (ev_q_tail == ev_q_head) {
        return 0U;
    }
    *ev_type = ev_queue[ev_q_tail].type;
    *ev_row = ev_queue[ev_q_tail].row;
    *ev_col = ev_queue[ev_q_tail].col;
    *ev_val = ev_queue[ev_q_tail].val;
    ev_q_tail = (uint8_t)((ev_q_tail + 1U) % EV_QUEUE_SIZE);
    return 1U;
}

const WifiStatus *WiFi_Net_GetStatus(void)
{
    return &status;
}

const char *WiFi_Net_StageText(void)
{
    switch (status.stage) {
        case WIFI_ST_OFF:        return "OFF";
        case WIFI_ST_PROBE:      return "INIT ESP8285";
        case WIFI_ST_CWMODE:     return "SET WIFI MODE";
        case WIFI_ST_AP_OR_JOIN: return (status.role == WIFI_MODE_HOST) ? "START AP MARIO-MP" : "JOIN MARIO-MP...";
        case WIFI_ST_CIPMUX:     return "SETUP UDP";
        case WIFI_ST_UDP_OPEN:   return "OPEN SOCKET 9000";
        case WIFI_ST_READY:      return (status.role == WIFI_MODE_HOST) ? "HOST READY" : "WAIT HOST ACK...";
        case WIFI_ST_CONNECTED:  return "CONNECTED!";
        case WIFI_ST_ERROR:      return "WIFI ERR / NO ESP";
        default:                 return "UNKNOWN";
    }
}
