#include <ncurses.h>
#include <stdio.h>
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

#define TICK 5 //8 // ms
#define MIN_TICKS_PER_CYCLE 20

#define IM_FEELIN_LUCKY 0

void show_info(WINDOW *win, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (win) {
        vw_printw(win, fmt, args);
        wrefresh(win);
        refresh();
    }
    va_end(args);
}

#define GAME_END        0
#define GAME_RESTART    1

int car_game(LEDMatrix *lm, WINDOW *info_win, int prob) {
    show_info(info_win, "---------------------------------\n");
    show_info(info_win, "Arrows to move. Press 'Q' to exit\n");

    // Obstacles data structure
    int obstacles[led_rows][led_cols] = {0};
    for (int i=0; i<led_rows-5; i+=2) {
        int has_escape = 0;
        for (int j=0; j<led_cols; j++) {
            // Populate given a uniform distribution
            obstacles[i][j] = (rand()%100) < prob ? OBS_COLOR : 0;
            if (!obstacles[i][j]) {
                has_escape = 1;
            }
        }
#if !(IM_FEELIN_LUCKY)
        // Salvation for all-in-a-row obstacles
        if (!has_escape) {
            int escape = rand() % 4;
            obstacles[i][escape] = 0;
        }
#endif
    }

    // Start position
    int car_row = led_rows - 2;
    int car_col = 1;

    int cycle = 0; // iterations count
    int ticks_per_update = 50;
    int ticks_this_cycle = 0;

    int running = 1;
    int printed_out_msg = 0;

    while (1) {
        int modulo_cycle = cycle % led_rows;
        // draw obstacles
        for (int i=0; i<led_rows; i++) {
            for (int j=0; j<led_cols; j++) {
                led_diode_set_value(lm, i, j, obstacles[(led_rows+i-modulo_cycle)%led_rows][j]);

            }
        }

        switch (led_getch(lm)) {
            case KEY_LEFT:
                if (!running)
                    break;
                led_diode_set_value(lm, car_row, car_col, 0);
                car_col--;
                break;
            case KEY_RIGHT:
                if (!running)
                    break;
                led_diode_set_value(lm, car_row, car_col, 0);
                car_col++;
                break;
            case 'q':
                return GAME_END;
            case 'r':
                led_diode_unset_attrs(lm, car_row, car_col, A_REVERSE);
                return GAME_RESTART;
        }
        car_col = (car_col >= led_cols) ? led_cols-1 : (car_col < 0 ? 0 : car_col);

        // draw car
        led_diode_set_value(lm, car_row, car_col, CAR_COLOR);
        led_draw(lm);

        napms(TICK);

        if (running) {
            ticks_this_cycle++;
        } else if (!printed_out_msg) {
            show_info(info_win, "== GAME OVER ==\nScore: %d\n", cycle/2);
            show_info(info_win, "Press 'R' to restart\n");
            printed_out_msg = 1;
        }

        if (ticks_this_cycle >= ticks_per_update) {
            ticks_this_cycle = 0;
            for (int j=0; j<led_cols; j++) {
                // Check collision
                if (obstacles[(led_rows+car_row-modulo_cycle)%led_rows][car_col]) {
                    led_diode_set_attrs(lm, car_row, car_col, A_REVERSE);
                    led_draw(lm);
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
}

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


    int info_rows = 5;
    int win_rows = -info_rows; // Negative X means "full size minus X"

    LEDMatrix lm;
    int err = led_init(&lm, led_rows /* rows of leds */, led_cols /* cols of leds */,
                            win_rows /* terminal rows */, 0 /* max terminal cols */,
                            0 /* window begin row */, 0 /* window begin col */,
                            0 /* you start ncurses */, 0 /*debug*/);
    if (err) {
        fprintf(stderr, "\nError starting LEDCurses\n");
        fprintf(stderr, "Lines: %d\nwin_rows: %d\ninfo_rows: %d\n", LINES, win_rows, info_rows);
        return 1;
    }
    win_rows = lm.win_rows;

    // Information window at the bottom
    WINDOW *info_win = newwin(info_rows, 0, win_rows, 0);
    if (!info_win) {
        fprintf(stderr, "Couldn't create info window\n");
        fprintf(stderr, "Lines: %d\nwin_rows: %d\ninfo_rows: %d\n", LINES, win_rows, info_rows);
        return 1;
    }
    scrollok(info_win, 1);

    // By default, color #1 is red
    init_pair(CAR_COLOR, COLOR_BLUE, COLOR_BLACK);

    keypad(lm.win, TRUE); // <-- Important for the arrows
    nodelay(lm.win, TRUE); // <-- Important for time to run
    curs_set(0);  // <-- Important because otherwise it's annoying

    while (car_game(&lm, info_win, prob) == GAME_RESTART);
    led_end(&lm);

    return 0;
}
