#ifndef _LIBAWD_BLOCKS_H
#define _LIBAWD_BLOCKS_H

#include <unistd.h>

#include "awd_types.h"

using namespace std;

class AWDBlock
{
    private:
        awd_baddr addr;

    protected:
        AWD_block_type type;
        virtual awd_uint32 calc_body_length(awd_bool)=0;
        virtual void write_body(int, awd_bool)=0;

    public:
        AWDBlock * next;

        awd_baddr get_addr();

        size_t write_block(int, awd_bool, awd_baddr);
};

class AWDBlockList
{
    private:
        int num_blocks;

    public:
        AWDBlock * first_block;
        AWDBlock * last_block;

        AWDBlockList();
        void append(AWDBlock *);
};

class AWDBlockIterator
{
    private:
        AWDBlockList * list;
        AWDBlock * cur_block;

    public:
        AWDBlockIterator(AWDBlockList *);
        AWDBlock * next();
        void reset();
};

#endif
