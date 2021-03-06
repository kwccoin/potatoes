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
 * Pong game.
 *
 * @author Daniel Bader
 * @author $LastChangedBy$
 * @version $Rev$
 */

#include "games.h"
#include "../kernel/pm/syscalls_cli.h"
#include "../kernel/include/stdio.h"
#include "../kernel/include/string.h"
#include "../kernel/include/const.h"
#include "../kernel/io/io.h"

extern int STDIN;
bool pong_endbeep = FALSE;

bool keydown(char key, int fd)
{
        bool keystate[256];
        _read(fd, keystate, sizeof(keystate));
        return (keystate[key]);
}

void shell_cmd_pong(int argc, char *argv[])
{
        bool multiplayer = (argc > 1 && !strcmp(argv[1], "-2p"));

        if (!multiplayer) {
                _printf("+++ P O N G +++\n\nControl your paddle with the cursor UP and DOWN keys\n"
                                "You can leave the game at any time by pressing the ESCAPE key.\n\n"
                                "To play a two player game run \"pong -2p\"\n"
                                "HAVE FUN!\n\n\n[Press any key to start playing]\n\n");
        } else {
                _printf("+++ P O N G +++\n\nMULTIPLAYER MODE\n\n"
                                "Controls for player one (blue):\n"
                                "\tPaddle up = A\n"
                                "\tPaddle down = S\n\n"
                                "Controls for player two (red):\n"
                                "\tPaddle up = K\n"
                                "\tPaddle down = L\n\n"
                                "You can leave the game at any time by pressing the ESCAPE key.\n\n"
                                "HAVE FUN!\n\n\n[Press any key to start playing]\n");
        }
        _fgetch(STDIN);

        // the backbuffer
        uint8 bbuf[25 * 80];

        int fd = _open("/dev/framebuffer", 0, 0);
        int keyboard = _open("/dev/keyboard", 0, 0);

        // ball position
        int ball_x = 4000;
        int ball_y = 1200;

        // ball velocity
        int ball_vel_x = 50;
        int ball_vel_y = 0;

        // player paddle
        int l_paddle_y = 10;
        int r_paddle_y = 10;

        // scores
        int player_score = 0;
        int cpu_score = 0;

        int frame = 0;

        // The rendering loop
        while (!keydown(ESCAPE, keyboard)) {
                //Test end sound
                if(pong_endbeep == TRUE) {
                        end_beep();
                        pong_endbeep = FALSE;
                }
                // Game over check
                if (player_score > 9 || cpu_score > 9)
                        break;

                // Player input
                if (!multiplayer) {
                        if (keydown(CURSOR_UP, keyboard)) l_paddle_y--;
                        if (keydown(CURSOR_DOWN, keyboard)) l_paddle_y++;
                } else {
                        if (keydown(KEY_A, keyboard)) l_paddle_y--;
                        if (keydown(KEY_S, keyboard)) l_paddle_y++;
                }
                LIMIT(l_paddle_y, 0, 20);

                // CPU player update
                if (!multiplayer) {
                        // select cpu think penalty depending on the
                        // score differences. that way the cpu will adjust
                        // its strength to the player's skill.
                        int fskip = cpu_score - player_score;
                        LIMIT(fskip, 2, 9);

                        if (ball_x > 7500) {  // cpu quick reaction distance
                                if (ball_y / 100 < r_paddle_y + 2)
                                        r_paddle_y--;
                                else if (ball_y / 100 > r_paddle_y + 2)
                                        r_paddle_y++;
                        } else if (frame % fskip == 0) { // only think every other frame
                                int cpu_move = ball_vel_y;
                                LIMIT(cpu_move, -1, 1);
                                r_paddle_y += cpu_move;
                        }
                } else {
                        if (keydown(KEY_K, keyboard)) r_paddle_y--;
                        if (keydown(KEY_L, keyboard)) r_paddle_y++;
                }
                LIMIT(r_paddle_y, 0, 20);

                // Move ball
                ball_x += ball_vel_x;
                ball_y += ball_vel_y;
                LIMIT(ball_x, 0, 7900);
                LIMIT(ball_y, 0, 2400);

                // Ceiling hit / Floor hit
                if (ball_y == 0 || ball_y == 2400) {
                        ball_vel_y = -ball_vel_y;
                        start_beep(HIT_SIDE_SOUND);
                        pong_endbeep=TRUE;
                }
                // Paddle hit
                if (ball_x <= 0) {
                        if (HIT_PADDLE(l_paddle_y, ball_y / 100)) {
                                ball_vel_x = -ball_vel_x;
                                ball_vel_y = -ball_vel_y + PADDLE_DEFLECTION(l_paddle_y, ball_y / 100);
                                ball_x += 100;
                                start_beep(HIT_RPADDLE_SOUND);
                                pong_endbeep=TRUE;
                        } else {
                                cpu_score++;
                                ball_x = 4000;
                                ball_y = 1200;
                                ball_vel_x = 50;
                                ball_vel_y = 0;
                        }
                } else if (ball_x >= 7900) {
                        if (HIT_PADDLE(r_paddle_y, ball_y / 100)) {
                                ball_vel_x = -ball_vel_x;
                                ball_vel_y = -ball_vel_y - PADDLE_DEFLECTION(r_paddle_y, ball_y / 100);
                                ball_x -= 100;
                                start_beep(HIT_LPADDLE_SOUND);
                                pong_endbeep=TRUE;
                        } else {
                                player_score++;
                                ball_x = 4000;
                                ball_y = 1200;
                                ball_vel_x = -50;
                                ball_vel_y = 0;
                        }
                }

                // Clear the backbuffer
                memset(bbuf, BLACK, sizeof(bbuf));

                // Draw score
                DRAW_GLYPH(34, 0, player_score, BLUE);
                DRAW_GLYPH(41, 0, cpu_score, RED);

                // Draw center line
                for (int y = 0; y < 25; y += 2)
                        SET_PIXEL(39, y, WHITE);

                // Draw the ball and paddles
                SET_PIXEL(ball_x / 100, ball_y / 100, YELLOW);
                DRAW_PADDLE(0, l_paddle_y, BLUE);
                DRAW_PADDLE(79, r_paddle_y, RED);

                // Display backbuffer on the screen
                _write(fd, bbuf, sizeof(bbuf));
                halt();
                frame++;
        }

        _close(fd);
        _close(keyboard);

        _printf("Game over.\nPlayer score: %d\nCPU score: %d\n", player_score, cpu_score);

        //flush stdin
        char ch;
        while (_read(STDIN, &ch, sizeof(ch)) != 0) ;
}
