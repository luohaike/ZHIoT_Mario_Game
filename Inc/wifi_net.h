#ifndef __WIFI_NET_H
#define __WIFI_NET_H

#include <stdint.h>

#define WIFI_MAX_PLAYERS 4U

typedef enum {
    WIFI_MODE_SOLO = 0,
    WIFI_MODE_HOST = 1,
    WIFI_MODE_JOIN = 2
} WifiRole;

typedef enum {
    WIFI_ST_OFF = 0,
    WIFI_ST_PROBE,
    WIFI_ST_CWMODE,
    WIFI_ST_AP_OR_JOIN,
    WIFI_ST_CIPMUX,
    WIFI_ST_UDP_OPEN,
    WIFI_ST_READY,      /* AP/STA UDP ready; Host waiting for clients or playing; Client searching host */
    WIFI_ST_CONNECTED,  /* Client assigned slot by Host */
    WIFI_ST_ERROR
} WifiStage;

typedef enum {
    NET_EV_NONE = 0,
    NET_EV_TILE = 1,
    NET_EV_ENEMY_KILL = 2,
    NET_EV_STAGE_CLEAR = 3,
    NET_EV_COIN_SCORE = 4,
    NET_EV_BOSS_AXE = 5,
    NET_EV_WARP = 6
} NetEventType;

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t spr;
    uint8_t flags;      /* bit0: flip_x, bit1: visible, bit2: alive */
    uint8_t active;
    uint32_t last_rx_ms;
} NetPlayerView;

typedef struct {
    WifiRole  role;
    WifiStage stage;
    uint8_t   my_slot;        /* 0 for Host, 0..3 for Client once assigned */
    uint8_t   player_count;   /* active players in room (1..4) */
    uint8_t   remote_room_state;
    uint8_t   remote_level;
    uint8_t   remote_lives;
    uint16_t  remote_coins;
    uint32_t  remote_score;
    uint8_t   rx_ok_count;
    NetPlayerView players[WIFI_MAX_PLAYERS];
} WifiStatus;

void WiFi_Net_Start(WifiRole role);
void WiFi_Net_Stop(void);
void WiFi_Net_Tick(uint32_t now_ms);

void WiFi_Net_SetLocalPlayer(int16_t x, int16_t y, uint8_t spr, uint8_t flags);
void WiFi_Net_SetHostRoom(uint8_t room_state, uint8_t level, uint8_t lives, uint16_t coins, uint32_t score);
void WiFi_Net_PushEvent(uint8_t ev_type, uint8_t ev_row, uint8_t ev_col, uint8_t ev_val);

/* Poll next incoming deduplicated world event (returns 1 if an event was popped, 0 if none) */
uint8_t WiFi_Net_PopEvent(uint8_t *ev_type, uint8_t *ev_row, uint8_t *ev_col, uint8_t *ev_val);

const WifiStatus *WiFi_Net_GetStatus(void);
const char *WiFi_Net_StageText(void);

/* Called by USART1_IRQHandler */
void WiFi_Net_USART1_IRQ(void);

#endif
