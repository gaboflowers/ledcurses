# LEDCurses

A curses/ncurses library for simulating LED matrix displays.

## Dependencies
- ncurses

## Usage

Once installed, a C program can use it like this
```c
#include <ledcurses.h>

int main() {
    LEDMatrix lm;
    led_init(&lm, 3 /* rows of leds */, 5 /* cols of leds */,
                  0 /* max terminal rows */, 100 /* terminal cols */
                  0 /* window begin row */, 0 /* window begin col */
                  0 /* you start ncurses */, 1 /*debug*/);

    led_diode_set_value(&lm, 2, 0, 1); // led (2, 0) has now value 1
                                       // (by default, 1 is red over black)
    led_draw(&lm);
    led_getch(&lm);  // press any key to close

    led_end(&lm);
    return 0;
}
```

## Examples

By building using the Makefile, the `examples/` folder will contain two versions for each example binary. For instance, the file `xmas.c` will build into both `xmas` and `xmas.static`. If you haven't properly copied the shared library `lib/ledcurses.so` into somewhere findable (such as `/usr/include`), you will have to tell your console where to find it when running the dynamically-linked version
```bash
cd examples
LD_LIBRARY_PATH=$(pwd)/../lib/ ./xmas
```
