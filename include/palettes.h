#pragma once
#include <Arduino.h>
#include "data_models.h"

// 6 Modern Curated Color Palettes (16-bit RGB565)
static const ColorPalette PALETTES[6] = {
    {
        "Cyber Cyan & Pink",
        0x07FF, // primary: electric cyan
        0xF81F, // secondary: electric fuchsia pink
        0x0842, // bg_dark: obsidian night
        0x18C5, // card_bg: semi-transparent slate glass
        0xA555, // text_dim: cool silver gray
        0x07F5  // highlight: pure neon mint
    },
    {
        "Nordic Gold & Teal",
        0xFDC0, // primary: sunset gold
        0x03FF, // secondary: deep royal teal
        0x0826, // bg_dark: midnight velvet
        0x10A8, // card_bg: dark navy glass card
        0x9CD0, // text_dim: warm bronze
        0xFFE0  // highlight: fluorescent amber
    },
    {
        "Tokyo Night Violet",
        0x981F, // primary: neon electric purple
        0xFA6A, // secondary: solar coral pink
        0x0804, // bg_dark: deep cyber abyss
        0x1808, // card_bg: translucent dark purple
        0x7172, // text_dim: muted violet
        0xF814  // highlight: vibrant neon pink
    },
    {
        "Matrix Mint & Emerald",
        0x07E0, // primary: vivid mint emerald
        0x07FF, // secondary: bright cyan aqua
        0x0842, // bg_dark: deep obsidian
        0x10A5, // card_bg: cyber dark card
        0x4D50, // text_dim: phosphor mint
        0xAFE0  // highlight: pure neon lime
    },
    {
        "Monochrome Cyber Ice",
        0xFFFF, // primary: pure ice white
        0xBDF7, // secondary: metallic platinum
        0x1082, // bg_dark: dark carbon
        0x2104, // card_bg: charcoal glass card
        0x7BEF, // text_dim: muted steel
        0xFFFF  // highlight: glowing diamond white
    },
    {
        "Sunset Coral & Lavender",
        0xFC08, // primary: coral orange
        0xB51F, // secondary: soft lavender
        0x0808, // bg_dark: velvet indigo
        0x180A, // card_bg: deep indigo glass
        0x94D3, // text_dim: soft rose
        0xFE30  // highlight: radiant peach
    }
};
