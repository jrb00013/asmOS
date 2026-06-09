/*
 * VGA mode 13h (320x200, 256 colors) and text mode 0x03.
 * Used only when not building for PS2 (no PS2_HARDWARE).
 */
#include "video.h"
#include "arch_x86.h"

#ifndef PS2_HARDWARE

/* Write to indexed VGA register: port = index, port+1 = data */
static void vga_write_indexed(uint16_t port, uint8_t index, uint8_t value) {
    outb(port, index);
    outb(port + 1, value);
}

/* Attribute controller: reset flip-flop by reading 0x3DA, then index then data at 0x3C0 */
static void vga_attr_write(uint8_t index, uint8_t value) {
    (void)inb(0x3DA);
    outb(0x3C0, index);
    outb(0x3C0, value);
}

void video_set_mode_13h(void) {
    /* Miscellaneous output */
    outb(0x3C2, 0x63);

    /* Sequencer */
    vga_write_indexed(0x3C4, 0x00, 0x03);
    vga_write_indexed(0x3C4, 0x01, 0x01);
    vga_write_indexed(0x3C4, 0x02, 0x0F);
    vga_write_indexed(0x3C4, 0x03, 0x00);
    vga_write_indexed(0x3C4, 0x04, 0x0E);

    /* CRTC: unlock registers 0-7 by clearing bit 7 of register 0x11 */
    outb(0x3D4, 0x11);
    {
        uint8_t v = inb(0x3D5) & 0x7F;
        vga_write_indexed(0x3D4, 0x11, v);
    }
    vga_write_indexed(0x3D4, 0x00, 0x5F);
    vga_write_indexed(0x3D4, 0x01, 0x4F);
    vga_write_indexed(0x3D4, 0x02, 0x50);
    vga_write_indexed(0x3D4, 0x03, 0x82);
    vga_write_indexed(0x3D4, 0x04, 0x54);
    vga_write_indexed(0x3D4, 0x05, 0x80);
    vga_write_indexed(0x3D4, 0x06, 0xBF);
    vga_write_indexed(0x3D4, 0x07, 0x1F);
    vga_write_indexed(0x3D4, 0x08, 0x00);
    vga_write_indexed(0x3D4, 0x09, 0x41);
    vga_write_indexed(0x3D4, 0x10, 0x00);
    vga_write_indexed(0x3D4, 0x11, 0x40);
    vga_write_indexed(0x3D4, 0x12, 0x00);
    vga_write_indexed(0x3D4, 0x13, 0x00);
    vga_write_indexed(0x3D4, 0x14, 0x00);
    vga_write_indexed(0x3D4, 0x15, 0x00);
    vga_write_indexed(0x3D4, 0x16, 0x00);
    vga_write_indexed(0x3D4, 0x17, 0x00);

    /* Graphics controller */
    vga_write_indexed(0x3CE, 0x00, 0x00);
    vga_write_indexed(0x3CE, 0x01, 0x00);
    vga_write_indexed(0x3CE, 0x02, 0x00);
    vga_write_indexed(0x3CE, 0x03, 0x00);
    vga_write_indexed(0x3CE, 0x04, 0x00);
    vga_write_indexed(0x3CE, 0x05, 0x40);
    vga_write_indexed(0x3CE, 0x06, 0x05);
    vga_write_indexed(0x3CE, 0x07, 0x0F);
    vga_write_indexed(0x3CE, 0x08, 0xFF);

    /* Attribute: palette 0..15 = 0..15, then enable display (index 0x20 bit 5) */
    for (uint8_t i = 0; i < 16; i++)
        vga_attr_write(i, i);
    vga_attr_write(0x10, 0x41);
    (void)inb(0x3DA);
    outb(0x3C0, 0x20);  /* enable attribute output */
}

void video_set_mode_text(void) {
    /* Standard VGA text mode 0x03: use BIOS would be ideal; we approximate
     * by re-programming for 80x25 text. Simplified: just set misc and
     * key CRTC values for mode 3. */
    outb(0x3C2, 0x67);  /* misc for 80x25 */
    vga_write_indexed(0x3C4, 0x00, 0x03);
    vga_write_indexed(0x3C4, 0x01, 0x00);
    vga_write_indexed(0x3C4, 0x02, 0x03);
    vga_write_indexed(0x3C4, 0x03, 0x00);
    vga_write_indexed(0x3C4, 0x04, 0x02);

    vga_write_indexed(0x3D4, 0x11, 0x0E);  /* unlock */
    vga_write_indexed(0x3D4, 0x00, 0x5F);
    vga_write_indexed(0x3D4, 0x01, 0x4F);
    vga_write_indexed(0x3D4, 0x02, 0x50);
    vga_write_indexed(0x3D4, 0x03, 0x82);
    vga_write_indexed(0x3D4, 0x04, 0x55);
    vga_write_indexed(0x3D4, 0x05, 0x81);
    vga_write_indexed(0x3D4, 0x06, 0xBF);
    vga_write_indexed(0x3D4, 0x07, 0x1F);
    vga_write_indexed(0x3D4, 0x08, 0x00);
    vga_write_indexed(0x3D4, 0x09, 0x40);
    vga_write_indexed(0x3D4, 0x10, 0x00);
    vga_write_indexed(0x3D4, 0x11, 0x8E);
    vga_write_indexed(0x3D4, 0x12, 0x00);
    vga_write_indexed(0x3D4, 0x13, 0x00);
    vga_write_indexed(0x3D4, 0x14, 0x00);
    vga_write_indexed(0x3D4, 0x15, 0x00);
    vga_write_indexed(0x3D4, 0x16, 0x00);
    vga_write_indexed(0x3D4, 0x17, 0x00);

    vga_write_indexed(0x3CE, 0x00, 0x00);
    vga_write_indexed(0x3CE, 0x01, 0x00);
    vga_write_indexed(0x3CE, 0x02, 0x00);
    vga_write_indexed(0x3CE, 0x03, 0x00);
    vga_write_indexed(0x3CE, 0x04, 0x00);
    vga_write_indexed(0x3CE, 0x05, 0x10);
    vga_write_indexed(0x3CE, 0x06, 0x0E);
    vga_write_indexed(0x3CE, 0x07, 0x00);
    vga_write_indexed(0x3CE, 0x08, 0xFF);

    for (uint8_t i = 0; i < 16; i++)
        vga_attr_write(i, i);
    vga_attr_write(0x10, 0x0C);
    (void)inb(0x3DA);
    outb(0x3C0, 0x20);
}

void video_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(0x3C8, index);
    outb(0x3C9, r);
    outb(0x3C9, g);
    outb(0x3C9, b);
}

#else

void video_set_mode_13h(void) { (void)0; }
void video_set_mode_text(void) { (void)0; }
void video_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    (void)index; (void)r; (void)g; (void)b;
}

#endif
