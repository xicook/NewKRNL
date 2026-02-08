#include "rtc.h"
#include <stdint.h>

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

static uint8_t read_cmos(uint8_t reg) {
    asm volatile ("outb %0, %1" : : "a"(reg), "dN"(CMOS_ADDR));
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "dN"(CMOS_DATA));
    return ret;
}

static int is_update_in_progress() {
    return read_cmos(0x0A) & 0x80;
}

static uint8_t bcd_to_bin(uint8_t bcd) {
    return ((bcd / 16) * 10) + (bcd & 0x0F);
}

void rtc_get_time(RTCTime* time) {
    while (is_update_in_progress());

    uint8_t second = read_cmos(0x00);
    uint8_t minute = read_cmos(0x02);
    uint8_t hour   = read_cmos(0x04);
    uint8_t day    = read_cmos(0x07);
    uint8_t month  = read_cmos(0x08);
    uint8_t year   = read_cmos(0x09);
    uint8_t registerB = read_cmos(0x0B);

    // Convert BCD to binary if necessary
    if (!(registerB & 0x04)) {
        second = bcd_to_bin(second);
        minute = bcd_to_bin(minute);
        hour   = bcd_to_bin(hour);
        day    = bcd_to_bin(day);
        month  = bcd_to_bin(month);
        year   = bcd_to_bin(year);
    }

    // Convert 12 hour clock to 24 hour clock if necessary
    if (!(registerB & 0x02) && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }

    time->second = second;
    time->minute = minute;
    time->hour   = hour;
    time->day    = day;
    time->month  = month;
    time->year   = 2000 + year; // Simplified
}
