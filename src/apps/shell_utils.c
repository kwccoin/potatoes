/* $Id$
      _   _  ____   _____
     | | (_)/ __ \ / ____|
  ___| |_ _| |  | | (___
 / _ \ __| | |  | |\___ \  Copyright 2008 Daniel Bader, Vincenz Doelle,
|  __/ |_| | |__| |____) |        Johannes Schamburger, Dmitriy Traytel
 \___|\__|_|\____/|_____/

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file
 * Shell utility functions.
 *
 * @author Daniel Bader
 * @author $LastChangedBy$
 * @version $Rev$
 */
 
#include "../kernel/include/types.h"
#include "../kernel/include/const.h"
#include "../kernel/pm/syscalls_cli.h"
#include "../kernel/include/string.h"
#include "shell_main.h"

/** The shell's STDIN file descriptor. Used by all internal commands. */
int STDIN = -1;

/** The shell's STDOUT file descriptor. Used by all internal commands. */
int STDOUT = -1;

/**
 * Writes a character to the given file.
 * 
 * @param ch the character to write
 * @param fd the file descriptor
 * @return the number of bytes written, -1 on error
 */
int _fputch(char ch, int fd)
{
        return _write(fd, &ch, sizeof(ch));
}

extern void halt();

/**
 * Waits until a character from the given file could be read and returns it.
 * 
 * @param fd the file descriptor
 * @return the character that was read
 */
int _fgetch(int fd)
{
        char ch;
        while (_read(fd, &ch, sizeof(ch)) == 0)
                halt();
        return ch;
}

/**
 * Reads a string from a file descriptor into a buffer.
 * 
 * @param s the string buffer
 * @param n the maximum number of bytes to read (ie the buffer size)
 * @param fd the file descriptor
 * @return the string buffer 
 */
char* _fgets(char *s, int n, int fd)
{
        // FIXME: This is ugly.

        char *start = s;
        int count = 0;
        char ch = 0;
        while ((n-- > 0) && (ch != '\n')) {
                ch = _fgetch(fd);

                // Tab
                if (ch == '\t') {
                        *s++ = ch;
                        break;
                }

                // Handle backspace
                if (ch != '\b') {
                        *s++ = ch;
                        count++;
                        _fputch(ch, STDOUT);
                } else {
                        if (count > 0) {
                                *s-- = '\0';
                                count--;
                                _fputch(ch, STDOUT);
                        }
                }

        }

        return s;

}

/**
 * Writes a string into a given file.
 * 
 * @param s the string to write
 * @param fd the file descriptor
 * @return the number of bytes written, -1 on error
 */
int _fputs(char *s, int fd)
{
        int count = strlen(s);
        return _write(fd, s, count);
}

/**
 * Prints formatted output to STDOUT. @see printf
 * This exists as a stub to ease the separation of the shell
 * from the kernel code (as of now, the shell could simply call the kernel printf).
 * 
 * TODO: refactor format logic into vprintf()  
 */
void _printf(char *fmt, ...) //TODO: @Daniel: redundant. --> better solution possible?
{
        if (fmt == NULL)
                return;

        char **arg = &fmt + 1;
        char ch;
        int character;
        char buf[40];

        while ((ch = *fmt++) != '\0')
                if (ch == '%') {
                        ch = *fmt++;
                        switch (ch) {
                        case '%': // print '%'
                                _fputch(ch, STDOUT);
                                break;
                        case 'i': // signed integer
                        case 'd':
                                _fputs(itoa((sint32)*arg++, buf, 10), STDOUT);
                                break;
                        case 'u': // unsigned integer
                                _fputs(itoa((uint32)*arg++, buf, 10), STDOUT);
                                break;
                        case 'o': // octal
                                _fputs(itoa((uint32)*arg++, buf, 8), STDOUT);
                                break;
                        case 'b': // binary
                                _fputs(itoa((uint32)*arg++, buf, 2), STDOUT);
                                break;
                        case 'c': // character
                                /* This is a bit peculiar but needed to shut up the
                                 * "cast from pointer to integer of different size"
                                 * compiler warning.
                                 * Code was: putchar((char)*arg++);
                                 */
                                character = (int) * arg++;
                                _fputch((char)character, STDOUT);
                                break;
                        case 's': // string
                                if (*arg != NULL) {
                                        while ((ch = *(*arg)++) != '\0')
                                                _fputch(ch, STDOUT);
                                } else {
                                        _fputs("(null)", STDOUT);
                                }
                                *arg++;
                                break;
                        case 'x': // hexadecimal integer
                        case 'p': // pointer
                                _fputs(itoa((uint32)*arg++, buf, 16), STDOUT);
                                break;
                        }
                } else
                        _fputch(ch, STDOUT);
}

/**
 * Makes a given path absolute if needed. shell_makepath checks for a leading slash
 * in path to decide whether a given path is already absolute. If the path is not absolute
 * it will be appended to the current working directory.
 * Calling this will invalidate the last result.
 * 
 * @param path the path to make absolute
 * @return the absolute path. This pointer is only valid until the next call to
 * 		   shell_makepath().
 */
char* shell_makepath(char *path)
{
        strcpy(path_buf, cwd);

        if (path[0] == '/') {
                // absolute path
                return path;
        } else {
                // Relative path.

                // Append trailing slash
                if (path_buf[strlen(path_buf)-1] != '/')
                        strcat(path_buf, "/");

                strcat(path_buf, path);
                path_buf[strlen(path_buf)] = '\0';
                return path_buf;
        }
}