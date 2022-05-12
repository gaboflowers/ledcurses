#include <ncurses.h>
#include <stdlib.h>  // rand/srand
#include <time.h>    // time
#include "ledcurses.h"

#define OFF_COLOR 0
#define SNAKE_COLOR 1
#define TARGET_COLOR 2

#define TICK 8 // ms

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

int uniform_prob(int n) {
    return rand() % n;
}

#define GAME_END        0
#define GAME_RESTART    1
#define DIR_LEFT    0
#define DIR_UP      1
#define DIR_DOWN    2
#define DIR_RIGHT   3
int arr_down_right[] = {DIR_DOWN, DIR_RIGHT};
int arr_down_left[] = {DIR_DOWN, DIR_LEFT};
int arr_up_right[] = {DIR_UP, DIR_RIGHT};
int arr_up_left[] = {DIR_UP, DIR_LEFT};

#if 0 // for debugging
void print_cells(WINDOW *win, int *snake_rows, int *snake_cols,
                               int snake_length, int arr_size, int head_index) {
    int i=0;
    show_info(win, "Cells: ");
    while (snake_length--) {
        int arr_index = (arr_size+head_index-i)%arr_size;
        int row = snake_rows[arr_index];
        int col = snake_cols[arr_index];
        show_info(win, "(%d, %d) ", row, col);
        i--;
    }
    show_info(win, "[FIN]\n");
}
#endif

void draw_snake(LEDMatrix *lm, int *snake_rows, int *snake_cols,
                               int snake_length, int arr_size, int head_index, int color) {
    int i=0;
    while (snake_length--) {
        int arr_index = (arr_size+head_index-i)%arr_size;
        led_diode_set_value(lm, snake_rows[arr_index], snake_cols[arr_index], color);
        i++;
    }
}

int collision_with_snake(int *snake_rows, int *snake_cols,
                         int snake_length, int arr_size, int head_index,
                         int row, int col) {
    /* SKIPS THE FIRST ELEMENT !!! */
    int i=1;
    while (snake_length--) {
        int arr_index = (arr_size+head_index-i)%arr_size;
        if (row == snake_rows[arr_index] && col == snake_cols[arr_index]) {
            return 1;
        }
        i++;
    }
    return 0;
}

void position_target(LEDMatrix *lm, int *tgt_row, int *tgt_col,
                                    int *snake_rows, int *snake_cols,
                                    int snake_length, int arr_size, int head_index) {
    // Make sure target and snake don't collide beforehand
    do {
        *tgt_row = uniform_prob(lm->led_rows);
        *tgt_col = uniform_prob(lm->led_cols);
    } while (collision_with_snake(snake_rows, snake_cols, snake_length, arr_size, head_index,
                                *tgt_row, *tgt_col) ||
             (*tgt_row == snake_rows[head_index] && *tgt_col == snake_cols[head_index]));
}


int snake_game(LEDMatrix *lm, WINDOW *info_win) {
    int cycle = 0; // iterations count
    int ticks_per_update = 50;
    int ticks_this_cycle = 0;

    // Start position
    int row = uniform_prob(lm->led_rows);
    int col = uniform_prob(lm->led_cols);
    int dir = uniform_prob(4);

    // Edge case: started on an edge or a corner
    // We don't want to collide before knowing where we are headed to.
    if (row == 0) {
        if (col == 0) {
            // Only down or right allowed
            dir = arr_down_right[uniform_prob(2)];
        } else if (col == lm->led_cols-1) {
            // Only down or left allowed
            dir = arr_down_left[uniform_prob(2)];
        }
    } else if (row == lm->led_rows-1) {
        if (col == 0) {
            // Only up or right allowed
            dir = arr_up_right[uniform_prob(2)];
        } else if (col == lm->led_cols-1) {
            // Only up or left allowed
            dir = arr_up_left[uniform_prob(2)];
        }
    }
    show_info(info_win, "---- Start position: (%d, %d) ----\n", row, col);
    show_info(info_win, "Arrows to move. Press 'Q' to exit\n");

    led_diode_set_value(lm, row, col, SNAKE_COLOR);

    // Snake's cells (circular array)
    int arr_size = lm->led_rows * lm->led_cols; // max length of snake
    int *snake_rows = calloc(arr_size, sizeof(int));
    int *snake_cols = calloc(arr_size, sizeof(int));
    if (!snake_rows || !snake_cols) {
        show_info(info_win, "Couldn't allocate corners array\ni");
        return GAME_END;
    }

    int head_index = 0;
    snake_rows[0] = row;
    snake_cols[0] = col;
    int snake_length = 1;
    led_diode_set_value(lm, row, col, SNAKE_COLOR);

    // Target position
    int tgt_row, tgt_col;
    show_info(info_win, "Allocating target... ");
    position_target(lm, &tgt_row, &tgt_col, snake_rows, snake_cols, snake_length, arr_size, head_index);
    show_info(info_win, "New target: (%d, %d)\n", tgt_row, tgt_col);
    led_diode_set_value(lm, tgt_row, tgt_col, TARGET_COLOR);

    int new_dir = dir; // The direction we're told to turn to after a keypress.

    int running = 1;
    int printed_out_msg = 0;
    int can_grow = 0;

    while (1) {
        led_draw(lm);

        switch (led_getch(lm)) {
            case KEY_DOWN:
                new_dir = DIR_DOWN;
                break;
            case KEY_UP:
                new_dir = DIR_UP;
                break;
            case KEY_LEFT:
                new_dir = DIR_LEFT;
                break;
            case KEY_RIGHT:
                new_dir = DIR_RIGHT;
                break;
            case 'q':
                return GAME_END;
            case 'r':
                led_diode_unset_attrs(lm, row, col, A_REVERSE);
                draw_snake(lm, snake_rows, snake_cols, snake_length, arr_size, head_index, OFF_COLOR);
                led_diode_set_value(lm, tgt_row, tgt_col, OFF_COLOR);
                snake_length = 0;
                return GAME_RESTART;
        }

        napms(TICK);

        if (running) {
            ticks_this_cycle++;
        } else if (!printed_out_msg) {
            show_info(info_win, "== GAME OVER ==\nScore: %d\n", snake_length-1);
            show_info(info_win, "Press 'R' to restart\n");
            printed_out_msg = 1;
        }

        if (ticks_this_cycle >= ticks_per_update) {
            ticks_this_cycle = 0;

            // Can't brake
            if ( !( ((new_dir == DIR_DOWN && dir == DIR_UP) ||
                     (new_dir == DIR_UP && dir == DIR_DOWN) ||
                     (new_dir == DIR_LEFT && dir == DIR_RIGHT) ||
                     (new_dir == DIR_RIGHT && dir == DIR_LEFT)) ) ) {
                dir = new_dir;
            }

            if (row == tgt_row && col == tgt_col) {
                // Collission with target
                if (snake_length >= (arr_size-1)) {
                    // If cannot position next target, gotta quit before getting into an endless loop.
                    show_info(info_win, "Full grid. Congrats!\n");
                    running = 0;
                    continue;
                }
                // Relocate target
                show_info(info_win, "Allocating target... ");
                position_target(lm, &tgt_row, &tgt_col, snake_rows, snake_cols, snake_length, arr_size, head_index);
                show_info(info_win, "New target: (%d, %d)\n", tgt_row, tgt_col);

                led_diode_set_value(lm, tgt_row, tgt_col, TARGET_COLOR);
                led_draw(lm);
                can_grow = 1;
            } else {
                can_grow = 0;
            }

            if ((row == 0 && dir == DIR_UP) ||
                (row == lm->led_rows-1 && dir == DIR_DOWN) ||
                (col == 0 && dir == DIR_LEFT) ||
                (col == lm->led_cols-1 && dir == DIR_RIGHT) ||
                collision_with_snake(snake_rows, snake_cols, snake_length, arr_size, head_index, row, col)) {
                // Collission with self or with border: Game over.
                led_diode_set_attrs(lm, row, col, A_REVERSE);
                running = 0;
            } else {
                // No collision: will move.
                // Turn off tail if we didn't grow
                if (!can_grow) {
                    int tail_index = (arr_size+head_index-snake_length+1)%arr_size;
                    led_diode_set_value(lm, snake_rows[tail_index], snake_cols[tail_index], OFF_COLOR);
                } else {
                    snake_length++;
                }
                // Now update the keypress
                //show_info(info_win, "%d - Going %s\n", cycle, dir==DIR_UP? "Up" : (dir==DIR_DOWN? "Down" : (dir==DIR_LEFT? "Left": "Right")));
                switch (dir) {
                    case DIR_UP:
                        row--;
                        break;
                    case DIR_DOWN:
                        row++;
                        break;
                    case DIR_LEFT:
                        col--;
                        break;
                    case DIR_RIGHT:
                        col++;
                        break;
                }
                head_index = (head_index+1) % arr_size;
                snake_rows[head_index] = row;
                snake_cols[head_index] = col;
            }
            //print_cells(info_win, snake_rows, snake_cols, snake_length, arr_size, head_index);

            cycle++;
        }
        draw_snake(lm, snake_rows, snake_cols, snake_length, arr_size, head_index, SNAKE_COLOR);
    }

}

int main(int argc, char *argv[]) {
    // If one argument, it's random seed
    int seed;
    if (argc > 1) {
        seed = atoi(argv[1]);
    } else {
        seed = time(NULL);
    }
    srand(seed);

    // If more arguments, it's field size
    int led_rows, led_cols;
    if (argc > 3) {
        led_rows = atoi(argv[2]); led_cols = atoi(argv[3]);
    } else {
        led_rows = 6; led_cols = 8;
    }

    int info_rows = 5;
    int win_rows = -info_rows; // Negative X means "full size minus X"

    LEDMatrix lm;
    int err = led_init(&lm, led_rows /* rows of leds */, led_cols /* cols of leds */,
                            win_rows /* terminal rows */, 100 /* terminal cols */,
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
    init_pair(TARGET_COLOR, COLOR_YELLOW, COLOR_BLACK);

    keypad(lm.win, TRUE); // <-- Important for the arrows
    nodelay(lm.win, TRUE); // <-- Important for time to run
    curs_set(0);  // <-- Important because otherwise it's annoying

    while (snake_game(&lm, info_win) == GAME_RESTART);

    led_end(&lm);
    return 0;
}
