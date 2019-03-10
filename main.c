#include <SDL/SDL.h>
#include <stdlib.h>


#define DISPLAY_ROWS 32
#define DISPLAY_COLUMNS 16
#define BLOCK_SIZE 3
#define DELAY_TICK_GAME_OVER 1
#define DELAY_FALL 3
#define BLOCK_NUM   7

typedef unsigned int Block[BLOCK_SIZE][BLOCK_SIZE];

// global variables
unsigned int display[DISPLAY_ROWS][DISPLAY_COLUMNS];
unsigned int row,col;
int p_i,p_j;
unsigned int state,game_state,collision_state;
unsigned int timer;
unsigned int current_block;
SDL_Surface *screen, *brick; // A janela principal
unsigned int done;

enum PixelState {
    OFF = 0,
    ON,
    P, // player
};


enum Collisions {
    COLLISION_NONE = 0x00,
    COLLISION_LEFT = 0x01,
    COLLISION_RIGHT = 0x02,
    COLLISION_BOTTOM = 0x04,
};

enum Block_id {T, L1, L2, O, S, Z, I,};

Block BLOCK_T = {
    {0, 0, 0},
    {0, P, 0},
    {P, P, P},
};

Block BLOCK_L1 = {
    { P, 0, 0},
    { P, 0, 0},
    { P, P, 0},
};

Block BLOCK_L2 = {
    {0, P, 0},
    {0, P, 0},
    {P, P, 0},
};

Block BLOCK_O = {
    {0, 0, 0},
    {P, P, 0},
    {P, P, 0},
};

Block BLOCK_S = {
    {0, 0, 0},
    {P, P, 0},
    {0, P, P},
};

Block BLOCK_Z = {
    {0, 0, 0},
    {0, P, P},
    {P, P, 0},};

Block BLOCK_I = {
    {0, P, 0},
    {0, P, 0},
    {0, P, 0},
};

void set_display(unsigned int value) {
    for (unsigned int i = 0; i < DISPLAY_ROWS; i++) {
        for (unsigned int j = 0; j < DISPLAY_COLUMNS; j++) {
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

unsigned int check_collision() {
    unsigned int collision = COLLISION_NONE;

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
                        // collision LEFT and collision WALL (RIGHT)
                        if (j == DISPLAY_COLUMNS - 1) {
                            collision |= COLLISION_RIGHT;
                            if (display[i][j-1] ==  ON) {
                                collision |= COLLISION_LEFT;
                            }
                        } // collision LEFT (WALL) and collision RIGHT
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
    for (int i = p_i; i < p_i + BLOCK_SIZE; i++) {
        for (int j = p_j; j < p_j + BLOCK_SIZE; j++) {
            if ((j >= 0) && (j < DISPLAY_COLUMNS)) {
                if (display[i][j] == P) {
                    display[i][j] = ON;
                }
            }
        }
    }
}

void fall_one_row() {
    for (int i = p_i + BLOCK_SIZE - 1;i > p_i - 1; i--) {
        for (int j = p_j;j <  p_j+BLOCK_SIZE; j++) {
            if ((j >=  0) && (j < DISPLAY_COLUMNS)) {
                if (display[i+1][j] != ON) {
                    display[i+1][j] = display[i][j];
                    if (i == p_i) {
                        display[i][j] = 0;
                    }
                }
            }
        }
    }
    p_i++;
}

unsigned int full_row(unsigned int row[DISPLAY_COLUMNS]) {
    for (unsigned int j = 0; j < DISPLAY_COLUMNS; j++) {
        if (row[j] == OFF) {
            return 0;
        }
    }
    return 1;
}


void fall_row_until(unsigned int n) {
    for(unsigned int i = n ; i > 0; i--)
        for(unsigned int j = 0; j < DISPLAY_COLUMNS; j++) {
            display[i][j] = display[i-1][j];
        }
}

void clean_full_rows() {
    for (unsigned int i = 1; i < DISPLAY_ROWS; i++) {
        if (full_row(display[i])) {
            fall_row_until(i);
        }
    }
}

void insert_block(Block p) {
    p_i = 0; p_j = 2;
    for (unsigned int i = 0; i < BLOCK_SIZE; i++) {
        for (unsigned int j = 0; j < BLOCK_SIZE; j++) {
            display[i+p_i][j+p_j] = p[i][j];
        }
    }
}

unsigned int check_game_over() {
    return ((p_i == 0)  &&  (collision_state & COLLISION_BOTTOM));
}

// An Inplace function to rotate a N x N matrix
// by 90 degrees in anti-clockwise direction
void rotate_matrix(unsigned int mat[BLOCK_SIZE][BLOCK_SIZE])
{

    // Consider all squares one by one
    for (unsigned int x = 0; x < BLOCK_SIZE / 2; x++)
    {
        // Consider elements in group of 4 in
        // current square
        for (unsigned int y = x; y < BLOCK_SIZE - x - 1; y++)
        {
            // store current cell in temp variable
            unsigned int temp = mat[x][y];

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
    unsigned int window[BLOCK_SIZE][BLOCK_SIZE];
    int i,j,ii,jj;

    if (current_block == O) return;
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
    for (unsigned int i = 0; i < DISPLAY_ROWS; i++) {
        for (unsigned int j = 0; j < DISPLAY_COLUMNS; j++) {
            if (j - 1 >= 0 && display[i][j] == P) {
                display[i][j-1] = display[i][j];
                display[i][j] = 0;
            } else {
                display[i][j] = display[i][j];
            }
        }
    }
    p_j--;
}

void move_player_to_right(void) {
    for (unsigned int i = 0; i < DISPLAY_ROWS; i++) {
        unsigned int new_row[DISPLAY_COLUMNS];
        for (unsigned int j = 0; j < DISPLAY_COLUMNS; j++) {
            new_row[j] = OFF;
        }
        for (unsigned int j = 0; j < DISPLAY_COLUMNS; j++) {
            if (j+1 < DISPLAY_COLUMNS && display[i][j] == P) {
                new_row[j+1] = display[i][j];
            } else if (new_row[j] != P) {
                new_row[j] = display[i][j];
            }
        }

        for (unsigned int j = 0; j < DISPLAY_COLUMNS; j++) {
            display[i][j] = new_row[j];
        }
    }
    p_j++;
}

void event_manager(void) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) // Loop de eventos
    {
        switch (event.type)
        {
        case SDL_QUIT:
            done = 1;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
            case SDLK_UP:
            case SDLK_DOWN:
                if (!(collision_state & (COLLISION_LEFT | COLLISION_RIGHT))) {
                    rotate_player();
                }
                break;

            case SDLK_RIGHT:
                if (!(collision_state & COLLISION_RIGHT)) {
                    move_player_to_right();
                }
                break;

            case SDLK_LEFT:
                if (!(collision_state & COLLISION_LEFT)) {
                    move_player_to_left();
                }
                break;

            case SDLK_q:
                done = 1;
                break;

            default:
                break;
            }
            break;

        default:
            break;
        }
    }
}

void game_manager() {
    static unsigned int i, j;

    // main logic
    switch (game_state) {
    case 0:
        spawn_block();
        game_state = 1;
        break;

    case 1:
        if (check_game_over()) { // implies COLLISION_BOTTOM && p_i == 0
            i = 0; j = 0;
            timer = DELAY_TICK_GAME_OVER;
            game_state = 3;

        }
        else if (collision_state & COLLISION_BOTTOM) {
            freeze_blocks();
            clean_full_rows();
            game_state = 0;
        }
        else {
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
            display[i][j] = ON;
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

    row = 0;
    col = 0;
    state = 0;
    game_state = 1;
    timer = 0;
    done = 0;

    SDL_Init(SDL_INIT_VIDEO); // Inicializa o SDL e o sistema de vídeo

    screen = SDL_SetVideoMode(256, 512, 16, SDL_SWSURFACE); // Cria a janela
    brick = SDL_LoadBMP("graphics/bloco-vermelho.bmp"); // Carrega a imagem no formato BMP
    // Verifica se carregou a imagem corretamente
    if (brick == NULL)
    {
        printf("bloco-vermelho.bmp not found\n");
        exit(1);
    }

    SDL_EnableKeyRepeat(50, 30);
}


void screen_update(void) {

    SDL_Rect dest; // Destino da imagem
    int i,j;

    SDL_FillRect(screen, NULL, 0x0); // Pinta de preto todo o screen

    for (dest.y = 0, i = 0; dest.y < 512; dest.y += 16, i++)
        for (dest.x = 0, j = 0; dest.x < 256; dest.x += 16, j++)
            if (display[i][j])
                SDL_BlitSurface(brick, NULL, screen, &dest);

    //SDL_Flip(screen);
    SDL_UpdateRect(screen, 0,0,0,0); // Atualiza o screen com a imagem blitada

}


int main(void) {

    init();
    clear_display();
    spawn_block();

    while (!done) {
        SDL_Delay(5);
        screen_update();
        collision_state = check_collision();
        switch (state) {
        case 0:
            event_manager();
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

    SDL_Quit(); // Fecha o SDL
    return 0;
}
