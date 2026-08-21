#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/keycodes.h>
#include <gint/rtc.h>
#include <stdint.h>
#include <vector> // Used in the ASM portions!
#include <hh3-clock/theme.hpp>
#include <cstring>
#include <stdexcept>
#include <cstdlib>

extern "C" char *getenv(const char *name) {
    (void)name; // Suppress unused parameter warning
    return nullptr;
}


//NOLINTBEGIN
HHK_AUTHOR("HopedCrown")
HHK_NAME("HH3-Clock")
HHK_VERSION("0.0.1-alpha.1")
HHK_DESCRIPTION("A Basic RTC-Based Clock via gint ('guin')")
//NOLINTEND

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

THEME UI = W_B_Theme;

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
extern font_t font;
void draw_tab(const char *tab, std::vector<const char *>t_vctr) {
  drect(0,10,320,395,UI.bg);
  dupdate();
  // placeholder
  if (strcmp(tab,"DIGITAL") == 0) {
    dfont(&font);
    // Draw the digital clock, so HH:MM:SS -> need a larger font size, so like 16x18? or 24x27? -> 16x27
    //                         WD, YYYY:MM:DD
    dprint(10,12,UI.fg,"%s:%s:%s", t_vctr[4],t_vctr[5],t_vctr[6]);
    dupdate();
  } else if(strcmp(tab,"ANALOG") == 0) {
    // Draw the Analog Clock
  } else if(strcmp(tab,"SETTINGS") == 0) {
    // Draw settings
  } else if(strcmp(tab,"ABOUT") == 0) {
    // Draw the About page
  } else {
    // syntax error
    throw std::runtime_error("Syntax Error for call to Function 'draw_tab'");
  }
}

int main(void) {
  rtc_time_t TIME;
  bool running = true;
  const char *drawTab = "DIGITAL";
  const font_t orig_font = dfont(&font);
  // This reminds me:
  // TODO: Add Ending Screen
  //draw_tab(drawTab); // get in the usage to stop some compiler errors 
  dtext(1,290,UI.fg,"TEST");
  dfont(&orig_font);
  dclear(C_WHITE); // -> for clearing beforehand draw_tab will be replacing it for now...
  dtext(0, 0, C_BLACK, "    HH3-Clock, Clock for Hollyhock-3");
  dupdate();

  // Help section, bottom half of screen...
  dhline(396,C_BLACK);
  dhline(397,C_BLACK);
  dhline(398,C_BLACK);
  dupdate();
  dtext(1,400,UI.helpTitleColor,"       Help Section - v0.0.1-alpha.1 ");
  dtext(1,410,UI.helpTextColor,"             [1] Digital Clock");
  dtext(1,420,UI.helpTextColor,"             [2] Analog Clock");
  dtext(1,430,UI.helpTextColor,"             [3] Settings");
  dtext(1,440,UI.helpTextColor,"             [CLEAR] Exit App");
  dtext(1,450,UI.helpTextColor,"             [<-] About");
  dupdate();
  std::vector<const char *> time_vector;
  key_event_t input;
  while (running) {
    rtc_get_time(&TIME);
    time_vector = convert_rtc_to_vector(TIME);
    // Clear the frame buffer on every loop to handle value updates
    drect(0,10,320,122,C_WHITE);
    dprint(0, 20, C_BLACK, "Year: %s", time_vector[0]);
    dprint(0, 30, C_BLACK, "Month: %s", time_vector[2]);
    dprint(0, 40, C_BLACK, "Month Date: %s", time_vector[3]);
    dprint(0, 50, C_BLACK, "Hour: %s", time_vector[4]);
    dprint(0, 60, C_BLACK, "Minute: %s",
           time_vector[5]); 
    dprint(0, 70, C_BLACK, "Second: %s",
           time_vector[6]); 
    dprint(0, 80, C_BLACK, "Weekday: %s", time_vector[1]);
    dtext(0, 100, C_BLACK,"Complete Date:");
    dprint(0,110,C_BLACK, "%s, %s %s, %s %s:%s:%s",time_vector[1],time_vector[2], time_vector[3],time_vector[0],time_vector[4],time_vector[5],time_vector[6]);
    dupdate();
    input = pollevent();
    if (input.type == KEYEV_DOWN){
      if(keydown(KEY_CLEAR) != 0 || keydown(KEY_ACON) != 0){
        running = false;
      } else if(keypressed(KEY_1) != 0) {
        // update drawing function that we are drawing the digital clock now.
        drawTab = "DIGITAL";
      } else if(keypressed(KEY_2) != 0){
        // Analog Clock drawing stuff, idk
        drawTab = "ANALOG";
      } else if(keypressed(KEY_3) != 0){
        // Settings drawing stuff
        drawTab = "SETTINGS";
      } else if(keypressed(KEY_DEL) != 0) {
        drawTab = "ABOUT";
      } else {continue;}
    } else {continue;}
  }
  return 1;
}

