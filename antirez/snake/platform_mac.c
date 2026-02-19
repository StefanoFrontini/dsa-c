#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static struct termios original;

void platform_init(void) {
  tcgetattr(STDIN_FILENO, &original);
  struct termios raw = original;
  raw.c_lflag &= ~(ICANON | ECHO);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  // Nasconde il cursore
  printf("\033[?25l");
}

void platform_cleanup(void) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
  printf("\033[?25h"); // Mostra il cursore
}

uint64_t platform_get_time_us(void) {
  struct timeval te;
  gettimeofday(&te, NULL);
  return te.tv_sec * 1000000LL + te.tv_usec;
}

void platform_sleep_us(uint64_t us) {
  usleep(us);
}

PlatformKey platform_get_input(void) {
  char c;
  ssize_t nread = read(STDIN_FILENO, &c, 1);
  if (nread == 0) return PLAT_KEY_NONE;
  if (c != '\033') return PLAT_KEY_NONE;

  char buf[2];
  nread = read(STDIN_FILENO, buf, sizeof(buf));
  if (nread == 0) return PLAT_KEY_ESC;

  if (buf[0] == '[') {
    switch (buf[1]) {
      case 'A':
        return PLAT_KEY_UP;
      case 'B':
        return PLAT_KEY_DOWN;
      case 'C':
        return PLAT_KEY_RIGHT;
      case 'D':
        return PLAT_KEY_LEFT;
    }
  }
  return PLAT_KEY_NONE;
}

void platform_clear_screen(void) {
  printf("\033[2J"); // Clean screen
  printf("\033[H");  // Cursor at Home
}

void platform_random_seed(void) {
  srand(time(NULL));
}