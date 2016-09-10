#include <gba.h>
#include <maxmod.h>

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

#define NUMBER_OF_SPRITES       21 * 2
#define MAP_SIZE                0x800
#define SCROLLER_ROW            18
#define FADE_FRAMES             (2 << 5)
#define HIHAT_SYNC              1
#define PATTERN_CHANGE_SYNC     2
#define SPRITE_WIDTH            32

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

u16 sync_offset = 0;
OBJATTR obj_buffer[128];
u16 current_palette[512 * 3];
u16 fade_deltas[512 * 3];

void initOAM() {
    u32 s_index, c_index = 0, palbank = 0;
    for (s_index = 0; s_index < NUMBER_OF_SPRITES; s_index += 2) {
        // Pyramid
        obj_buffer[s_index].attr0 = OBJ_DISABLE;
        obj_buffer[s_index].attr2 = OBJ_CHAR(c_index) | ATTR2_PALETTE(palbank) | OBJ_SQUARE;
        // Shadow
        obj_buffer[s_index + 1].attr2 = OBJ_CHAR(48) | ATTR2_PALETTE(0) | OBJ_SQUARE;

        if (c_index < 32) {
            c_index += 16;
        } else {
            c_index = 0;
            palbank = (palbank + 1) & 0xf;
        }
    }
}

inline void updateSpritesPos(u32 theta, u32 theta_p) {
    u32 s_index;
    s16 x, y, offset;
    s32 sin, cos;
    for (s_index = 0; s_index < NUMBER_OF_SPRITES; s_index += 2) {
        offset = (s16) (s_index * 5);
        // Table has 512 entries, use bitmask for wrapping
        // Shift to get cosine
        sin = sin_lut[(theta + offset) & 0x1ff];
        cos = sin_lut[(theta_p + offset + 90) & 0x1ff];

        // Ranges are:
        // [-208, 208] & [-128, 128]
        x = (s16) fx2int(fxmul(cos, int2fx(SCREEN_WIDTH - SPRITE_WIDTH)));
        y = (s16) fx2int(fxmul(sin, int2fx(SCREEN_HEIGHT - SPRITE_WIDTH)));

        // Translate and scale so we get the following ranges:
        // [0, 208] & [0, 128]
        x = (s16) ((x + SCREEN_WIDTH - SPRITE_WIDTH) >> 1);
        y = (s16) ((y + SCREEN_HEIGHT - SPRITE_WIDTH) >> 1);

        // Pyramid
        obj_buffer[s_index].attr0 = OBJ_16_COLOR | ATTR0_SQUARE | OBJ_Y(y);
        obj_buffer[s_index].attr1 = OBJ_SIZE(Sprite_32x32) | OBJ_X(x);
        // Shadow
        obj_buffer[s_index + 1].attr0 = OBJ_Y(y + 5) | OBJ_16_COLOR | ATTR0_SQUARE;
        obj_buffer[s_index + 1].attr1 = OBJ_SIZE(Sprite_32x32) | OBJ_X(x + 3);
    }
}

inline void copyBufferToOAM(const OBJATTR buf[]) {
    u32 oam_index;
    for (oam_index = 0; oam_index < 128; ++oam_index) {
        OAM[oam_index] = buf[oam_index];
    }
}

void createBgTilemap(u16 map_base, u32 first_chrs, u32 second_chrs) {
    u16 ii, jj;
    u32 *map_ptr = (u32 *) MAP_BASE_ADR(map_base);
    for (ii = 0; ii < 12; ++ii) {
        for (jj = 0; jj < 16; ++jj) {
            if (ii & 1) {
                *map_ptr++ = first_chrs;
                *map_ptr++ = second_chrs;
            } else {
                *map_ptr++ = second_chrs;
                *map_ptr++ = first_chrs;
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
        *map_ptr++ = message[m_index++];
    }
}

inline void buildFadeDeltas(u16 pal_bin[], u16 fade_deltas[], u32 pal_size) {
    u16 color, r, g, b;
    u32 p_index;
    for (p_index = 0; p_index < (pal_size / 2); ++p_index) {
        color = *pal_bin++;
        r = (u16) (color & 0x1f);
        g = (u16) (color >> 5 & 0x1f);
        b = (u16) (color >> 10 & 0x1f);

        *fade_deltas++ = (u16) (int2fx((s32) r) / FADE_FRAMES);
        *fade_deltas++ = (u16) (int2fx((s32) g) / FADE_FRAMES);
        *fade_deltas++ = (u16) (int2fx((s32) b) / FADE_FRAMES);
    }
}

inline void fade(u16 current_pal[], u16 fade_deltas[], u32 pal_size) {
    u16 color, r, g, b;
    u32 p_index;
    for (p_index = 0; p_index < (pal_size / 2); ++p_index) {
        // Add delta and update current palette
        r = *current_pal;
        *current_pal++ = r + *fade_deltas++;
        g = *current_pal;
        *current_pal++ = g + *fade_deltas++;
        b = *current_pal;
        *current_pal++ = b + *fade_deltas++;

        // Write color out
        color = (u16) (fx2uint((s32) r) | (fx2uint((s32) g) << 5) | (fx2uint((s32) b) << 10));
        BG_PALETTE[p_index] = color;
    }
}

mm_word songEventHandler(mm_word msg, mm_word param) {
    switch (msg) {
        case MMCB_SONGMESSAGE:
            switch (param) {
                case HIHAT_SYNC:
                    sync_offset = 15;
                    break;
                case PATTERN_CHANGE_SYNC:
                    break;
                default:
                    break;
            }
            break;
        case MMCB_SONGFINISHED:
            break;
        default:
            break;
    }
    return msg;
}

int main(void) {
    irqInit();
    irqSet(IRQ_VBLANK, mmVBlank);
    irqEnable(IRQ_VBLANK);

    //Load gfx data
    LZ77UnCompVram(&sprites_pal_bin, SPRITE_PALETTE);
    LZ77UnCompVram(&font_bin, (void *) VRAM);
    LZ77UnCompVram(&sprites_bin, SPRITE_GFX);
    LZ77UnCompVram(&logo_bin, TILE_BASE_ADR(1));
    RLUnCompVram(&background_bin, TILE_BASE_ADR(2));

    //Load tilemap for logo (30x20)
    u16 ii, jj;
    u16 *map_ptr = (u16 *) MAP_BASE_ADR(30), *data_ptr = (u16 *) logo_map_bin;
    for (ii = 0; ii < 20; ++ii) {
        for (jj = 0; jj < 30; ++jj) {
            *map_ptr++ = *data_ptr++;
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

    u32 m_index = 0, frame_count = 0, fade_count = 0;
    u16 x_scroll = 0, x_bg2 = 0, x_bg3 = 0;
    u32 theta = 45, theta_p = 45;

    buildFadeDeltas((u16 *) logo_pal_bin, fade_deltas, logo_pal_bin_size);

    while (true) {
        VBlankIntrWait();
        mmFrame();

        if (fade_count < FADE_FRAMES) {
            fade(current_palette, fade_deltas, logo_pal_bin_size);
            ++fade_count;
        }

        updateSpritesPos(theta, theta_p);
        --theta;
        theta_p -= 3;

        if (sync_offset > 0) {
            --sync_offset;
        }
        // Set horizontal and vertical sync_offset
        REG_MOSAIC = (sync_offset << 4) | sync_offset;

        copyBufferToOAM((OBJATTR *) obj_buffer);
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
            BG_OFFSET[0].y = sync_offset;
            BG_OFFSET[2].x = x_bg2--;
        }
        else if (frame_count & 3) {
            BG_OFFSET[3].x = x_bg3--;
        }
        frame_count++;
    }
}
