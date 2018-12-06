/*
 * File:   main.c
 * Author: lerax and mmssouza
 * Project: Tetris in PIC
 *
 * Sun 07 Oct 2018 07:01:09 PM -03
 */

#include "18f4550.h"
#include <xc.h>

#define _XTAL_FREQ 16000000
#define DISPLAY_ROWS 16
#define DISPLAY_COLUMNS 8
#define BLOCK_SIZE 3
#define DISPLAY_COLUMNS_PORT PORTD
#define INHIBIT_PIN   PORTDbits.RD4
#define DISPLAY_ROW_PORT PORTA
#define DELAY_TICK 250
#define DELAY_TICK_GAME_OVER 10
#define DELAY_FALL 50


typedef unsigned char byte;

typedef byte Block[BLOCK_SIZE][BLOCK_SIZE];

// global variables
byte volatile display[DISPLAY_ROWS][DISPLAY_COLUMNS];
byte row,col;
int p_i,p_j;
volatile byte key;
byte state,game_state,collision_state;
unsigned int timer;

enum PixelState {
    OFF = 0,
    ON,
    P, // player
};

enum KeyState {
    KEY_NONE = 0,
    KEY_RIGHT,
    KEY_LEFT,
    KEY_ROTATE,
};

enum Collisions {
    COLLISION_NONE = 0x00,
    COLLISION_LEFT = 0x01,
    COLLISION_RIGHT = 0x02,
    COLLISION_BOTTOM = 0x04,
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

inline void disable_interrupt() {
    INTCONbits.GIE = 0;
}

inline void enable_interrupt() {
    INTCONbits.GIE = 1;
}

void set_display(byte value) {
    TMR2IE = 0;
    for (byte i = 0; i < DISPLAY_ROWS; i++) {
        for (byte j = 0; j < DISPLAY_COLUMNS; j++) {
            display[i][j] = value;
        }
    }
    TMR2IE = 1;
}

inline void fill_display() {
    set_display(1);
}

inline void clear_display() {
    set_display(0);
}

// a1 <- a2
void copy_array(byte volatile a1[DISPLAY_COLUMNS], byte volatile a2[DISPLAY_COLUMNS]) {
    for (byte i = 0; i < DISPLAY_COLUMNS; i++) {
        a1[i] = a2[i];
    }
}


byte check_collision() {
    byte collision = COLLISION_NONE;

    for (int i = p_i;i < p_i + BLOCK_SIZE;i++) {
        for (int j = p_j;j <  p_j + BLOCK_SIZE; j++) {
            if (i < DISPLAY_ROWS ) {
                if ((j >=  0) && (j < DISPLAY_COLUMNS)) {
                    if (display[i][j] == P) {
                        // collision BOTTOM
                        if (i == DISPLAY_ROWS-1) {
                            collision |= COLLISION_BOTTOM;
                        }
                        else if (display[i+1][j] ==  ON) {
                            collision |= COLLISION_BOTTOM;
                        }
                        // collision LEFT and collision RIGHT
                        if (j == DISPLAY_COLUMNS - 1) {
                            collision |= COLLISION_RIGHT;
                            if (display[i][j-1] ==  ON) {
                                collision |= COLLISION_LEFT;
                            }
                        }
                        else if (j == 0) {
                            collision |= COLLISION_LEFT;
                            if (display[i][j+1] ==  ON) {
                                collision |= COLLISION_RIGHT;
                            }
                        }
                        else {
                            if (display[i][j+1] ==  ON) {
                                collision |= COLLISION_RIGHT;
                            }
                            if (display[i][j-1] ==  ON) {
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
    for (int i = p_i + BLOCK_SIZE - 1;i > p_i - 1; i--) {
        for (int j = p_j;j <  p_j+BLOCK_SIZE; j++) {
            if ((j >=  0) && (j < DISPLAY_COLUMNS)) {
                if (display[i+1][j] != ON) {
                    display[i+1][j] = display[i][j];
                }
                if (i == p_i) {
                    display[i][j] = 0;
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
    byte last_row[DISPLAY_COLUMNS];
    byte current_row[DISPLAY_COLUMNS];
    byte zero_row[DISPLAY_COLUMNS] = {0, 0, 0, 0, 0, 0, 0, 0};

    copy_array(last_row, display[0]);
    copy_array(display[0], zero_row);
    for (byte i = 1; i <= n; i++) {
        copy_array(current_row, display[i]);
        copy_array(display[i], last_row);
        copy_array(last_row, current_row);
    }
}

void clean_full_rows() {
    TMR2IE = 0; // disable timer
    for (byte i = 0; i < DISPLAY_ROWS; i++) {
        if (full_row(display[i])) {
            fall_row_until(i);
        }
    }
    TMR2IE = 1; // enable timer
}

void insert_block(Block const p) {
    TMR2IE = 0; // disable timer
    p_i = 0; p_j = 2;
    for (byte i = 0; i < BLOCK_SIZE; i++) {
        for (byte j = 0; j < BLOCK_SIZE; j++) {
            display[i+p_i][j+p_j] = p[i][j];
        }
    }
    TMR2IE = 1; // enable timer
}

int check_game_over() {
    byte prefix = 2;
    for (byte i = 0; i < BLOCK_SIZE; i++) {
        for (byte j = 0; j < BLOCK_SIZE; j++) {
            if (display[i][j+prefix] == ON) {
                return 1;
            }
        }
    }
    return 0;
}

// An Inplace function to rotate a N x N matrix
// by 90 degrees in anti-clockwise direction
void rotate_matrix(byte mat[BLOCK_SIZE][BLOCK_SIZE])
{

    // Consider all squares one by one
    for (byte x = 0; x < 2; x++)
    {
        // Consider elements in group of 4 in
        // current square
        for (byte y = x; y < BLOCK_SIZE-x-1; y++)
        {
            // store current cell in temp variable
            byte temp = mat[x][y];

            // move values from right to top
            mat[x][y] = mat[y][BLOCK_SIZE-1-x];

            // move values from bottom to right
            mat[y][BLOCK_SIZE-1-x] = mat[BLOCK_SIZE-1-x][BLOCK_SIZE-1-y];

            // move values from left to bottom
            mat[BLOCK_SIZE-1-x][BLOCK_SIZE-1-y] = mat[BLOCK_SIZE-1-y][x];

            // assign temp to left
            mat[BLOCK_SIZE-1-y][x] = temp;
        }
    }
}

void rotate_player() {
    byte window[BLOCK_SIZE][BLOCK_SIZE];
    int i,j,ii,jj;
    for (i = 0, ii = p_i; i < BLOCK_SIZE; i++,ii++) {
        for (j = 0, jj = p_j; j < BLOCK_SIZE; j++,jj++) {
            if ((jj >=  0) && (jj < DISPLAY_COLUMNS)) {
                window[i][j] = display[ii][jj];
            }
            else {
                window[i][j] = 0;
            }
        }
    }

    rotate_matrix(window);

    for (i = 0, ii = p_i; i < BLOCK_SIZE; i++,ii++) {
        for (j = 0, jj = p_j; j < BLOCK_SIZE; j++,jj++) {
            if ((jj >=  0) && (jj < DISPLAY_COLUMNS)) {
                display[ii][jj] = window[i][j];
            }
        }
    }
}

void spawn_block() {
    static int counter = 1;
    switch (counter % 7) {
    case 0:
        insert_block(BLOCK_I);
        break;
    case 1:
        insert_block(BLOCK_T);
        break;
    case 2:
        insert_block(BLOCK_S);
        break;
    case 3:
        insert_block(BLOCK_L1);
        break;
    case 4:
        insert_block(BLOCK_Z);
        break;
    case 5:
        insert_block(BLOCK_O);
        break;
    case 6:
        insert_block(BLOCK_L2);
        break;
    }
    counter++;
}

void move_player_to_left(void) {
    TMR2IE = 0; // disable timer
    for (byte i = 0; i < DISPLAY_ROWS; i++) {
        for (byte j = 0; j < DISPLAY_COLUMNS; j++) {
            if (j - 1 >= 0 && display[i][j] == P) {
                display[i][j-1] = display[i][j];
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
            if (j+1 < DISPLAY_COLUMNS && display[i][j] == P) {
                new_row[j+1] = display[i][j];
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


void interrupt isr(void) {
    if (TMR2IF) {
        TMR2IF = 0;
        INHIBIT_PIN = 1;
        DISPLAY_ROW_PORT = row;
        if (display[row][col] != OFF ) {
            DISPLAY_COLUMNS_PORT =  ((row & 0b00001000) |  col);
            INHIBIT_PIN = 0;
        }
        col =  (col + 1) &  (DISPLAY_COLUMNS - 1);
        if (col == 0) {
            row = (row + 1) & (DISPLAY_ROWS - 1);
        }
    } else if (INT0F) { // left button
        INT0F = 0;
        key = KEY_LEFT;

    } else if (INT1F) { // right button
        INT1F = 0;
        key = KEY_RIGHT;
    } else if (INT2F) {
        INT2F = 0;
        key = KEY_ROTATE;
    }
}

void kbd_manager(void) {
    byte key_aux = key;

    switch (key_aux) {
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
        //if (!(collision_state & (COLLISION_RIGHT|COLLISION_LEFT)))
        rotate_player();
        break;

    }

    if (key_aux  == key) {
        key = KEY_NONE;
    }
}

void game_manager() {
    static byte i, j;

    // main logic
    switch (game_state) {
    case 0:
        spawn_block();
        game_state = 1;
        break;

    case 1:
        if (collision_state & COLLISION_BOTTOM) {
            freeze_blocks();
            clean_full_rows();
            if (check_game_over()) {
                i = 0; j = 0;
                timer = DELAY_TICK_GAME_OVER;
                game_state = 3;
            } else {
                game_state = 0;
            }
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
        if  (!timer) {
            timer = DELAY_TICK_GAME_OVER;
            TMR2IE = 0;
            display[i][j] = ON;
            TMR2IE = 1;
            j++;
            if  (j  == DISPLAY_COLUMNS) {
                j = 0;
                i++;
                if  (i == DISPLAY_ROWS) {
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
    T2CONbits.T2CKPS = 0b00;   // PRESCALER
    T2CONbits.TOUTPS = 0b0011; // POSTSCALER
    PR2 = DELAY_TICK;
    INTCONbits.INT0IE = 1;  // Habilita interrupção INT0
    INTCON3bits.INT1IE = 1; // Habilita interrupção INT1
    INTCON3bits.INT2IE = 1; // Habilitar interrupção INT2
    TMR2IE = 1; // Ativa interrupção do timer 2
    key = KEY_NONE;
    row = 0;
    col = 0;
    state = 0;
    game_state = 1;
    timer = 0;
    T2CONbits.TMR2ON = 1; // Ativa contagem do timer 2
    INTCONbits.GIE = 1;     // Habilita interrupções globalmente
    INTCONbits.PEIE = 1;    // Habilita int. dos periféricos

}


int main(void) {

    init();
    clear_display();
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
            if (timer) timer--;
            state = 0;
            break;
        }
    }
    return 0;
}
