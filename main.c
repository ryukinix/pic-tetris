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
#define DELAY_TICK_GAME_OVER 5
#define DELAY_FALL 15


typedef unsigned char byte;

typedef byte Piece[4][4];

enum PixelState {
    OFF,
    ON,
    P, // player
};

Piece PIECE_T = {
    {0, P, P, P},
    {0, 0, P, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
};

Piece PIECE_L1 = {
    {0, P, 0, 0},
    {0, P, 0, 0},
    {0, P, P, 0},
    {0, 0, 0, 0},
};

Piece PIECE_L2 = {
    {0, 0, P, 0},
    {0, 0, P, 0},
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
    {0, P, P, 0},
    {0, 0, P, 0},
    {0, 0, 0, 0},
};

Piece PIECE_Z = {
    {0, 0, P, 0},
    {0, P, P, 0},
    {0, P, 0, 0},
    {0, 0, 0, 0},
};

Piece PIECE_I = {
    {0, P, 0, 0},
    {0, P, 0, 0},
    {0, P, 0, 0},
    {0, P, 0, 0},
};

Piece PIECE_FULL = {
    {1, 1, 1, 1},
    {1, 1, 1, 1},
    {1, 1, 1, 1},
    {1, 1, 1, 1}
};


static byte display[DISPLAY_ROWS][DISPLAY_COLUMNS];

void disable_interrupt(void) {
    INTCONbits.GIE = 0;
}

void enable_interrupt(void) {
    INTCONbits.GIE = 1;
}

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

void copy_array(byte a1[DISPLAY_COLUMNS], byte a2[DISPLAY_COLUMNS]) {
    byte i;
    for (i = 0; i < DISPLAY_COLUMNS; i++) {
        a1[i] = a2[i];
    }
}


int check_collision(byte a1[DISPLAY_COLUMNS], byte a2[DISPLAY_COLUMNS]) {
    byte i;
    for (i = 0; i < DISPLAY_COLUMNS; i++) {
        if (a1[i] == P && a2[i] == ON) {
            return 1;
        }
    }
    return 0;
}

void freeze_blocks() {
    byte i, j;
    for (i = 0; i < DISPLAY_ROWS; i++) {
        for (j = 0; j < DISPLAY_COLUMNS; j++) {
            if (display[i][j] != OFF) {
                display[i][j] = ON;
            }
        }
    }
}

int has_player(byte row[DISPLAY_COLUMNS]) {
    byte i;
    for (i = 0; i < DISPLAY_COLUMNS; i++) {
        if (row[i] == P) {
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
    disable_interrupt();
    copy_player(last_row, display[0]);
    copy_player(display[0], zero_row);
    for (i = 1; i < DISPLAY_ROWS; i++) {
        copy_player(current_row, display[i]);
        copy_player(display[i], last_row);
        copy_player(last_row, current_row);
    }
    enable_interrupt();

    for (i = 0; i < DELAY_FALL; i++) {
        draw();
    }

}

int full_row(byte row[DISPLAY_COLUMNS]) {
    int j;
    for (j = 0; j < DISPLAY_COLUMNS; j++) {
        if (row[j] == OFF) {
            return 0;
        }
    }
    return 1;
}

void fall_row_until(int n) {
    byte last_row[DISPLAY_COLUMNS];
    byte current_row[DISPLAY_COLUMNS];
    byte zero_row[DISPLAY_COLUMNS] = {0, 0, 0, 0, 0, 0, 0, 0};
    byte i;
    copy_array(last_row, display[0]);
    copy_array(display[0], zero_row);
    for (i = 1; i <= n; i++) {
        copy_array(current_row, display[i]);
        copy_array(display[i], last_row);
        copy_array(last_row, current_row);
    }
}

void clean_full_rows() {
    int i;
    for (i = 0; i < DISPLAY_ROWS; i++) {
        if (full_row(display[i])) {
            fall_row_until(i);
        }
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

int check_game_over() {
    int i, j;
    byte prefix = 2;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (display[i][j+prefix] == ON) {
                return 1;
            }
        }
    }
    return 0;
}

void game_over_animation() {
    int i, j;

    for(i = 0; i < DISPLAY_ROWS; i++) {
        for (j = 0; j < DISPLAY_COLUMNS; j++) {
            display[i][j] = ON;
            draw();
            __delay_ms(DELAY_TICK_GAME_OVER);
        }
    }
}

void rotate_player() {
    byte player_pieces[4][2];
    byte window[4][4];
    int piece_count = 0;
    int i, j;

    // initialize window and window_coordinantes to 0
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            window[i][j] = 0;
        }
    }

    // search for player pieces
    for (i = 0; i < DISPLAY_ROWS; i++) {
        for (j = 0; j < DISPLAY_COLUMNS; j++) {
            if (display[i][j] == P) {
                player_pieces[piece_count][0] = i;
                player_pieces[piece_count][1] = j;
                piece_count++;
            }
        }
    }

    // get window of pieces {4,4}
    int min_i = player_pieces[0][0];
    int min_j = player_pieces[0][1];
    for (i = 1; i < 4; i++) {
        if (player_pieces[i][0] < min_i) {
            min_i = player_pieces[i][0];
        }
        if (player_pieces[i][1] < min_j) {
            min_j = player_pieces[i][1];
        }
    }


    for (i = 0; i < 4; i++) {
        int p_i = player_pieces[i][0];
        int p_j = player_pieces[i][1];
        int w_i = p_i - min_i;
        int w_j = p_j - min_j;
        window[w_i][w_j] = display[p_i][p_j];
    }


    // write window back transposed
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            display[i+min_i][j+min_j] = window[j][i];
        }
    }
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

byte check_left_collision(void) {
    int i, j;
    for (i = 0; i < DISPLAY_ROWS; i++) {
        for (j = 0; j < DISPLAY_COLUMNS; j++) {
            if (display[i][j] == P) {
                if (j == 0) {
                    return 1;
                } else if (display[i][j-1] == ON) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

byte check_right_collision(void) {
    int i, j;
    for (i = 0; i < DISPLAY_ROWS; i++) {
        for (j = 0; j < DISPLAY_COLUMNS; j++) {
            if (display[i][j] == P) {
                if (j == DISPLAY_COLUMNS - 1) {
                    return 1;
                } else if (display[i][j+1] == ON) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

void move_player_to_left(void) {
    int i, j;
    for (i = 0; i < DISPLAY_ROWS; i++) {
        for (j = 0; j < DISPLAY_COLUMNS; j++) {
            if (j-1 >= 0 && display[i][j] == P) {
                display[i][j-1] = display[i][j];
                display[i][j] = 0;
            } else {
                display[i][j] = display[i][j];
            }
        }
    }
}

void move_player_to_right(void) {
    int i, j;
    for (i = 0; i < DISPLAY_ROWS; i++) {
        byte new_row[DISPLAY_COLUMNS];
        for (j = 0; j < DISPLAY_COLUMNS; j++) {
            new_row[j] = OFF;
        }
        for (j = 0; j < DISPLAY_COLUMNS; j++) {
            if (j+1 < DISPLAY_COLUMNS && display[i][j] == P) {
                new_row[j+1] = display[i][j];
            } else if (new_row[j] != P) {
                new_row[j] = display[i][j];
            }
        }

        for (j = 0; j < DISPLAY_COLUMNS; j++) {
            display[i][j] = new_row[j];
        }
    }
}

/**
 * DANGER: There is a re-entrant problem in this routine.
 *         move_player_to_{left, right} procedures changes the
 *         global variable display (which is changed by other procedures)
 */
void interrupt isr(void) {
    if (INT0F) { // left button
        INT0F = 0;
        if (!check_left_collision()) {
            move_player_to_left();
        }
    } else if (INT1F) { // right button
        INT1F = 0;
        if (!check_right_collision()) {
            move_player_to_right();
        }
    } else if (INT2F) {
        INT2F = 0;
        rotate_player();
    }
}

void start_game() {
    clear_display();

    spawn_piece();
    while (1) {
        // main logic
        draw();
        if (check_fall_collision()) {
            freeze_blocks();
            clean_full_rows();
            if (check_game_over()) {
                game_over_animation();
                clear_display();
            }
            spawn_piece();
        } else {
            fall_one_row();
        }
    }
}

int main(void) {
    TRISD = 0;
    TRISA = 0xf0;
    TRISBbits.RB0 = 1;
    TRISBbits.RB1 = 1;
    INTCONbits.INT0IE = 1;  // Habilita interrupção INT0
    INTCON3bits.INT1IE = 1; // Habilita interrupção INT1
    INTCON3bits.INT2IE = 1; // Habilitar interrupção INT2
    INTCONbits.GIE = 1;     // Habilita interrupções globalmente
    INTCONbits.PEIE = 1;    // Habilita int. dos periféricos
    DISPLAY_COLUMNS_PORT = 0;
    DISPLAY_ROW_PORT = 0;

    start_game();
    return 0;
}
