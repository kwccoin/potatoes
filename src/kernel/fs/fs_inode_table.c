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
 * The inode table.
 * This table contains the currently opened inodes.
 * This table is only existent in memory.
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
#include "../include/assert.h"

#include "fs_const.h"
#include "fs_types.h"

#include "fs_buf.h"
#include "fs_block_dev.h"
#include "fs_inode_table.h"

/**
 * Initialize all inodes with i_num = NIL_INODE = -1
 * -> sign that inode is unused
 */
void init_inode_table()
{
        fs_dprintf("[fs_inode_table] resetting inode_table\n");
        for (int i = 0; i < NUM_INODES; i++) {
                inode_table[i].i_num = NIL_INODE;
        }
}

/**
 * Create a new root inode.
 */
void create_root()
{
        m_inode *new_root = new_minode(ROOT_INODE_BLOCK, DIRECTORY, FALSE);
        if (new_root == NULL){
                return;
        }
        memcpy(&inode_table[ROOT_INODE], new_root, sizeof(m_inode));

        root = &inode_table[ROOT_INODE];
        root->i_num = ROOT_INODE;

        ASSERT(inode_table[ROOT_INODE].i_num != NIL_INODE);
}

/**
 * Load root inode from HD to inode table.
 */
void load_root()
{
        read_minode(&inode_table[ROOT_INODE], ROOT_INODE_BLOCK);
        root = &inode_table[ROOT_INODE];
        root->i_adr = ROOT_INODE_BLOCK;
        root->i_num = ROOT_INODE;

        fs_dprintf("[fs_inode_table] root block: %d\n", root->i_adr);

        ASSERT(root != (m_inode *) NULL);

}

/**
 * Write root to HD.
 */
void write_root()
{
        d_inode *d_root = mallocn(sizeof(d_inode), "tmp d_inode");
        ASSERT(d_root != (void *) NULL);

        write_inode(root);
}

/**
 * Returns a pointer to an inode with inode_nr = i_num.
 *
 * @param i_num The inode number of the inode
 * @return Pointer to the inode.
 */
m_inode* get_inode(inode_nr i_num)
{
        for (int i = 0; i < NUM_INODES; i++) {
                if (inode_table[i].i_num == i_num) {
                        return &inode_table[i];
                }
        }
        return NULL;
}

/**
 * Read a inode from HD into a disk inode.
 *
 * @param inode         destination
 * @param inode_blk     source block on HD
 */
void read_dinode(d_inode *inode, block_nr inode_blk)
{
        rd_block(inode, inode_blk, sizeof(d_inode));
}

/**
 * Read a inode from HD into a memory inode.
 *
 * @param inode         destination
 * @param inode_blk     source block on HD
 */
void read_minode(m_inode *inode, block_nr inode_blk)
{
        bzero(&d_inode_cache, sizeof(d_inode_cache));

        read_dinode(&d_inode_cache, inode_blk);
        cpy_dinode_to_minode(inode, &d_inode_cache);
        inode->i_adr = inode_blk;
}

/**
 * Write a memory inode to disk.
 *
 * @param inode         pointer to the memory inode which should be written
 * @return boolean      status of operation
 */
void write_inode(m_inode *inode)
{
        cpy_minode_to_dinode(&d_inode_cache, inode); //transform memory inode to a disk inode.

        wrt_block(inode->i_adr, &d_inode_cache, sizeof(d_inode));
}

/**
 * Write all inodes from inode table to HD.
 */
void write_inodes()
{
        for (int i = 0; i < NUM_INODES; i++) {
                write_inode(&inode_table[i]);
        }
}

/**
 * Copy common content of a memory inode to a disk inode.
 *
 * @param di    disk inode
 * @param mi    memory inode
 */
void cpy_minode_to_dinode(d_inode *di, m_inode *mi)
{
        di->i_mode      = mi->i_mode;
        di->i_size      = mi->i_size;
        di->i_create_ts = mi->i_create_ts;
        di->i_modify_ts = mi->i_modify_ts;

        for (int i = 0; i < NUM_DIRECT_POINTER; i++) {
                di->i_direct_pointer[i] = mi->i_direct_pointer[i];
        }

        di->i_single_indirect_pointer = mi->i_single_indirect_pointer;
        di->i_double_indirect_pointer = mi->i_double_indirect_pointer;
}

/**
 * Copy common content of a disk inode to a memory inode.
 *
 * @param mi    memory inode
 * @param di    disk inode
 */
void cpy_dinode_to_minode(m_inode *mi, d_inode *di)
{
        mi->i_mode                      = di->i_mode;
        mi->i_size                      = di->i_size;
        mi->i_create_ts                 = di->i_create_ts;
        mi->i_modify_ts                 = di->i_modify_ts;

        for (int i = 0; i < NUM_DIRECT_POINTER; i++) {
                mi->i_direct_pointer[i] = di->i_direct_pointer[i];
        }

        mi->i_single_indirect_pointer   = di->i_single_indirect_pointer;
        mi->i_double_indirect_pointer   = di->i_double_indirect_pointer;
}

/**
 * Create a new memory inode.
 *
 * @param adr            block address
 * @param mode           DATA_FILE | DIRECTORY (@see fs_const.h)
 * @param to_inode_table should the new inode be inserted into the inode table?
 * @return               pointer to new inode
 */
m_inode* new_minode(block_nr adr, int mode, bool to_inode_table)
{
        m_inode *mi;

        if (!to_inode_table) {
                mi = mallocn(sizeof(m_inode), "new m_inode");
                if (mi == NULL) {
                        return (m_inode *) NULL;
                }
                bzero(mi, sizeof(mi));
                mi->i_num = NIL_INODE;
        } else {
                mi = alloc_inode();
        }

        mi->i_adr  = adr;
        mi->i_mode = mode;
        mi->i_size = 0;

        mi->i_create_ts = time;
        mi->i_modify_ts = time;

        for (int i = 0; i < NUM_DIRECT_POINTER; i++) {
                mi->i_direct_pointer[i] = NULL;
        }

        mi->i_single_indirect_pointer = NULL;
        mi->i_double_indirect_pointer = NULL;

        return mi;
}

/**
 * Sets an inode as unused.
 *
 * @param i_num The inode number
 */
void free_inode(inode_nr i_num)
{
        m_inode *i = get_inode(i_num);
        i->i_num = NIL_INODE;             //overwrite inode number with -1 -> "unused"
}

/**
 * Returns an unused inode.
 *
 * @return Pointer to the inode
 */
m_inode* alloc_inode(void)
{
        for (int i = 0; i < NUM_INODES; i++) {
                if (inode_table[i].i_num == NIL_INODE) {
                        inode_table[i].i_num = i;
                        return &inode_table[i];
                }
        }
        return NULL;
}

/********* DEBUG *********/

/**
 * Print out a inode's attributes for debug purposes.
 *
 * @param mi    inode to be printed
 */
void dump_inode(m_inode *mi)
{
        fs_dprintf("[fs_inode_table] INODE: num = %d; adr = %d; sip = %d; dip = %d; mode = %d\n",
                   mi->i_num, mi->i_adr,
                   mi->i_single_indirect_pointer,
                   mi->i_double_indirect_pointer,
                   mi->i_mode);
        for (int i = 0; i < NUM_DIRECT_POINTER; i++) {
                fs_dprintf("dp[%d] = %d; ", i, mi->i_direct_pointer[i]);
        }
        fs_dprintf("\n");
}

/**
 * Print out a disk inode's attributes for debug purposes.
 *
 * @param di    inode to be printed
 */
void dump_dinode(d_inode *di)
{
        fs_dprintf("[fs_inode_table] INODE: sip = %d; dip = %d; mode = %d\n",
                   di->i_single_indirect_pointer,
                   di->i_double_indirect_pointer,
                   di->i_mode);
        for (int i = 0; i < NUM_DIRECT_POINTER; i++) {
                fs_dprintf("dp[%d] = %d; ", i, di->i_direct_pointer[i]);
        }
        fs_dprintf("\n");
}

/**
 * Print out inode table for debug purposes.
 */
void dump_inodes()
{
        for (int i = 0; i < NUM_INODES; i++) {
                dump_inode(&inode_table[i]);
        }
}
