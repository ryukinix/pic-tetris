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
#define DELAY_FALL 50

typedef unsigned char byte;

typedef byte piece[4][4];

enum PixelState {
	 OFF,
	 ON,
	 PLAYER
};

byte PIECE_T[4][4] = {
	 {0, 1, 1, 1},
	 {0, 0, 1, 0},
	 {0, 0, 0, 0},
	 {0, 0, 0, 0},
};

byte PIECE_L1[4][4] = {
	 {0, 1, 0, 0},
	 {0, 1, 0, 0},
	 {0, 1, 1, 0},
	 {0, 0, 0, 0},
};

byte PIECE_L2[4][4] = {
	 {0, 0, 1, 0},
	 {0, 0, 1, 0},
	 {0, 1, 1, 0},
	 {0, 0, 0, 0},
};

byte PIECE_O[4][4] = {
	 {0, 1, 1, 0},
	 {0, 1, 1, 0},
	 {0, 0, 0, 0},
	 {0, 0, 0, 0},
};

byte PIECE_S[4][4] = {
	 {0, 1, 0, 0},
	 {0, 1, 1, 0},
	 {0, 0, 1, 0},
	 {0, 0, 0, 0},
};

byte PIECE_Z[4][4] = {
	 {0, 0, 1, 0},
	 {0, 1, 1, 0},
	 {0, 1, 0, 0},
	 {0, 0, 0, 0},
};

byte PIECE_I[4][4] = {
	 {0, 1, 0, 0},
	 {0, 1, 0, 0},
	 {0, 1, 0, 0},
	 {0, 1, 0, 0},
};

byte PIECE_FULL[4][4] = {
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
		  result += array[i] << ((DISPLAY_COLUMNS - 1) - i);
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
void copy_array(byte a1[DISPLAY_COLUMNS], byte a2[DISPLAY_COLUMNS]) {
	 byte i;
	 for (i = 0; i < DISPLAY_COLUMNS; i++) {
		  a1[i] = a2[i];
	 }
}

void fall_one_row() {
	 byte last_row[DISPLAY_COLUMNS];
	 byte current_row[DISPLAY_COLUMNS];
	 byte zero_row[DISPLAY_COLUMNS] = {0, 0, 0, 0, 0, 0, 0, 0};
	 byte i;
	 copy_array(last_row, display[0]);
	 copy_array(display[0], zero_row);
	 for (i = 1; i < DISPLAY_ROWS; i++) {
		  copy_array(current_row, display[i]);
		  copy_array(display[i], last_row);
		  copy_array(last_row, current_row);
	 }

	 for (i = 0; i < DELAY_FALL; i++) {
		  draw();
	 }
}

void insert_piece(byte p[4][4]) {
	 byte i, j;
	 byte prefix = 2;
	 for (i = 0; i < 4; i++) {
		  for (j = 0; j < 4; j++) {
			   display[i][j+prefix] = p[i][j];
		  }
	 }
}

void rotate_piece(piece p) {

}

void spawn_piece() {
	 static int counter = 0;
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

int main(void) {
	 TRISD = 0;
	 TRISA = 0xf0;
	 DISPLAY_COLUMNS_PORT = 0;
	 DISPLAY_ROW_PORT = 0;
	 clear_display();
	 int i;

	 while (1) {
		  spawn_piece();
		  draw();
		  for (i = 0; i < DISPLAY_COLUMNS; i++) {
			   fall_one_row();
		  }
	 }

	 return 0;
}
