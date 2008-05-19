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
 * Headers for string.c
 *
 * @author Dmitriy Traytel
 * @author $LastChangedBy$
 * @version $Rev$
 */

#ifndef __STRING_H
#define __STRING_H
 
uint32 strlen(char* str);
char* strcpy(char *dest, char *src);
char* strncpy(char *dest, char *src, uint32 n);
char* strchr(char *str, char ch);
char* strcat(char *s1, char *s2);
char* strncat(char *s1, char *s2, uint32 n);
char* strdup(char* str);
char* strsep(char **str_ptr, char *delims);
sint32 strcmp(char *s1, char *s2);

void* memset(void *dest, uint8 value, uint32 count);
void bzero(void *dest, uint32 count);
void* memcpy(void *dest, void *src, uint32 count);

char* strreverse(char *str);
char* itoa(int n, char *str, unsigned int base);

#endif /* string.h */
