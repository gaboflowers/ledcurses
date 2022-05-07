#ifndef LEDCURSES_H
#define LEDCURSES_H

/*
 * This file is part of LEDCurses.
 *
 * LEDCurses is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LEDCurses is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LEDCurses.  If not, see <https://www.gnu.org/licenses/>.
 * */

#include <ncurses.h>
#include <stdlib.h> // calloc
#include <stdarg.h>

#define BIT_FIELD(name) unsigned int name : 1
#define SQUARE(x) ({ __typeof__(x) _x = x; _x*_x; })
#define DEBUG_LINES 10

typedef struct single_led {
    int value;
    int ch_attrs;
} Diode;

typedef struct led_matrix {
    WINDOW *win;
    WINDOW *dbgwin;
    Diode *matrix;
    int led_rows;
    int led_cols;
    int led_size;
    int char_ratio; // ratio of height to width pixels of a char (cols per row)
    int led_size_ratioed; // led_size*char_ratio, we use it a lot
    int led_halfsize_sq;    // SQUARE(led_size/2)
    int led_halfsize_m1_sq; // SQUARE(lm->led_size/2-1)
    int win_rows;
    int win_cols;
    chtype ch_edge_on;
    chtype ch_edge_off;
    chtype ch_inner_on;
    chtype ch_inner_off;
    BIT_FIELD(i_started_curses);
    BIT_FIELD(uses_color);
    BIT_FIELD(grid_available);
    BIT_FIELD(grid_enabled);
} LEDMatrix;


void err(LEDMatrix *lm, char *msg);
void info(LEDMatrix *lm, const char *fmt, ...);
int led_init(LEDMatrix *lm, int led_rows, int led_cols,
                            int rows, int cols,
                            int begin_row, int begin_col, int curses_started, int debug);
/* led_set_grid: if `value` is not 0, will try to enable the grid,
 *               otherwise, disables the grid
 * returns 1 on failure, 0 on success.
 * */
int led_set_grid(LEDMatrix *lm, int value);
/* led_get_diode: returns a pointer to the Diode at the given (row, col).
 *                If out of bounds, will return NULL.
 * */
Diode *led_get_diode(LEDMatrix *lm, int row, int col);
/* led_diode_set_value: if `value` is 0, the diode is considered off
 *                      otherwise, diode will be colored with the
 *                      COLOR_PAIR(value).
 *      By default, only COLOR_PAIR(1) is initialized,  but you can
 *      use whatever value you may have init_pair'd.
 *
 *      Using an uninitialized value is undefined.
 * */
void led_diode_set_value(LEDMatrix *lm, int row, int col, int value);
void led_diode_set_attrs(LEDMatrix *lm, int row, int col, int attrs);
void led_diode_unset_attrs(LEDMatrix *lm, int row, int col, int attrs);
void led_draw_diode(LEDMatrix *lm, int row, int col);
void led_draw_grid(LEDMatrix *lm);
/* led_draw: draw the LEDMatrix to the window.
 *           call this after each change (for instance,
 *           led_diode_set_value calls)
 * */
void led_draw(LEDMatrix *lm);
int led_get_row_center_pos(LEDMatrix *lm, int led_row);
int led_get_col_center_pos(LEDMatrix *lm, int led_col);
void led_draw_diode(LEDMatrix *lm, int led_row, int led_col);
/* led_getch: the getch for this window
 * */
int led_getch(LEDMatrix *lm);
/* led_end: destructor for the LEDMatrix.
 *          Be sure to call it at the end to prevent memory leaks.
 * */
int led_end(LEDMatrix *lm);

#endif // LEDCURSES_H
