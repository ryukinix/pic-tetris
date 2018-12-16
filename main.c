/*
 * File:   main.c
 * Author: lerax and mmssouza
 * Project: Tetris in PIC
 *
 * Sun 07 Oct 2018 07:01:09 PM -03
 */

#include "18f4550.h"
#include <stdlib.h>
#include <xc.h>

#define _XTAL_FREQ 16000000
#define DISPLAY_ROWS 16
#define DISPLAY_COLUMNS 8
#define BLOCK_SIZE 3
#define DISPLAY_COLUMNS_PORT PORTD
#define INHIBIT_PIN PORTDbits.RD4
#define KEY_LEFT_PIN PORTBbits.RB0
#define KEY_RIGHT_PIN PORTBbits.RB1
#define KEY_ROTATE_PIN PORTBbits.RB2
#define DISPLAY_ROW_PORT PORTA
#define DELAY_TICK 255
#define DELAY_TICK_GAME_OVER 10
#define DELAY_FALL 50
#define DELAY_KEY_DEBOUNCE 30
#define DELAY_KEY_REPEAT 20
#define BLOCK_NUM 7

typedef unsigned char byte;

typedef byte Block[BLOCK_SIZE][BLOCK_SIZE];

// global variables
byte volatile display[DISPLAY_ROWS][DISPLAY_COLUMNS];
byte row, col;
int p_i, p_j;
byte state, game_state, collision_state;
unsigned int timer;

byte seed_idx;
byte seed[4];
byte current_block;

enum PixelState {
    OFF = 0,
    ON,
    P, // player
};

enum KeyState {
    KEY_NONE = 0,
    KEY_LEFT = 0x01,
    KEY_RIGHT = 0x02,
    KEY_ROTATE = 0x04,
};

enum Collisions {
    COLLISION_NONE = 0x00,
    COLLISION_LEFT = 0x01,
    COLLISION_RIGHT = 0x02,
    COLLISION_BOTTOM = 0x04,
};

enum Block_id {
    T,
    L1,
    L2,
    O,
    S,
    Z,
    I,
};

Block const BLOCK_T = {
    {0, P, 0},
    {P, P, P},
    {0, 0, 0},
};

Block const BLOCK_L1 = {
    {0, P, 0},
    {0, P, 0},
    {0, P, P},
};

Block const BLOCK_L2 = {
    {0, P, 0},
    {0, P, 0},
    {P, P, 0},
};

Block const BLOCK_O = {
    {P, P, 0},
    {P, P, 0},
    {0, 0, 0},
};

Block const BLOCK_S = {
    {0, P, P},
    {P, P, 0},
    {0, 0, 0},
};

Block const BLOCK_Z = {
    {P, P, 0},
    {0, P, P},
    {0, 0, 0},
};

Block const BLOCK_I = {
    {0, P, 0},
    {0, P, 0},
    {0, P, 0},
};

Block const BLOCK_FULL = {
    {1, 1, 1},
    {1, 1, 1},
    {1, 1, 1},
};

Block const BLOCK_EMPTY = {
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
};

/*const Block *Blocks[] =    { BLOCK_T,
  BLOCK_L1,
  BLOCK_L2,
  BLOCK_O,
  BLOCK_S,
  BLOCK_Z,
  BLOCK_I };

*/
inline void disable_interrupt() { INTCONbits.GIE = 0; }

inline void enable_interrupt() { INTCONbits.GIE = 1; }

void set_display(byte value) {
    TMR2IE = 0;
    for (byte i = 0; i < DISPLAY_ROWS; i++) {
        for (byte j = 0; j < DISPLAY_COLUMNS; j++) {
            display[i][j] = value;
        }
    }
    TMR2IE = 1;
}

inline void fill_display() { set_display(1); }

inline void clear_display() { set_display(0); }

byte check_collision() {
    byte collision = COLLISION_NONE;

    for (int i = p_i; i < p_i + BLOCK_SIZE; i++) {
        for (int j = p_j; j < p_j + BLOCK_SIZE; j++) {
            if (i < DISPLAY_ROWS) {
                if ((j >= 0) && (j < DISPLAY_COLUMNS)) {
                    if (display[i][j] == P) {
                        // collision BOTTOM
                        if (i == DISPLAY_ROWS - 1) {
                            collision |= COLLISION_BOTTOM;
                        } else if (display[i + 1][j] == ON) {
                            collision |= COLLISION_BOTTOM;
                        }
                        // collision LEFT and collision WALL (RIGHT)
                        if (j == DISPLAY_COLUMNS - 1) {
                            collision |= COLLISION_RIGHT;
                            if (display[i][j - 1] == ON) {
                                collision |= COLLISION_LEFT;
                            }
                        } // collision LEFT (WALL) and collision RIGHT
                        else if (j == 0) {
                            collision |= COLLISION_LEFT;
                            if (display[i][j + 1] == ON) {
                                collision |= COLLISION_RIGHT;
                            }
                        } else {
                            if (display[i][j + 1] == ON) {
                                collision |= COLLISION_RIGHT;
                            }
                            if (display[i][j - 1] == ON) {
                                collision |= COLLISION_LEFT;
                            }
                        }
                    }
                }
            }
        }
    }
    return collision;
}

void freeze_blocks() {
    TMR2IE = 0;
    for (int i = p_i; i < p_i + BLOCK_SIZE; i++) {
        for (int j = p_j; j < p_j + BLOCK_SIZE; j++) {
            if ((j >= 0) && (j < DISPLAY_COLUMNS)) {
                if (display[i][j] == P) {
                    display[i][j] = ON;
                }
            }
        }
    }
    TMR2IE = 1;
}

void fall_one_row() {
    TMR2IE = 0; // disable timer
    for (int i = p_i + BLOCK_SIZE - 1; i > p_i - 1; i--) {
        for (int j = p_j; j < p_j + BLOCK_SIZE; j++) {
            if ((j >= 0) && (j < DISPLAY_COLUMNS)) {
                if (display[i + 1][j] != ON) {
                    display[i + 1][j] = display[i][j];
                    if (i == p_i) {
                        display[i][j] = 0;
                    }
                }
            }
        }
    }
    p_i++;
    TMR2IE = 1; // enable timer
}

byte full_row(byte volatile row[DISPLAY_COLUMNS]) {
    for (byte j = 0; j < DISPLAY_COLUMNS; j++) {
        if (row[j] == OFF) {
            return 0;
        }
    }
    return 1;
}

void fall_row_until(byte n) {
    TMR2IE = 0;
    for (byte i = n; i > 0; i--)
        for (byte j = 0; j < DISPLAY_COLUMNS; j++) {
            display[i][j] = display[i - 1][j];
        }
    TMR2IE = 1;
}

void clean_full_rows() {
    for (byte i = 1; i < DISPLAY_ROWS; i++) {
        if (full_row(display[i])) {
            fall_row_until(i);
        }
    }
}

void insert_block(Block const p) {
    TMR2IE = 0; // disable timer
    p_i = 0;
    p_j = 2;
    for (byte i = 0; i < BLOCK_SIZE; i++) {
        for (byte j = 0; j < BLOCK_SIZE; j++) {
            display[i + p_i][j + p_j] = p[i][j];
        }
    }
    TMR2IE = 1; // enable timer
}

inline byte check_game_over() {
    return ((p_i == 0) && (collision_state & COLLISION_BOTTOM));
}

// An Inplace function to rotate a N x N matrix
// by 90 degrees in anti-clockwise direction
void rotate_matrix(byte mat[BLOCK_SIZE][BLOCK_SIZE]) {

    // Consider all squares one by one
    for (byte x = 0; x < 2; x++) {
        // Consider elements in group of 4 in
        // current square
        for (byte y = x; y < BLOCK_SIZE - x - 1; y++) {
            // store current cell in temp variable
            byte temp = mat[x][y];

            // move values from right to top
            mat[x][y] = mat[y][BLOCK_SIZE - 1 - x];

            // move values from bottom to right
            mat[y][BLOCK_SIZE - 1 - x] = mat[BLOCK_SIZE - 1 - x][BLOCK_SIZE - 1 - y];

            // move values from left to bottom
            mat[BLOCK_SIZE - 1 - x][BLOCK_SIZE - 1 - y] = mat[BLOCK_SIZE - 1 - y][x];

            // assign temp to left
            mat[BLOCK_SIZE - 1 - y][x] = temp;
        }
    }
}

void rotate_player() {
    byte window[BLOCK_SIZE][BLOCK_SIZE];
    int i, j, ii, jj;

    if (current_block == O)
        return;
    for (i = 0, ii = p_i; i < BLOCK_SIZE; i++, ii++) {
        for (j = 0, jj = p_j; j < BLOCK_SIZE; j++, jj++) {
            if ((jj >= 0) && (jj < DISPLAY_COLUMNS)) {
                window[i][j] = display[ii][jj];
            } else {
                window[i][j] = 0;
            }
        }
    }

    rotate_matrix(window);
    TMR2IE = 0;
    for (i = 0, ii = p_i; i < BLOCK_SIZE; i++, ii++) {
        for (j = 0, jj = p_j; j < BLOCK_SIZE; j++, jj++) {
            if ((jj >= 0) && (jj < DISPLAY_COLUMNS)) {
                display[ii][jj] = window[i][j];
            }
        }
    }
    TMR2IE = 1;
}

void spawn_block() {
    current_block = rand() % BLOCK_NUM;

    switch (current_block) {
    case I:
        insert_block(BLOCK_I);
        break;
    case T:
        insert_block(BLOCK_T);
        break;
    case S:
        insert_block(BLOCK_S);
        break;
    case L1:
        insert_block(BLOCK_L1);
        break;
    case Z:
        insert_block(BLOCK_Z);
        break;
    case O:
        insert_block(BLOCK_O);
        break;
    case L2:
        insert_block(BLOCK_L2);
        break;
    }
}

void move_player_to_left(void) {
    TMR2IE = 0; // disable timer
    for (byte i = 0; i < DISPLAY_ROWS; i++) {
        for (byte j = 0; j < DISPLAY_COLUMNS; j++) {
            if (j - 1 >= 0 && display[i][j] == P) {
                display[i][j - 1] = display[i][j];
                display[i][j] = 0;
            } else {
                display[i][j] = display[i][j];
            }
        }
    }
    p_j--;
    TMR2IE = 1; // enable timer
}

void move_player_to_right(void) {
    TMR2IE = 0;
    for (byte i = 0; i < DISPLAY_ROWS; i++) {
        byte new_row[DISPLAY_COLUMNS];
        for (byte j = 0; j < DISPLAY_COLUMNS; j++) {
            new_row[j] = OFF;
        }
        for (byte j = 0; j < DISPLAY_COLUMNS; j++) {
            if (j + 1 < DISPLAY_COLUMNS && display[i][j] == P) {
                new_row[j + 1] = display[i][j];
            } else if (new_row[j] != P) {
                new_row[j] = display[i][j];
            }
        }

        for (byte j = 0; j < DISPLAY_COLUMNS; j++) {
            display[i][j] = new_row[j];
        }
    }
    p_j++;
    TMR2IE = 1;
}

void seed_updt(byte val, byte refresh) {
    seed[seed_idx] = val;
    seed_idx = ++seed_idx & 0x03;

    long *seed_ptr = (long *)seed;

    if (refresh)
        srand(*seed_ptr);
}

void kbd_manager(void) {

    static byte key_ant = KEY_NONE;
    static byte key = KEY_NONE;
    static byte key_cmd = KEY_NONE;

    static byte key_tmr1 = 0, key_tmr2 = 0;
    static bit key_flag;

    byte aux = PORTB & 0x07;

    if (aux == key_ant) {
        if (++key_tmr1 > DELAY_KEY_DEBOUNCE) {
            key_tmr1 = DELAY_KEY_DEBOUNCE;
            key = aux;
        } else {
            key = KEY_NONE;
        }
    } else {
        key_tmr1 = 0;
        key = KEY_NONE;
    }

    key_ant = aux;

    if (!key) {
        key_flag = 1;
        key_tmr2 = 0;
        key_cmd = KEY_NONE;
    } else if (key_flag) {
        seed_updt(TMR2, 0);
        key_cmd = key;
        key_flag = 0;
    } else if (++key_tmr2 > DELAY_KEY_REPEAT) {
        key_tmr2 = 0;
        key_cmd = key;
    } else
        key_cmd = KEY_NONE;

    switch (key_cmd) {
    case KEY_LEFT:
        if (!(collision_state & COLLISION_LEFT)) {
            move_player_to_left();
        }
        break;

    case KEY_RIGHT:
        if (!(collision_state & COLLISION_RIGHT)) {
            move_player_to_right();
        }
        break;

    case KEY_ROTATE:

        if (!(collision_state & (COLLISION_LEFT | COLLISION_RIGHT))) {
            rotate_player();
        }
        break;
    }

    key_cmd = KEY_NONE;
}

void game_manager() {
    static byte i, j;
    static byte one_time = 1;

    // main logic
    switch (game_state) {
    case 0:
        spawn_block();
        game_state = 1;
        break;

    case 1:
        if (check_game_over()) { // implies COLLISION_BOTTOM && p_i == 0
            i = 0;
            j = 0;
            seed_updt(TMR2, 1);
            timer = DELAY_TICK_GAME_OVER;
            game_state = 3;

        } else if (collision_state & COLLISION_BOTTOM) {
            seed_updt(TMR2, one_time);
            one_time = 0;
            freeze_blocks();
            clean_full_rows();
            game_state = 0;
        } else {
            fall_one_row();
            timer = DELAY_FALL;
            game_state = 2;
        }
        break;

    case 2:
        if (!timer) {
            game_state = 1;
        }
        break;

    case 3:
        if (!timer) {
            timer = DELAY_TICK_GAME_OVER;
            TMR2IE = 0;
            display[i][j] = ON;
            TMR2IE = 1;
            j++;
            if (j == DISPLAY_COLUMNS) {
                j = 0;
                i++;
                if (i == DISPLAY_ROWS) {
                    i = 0;
                    clear_display();
                    game_state = 0;
                }
            }
        }
        break;
    }
}

void init(void) {
    DISPLAY_ROW_PORT = 0;
    INHIBIT_PIN = 1;
    TRISD = 0;
    TRISA = 0xf0;
    TRISBbits.RB0 = 1;
    TRISBbits.RB1 = 1;
    TRISBbits.RB2 = 1;
    T2CONbits.T2CKPS = 0b00;   // PRESCALER
    T2CONbits.TOUTPS = 0b0011; // POSTSCALER
    PR2 = DELAY_TICK;
    TMR2IE = 1; // Ativa interrupção do timer 2
    row = 0;
    col = 0;
    state = 0;
    game_state = 1;
    timer = 0;
    INTCONbits.GIE = 1;   // Habilita interrupções globalmente
    INTCONbits.PEIE = 1;  // Habilita int. dos periféricos
    T2CONbits.TMR2ON = 1; // Ativa contagem do timer 2

    for (byte i = 0; i < 4; i++) {
        __delay_us(10);
        seed_updt(TMR2, 0);
    }
}

int main(void) {

    init();
    clear_display();
    seed_updt(TMR2, 1);
    spawn_block();

    while (1) {
        __delay_ms(1);
        if (state != 2)
            collision_state = check_collision();

        switch (state) {
        case 0:
            kbd_manager();
            state++;
            break;

        case 1:
            game_manager();
            state++;
            break;

        case 2:
            if (timer)
                timer--;
            state = 0;
            break;
        }
    }
    return 0;
}

void interrupt isr(void) {
    if (TMR2IF) {
        TMR2IF = 0;
        INHIBIT_PIN = 1;
        DISPLAY_ROW_PORT = row;
        if (display[row][col] != OFF) {
            DISPLAY_COLUMNS_PORT = ((row & 0b00001000) | col);
            INHIBIT_PIN = 0;
        }
        col = (col + 1) & (DISPLAY_COLUMNS - 1);
        if (col == 0) {
            row = (row + 1) & (DISPLAY_ROWS - 1);
        }
    }
}
