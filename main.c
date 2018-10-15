/*
 * File:   main.c
 * Author: lerax
 * Project: Tetris in PIC
 *
 * Sun 07 Oct 2018 07:01:09 PM -03
 */

#include "18f4550.h"
#include <xc.h>

#define _XTAL_FREQ 4000000
#define DISPLAY_ROWS 16
#define DISPLAY_COLUMNS 8
#define DISPLAY_COLUMNS_PORT PORTD
#define DISPLAY_ROW_PORT PORTA
#define DELAY_TICK 1
#define DELAY_FALL 20


typedef unsigned char byte;

typedef byte Piece[4][4];

enum PixelState {
    OFF,
    ON,
    P, // player
    C, // center
};

Piece PIECE_T = {
    {0, P, C, P},
    {0, 0, P, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
};

Piece PIECE_L1 = {
    {0, P, 0, 0},
    {0, C, 0, 0},
    {0, P, P, 0},
    {0, 0, 0, 0},
};

Piece PIECE_L2 = {
    {0, 0, P, 0},
    {0, 0, C, 0},
    {0, P, P, 0},
    {0, 0, 0, 0},
};

Piece PIECE_O = {
    {0, P, P, 0},
    {0, P, P, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
};

Piece PIECE_S = {
    {0, P, 0, 0},
    {0, C, P, 0},
    {0, 0, P, 0},
    {0, 0, 0, 0},
};

Piece PIECE_Z = {
    {0, 0, P, 0},
    {0, P, C, 0},
    {0, P, 0, 0},
    {0, 0, 0, 0},
};

Piece PIECE_I = {
    {0, P, 0, 0},
    {0, C, 0, 0},
    {0, P, 0, 0},
    {0, P, 0, 0},
};

Piece PIECE_FULL = {
    {1, 1, 1, 1},
    {1, 1, 1, 1},
    {1, 1, 1, 1},
    {1, 1, 1, 1}
};


byte display[16][8];

byte array_to_byte(byte array[DISPLAY_COLUMNS]) {
    byte result = 0;
    byte i;
    for (i = 0; i < DISPLAY_COLUMNS; i++) {
        if (array[i] != OFF) {
            result += 1 << ((DISPLAY_COLUMNS - 1) - i);
        }
    }
    return result;
}

void draw() {
    byte i;
    for (i = 0; i < DISPLAY_ROWS; i++) {
        DISPLAY_ROW_PORT = i;
        DISPLAY_COLUMNS_PORT = array_to_byte(display[i]);
        __delay_ms(DELAY_TICK);
        DISPLAY_COLUMNS_PORT = 0;
    }
}

void set_display(byte value) {
    byte i, j;
    for (i = 0; i < DISPLAY_ROWS; i++) {
        for (j = 0; j < DISPLAY_COLUMNS; j++) {
            display[i][j] = value;
        }
    }
}

void fill_display() {
    set_display(1);
}

void clear_display() {
    set_display(0);
}

// a1 <- a2
void copy_player(byte a1[DISPLAY_COLUMNS], byte a2[DISPLAY_COLUMNS]) {
    byte i;
    for (i = 0; i < DISPLAY_COLUMNS; i++) {
        if (a1[i] != ON && a2[i] != ON) {
            a1[i] = a2[i];
        }
    }
}

int check_collision(byte a1[DISPLAY_COLUMNS], byte a2[DISPLAY_COLUMNS]) {
    byte i;
    for (i = 0; i < DISPLAY_COLUMNS; i++) {
        if ((a1[i] == C || a1[i] == P) && a2[i] == ON) {
            return 1;
        }
    }
    return 0;
}

void freeze_blocks() {
    byte i, j;
    for (i = 0; i < DISPLAY_ROWS; i++) {
        for (j = 0; j < DISPLAY_COLUMNS; j++) {
            if (display[i][j] == P || display[i][j] == C) {
                display[i][j] = ON;
            }
        }
    }
}

int has_player(byte row[DISPLAY_COLUMNS]) {
    byte i;
    for (i = 0; i < DISPLAY_COLUMNS; i++) {
        if (row[i] == P || row[i] == C) {
            return 1;
        }
    }
    return 0;
}

void fall_one_row() {
    byte last_row[DISPLAY_COLUMNS];
    byte current_row[DISPLAY_COLUMNS];
    byte zero_row[DISPLAY_COLUMNS] = {0, 0, 0, 0, 0, 0, 0, 0};
    byte i;
    copy_player(last_row, display[0]);
    copy_player(display[0], zero_row);
    for (i = 1; i < DISPLAY_ROWS; i++) {
        copy_player(current_row, display[i]);
        copy_player(display[i], last_row);
        copy_player(last_row, current_row);
    }

    for (i = 0; i < DELAY_FALL; i++) {
        draw();
    }

}

byte check_fall_collision() {
    byte i;
    for (i = 1; i < DISPLAY_ROWS; i++) {
        if ((i + 1) == DISPLAY_ROWS && has_player(display[i])) {
            return 1;
        } else if (check_collision(display[i], display[i+1])) {
            return 1;
        }
    }

    return 0;
}

void insert_piece(Piece p) {
    byte i, j;
    byte prefix = 2;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            display[i][j+prefix] = p[i][j];
        }
    }
}

void rotate_piece(Piece p) {

}

void spawn_piece() {
    static int counter = 1;
    switch (counter % 7) {
    case 0:
        insert_piece(PIECE_I);
        break;
    case 1:
        insert_piece(PIECE_T);
        break;
    case 2:
        insert_piece(PIECE_S);
        break;
    case 3:
        insert_piece(PIECE_L1);
        break;
    case 4:
        insert_piece(PIECE_Z);
        break;
    case 5:
        insert_piece(PIECE_O);
        break;
    case 6:
        insert_piece(PIECE_L2);
        break;
    }
    counter++;
}
}

int main(void) {
    TRISD = 0;
    TRISA = 0xf0;
    DISPLAY_COLUMNS_PORT = 0;
    DISPLAY_ROW_PORT = 0;
    clear_display();

    byte i;
    spawn_piece();
    while (1) {
        draw();
        if (check_fall_collision()) {
            freeze_blocks();
            spawn_piece();
        } else {
            fall_one_row();
        }
    }

    return 0;
}
