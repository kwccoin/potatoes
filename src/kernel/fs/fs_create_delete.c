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
 * Functions concerning syscalls "create" and "delete"
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
#include "fs_bmap.h"
#include "fs_inode_table.h"
#include "fs_file_table.h"
#include "fs_dir.h"
#include "fs_io_functions.h"

/**
 * Creates a file from absolute path by inserting the name into the containing directory,
 * creating a new inode and writing it to HD.
 *
 * @param abs_path   absolute file path
 * @param data_type  DATA_FILE | DIRECTORY (@see fs_const.h)
 * @return           result status of the create operation
 */
bool fs_create(char *abs_path, int data_type)
{
        return fs_create_delete(abs_path, CREATE, data_type);
}


/**
 * Deletes a file by removing it from the containing directory.
 *
 * @param abs_path absolute file path
 * @return         result status of the delete operation
 */
bool fs_delete(char *abs_path)
{
        return fs_create_delete(abs_path, DELETE, NULL);
}

/**
 * Handles the creation and deletion process.
 *
 * @param abs_path  absolute file path
 * @param mode      CREATE | DELETE (@see fs_const.h)
 * @param data_type DATA_FILE | DIRECTORY (@see fs_const.h)
 * @return          result status of the create/delete operation
 */
bool fs_create_delete(char *abs_path, int mode, int data_type)
{
        if (mode == CREATE) {
                fs_dprintf("[fs_create_delete] CREATING file '%s'...\n", abs_path);
        } else {
                fs_dprintf("[fs_create_delete] DELETING file '%s'...\n", abs_path);
        }

        if (mode == CREATE && search_file(abs_path) != NOT_FOUND) {
                printf("%{ERROR}: file '%s' already exists!\n", RED, abs_path);
                return FALSE;
        }

        if (mode == DELETE && search_file(abs_path) == NOT_FOUND) {
                printf("%{ERROR}: file '%s' does not exist!\n", RED, abs_path);
                return FALSE;
        }

        char *path = get_path(abs_path);
        if (path == NULL) {
                return FALSE;
        }

        block_nr dir_inode_block;

        if (strcmp(path, "/") == 0) {
                dir_inode_block = ROOT_INODE_BLOCK;
        } else {
                dir_inode_block = search_file(path); //find dir
        }

        if (dir_inode_block == NOT_FOUND) {
                return FALSE;
        }

        char *file_name = get_filename(abs_path);
        if (file_name == NULL) {
                return FALSE;
        }

        block_nr file_block;

        if (mode == CREATE) {
                file_block = insert_file_into_dir(dir_inode_block, file_name);
        } else if (mode == DELETE) {
                free_data_blocks(abs_path);
                file_block = delete_file_from_dir(dir_inode_block, file_name);
        }

        if (file_block == NOT_POSSIBLE) {
                return FALSE;
        }

        if (mode == CREATE) {
                //create new inode and write it to HD
                fs_dprintf("[fs_c_d] create new inode (type = %d) and write it to block %d\n", data_type, file_block);
                m_inode *inode = new_minode(file_block, data_type, FALSE);

                if (inode == NULL) {
                        return FALSE;
                }

                write_inode(inode);

                free(inode); //free memory, load from HD if needed
        }

        free(file_name);
        free(path);

        return TRUE;
}

void free_data_blocks(char* abs_path)
{
        file_nr fd = fs_open(abs_path);
        file* file = get_file(fd);
        block_nr blk = NOT_FOUND;
        size_t pos = 0;
        
        do { //scan through file in order to discover allocated blocks
                blk = get_data_block(file->f_inode, pos, FALSE);
                pos += BLOCK_SIZE;
                
                if (blk != NOT_FOUND) {
                        mark_block(blk, FALSE); //set block as unused
                        fs_dprintf("[fs_c_d] marked data block %d as FALSE\n", blk);
                }
                
        } while(blk != NOT_FOUND);
        
        fs_close(fd);
}
