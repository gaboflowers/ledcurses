#include <ncurses.h>
#include "ledcurses.h"


int main() {
    LEDMatrix lm;
    led_init(&lm, 3 /* rows of leds */, 5 /* cols of leds */,
                  0 /* max terminal rows */, 100 /* terminal cols */,
                  0 /* window begin row */, 0 /* window begin col */,
                  0 /* you start ncurses */, 1 /*debug*/);

    led_diode_set_value(&lm, 2, 0, 1); // led (2, 0) has now value 1
    led_draw(&lm);
    led_getch(&lm);

    led_end(&lm);
    return 0;
}
