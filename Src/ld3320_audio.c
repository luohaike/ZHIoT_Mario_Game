#include "ld3320_audio.h"
#include "stm32l4xx_hal.h"
#include "spi_api.h"

extern SPI_HandleTypeDef hspi3;

/* Board wiring (20020/20021 schematic):
 *  SPI3: PC10=SCK, PC11=MISO, PC12=MOSI
 *  PA12 -> WRB/SPIS (held LOW for SPI mode)
 *  PD0  -> CSB/SCS  (chip select, active LOW)
 *  PD10 -> RSTB     (reset, active LOW)
 *  PB12 -> INTB     (interrupt request, active LOW)
 */
#define LD_CS_LOW()    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET)
#define LD_CS_HIGH()   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET)
#define LD_RST_LOW()   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET)
#define LD_RST_HIGH()  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET)
#define LD_SPIS_LOW()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET)
#define LD_INT_IS_LOW() (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET)

static uint8_t        ld_ready = 0U;
static uint8_t        ld_playing = 0U;
static uint8_t        ld_eof_sent = 0U;
static uint8_t        ld_loop = 0U;
static uint8_t        ld_volume = 15U;
static const uint8_t *ld_data = NULL;
static uint32_t       ld_len = 0U;
static uint32_t       ld_pos = 0U;
static uint32_t       ld_eof_ms = 0U;
static uint32_t       ld_lock_until_ms = 0U;

volatile struct {
    uint8_t  probe_35;
    uint8_t  probe_1b;
    uint8_t  probe_b3;
    uint8_t  reg06_before;
    uint8_t  reg06_after_feed;
    uint8_t  regba_after_start;
    uint8_t  regba_mid;
    uint8_t  reg2b_mid;
    uint8_t  reg8e_read;
    uint8_t  reg85_read;
    uint8_t  intb_saw_low;
    uint8_t  spi_mode_used;
    uint32_t initial_fed;
    uint32_t afull_stops;
    uint32_t play_ms;
    uint32_t start_ms;
} g_ld_diag;

static uint8_t ld_read_reg(uint8_t reg)
{
    uint8_t val;
    LD_CS_LOW();
    (void)spi_ReadWriteByte(spi3, 0x05U);
    (void)spi_ReadWriteByte(spi3, reg);
    val = spi_ReadWriteByte(spi3, 0x00U);
    LD_CS_HIGH();
    return val;
}

static void ld_write_reg(uint8_t reg, uint8_t val)
{
    LD_CS_LOW();
    (void)spi_ReadWriteByte(spi3, 0x04U);
    (void)spi_ReadWriteByte(spi3, reg);
    (void)spi_ReadWriteByte(spi3, val);
    LD_CS_HIGH();
}

static void ld_apply_volume(void)
{
    uint8_t reg8e;
    if (ld_volume == 0U) {
        reg8e = 0xFFU;
    } else {
        uint8_t v = (ld_volume > 15U) ? 15U : ld_volume;
        reg8e = (uint8_t)(((15U - v) << 2) | 0xC3U);
    }
    ld_write_reg(0x8EU, reg8e);
    ld_write_reg(0x8DU, 0xFFU);
    /* Trigger volume/DAC update via 0x87 bit 3 */
    ld_write_reg(0x87U, 0xF7U);
    HAL_Delay(1);
    ld_write_reg(0x87U, 0xFFU);
}

static void ld_init_common(void)
{
    (void)ld_read_reg(0x06U);
    ld_write_reg(0x17U, 0x35U);
    HAL_Delay(10);
    (void)ld_read_reg(0x06U);

    ld_write_reg(0x89U, 0x03U);
    HAL_Delay(5);
    ld_write_reg(0xCFU, 0x43U);
    HAL_Delay(5);
    ld_write_reg(0xCBU, 0x02U);

    /* 24 MHz crystal PLL configuration */
    ld_write_reg(0x11U, 0x0BU);
    ld_write_reg(0x1EU, 0x00U);
    ld_write_reg(0x19U, 0x0FU);
    ld_write_reg(0x1BU, 0x18U);
    ld_write_reg(0x1DU, 0x2CU);
    HAL_Delay(10);

    ld_write_reg(0xCDU, 0x04U);
    ld_write_reg(0x17U, 0x4CU);
    HAL_Delay(5);
    ld_write_reg(0xB9U, 0x00U);
    ld_write_reg(0xCFU, 0x4FU);
    ld_write_reg(0x6FU, 0xFFU);
}

static void ld_init_mp3_mode(uint32_t settle_ms)
{
    ld_init_common();
    ld_write_reg(0x29U, 0x00U);
    ld_write_reg(0x02U, 0x00U);
    ld_write_reg(0xBDU, 0x02U);
    ld_write_reg(0x17U, 0x48U);
    HAL_Delay(settle_ms > 0U ? settle_ms : 2U);

    ld_write_reg(0x85U, 0x52U);
    ld_write_reg(0x8FU, 0x00U);
    ld_write_reg(0x81U, 0x00U);
    ld_write_reg(0x83U, 0x00U);
    ld_write_reg(0x8EU, 0xFFU);
    ld_write_reg(0x8DU, 0xFFU);
    HAL_Delay(1);
    ld_write_reg(0x87U, 0xFFU);
    ld_write_reg(0x89U, 0xFFU);
    HAL_Delay(1);

    /* Configure FIFO_DATA watermarks (0x0000..0x07EF, AFULL=0x0377, AEMPTY=0x01BB) */
    ld_write_reg(0x22U, 0x00U);
    ld_write_reg(0x23U, 0x00U);
    ld_write_reg(0x20U, 0xEFU);
    ld_write_reg(0x21U, 0x07U);
    ld_write_reg(0x24U, 0x77U);
    ld_write_reg(0x25U, 0x03U);
    ld_write_reg(0x26U, 0xBBU);
    ld_write_reg(0x27U, 0x01U);

    ld_apply_volume();
}

static void ld_feed_fifo(uint32_t max_bytes)
{
    uint32_t fed = 0U;
    while (fed < max_bytes) {
        if (ld_pos >= ld_len) {
            if (ld_loop != 0U && ld_len > 0U) {
                ld_pos = 0U;
            } else {
                break;
            }
        }
        if ((ld_read_reg(0x06U) & 0x08U) != 0U) {
            g_ld_diag.afull_stops++;
            break;
        }
        /* Write 1 byte per AFULL check for full safety */
        ld_write_reg(0x01U, ld_data[ld_pos++]);
        fed++;
    }
}

static uint8_t ld_is_mp3_hdr(const uint8_t *d, uint32_t i, uint32_t len)
{
    if (i + 4U > len) return 0U;
    return (uint8_t)(d[i] == 0xFFU && (d[i + 1U] & 0xFEU) == 0xFAU &&
                     (d[i + 2U] & 0xF0U) != 0xF0U && (d[i + 2U] & 0x0CU) == 0x00U &&
                     (d[i + 3U] & 0xC0U) == 0xC0U);
}

static uint32_t ld_find_mp3_sync(const uint8_t *d, uint32_t len, uint32_t start_pos)
{
    if (start_pos == 0U || len < 120U || start_pos + 110U >= len) {
        return 0U;
    }
    for (uint32_t i = start_pos; i + 4U <= len; i++) {
        if (ld_is_mp3_hdr(d, i, len)) {
            /* Verify next CBR frame header (32 kbps: 104/105 B, 48 kbps: 156/157 B) or EOF */
            if (i + 160U >= len ||
                ld_is_mp3_hdr(d, i + 104U, len) || ld_is_mp3_hdr(d, i + 105U, len) ||
                ld_is_mp3_hdr(d, i + 156U, len) || ld_is_mp3_hdr(d, i + 157U, len)) {
                return i;
            }
        }
    }
    return 0U;
}

uint8_t LD3320_Audio_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    LD_SPIS_LOW();

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    LD_CS_HIGH();
    LD_RST_HIGH();

    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_SPI_DeInit(&hspi3);
    spi_open(spi3, SPI_BAUDRATEPRESCALER_64, Rising);
    g_ld_diag.spi_mode_used = 0U;

    /* Hardware reset pulse */
    LD_RST_LOW();
    HAL_Delay(10);
    LD_RST_HIGH();
    HAL_Delay(15);

    /* Wake SPI port */
    LD_CS_LOW();
    HAL_Delay(2);
    LD_CS_HIGH();
    HAL_Delay(2);

    /* Verify multi-register R/W probe before configuring PLL */
    (void)ld_read_reg(0x06U);
    ld_write_reg(0x35U, 0x33U);
    ld_write_reg(0x1BU, 0x55U);
    ld_write_reg(0xB3U, 0xAAU);
    g_ld_diag.probe_35 = ld_read_reg(0x35U);
    g_ld_diag.probe_1b = ld_read_reg(0x1BU);
    g_ld_diag.probe_b3 = ld_read_reg(0xB3U);

    if (g_ld_diag.probe_35 != 0x33U || g_ld_diag.probe_1b != 0x55U || g_ld_diag.probe_b3 != 0xAAU) {
        /* Fallback to SPI Mode 1 (Falling) */
        HAL_SPI_DeInit(&hspi3);
        spi_open(spi3, SPI_BAUDRATEPRESCALER_64, Falling);
        g_ld_diag.spi_mode_used = 1U;
        LD_RST_LOW();
        HAL_Delay(10);
        LD_RST_HIGH();
        HAL_Delay(15);
        LD_CS_LOW();
        HAL_Delay(2);
        LD_CS_HIGH();
        HAL_Delay(2);
        (void)ld_read_reg(0x06U);
        ld_write_reg(0x35U, 0x33U);
        ld_write_reg(0x1BU, 0x55U);
        ld_write_reg(0xB3U, 0xAAU);
        g_ld_diag.probe_35 = ld_read_reg(0x35U);
        g_ld_diag.probe_1b = ld_read_reg(0x1BU);
        g_ld_diag.probe_b3 = ld_read_reg(0xB3U);
    }

    if (g_ld_diag.probe_35 != 0x33U || g_ld_diag.probe_1b != 0x55U || g_ld_diag.probe_b3 != 0xAAU) {
        ld_ready = 0U;
        return 0U;
    }

    ld_init_mp3_mode(10U);
    ld_ready = 1U;
    ld_playing = 0U;
    return 1U;
}

uint8_t LD3320_Audio_IsReady(void)
{
    return ld_ready;
}

uint8_t LD3320_Audio_IsPlaying(void)
{
    return ld_playing;
}

void LD3320_Audio_SetVolume(uint8_t vol_0_to_15)
{
    ld_volume = (vol_0_to_15 > 15U) ? 15U : vol_0_to_15;
    if (ld_ready) {
        ld_apply_volume();
    }
}

uint32_t LD3320_Audio_GetPos(void)
{
    return ld_pos;
}

void LD3320_Audio_Stop(void)
{
    if (!ld_ready) {
        return;
    }
    ld_write_reg(0x29U, 0x00U);
    ld_write_reg(0x02U, 0x00U);
    ld_write_reg(0xBCU, 0x01U);
    ld_write_reg(0x85U, 0x52U);
    ld_playing = 0U;
    ld_eof_sent = 0U;
    ld_loop = 0U;
    ld_lock_until_ms = 0U;
}

static void ld_start_stream(const uint8_t *data, uint32_t len, uint32_t start_pos,
                            uint8_t priority, uint32_t lock_ms, uint8_t loop)
{
    if (!ld_ready || data == NULL || len == 0U) {
        return;
    }
    uint32_t now = HAL_GetTick();
    if (priority == 0U && ld_playing != 0U && now < ld_lock_until_ms) {
        return;
    }

    if (ld_playing != 0U) {
        ld_write_reg(0x29U, 0x00U);
        ld_write_reg(0x02U, 0x00U);
        ld_write_reg(0xBCU, 0x01U);
        HAL_Delay(1);
    }

    ld_data = data;
    ld_len = len;
    ld_pos = ld_find_mp3_sync(data, len, start_pos);
    ld_eof_sent = 0U;
    ld_loop = loop;
    ld_lock_until_ms = (lock_ms > 0U) ? (now + lock_ms) : 0U;

    ld_init_mp3_mode(2U);
    g_ld_diag.reg06_before = ld_read_reg(0x06U);
    ld_feed_fifo(2048U);
    g_ld_diag.initial_fed = ld_pos;
    g_ld_diag.reg06_after_feed = ld_read_reg(0x06U);
    g_ld_diag.start_ms = now;

    ld_write_reg(0xBAU, 0x00U);
    ld_write_reg(0x17U, 0x48U);
    ld_write_reg(0x33U, 0x01U);
    ld_write_reg(0x29U, 0x04U);
    ld_write_reg(0x02U, 0x01U);
    ld_write_reg(0x85U, 0x5AU);

    g_ld_diag.regba_after_start = ld_read_reg(0xBAU);
    g_ld_diag.reg8e_read = ld_read_reg(0x8EU);
    g_ld_diag.reg85_read = ld_read_reg(0x85U);

    if (ld_loop == 0U && ld_pos >= ld_len) {
        ld_write_reg(0xBCU, 0x01U);
        ld_eof_sent = 1U;
        ld_eof_ms = HAL_GetTick();
    }
    ld_playing = 1U;
}

void LD3320_Audio_Start(const uint8_t *data, uint32_t len, uint8_t priority, uint32_t lock_ms)
{
    ld_start_stream(data, len, 0U, priority, lock_ms, 0U);
}

void LD3320_Audio_StartLoop(const uint8_t *data, uint32_t len, uint32_t start_pos)
{
    ld_start_stream(data, len, start_pos, 0U, 0U, 1U);
}

void LD3320_Audio_Tick(void)
{
    if (!ld_ready || !ld_playing) {
        return;
    }

    if (ld_eof_sent) {
        uint8_t st = ld_read_reg(0xBAU);
        if ((st & 0x04U) != 0U || LD_INT_IS_LOW() || (HAL_GetTick() - ld_eof_ms) >= 600U) {
            (void)ld_read_reg(0x2BU);
            ld_write_reg(0x29U, 0x00U);
            ld_write_reg(0x02U, 0x00U);
            ld_playing = 0U;
            ld_eof_sent = 0U;
            g_ld_diag.play_ms = HAL_GetTick() - g_ld_diag.start_ms;
        }
        return;
    }

    if (LD_INT_IS_LOW()) {
        g_ld_diag.intb_saw_low++;
        g_ld_diag.reg2b_mid = ld_read_reg(0x2BU);
        g_ld_diag.regba_mid = ld_read_reg(0xBAU);
        ld_write_reg(0x29U, 0x00U);
        ld_write_reg(0x02U, 0x00U);

        ld_feed_fifo(2048U);

        if (ld_loop == 0U && ld_pos >= ld_len) {
            ld_write_reg(0xBCU, 0x01U);
            ld_eof_sent = 1U;
            ld_eof_ms = HAL_GetTick();
        } else {
            ld_write_reg(0x29U, 0x04U);
            ld_write_reg(0x02U, 0x01U);
        }
    }
}

void LD3320_Audio_PlaySync(const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    if (!ld_ready || data == NULL || len == 0U) {
        return;
    }
    uint32_t start = HAL_GetTick();
    LD3320_Audio_Start(data, len, 1U, 0U);
    while (ld_playing && (HAL_GetTick() - start) < timeout_ms) {
        LD3320_Audio_Tick();
        HAL_Delay(2);
    }
    if (ld_playing) {
        LD3320_Audio_Stop();
    }
}
