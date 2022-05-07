#include <ncurses.h>
#include "ledcurses.h"


int main(int argc, char *argv[]) {
    int led_rows, led_cols;
    if (argc < 3) {
        led_rows = 6; led_cols = 8;
    } else {
        led_rows = atoi(argv[1]); led_cols = atoi(argv[2]);
    }
    LEDMatrix lm;
    led_init(&lm, led_rows /* rows of leds */, led_cols /* cols of leds */,
                  0 /* max terminal rows */, 100 /* terminal cols */,
                  0 /* window begin row */, 0 /* window begin col */,
                  0 /* you start ncurses */, 1 /*debug*/);

    keypad(lm.win, TRUE); // <-- Important for the arrows
    int row = 2 >= led_rows? led_rows-1 : 2;
    int col = 3 >= led_cols? led_cols-1 : 3;

    while (1) {
        led_diode_set_value(&lm, row, col, 1);
        led_draw(&lm);

        info(&lm, "Arrows to move. PRESS ENTER TO EXIT\n");
        switch (led_getch(&lm)) {
            case KEY_DOWN:
                led_diode_set_value(&lm, row, col, 0);
                row++;
                break;
            case KEY_UP:
                led_diode_set_value(&lm, row, col, 0);
                row--;
                break;
            case KEY_LEFT:
                led_diode_set_value(&lm, row, col, 0);
                col--;
                break;
            case KEY_RIGHT:
                led_diode_set_value(&lm, row, col, 0);
                col++;
                break;
            case '\n':
                goto end;
        }
        row = row < 0 ? 0 : row;
        col = col < 0 ? 0 : col;
        row = row >= led_rows ? led_rows-1 : row;
        col = col >= led_cols ? led_cols-1 : col;
        led_diode_set_value(&lm, row, col, 1);
    }
end:
    led_end(&lm);
    return 0;
}
