#include <ncurses.h>
#include "ledcurses.h"


int main() {
    LEDMatrix lm;
    led_init(&lm, 3 /* rows of leds */, 5 /* cols of leds */,
                  0 /* max terminal rows */, 100 /* terminal cols */,
                  0 /* window begin row */, 0 /* window begin col */,
                  0 /* you start ncurses */, 1 /*debug*/);

    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);

    char round = 0;
    while (1) {
        for (int i=0; i<15; i++) {
            int row = i/5;
            int col = i%5;
            led_diode_set_value(&lm, row, col, (round+i)%3 + 1 /* 1, 2 or 3 */);
        }
        round++;
        led_draw(&lm);

        info(&lm, "Any key to continue. PRESS SPACE BAR TO EXIT\n");
        if (led_getch(&lm) == ' ') {
            break;
        }
    }

    led_end(&lm);
    return 0;
}
