#include <gba.h>
#include <maxmod.h>
#include <stdbool.h>

#include "fixed.h"
#include "lut.h"

#include "font_bin.h"
#include "logo_bin.h"
#include "logo_map_bin.h"
#include "logo_pal_bin.h"
#include "background_bin.h"
#include "sprites_bin.h"
#include "sprites_pal_bin.h"

#include "music.h"
#include "music_bin.h"

#define NUMBER_OF_SPRITES   20
#define MAP_SIZE            0x800
#define SCROLLER_ROW        18
#define MOSAIC_EFFECT       1
#define FADE_FRAMES         50

const u8 message[] = {
        "                                " \
        "DRINKING CHEAP BEER, DUSTING OFF OLD " \
        "HARDWARE AND WRITING SCROLLTEXTS. " \
        "WHAT IS THIS?! SOME SORT OF MIDLIFE CRISIS?! " \
        "WE'RE NOT THAT OLD ARE WE?! I GUESS IT BEATS " \
        "BECOMING A MIDDLE MANAGER AND TAKING UP " \
        "TOUR DE FRANCE BIKING AND/OR CROSS-COUNTRY SKIING. " \
        "AND A LOT CHEAPER THAN BUYING A MOTORBIKE I GUESS. " \
        "         " \
        "ANYWAYS... THREE RELEASES IN 2015 SO FAR! " \
        "         " \
        "HUGE THANKS TO GEMINI FOR STEPPING UP AND " \
        "CREATING THE AWESOME MUSIC YOU'RE HEARING. " \
        "SO DO TURN UP THE VOLUME. " \
        "         " \
        "HEY MR. ZM WHAT'S UP?! I WISH I COULD " \
        "ADVERTISE A BBS OF YOURS BUT I GUESS " \
        "THE TIMES THEY ARE A-CHANGIN'. " \
        "THE SAME GOES FOR YOU R\x8fNALD MCD\x8fNALD. " \
        "SEE YOU AT THE PUB." \
        "      " \
        "FUSE! SPECTRE! YOU GUYS ROCK; AS ALWAYS! " \
        "         " \
        "CODE/GFX: VIOLATOR MUSIC: GEMINI " \
};

u16 stretch = 0;
OBJATTR obj_buffer[128];
s16 current_palette[512*3];
s16 fade_deltas[512*3];


void initOAM() {
    u16 ii;
    for (ii = 0; ii < 128; ++ii) {
        obj_buffer[ii].attr0 = OBJ_DISABLE;
    }
}

void updateSpritesPos(u16 theta) {
    u32 s_index;
    s16 x, y, offset;
    s32 sin, cos;
    // Piece of crap, should be cleaned up... makes me sad...
    for (s_index = 0; s_index < NUMBER_OF_SPRITES; ++s_index) {
        // Table has 512 entries, use bitmask for wrapping
        offset = s_index * 10;
        sin = sin_lut[(theta + offset) & 0x1FF];
        // Use same LUT, shift to get cosine
        cos = sin_lut[(theta + offset + 90) & 0x1FF];
        x = fx2int(fxmul(cos, int2fx(SCREEN_WIDTH)));
        y = fx2int(fxmul(sin, int2fx(SCREEN_HEIGHT)));
        // Scale x to [0, 240] and y to [0, 160]
        x = ((x + SCREEN_WIDTH) >> 1);
        y = ((y + SCREEN_HEIGHT) >> 1);
        obj_buffer[s_index].attr0 = OBJ_256_COLOR | ATTR0_SQUARE | OBJ_Y(y);
        obj_buffer[s_index].attr1 = OBJ_SIZE(Sprite_32x32) | OBJ_X(x);
        obj_buffer[s_index].attr2 = OBJ_CHAR((s_index * 32) % 160) | OBJ_SQUARE;
    }
}

inline void copyBufferToOAM(const OBJATTR *buf) {
    OBJATTR *temp_oam_ptr;
    u32 oam_index;
    temp_oam_ptr = OAM;
    for (oam_index = 0; oam_index < 128; ++oam_index) {
        *(OBJATTR *) temp_oam_ptr++ = buf[oam_index];
    }
}

void createBgTilemap(u16 map_base, u32 first_chrs, u32 second_chrs) {
    u16 ii, jj;
    u32 *map_ptr = (u32 *) MAP_BASE_ADR(map_base);
    for (ii = 0; ii < 12; ++ii) {
        for (jj = 0; jj < 16; ++jj) {
            if (ii & 1) {
                *(u32 *) map_ptr++ = first_chrs;
                *(u32 *) map_ptr++ = second_chrs;
            } else {
                *(u32 *) map_ptr++ = second_chrs;
                *(u32 *) map_ptr++ = first_chrs;
            }
        }
    }
}

inline void updateScrollText(u32 m_index) {
    u16 *map_ptr;
    map_ptr = (u16 *) MAP_BASE_ADR(31) + (SCROLLER_ROW * 32);
    u32 ii;
    for (ii = 0; ii < 32; ++ii) {
        if (message[m_index] == 0) {
            m_index = 0;
        }
        *(u16 *) map_ptr++ = message[m_index++];
    }
}

inline void buildFadeDeltas() {
    u16 *pal_bin_ptr = (u16 *) logo_pal_bin;
    s16 *fade_dlts_ptr = fade_deltas;
    s16 color, r, g, b;
    u32 p_index;
    for (p_index = 0; p_index < (logo_pal_bin_size / 2); ++p_index) {
        color = *(u16 *) pal_bin_ptr++;
        r = (color & 0x1f);
        g = (color >> 5 & 0x1f);
        b = (color >> 10 & 0x1f);

        * (s16 *) fade_dlts_ptr++ = (s16) int2fx((s32) r) / FADE_FRAMES;
        * (s16 *) fade_dlts_ptr++ = (s16) int2fx((s32) g) / FADE_FRAMES;
        * (s16 *) fade_dlts_ptr++ = (s16) int2fx((s32) b) / FADE_FRAMES;
    }
}

inline void fade() {
    u16 *pal_ptr = BG_PALETTE;
    s16 *current_pal_ptr = current_palette;
    s16 *fade_dlts_ptr = fade_deltas;
    s16 color, r, g, b;
    u32 p_index;
    for (p_index = 0; p_index < (logo_pal_bin_size / 2); ++p_index) {
        // Add delta and update current palette
        r = *(s16 *) current_pal_ptr;
        *(s16 *) current_pal_ptr++ = r + *(s16 *) fade_dlts_ptr++;
        g = *(s16 *) current_pal_ptr;
        *(s16 *) current_pal_ptr++ = g + *(s16 *) fade_dlts_ptr++;
        b = *(s16 *) current_pal_ptr;
        *(s16 *) current_pal_ptr++ = b + *(s16 *) fade_dlts_ptr++;

        // Write color out
        color = (fx2int((s32) r) | (fx2int((s32) g) << 5) | (fx2int((s32) b) << 10));
        *(u16 *) pal_ptr++ = color;
    }
}

mm_word songEventHandler(mm_word msg, mm_word param) {
    switch (msg) {
        case MMCB_SONGMESSAGE:
            switch (param) {
                case MOSAIC_EFFECT:
                    stretch = 15;
                    break;
            }
            break;
        case MMCB_SONGFINISHED:
            break;
            // A song has finished playing
    }
    return msg;
}

int main(void) {
    irqInit();
    irqSet(IRQ_VBLANK, mmVBlank);
    irqEnable(IRQ_VBLANK);

    //CpuFastSet(logo_pal_bin, BG_PALETTE, (logo_pal_bin_size / 4) | COPY32);
    CpuFastSet(sprites_pal_bin, SPRITE_PALETTE, (sprites_pal_bin_size / 4) | COPY32);

    CpuFastSet(font_bin, (u16 *) VRAM, (font_bin_size / 4) | COPY32);
    CpuFastSet(sprites_bin, SPRITE_GFX, (sprites_bin_size / 4) | COPY32);
    CpuFastSet(logo_bin, (u16 *) TILE_BASE_ADR(1), (logo_bin_size / 4) | COPY32);
    CpuFastSet(background_bin, (u16 *) TILE_BASE_ADR(2), (background_bin_size / 4) | COPY32);

    //Load tilemap for logo (30x20)
    u16 ii, jj;
    u16 *map_ptr = (u16 *) MAP_BASE_ADR(30), *data_ptr = (u16 *) logo_map_bin;
    for (ii = 0; ii < 20; ++ii) {
        for (jj = 0; jj < 30; ++jj) {
            *(u16 *) map_ptr++ = *(u16 *) data_ptr++;
        }
        map_ptr += 2;
    }

    // Generate checkerboard tilemaps for background
    createBgTilemap((u16) 29, 0x00000000, 0x00020002);
    createBgTilemap((u16) 28, 0x10011001, 0x10021002);

    // Clear font tilemap using tile 0x20 (space)
    *((u32 *) MAP_BASE_ADR(31)) = 0x00200020;
    CpuFastSet(MAP_BASE_ADR(31), MAP_BASE_ADR(31), FILL | COPY32 | (MAP_SIZE / 4));

    // Disable all sprites
    initOAM();

    // Init scroll register
    BG_OFFSET[0].x = 0;
    BG_OFFSET[0].y = 0;
    BG_OFFSET[2].y = 0;
    BG_OFFSET[2].y = 0;
    BG_OFFSET[3].x = 8;
    BG_OFFSET[3].y = 8;

    // Set screen- and tile base
    BGCTRL[0] = BG_MAP_BASE(31) | TILE_BASE(0);
    BGCTRL[1] = BG_MAP_BASE(30) | TILE_BASE(1) | BG_MOSAIC;
    BGCTRL[2] = BG_MAP_BASE(29) | TILE_BASE(2);
    BGCTRL[3] = BG_MAP_BASE(28) | TILE_BASE(2);

    // Start module playback
    mmInitDefault((mm_addr) music_bin, 6);
    mmSetEventHandler(songEventHandler);
    mmSetModuleVolume(1024);
    mmStart(MOD_GMN_MYTL, MM_PLAY_LOOP);

    // Set graphics mode
    SetMode(MODE_0 | BG0_ON | BG1_ON | BG2_ON | BG3_ON | OBJ_ON | OBJ_1D_MAP);

    u16 frame_count = 0, x_scroll = 0, x_bg2 = 0, x_bg3 = 0, theta = 45;
    u32 m_index = 0, fade_count = 0;

    buildFadeDeltas();

    while (true) {
        VBlankIntrWait();
        mmFrame();

        if (fade_count < FADE_FRAMES) {
            fade();
            ++fade_count;
        }

        updateSpritesPos(theta);
        --theta;

        if (stretch > 0) {
            --stretch;
        }
        // Set horizontal and vertical stretch
        REG_MOSAIC = (stretch << 4) | stretch;

        copyBufferToOAM(obj_buffer);
        if (frame_count & 1) {
            x_scroll &= 0x7;  // Wrap on tile width - 1
            if (!x_scroll) {
                if (message[m_index] == 0) {
                    m_index = 0;
                } else {
                    ++m_index;
                }
                updateScrollText(m_index);
            }
            BG_OFFSET[0].x = x_scroll++;
            BG_OFFSET[2].x = x_bg2--;
        }
        else if (frame_count & 3) {
            BG_OFFSET[3].x = x_bg3--;
        }
        frame_count++;
    }
}
