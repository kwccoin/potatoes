/* $Id$
********************************************************************************
* _____   ____ _______    _______ ____  ______  _____                          *
*|  __ \ / __ \__   __|/\|__   __/ __ \|  ____|/ ____|          Copyright 2008 *
*| |__) | |  | | | |  /  \  | | | |  | | |__  | (___              Daniel Bader *
*|  ___/| |  | | | | / /\ \ | | | |  | |  __|  \___ \           Vincenz Doelle *
*| |    | |__| | | |/ ____ \| | | |__| | |____ ____) |    Johannes Schamburger *
*|_|     \____/  |_/_/    \_\_|  \____/|______|_____/          Dmitriy Traytel *
*                                                                              *
*      Practical Oriented TeAching Tool, Operating (and) Educating System      *
*                                                                              *
*                           www.potatoes-project.tk                            *
*******************************************************************************/

/**
 * @file
 * Snake game.
 *
 * @author Dmitriy Traytel
 * @author $LastChangedBy$
 * @version $Rev$
 */
#include "games.h"
#include "../kernel/pm/syscalls_cli.h"
#include "../kernel/include/stdio.h"
#include "../kernel/include/string.h"
#include "../kernel/include/stdlib.h"
#include "../kernel/include/const.h"
#include "../kernel/io/io.h"
#include "../kernel/include/ringbuffer.h"

extern int STDIN;
ring_fifo *snake1 = NULL;
ring_fifo *snake2 = NULL;
uint8 direction1 = 0;
uint8 direction2 = 0;
bool snake_endbeep = FALSE;

// the backbuffer
uint8 bbuf[25 * 80];

uint16 draw_snake(ring_fifo *snake, uint8 color, uint8 headcolor)
{
        uint16 ch;
        ring_fifo *temp = rf_copy(snake);
        while (rf_getlength(temp) != 2) {
                rf_read(temp, (uint8*)&ch, 2);
                SET_PIXEL(ch >> 8, ch & 0xFF, color);
        }
        rf_read(temp, (uint8*)&ch, 2);
        SET_PIXEL(ch >> 8, ch & 0xFF, headcolor);
        rf_free(temp);
        return ch;
}

bool body_collision(ring_fifo *snake, uint16 pos)
{
        uint16 ch;
        ring_fifo *temp = rf_copy(snake);
        while (rf_getlength(temp) != 2) {
                rf_read(temp, (uint8*)&ch, 2);
                if (ch == pos) {
                        rf_free(temp);
                        return TRUE;
                }
        }
        rf_free(temp);
        return FALSE;
}

void shell_cmd_snake(int argc, char *argv[])
{
        bool multiplayer = (argc > 1 && !strcmp(argv[1], "-2p"));

        if (!multiplayer) {
                _printf("+++ S N A K E +++\n\n"
                                "Control your snake with the cursor UP, DOWN, LEFT & RIGHT keys\n"
                                "You can leave the game at any time by pressing the ESCAPE key.\n\n"
                                "To play a two player game run \"snake -2p\"\n"
                                "HAVE FUN!\n\n\n[Press any key to start playing]\n\n");
        } else {
                _printf("+++ S N A K E +++\n\nMULTIPLAYER MODE\n\n"
                                "Controls for player one (green):\n"
                                "\tUp = W\n"
                                "\tDown = S\n"
                                "\tLeft = A\n"
                                "\tRight = D\n"
                                "Controls for player two (blue):\n"
                                "\tUp = I\n"
                                "\tDown = J\n"
                                "\tLeft = K\n"
                                "\tRight = L\n"
                                "You can leave the game at any time by pressing the ESCAPE key.\n\n"
                                "HAVE FUN!\n\n\n[Press any key to start playing]\n");
        }
        _fgetch(STDIN);

        int fd = _open("/dev/framebuffer", 0, 0);
        int keyboard = _open("/dev/keyboard", 0, 0);

        uint8 loser = 0;
        uint16 head1;
        uint16 head2;
        uint16 trash;
        srand(get_ticks());
        srand(rand());
        uint16 apple = ((rand() % 80) << 8) + (rand() % 25);

        snake1 = rf_alloc(2 * 80 * 25);
        head1 = (0 << 8) + 12;
        rf_write(snake1, (uint8*)&head1, 2);
        head1 = (1 << 8) + 12;
        rf_write(snake1, (uint8*)&head1, 2);
        head1 = (2 << 8) + 12;
        rf_write(snake1, (uint8*)&head1, 2);
        direction1 = 0;
        if (multiplayer) {
                direction2 = 0;
                snake2 = rf_alloc(2 * 80 * 25);
                head2 = (79 << 8) + 12;
                rf_write(snake2, (uint8*)&head2, 2);
                head2 = (78 << 8) + 12;
                rf_write(snake2, (uint8*)&head2, 2);
                head2 = (77 << 8) + 12;
                rf_write(snake2, (uint8*)&head2, 2);
        }

        int frame = 0;

        // The rendering loop
        while (!keydown(ESCAPE, keyboard)) {
                //Test end sound
                if(snake_endbeep == TRUE) {
                        end_beep();
                        snake_endbeep = FALSE;
                }
                // Test collision
                if (body_collision(snake1, head1)) {
                        SET_PIXEL(head1 >> 8, head1 & 0xFF, RED);
                        loser = 1;
                        break;
                }
                if (multiplayer) {
                        if (body_collision(snake2, head1)) {
                                SET_PIXEL(head1 >> 8, head1 & 0xFF, RED);
                                loser = 1;
                        }
                        if (body_collision(snake1, head2)) {
                                SET_PIXEL(head2 >> 8, head2 & 0xFF, RED);
                                loser = (loser == 0) ? 2 : 3;
                        }
                        if (body_collision(snake2, head2)) {
                                SET_PIXEL(head2 >> 8, head2 & 0xFF, RED);
                                loser = (loser == 0) ? 2 : 3;
                        }
                        if (head1 == head2) {
                                loser = 3;
                                SET_PIXEL(head1 >> 8, head1 & 0xFF, RED);
                        }
                }
                if (loser != 0) break;

                // Player input
                if (!multiplayer) {
                        if (keydown(CURSOR_UP, keyboard)) direction1 = SNAKE_UP;
                        if (keydown(CURSOR_DOWN, keyboard)) direction1 = SNAKE_DOWN;
                        if (keydown(CURSOR_RIGHT, keyboard)) direction1 = SNAKE_RIGHT;
                        if (keydown(CURSOR_LEFT, keyboard)) direction1 = SNAKE_LEFT;
                } else {
                        if (keydown(KEY_W, keyboard)) direction1 = SNAKE_UP;
                        if (keydown(KEY_S, keyboard)) direction1 = SNAKE_DOWN;
                        if (keydown(KEY_D, keyboard)) direction1 = SNAKE_RIGHT;
                        if (keydown(KEY_A, keyboard)) direction1 = SNAKE_LEFT;
                        if (keydown(KEY_I, keyboard)) direction2 = SNAKE_UP;
                        if (keydown(KEY_K, keyboard)) direction2 = SNAKE_DOWN;
                        if (keydown(KEY_L, keyboard)) direction2 = SNAKE_RIGHT;
                        if (keydown(KEY_J, keyboard)) direction2 = SNAKE_LEFT;
                }

                //Update snake
                if (frame % 5 == 0) {
                        switch (direction1) {
                        case SNAKE_UP:
                                if (head1 != apple) {
                                        rf_read(snake1, (uint8*)&trash, 2);
                                } else {
                                        start_beep(EAT_APPLE_SOUND);
                                        snake_endbeep=TRUE;
                                        while (head1 == apple || body_collision(snake1, apple)) {
                                                apple = ((rand() % 80) << 8) + (rand() % 25);
                                        }
                                }
                                head1 = (head1 & 0xFF00) + (((head1 & 0xFF) - 1 + 25) % 25);
                                rf_write(snake1, (uint8*)&head1, 2);
                                break;
                        case SNAKE_RIGHT:
                                if (head1 != apple) {
                                        rf_read(snake1, (uint8*)&trash, 2);
                                } else {
                                        start_beep(EAT_APPLE_SOUND);
                                        snake_endbeep=TRUE;
                                        while (head1 == apple || body_collision(snake1, apple)) {
                                                apple = ((rand() % 80) << 8) + (rand() % 25);
                                        }
                                }
                                head1 = ((((head1 >> 8) + 1) % 80) << 8) + (head1 & 0xFF);
                                rf_write(snake1, (uint8*)&head1, 2);
                                break;
                        case SNAKE_DOWN:
                                if (head1 != apple) {
                                        rf_read(snake1, (uint8*)&trash, 2);
                                } else {
                                        start_beep(EAT_APPLE_SOUND);
                                        snake_endbeep=TRUE;
                                        while (head1 == apple || body_collision(snake1, apple)) {
                                                apple = ((rand() % 80) << 8) + (rand() % 25);
                                        }
                                }
                                head1 = (head1 & 0xFF00) + (((head1 & 0xFF) + 1) % 25);
                                rf_write(snake1, (uint8*)&head1, 2);
                                break;
                        case SNAKE_LEFT:
                                if (head1 != apple) {
                                        rf_read(snake1, (uint8*)&trash, 2);
                                } else {
                                        start_beep(EAT_APPLE_SOUND);
                                        snake_endbeep=TRUE;
                                        while (head1 == apple || body_collision(snake1, apple)) {
                                                apple = ((rand() % 80) << 8) + (rand() % 25);
                                        }
                                }
                                head1 = ((((head1 >> 8) - 1 + 80) % 80) << 8) + (head1 & 0xFF);
                                rf_write(snake1, (uint8*)&head1, 2);
                                break;
                        }
                        if (multiplayer) {
                                switch (direction2) {
                                case SNAKE_UP:
                                        if (head2 != apple) {
                                                rf_read(snake2, (uint8*)&trash, 2);
                                        } else {
                                                apple = ((rand() % 80) << 8) + (rand() % 25);
                                        }
                                        head2 = (head2 & 0xFF00) + (((head2 & 0xFF) - 1 + 25) % 25);
                                        rf_write(snake2, (uint8*)&head2, 2);
                                        break;
                                case SNAKE_RIGHT:
                                        if (head2 != apple) {
                                                rf_read(snake2, (uint8*)&trash, 2);
                                        } else {
                                                apple = ((rand() % 80) << 8) + (rand() % 25);
                                        }
                                        head2 = ((((head2 >> 8) + 1) % 80) << 8) + (head2 & 0xFF);
                                        rf_write(snake2, (uint8*)&head2, 2);
                                        break;
                                case SNAKE_DOWN:
                                        if (head2 != apple) {
                                                rf_read(snake2, (uint8*)&trash, 2);
                                        } else {
                                                apple = ((rand() % 80) << 8) + (rand() % 25);
                                        }
                                        head2 = (head2 & 0xFF00) + (((head2 & 0xFF) + 1) % 25);
                                        rf_write(snake2, (uint8*)&head2, 2);
                                        break;
                                case SNAKE_LEFT:
                                        if (head2 != apple) {
                                                rf_read(snake2, (uint8*)&trash, 2);
                                        } else {
                                                apple = ((rand() % 80) << 8) + (rand() % 25);
                                        }
                                        head2 = ((((head2 >> 8) - 1 + 80) % 80) << 8)
                                        + (head2 & 0xFF);
                                        rf_write(snake2, (uint8*)&head2, 2);
                                        break;
                                }
                        }
                }

                // Clear the backbuffer
                memset(bbuf, BLACK, sizeof(bbuf));

                // Draw snake
                head1 = draw_snake(snake1, LIGHTGREEN, GREEN);
                if (multiplayer) {
                        head2 = draw_snake(snake2, LIGHTBLUE, BLUE);
                }

                // Draw apple
                SET_PIXEL(apple >> 8, apple & 0xFF, ORANGE);

                // Display backbuffer on the screen
                _write(fd, bbuf, sizeof(bbuf));
                halt();
                frame++;
        }

        //flush stdin
        char ch;
        while (_read(STDIN, &ch, sizeof(ch)) != 0) ;

        if (loser != 0) {
                _write(fd, bbuf, sizeof(bbuf));
                _fgetch(STDIN);
        }

        _close(fd);
        _close(keyboard);

        if (multiplayer) {
                if (loser == 3) {
                        _printf("\nBoth players lost!\n");
                }
                if (loser == 2) {
                        _printf("\nPlayer 2 lost!\n");
                }
                if (loser == 1) {
                        _printf("\nPlayer 1 lost!\n");
                }
                _printf("Player 1:\t%d pts\nPlayer 2:\t%d pts\n",
                                (rf_getlength(snake1) / 2) - 3, (rf_getlength(snake2) / 2) - 3);
        } else {
                _printf("\nGame over!\nYour points:\t%d\n", (rf_getlength(snake1) / 2) - 3);
        }
        rf_free(snake1);
        if (multiplayer) {
                rf_free(snake2);
        }
}
