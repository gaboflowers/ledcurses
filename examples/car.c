#include <ncurses.h>
#include <stdlib.h>  // rand/srand
#include <time.h>    // time
#include "ledcurses.h"

#define max(a, b) ({__typeof__(a) _a = (a); \
                    __typeof__(b) _b = (b); \
                    _a > _b ? _a : _b;})

#define OBS_COLOR 1
#define CAR_COLOR 2

#define led_rows 12
#define led_cols 4

#define TICK 8 // ms
#define MIN_TICKS_PER_CYCLE 20

#define IM_FEELIN_LUCKY 0

int main(int argc, char *argv[]) {
    int seed;
    if (argc > 1) {
        seed = atoi(argv[1]);
    } else {
        seed = time(NULL);
    }
    srand(seed);
    int prob = 30; // 30 in 100 is the probability for a cell to have an obstacle
                   // No salvation kill: 1 in 0.3^4 ~ 0.81% (1 in 123 rows approx.)

    int obstacles[led_rows][led_cols] = {0};
    for (int i=0; i<led_rows-5; i+=2) {
        int has_escape = 0;
        for (int j=0; j<led_cols; j++) {
            obstacles[i][j] = (rand()%100) < prob ? OBS_COLOR : 0;
            if (!obstacles[i][j]) {
                has_escape = 1;
            }
        }
#if !(IM_FEELIN_LUCKY)
        // Salvation
        if (!has_escape) {
            int escape = rand() % 4;
            obstacles[i][escape] = 0;
        }
#endif
    }

    LEDMatrix lm;
    led_init(&lm, led_rows /* rows of leds */, led_cols /* cols of leds */,
                  0 /* max terminal rows */, 0 /* max terminal cols */,
                  0 /* window begin row */, 0 /* window begin col */,
                  0 /* you start ncurses */, 0 /*debug*/);

    // By default, color #1 is red
    init_pair(CAR_COLOR, COLOR_BLUE, COLOR_BLACK);


    keypad(lm.win, TRUE); // <-- Important for the arrows
    nodelay(lm.win, TRUE); // <-- Important for time to run
    curs_set(0);  // <-- Important because otherwise it's annoying

    // Start position
    int car_row = led_rows - 2;
    int car_col = 1;

    int cycle = 0; // iterations count
    int ticks_per_update = 50;
    int ticks_this_cycle = 0;

    int running = 1;
    while (1) {
        int modulo_cycle = cycle % led_rows;
        // draw obstacles
        for (int i=0; i<led_rows; i++) {
            for (int j=0; j<led_cols; j++) {
//                led_diode_set_value(&lm, (i+cycle)%led_rows, j, obstacles[i][j]);
                led_diode_set_value(&lm, i, j, obstacles[(led_rows+i-modulo_cycle)%led_rows][j]);

            }
        }

        info(&lm, "CYCLE %d\n - Arrows to move. PRESS 'Q' TO EXIT\n", cycle);
        switch (led_getch(&lm)) {
            case KEY_LEFT:
                if (!running)
                    break;
                led_diode_set_value(&lm, car_row, car_col, 0);
                car_col--;
                break;
            case KEY_RIGHT:
                if (!running)
                    break;
                led_diode_set_value(&lm, car_row, car_col, 0);
                car_col++;
                break;
            case 'q':
                goto end;
        }
        car_col = (car_col >= led_cols) ? led_cols-1 : (car_col < 0 ? 0 : car_col);

        // draw car
        led_diode_set_value(&lm, car_row, car_col, CAR_COLOR);
        led_draw(&lm);

        napms(TICK);

        if (running) {
            ticks_this_cycle++;
        }

        if (ticks_this_cycle >= ticks_per_update) {
            ticks_this_cycle = 0;
            for (int j=0; j<led_cols; j++) {
                // Check collision
                if (obstacles[(led_rows+car_row-modulo_cycle)%led_rows][car_col]) {
                    led_diode_set_attrs(&lm, car_row, car_col, A_REVERSE);
                    led_draw(&lm);
                    running = 0;
                }

                // Clear outgoing obstacles
                obstacles[(2*led_rows-1-modulo_cycle)%led_rows][j] = 0;
            }

            if (cycle%2 == 0) {
                // Gotta populate the new obstacles
                int new_row_index = (led_rows-1-modulo_cycle)%led_rows;
                int has_escape = 0;
                for(int j=0; j<led_cols; j++) {
                    obstacles[new_row_index][j] = (rand()%100) < prob ? OBS_COLOR : 0;
                }
#if !(IM_FEELIN_LUCKY)
                // Salvation
                if (!has_escape) {
                    int escape = rand() % 4;
                    obstacles[new_row_index][escape] = 0;
                }
#endif

                if (cycle%100 == 0) {
                    // Make the game faster every 100 rows
                    ticks_per_update = max(ticks_per_update-5, MIN_TICKS_PER_CYCLE);
                }
            }

            cycle++;
        }
    }
end:
    led_end(&lm);
    printf("Score: %d\n", cycle/2);
    return 0;
}
