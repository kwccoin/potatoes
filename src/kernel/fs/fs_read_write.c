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
 * Functions concerning syscalls "read" and "write"
 * 
 * @author Vincenz Doelle
 * @author $LastChangedBy$
 * @version $Rev$
 */
#include "../include/const.h"
#include "../include/types.h"
#include "../include/string.h"
#include "../include/stdlib.h"
#include "../include/debug.h"

#include "fs_const.h"
#include "fs_types.h"
#include "fs_block_dev.h"
#include "fs_buf.h"
#include "fs_io_functions.h"


block_nr fs_read(void *buf, m_inode *inode, size_t num_bytes, uint32 pos, bool allow_scaling)
{
        if (pos > inode->i_size) {
                return NOT_POSSIBLE;
        }
        
        block_nr data_blk = get_data_block(inode, pos, allow_scaling); //see block_dev.c
        
        fs_dprintf("[fs_r_w] found block to read content from: %d\n", data_blk);
        
        if (data_blk == NOT_FOUND){
                return NOT_POSSIBLE;
        }
        
        cache_block(data_blk, BLOCK_SIZE);
        
        uint16 offset = pos % BLOCK_SIZE; //offset within the block because of position pos
        fs_dprintf("[fs_r_w] offset within the content block (%d) + num_bytes (%d) = %d\n", offset, num_bytes, offset + num_bytes);
        
        if (offset + num_bytes <= BLOCK_SIZE){
                memcpy(buf, read_cache.cache + offset, num_bytes); //TODO: counter-check read_cache.cache + offset
        } else {
                uint16 num_readable_bytes = BLOCK_SIZE - offset;
                memcpy(buf, read_cache.cache + offset, num_readable_bytes);
                
                fs_dprintf("[fs_r_w] reading over block edge...\n");
                fs_dprintf("[fs_r_w] num_readable_bytes = %d, left bytes = %d, new pos = %d\n", num_readable_bytes, num_bytes - (num_readable_bytes), pos + num_readable_bytes);
                
                return fs_read(buf + num_readable_bytes, inode, num_bytes - (num_readable_bytes), pos + num_readable_bytes, allow_scaling);
        }
        
        return data_blk;
}

block_nr fs_write(m_inode *inode, void *buf, size_t num_bytes, uint32 pos, bool allow_scaling)
{
        block_nr dest_blk = get_data_block(inode, pos, allow_scaling); //get_block() in block_dev.c

        if (dest_blk == NOT_FOUND){
                return NOT_POSSIBLE;
        }
        
        uint16 offset = pos % BLOCK_SIZE; //offset within the block because of position pos
        if (offset + num_bytes <= BLOCK_SIZE){
                cache_block(dest_blk, BLOCK_SIZE); //dest: read_cache
                
                //overwrite modified bytes
                memcpy(read_cache.cache + offset, buf, num_bytes); //TODO: counter-check write_cache.cache + offset
                wrt_cache(&read_cache, BLOCK_SIZE);
        } else {
                uint16 num_writeable_bytes = BLOCK_SIZE - offset;
                cache_block(dest_blk, BLOCK_SIZE);
                
                memcpy(read_cache.cache + offset, buf, num_writeable_bytes);
                wrt_cache(&read_cache, BLOCK_SIZE);
                
                fs_dprintf("[fs_r_w] writing over block edge...\n");
                fs_dprintf("[fs_r_w] num_writeable_bytes = %d, left bytes = %d, new pos = %d\n", num_writeable_bytes, num_bytes - (num_writeable_bytes), pos + num_writeable_bytes);
                           
                return fs_write(inode, buf + num_writeable_bytes, num_bytes - (num_writeable_bytes), pos + num_writeable_bytes, allow_scaling);
        }
        
        if (inode->i_size < pos + num_bytes){
                fs_dprintf("[fs_r_w] new file's size: %d\n", pos + num_bytes);
                inode->i_size = pos + num_bytes;
                write_inode(inode);
        }
        
        return dest_blk;
}
