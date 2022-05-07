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

#include "ledcurses.h"

void err(LEDMatrix *lm, char *msg) {
    if (lm->dbgwin) {
        wprintw(lm->dbgwin, "%s", msg);
    } else {
        puts(msg);
    }
}

void info(LEDMatrix *lm, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (lm->dbgwin) {
        vw_printw(lm->dbgwin, fmt, args);
        wrefresh(lm->dbgwin);
    }
    va_end(args);
}

int led_init(LEDMatrix *lm, int led_rows, int led_cols,
                            int rows, int cols,
                            int begin_row, int begin_col, int curses_started, int debug) {
    if (!lm) {
        err(lm, "LEDMatrix pointer mustn't be null\n");
        return 1;
    }
    lm->dbgwin = NULL;

    // Start NCurses if we're asked to do so
    lm->uses_color = 0;
    if (!curses_started) {
        initscr();
        lm->i_started_curses = 1;
        if (has_colors()) {
            start_color();
            lm->uses_color = 1;
        }
    }
    refresh();

    // If debug is enabled, and rows is 0 (full height), then we have
    // to reserve manually some debugging rows.
    if (debug) {
        if (rows == 0) {
            rows = LINES - DEBUG_LINES;
        } else if (rows < 0) {
            // See next comment
            rows = LINES + rows - DEBUG_LINES;
        } else {
            rows = rows - DEBUG_LINES;
        }
    }

    // Negative values for `rows` or `cols` means full size minus the positive value.
    if (rows < 0) rows = LINES + rows;
    if (cols < 0) cols = COLS + cols;

    // Create the window where our LED grid will live on
    lm->win = newwin(rows, cols, begin_row, begin_col);
    if (!lm->win) {
        err(lm, "Couldn't create window\n");
        return 1;
    }

    // Check if either rows or cols was 0
    int max_rows, max_cols;
    getmaxyx(lm->win, max_rows, max_cols);
    if (rows == 0) rows = max_rows;
    if (cols == 0) cols = max_cols;
    lm->win_rows = rows;
    lm->win_cols = cols;

    // Create debug window if asked
    if (debug) {
        lm->dbgwin = newwin(DEBUG_LINES, cols, begin_row+rows, begin_col);
        if (!lm->dbgwin) {
            err(lm, "Couldn't create debug window\n");
            return 1;
        }
        scrollok(lm->dbgwin, 1);
    }

    // Check if we can fit enough LEDs
    if (led_rows > rows || led_cols > cols) {
        err(lm, "Cannot have more than one LED per character\n");
        return 1;
    }

    lm->led_rows = led_rows;
    lm->led_cols = led_cols;

    // Inner representation of the LEDs
    lm->matrix = (Diode*)calloc(rows*cols, sizeof(Diode));
    if (!lm->matrix) {
        err(lm, "Couldn't allocate Diode matrix\n");
        return 1;
    }

    // Cells usually are not a square
    lm->char_ratio = 2;

    // Calculate the LED diameter/width (assuming square/round LEDs)
    float rows_per_led = (float)rows/led_rows;
    float cols_per_led = (float)cols/(led_cols*lm->char_ratio);
    if (cols_per_led > rows_per_led) {
        // Height (inter-row space) is what constrains us
        lm->led_size = (int)rows_per_led;

    } else {
        // Width (inter-column space) is what constrains us
        lm->led_size = (int)cols_per_led;
    }

    // Odd sizes are nicer
    if (lm->led_size > 5 && lm->led_size%2 == 0) {
        lm->led_size--;
    }

    // Don't repeat calculations
    lm->led_size_ratioed = lm->led_size*lm->char_ratio;
    lm->led_halfsize_sq = SQUARE(lm->led_size/2);
    lm->led_halfsize_m1_sq = SQUARE(lm->led_size/2 - 1);

    // Can we fit a grid?
    lm->grid_enabled = 0;
    lm->grid_available = 0;
    if ((rows - lm->led_size*led_rows >= (led_rows-1)) &&
        (cols - lm->led_size_ratioed*led_cols >= (led_cols-1))) {
        lm->grid_available = 1;
    }

    // Default chars
    lm->ch_edge_on = A_BOLD | 'O';
    lm->ch_edge_off = 'O';
    lm->ch_inner_on = A_BOLD | '+'; //A_ALTCHARSET | A_BOLD | ACS_CKBOARD;
    lm->ch_inner_off = ' ';

    if (lm->uses_color) {
        init_pair(1, COLOR_RED, COLOR_BLACK);
    } else {
        lm->ch_edge_on |= A_REVERSE;
    }

    if (debug) {
        info(lm, "LED matrix size is %d LED rows by %d LED cols.\n", lm->led_rows, lm->led_cols);
        info(lm, "The window has size %d cell rows by %d cell cols.\n", rows, cols);
        info(lm, "A single LED size is %d, and its ratioed size is %d.\n", lm->led_size, lm->led_size_ratioed);
        info(lm, "Its halfsize squared is %d.\n", lm->led_halfsize_sq);
        info(lm, "Grid is available?: %s\n", lm->grid_available ? "yes" : "no");
    } else {
        curs_set(0); // Cursor off
    }

    return 0;
}

/* led_set_grid: if `value` is not 0, will try to enable the grid,
 *               otherwise, disables the grid
 * returns 1 on failure, 0 on success.
 * */
int led_set_grid(LEDMatrix *lm, int value) {
    if (value && !lm->grid_available) {
        return 1;
    }
    lm->grid_enabled = value ? 1 : 0;
    return 0;
}

/* led_get_diode: returns a pointer to the Diode at the given (row, col).
 *                If out of bounds, will return NULL.
 * */
Diode *led_get_diode(LEDMatrix *lm, int row, int col) {
    if (row > lm->led_rows || col > lm->led_cols) {
        err(lm, "LED position out of grid\n");
        return NULL;
    }
    return &(lm->matrix[row*lm->led_cols + col]);
}

/* led_diode_set_value: if `value` is 0, the diode is considered off
 *                      otherwise, diode will be colored with the
 *                      COLOR_PAIR(value).
 *      By default, only COLOR_PAIR(1) is initialized,  but you can
 *      use whatever value you may have init_pair'd.
 *
 *      Using an uninitialized value is undefined.
 * */
void led_diode_set_value(LEDMatrix *lm, int row, int col, int value) {
    Diode *diode = led_get_diode(lm, row, col);
    diode->value = value;
}

void led_diode_set_attrs(LEDMatrix *lm, int row, int col, int attrs) {
    Diode *diode = led_get_diode(lm, row, col);
    diode->ch_attrs |= attrs;
}

void led_diode_unset_attrs(LEDMatrix *lm, int row, int col, int attrs) {
    Diode *diode = led_get_diode(lm, row, col);
    diode->ch_attrs &= ~attrs;
}

void led_draw_grid(LEDMatrix *lm);

/* led_draw: draw the LEDMatrix to the window.
 *           call this after each change (for instance,
 *           led_diode_set_value calls)
 * */
void led_draw(LEDMatrix *lm) {
    for (int i=0; i<lm->led_rows; i++) {
        for (int j=0; j<lm->led_cols; j++) {
            led_draw_diode(lm, i, j);
        }
    }

    if (lm->grid_enabled) {
        info(lm, "Grid is enabled. Drawing it.\n");
        led_draw_grid(lm);
    } else {
        info(lm, "Grid is not enabled.\n");
    }
    wrefresh(lm->win);
}

int led_get_row_center_pos(LEDMatrix *lm, int led_row) {
    int grid_cell = lm->grid_enabled ? 1 : 0;
    return led_row*(lm->led_size + grid_cell) + (lm->led_size)/2;
}

int led_get_col_center_pos(LEDMatrix *lm, int led_col) {
    int grid_cell = lm->grid_enabled ? 1 : 0;
    return led_col*lm->char_ratio*(lm->led_size + grid_cell) + lm->led_size_ratioed/2;
}

void led_draw_diode(LEDMatrix *lm, int led_row, int led_col) {
    // Center of diode
    int center_row = led_get_row_center_pos(lm, led_row);
    int center_col = led_get_col_center_pos(lm, led_col);
    Diode *diode = led_get_diode(lm, led_row, led_col);

    /*
    int halfsize_squared = SQUARE(lm->led_size/2);
    int halfsize_m1_squared = SQUARE(lm->led_size/2-1);
    int halfsize_p1_squared = SQUARE(lm->led_size/2+1);
    */

    if (diode->value && lm->uses_color) {
        wattrset(lm->win, COLOR_PAIR(diode->value));
        info(lm, "Diode (%d, %d) has color %d\n.", led_row, led_col, diode->value);
    } else {
        wattrset(lm->win, COLOR_PAIR(0));
    }

    // The double loop only covers the lower-right part of the diode (positive offsets)
    // but it will paint the four quadrants each time
    for (int d_i = 0; d_i < lm->led_size/2; d_i++) {
        for (int d_j = 0; d_j < lm->led_size_ratioed/2; d_j++) {
            int dist_squared = SQUARE(d_i) + SQUARE((float)d_j/lm->char_ratio);
            // Edge of the diode (circle)
            chtype to_draw;
            // /* nope */ if (lm->led_halfsize_sq <= dist_squared && dist_squared <= halfsize_p1_squared) {
            // /* too much */ if (lm->led_halfsize_m1_sq <= dist_squared && dist_squared <= halfsize_p1_squared) {
            // /* too little */ if (lm->led_halfsize_sq == dist_squared) {
            if (lm->led_halfsize_m1_sq <= dist_squared && dist_squared <= lm->led_halfsize_sq) {
                to_draw = diode->value ? lm->ch_edge_on : lm->ch_edge_off;
            } else if (dist_squared < lm->led_halfsize_m1_sq) {
                to_draw = diode->value ? lm->ch_inner_on : lm->ch_inner_off;
            } else {
                continue;
            }
            to_draw |= diode->ch_attrs;
            mvwaddch(lm->win, center_row+d_i, center_col+d_j, to_draw);
            mvwaddch(lm->win, center_row-d_i, center_col+d_j, to_draw);
            mvwaddch(lm->win, center_row+d_i, center_col-d_j, to_draw);
            mvwaddch(lm->win, center_row-d_i, center_col-d_j, to_draw);
        }
    }

    mvwprintw(lm->win, center_row, center_col, "x");
}

void led_draw_grid(LEDMatrix *lm) {
    int count;
    for (int i=lm->led_size, count=0; i<lm->win_rows &&
                                      count < (lm->led_rows-1); i+=(lm->led_size+1), count++) {
        mvwhline(lm->win, i, 0, ACS_HLINE, lm->win_cols);
    }

    for (int j=lm->led_size*lm->char_ratio,
                                   count=0; j<lm->win_cols &&
                                            count < (lm->led_cols-1); j+=(lm->led_size+1)*lm->char_ratio,
                                                                      count++) {
        mvwvline(lm->win, 0, j, ACS_VLINE, lm->win_rows);
    }
    wrefresh(lm->win);
}

/* led_getch: the getch for this window
 * */
int led_getch(LEDMatrix *lm) {
    return wgetch(lm->win);
}

/* led_end: destructor for the LEDMatrix.
 *          Be sure to call it at the end to prevent memory leaks.
 * */
int led_end(LEDMatrix *lm) {
    int ret = 0;
    if (lm->i_started_curses) {
        ret = endwin();
    }
    free(lm->matrix);
    return ret != ERR;
}

