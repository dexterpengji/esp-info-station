#pragma once
#include <Arduino.h>
#include "data_models.h"

// 6 Curated Vibrant Color Palettes (RGB565 format)
// Palette 0 is custom-matched for the Red (front) & Purple (back) 3D-printed shell!
static const ColorPalette PALETTES[6] = {
    {
        "Red/Purple Cyber",
        0xF9A0, // primary: vivid crimson red
        0x981F, // secondary: electric violet purple
        0x0802, // bg_dark: midnight velvet purple-black
        0x2004, // card_bg: deep crimson-purple card
        0xB333, // text_dim: rose tinted dim text
        0xFA0A  // highlight: intense neon fuchsia pink
    },
    {
        "Hot Magenta",
        0xF814, // primary: neon hot pink
        0xFCB2, // secondary: coral peach
        0x1002, // bg_dark: dark abyss
        0x2005, // card_bg: crimson tint card
        0x6186, // text_dim: dim rose
        0xFF77  // highlight: intense fluorescent pink
    },
    {
        "Cyber Cyan",
        0x07FF, // primary: electric cyan
        0x05FD, // secondary: bright sky blue
        0x0004, // bg_dark: deep midnight black-blue
        0x0108, // card_bg: dark cyber card
        0x4354, // text_dim: muted steel cyan
        0x87FF  // highlight: pure glowing neon cyan
    },
    {
        "Matrix Emerald",
        0x07E0, // primary: vivid neon green
        0x5FEA, // secondary: mint green
        0x0080, // bg_dark: pitch dark forest
        0x01E2, // card_bg: cyber matrix dark card
        0x23E4, // text_dim: dim phosphor green
        0x87F2  // highlight: intense neon lime
    },
    {
        "Solar Amber",
        0xFDE0, // primary: bright golden yellow
        0xFA60, // secondary: solar orange
        0x1000, // bg_dark: dark volcanic obsidian
        0x2080, // card_bg: warm dark card
        0x6320, // text_dim: dim golden bronze
        0xFFE0  // highlight: radiant white-gold
    },
    {
        "Glacier Silver",
        0xDEFB, // primary: frosty ice white
        0x9E7F, // secondary: crisp glacier blue
        0x0882, // bg_dark: deep arctic slate
        0x10E5, // card_bg: frosted glass dark card
        0x5AEC, // text_dim: cool metallic grey
        0xFFFF  // highlight: pure crystalline white
    }
};
