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
 * Functions concerning syscalls "create" and "delete"
 * 
 * @author Vincenz Doelle
 * @author $LastChangedBy$
 * @version $Rev$
 */

#include "../include/const.h"
#include "../include/types.h"
#include "../include/string.h"

#include "fs_const.h"
#include "fs_types.h"
#include "fs_block_dev.h"
#include "fs_buf.h"
#include "fs_bmap.h"
#include "fs_inode_table.h"
#include "fs_file_table.h"
#include "fs_dir.h"

/**
 * Create a file from abs. path
 * 
 * @param path The file's absolute path
 * @return the file's file descriptor
 */
file_nr create(char *path)
{
        block_nr blk_nr = search_file(path);
        if (blk_nr != NOT_FOUND){
                return NOT_POSSIBLE;    //file is already existent
        }
        
        blk_nr = search_file(splice_path(path));
        
        blk_nr = alloc_block(blk_nr);
        
}
