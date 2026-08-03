#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/keycodes.h>
#include <gint/rtc.h>
#include <stdint.h>
#include <vector>

// Lookup lists mapped directly to read-only target Flash space
const char *WEEKDAY_NAMES[] = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                               "Thursday", "Friday", "Saturday"};
const char *MONTH_NAMES[] = {"January",   "February", "March",    "April",
                             "May",       "June",     "July",     "August",
                             "September", "October",  "November", "December"};

// Global static text scratchpad to bypass dynamic heap boundaries.
// Allocated as individual, isolated buffers to prevent overlapping string
// states.
static char s_year_buf[5];
static char s_day_buf[3];
static char s_hour_buf[3];
static char s_min_buf[3];
static char s_sec_buf[3];

// Convert a 0-99 value into a two-character ASCII string using SH-4A inline
// assembly for the digit split.
inline const char *u8_to_ascii_2dig(uint8_t value, char *dest) {
  uint32_t tens;
  uint32_t ones;
  uint32_t magic = 0x1999999A;

  __asm__ volatile("dmulu.l %2, %3\n\t"
                   "sts mach, %0\n\t"
                   "mov %0, r1\n\t"
                   "shll2 r1\n\t"
                   "add %0, r1\n\t"
                   "shll r1\n\t"
                   "mov %2, %1\n\t"
                   "sub r1, %1\n\t"
                   : "=&r"(tens), "=&r"(ones)
                   : "r"((uint32_t)value), "r"(magic)
                   : "r1", "mach", "macl");

  dest[0] = static_cast<char>(tens + '0');
  dest[1] = static_cast<char>(ones + '0');
  dest[2] = '\0';
  return dest;
}

// Convert a 0-9999 value into a four-character ASCII string.
inline const char *u16_to_ascii_4dig(uint16_t value, char *dest) {
  uint32_t high_2;
  uint32_t low_2;
  uint32_t magic_100 = 0x028F5C29;

  __asm__ volatile("dmulu.l %2, %3\n\t"
                   "sts mach, %0\n\t"
                   "mov %0, r1\n\t"
                   "shll2 r1\n\t"
                   "add %0, r1\n\t"
                   "shll2 r1\n\t"
                   "add %0, r1\n\t"
                   "shll2 r1\n\t"
                   "mov %0, r2\n\t"
                   "shll2 r2\n\t"
                   "shll2 r2\n\t"
                   "add r2, r1\n\t"
                   "mov %2, %1\n\t"
                   "sub r1, %1\n\t"
                   : "=&r"(high_2), "=&r"(low_2)
                   : "r"((uint32_t)value), "r"(magic_100)
                   : "r1", "r2", "mach", "macl");

  u8_to_ascii_2dig(static_cast<uint8_t>(high_2), dest);
  u8_to_ascii_2dig(static_cast<uint8_t>(low_2), dest + 2);
  return dest;
}

// Pass your struct instance by reference directly into the function
std::vector<const char *> convert_rtc_to_vector(const rtc_time_t &time) {
  std::vector<const char *> result;

  // Limits calculator system heap expansion cycles completely
  result.reserve(7);

  // Directly read from the struct elements into distinct string cache pools
  result.push_back(u16_to_ascii_4dig(time.year, s_year_buf));
  result.push_back((time.week_day < 7) ? WEEKDAY_NAMES[time.week_day]
                                       : "Unknown");
  result.push_back((time.month < 12) ? MONTH_NAMES[time.month] : "Unknown");
  result.push_back(u8_to_ascii_2dig(time.month_day, s_day_buf));
  result.push_back(u8_to_ascii_2dig(time.hours, s_hour_buf));
  result.push_back(u8_to_ascii_2dig(time.minutes, s_min_buf));
  result.push_back(u8_to_ascii_2dig(time.seconds, s_sec_buf));

  return result;
}

int main(void) {
  rtc_time_t TIME;
  bool running = true;

  dclear(C_WHITE);
  dtext(0, 0, C_BLACK, "HH3-Clock, Clock for Hollyhock-3");
  dupdate();

  std::vector<const char *> time_vector;

  while (running) {
    rtc_get_time(&TIME);
    time_vector = convert_rtc_to_vector(TIME);

    dclear(C_WHITE); // Clear the frame buffer on every loop to handle value
                     // updates
    dtext(0, 0, C_BLACK, "HH3-Clock, Clock for Hollyhock-3");

    dprint(0, 20, C_BLACK, "Year: %s", time_vector[0]);
    dprint(0, 40, C_BLACK, "Month: %s", time_vector[2]);
    dprint(0, 60, C_BLACK, "Month Date: %s", time_vector[3]);
    dprint(0, 80, C_BLACK, "Hour: %s", time_vector[4]);
    dprint(0, 100, C_BLACK, "Minute: %s",
           time_vector[5]); // Adjusted Y coordinate to 100 (was overlapping 80)
    dprint(0, 120, C_BLACK, "Second: %s",
           time_vector[6]); // Adjusted Y coordinate to 120
    dprint(0, 140, C_BLACK, "Weekday: %s", time_vector[1]);
    dupdate();

    if (keydown(KEY_CLEAR) != 0 || keydown(KEY_EXE) != 0) {
      return 0;
    }
  }
  return 1;
}
