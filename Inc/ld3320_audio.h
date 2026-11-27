#ifndef __LD3320_AUDIO_H
#define __LD3320_AUDIO_H

#include <stdint.h>

uint8_t LD3320_Audio_Init(void);
uint8_t LD3320_Audio_IsReady(void);
uint8_t LD3320_Audio_IsPlaying(void);

/* priority: 0 = regular SFX (won't cut off locked voiceover), 1 = high priority / voiceover */
void LD3320_Audio_Start(const uint8_t *data, uint32_t len, uint8_t priority, uint32_t lock_ms);
void LD3320_Audio_StartLoop(const uint8_t *data, uint32_t len, uint32_t start_pos);
uint32_t LD3320_Audio_GetPos(void);
void LD3320_Audio_PlaySync(const uint8_t *data, uint32_t len, uint32_t timeout_ms);
void LD3320_Audio_Stop(void);
void LD3320_Audio_Tick(void);
void LD3320_Audio_SetVolume(uint8_t vol_0_to_15);

#endif
