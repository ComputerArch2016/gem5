/*
 * Copyright (c) 2013 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2009 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Lisa Hsu
 */

/**
 * @file
 * Declaration of an associative set
 */

#ifndef __CACHESET_HH__
#define __CACHESET_HH__

#include <cassert>
#include <iostream>

#include "mem/cache/blk.hh" // base class

/**
 * An associative set of cache blocks.
 */
struct node
{
    int val = 0;
    node *next = NULL;
};
struct queue
{
    int num = 0;
    node *front = NULL;
    node *rear = NULL;
};

template <class Blktype>
class CacheSet
{
  public:
    /** The associativity of this set. */
    int assoc;

    /** Cache blocks in this set, maintained in LRU order 0 = MRU. */
    Blktype **blks;
    int *MRU;
    int MRU_count;
    queue A1, Am;
    int A1thres;

    /**
     * Find a block matching the tag in this set.
     * @param way_id The id of the way that matches the tag.
     * @param tag The Tag to find.
     * @param is_secure True if the target memory space is secure.
     * @return Pointer to the block if found. Set way_id to assoc if none found
     */
    Blktype* findBlk(Addr tag, bool is_secure, int& way_id) const ;
    Blktype* findBlk(Addr tag, bool is_secure) const ;

    /**
     * Move the given block to the head of the list.
     * @param blk The block to move.
     */
    void moveToHead(Blktype *blk);

    /**
     * Move the given block to the tail of the list.
     * @param blk The block to move
     */
    void moveToTail(Blktype *blk);

    void setMRUUp(Blktype *blk);

    void setMRUDown(Blktype *blk);

    int findMRU();

    void setTREEUp(Blktype *blk);

    void setTREEDown(Blktype *blk);

    int findTREE();

    void hit2q(Blktype *blk);

    void miss2q(Blktype *blk);

    void insert2q(Blktype *blk);

    int find2q();
};

template <class Blktype>
Blktype*
CacheSet<Blktype>::findBlk(Addr tag, bool is_secure, int& way_id) const
{
    /**
     * Way_id returns the id of the way that matches the block
     * If no block is found way_id is set to assoc.
     */
    way_id = assoc;
    for (int i = 0; i < assoc; ++i) {
        if (blks[i]->tag == tag && blks[i]->isValid() &&
            blks[i]->isSecure() == is_secure) {
            way_id = i;
            return blks[i];
        }
    }
    return NULL;
}

template <class Blktype>
Blktype*
CacheSet<Blktype>::findBlk(Addr tag, bool is_secure) const
{
    int ignored_way_id;
    return findBlk(tag, is_secure, ignored_way_id);
}

template <class Blktype>
void
CacheSet<Blktype>::moveToHead(Blktype *blk)
{
    // nothing to do if blk is already head
    if (blks[0] == blk)
        return;

    // write 'next' block into blks[i], moving up from MRU toward LRU
    // until we overwrite the block we moved to head.

    // start by setting up to write 'blk' into blks[0]
    int i = 0;
    Blktype *next = blk;

    do {
        assert(i < assoc);
        // swap blks[i] and next
        Blktype *tmp = blks[i];
        blks[i] = next;
        next = tmp;
        ++i;
    } while (next != blk);
}

template <class Blktype>
void
CacheSet<Blktype>::moveToTail(Blktype *blk)
{
    // nothing to do if blk is already tail
    if (blks[assoc - 1] == blk)
        return;

    // write 'next' block into blks[i], moving from LRU to MRU
    // until we overwrite the block we moved to tail.

    // start by setting up to write 'blk' into tail
    int i = assoc - 1;
    Blktype *next = blk;

    do {
        assert(i >= 0);
        // swap blks[i] and next
        Blktype *tmp = blks[i];
        blks[i] = next;
        next = tmp;
        --i;
    } while (next != blk);
}

template <class Blktype>
void
CacheSet<Blktype>::setMRUUp(Blktype *blk)
{
    int i = assoc - 1;
    for(;i >= 0;--i)
    {
        if(blks[i] == blk)
        {
            MRU[i] = 1;
            MRU_count += 1;
            if(MRU_count >= assoc)
            {
                for(int j = 0; j < assoc; j++)
                    MRU[j] = 0;
                MRU[i] = 1;
                MRU_count = 1;
            }
            break;
        }
    }
}

template <class Blktype>
void
CacheSet<Blktype>::setMRUDown(Blktype *blk)
{
    int i = assoc - 1;
    for(;i >= 0;--i)
    {
        if(blks[i] == blk)
        {
            if(MRU[i] == 1)
                MRU_count -= 1;
            MRU[i] = 0;
            break;
        }
    }
}

template <class Blktype>
int
CacheSet<Blktype>::findMRU()
{
    for(int i = 0; i < assoc; i++)
        if(MRU[i] == 0)
            return i;
    return assoc - 1;
}

template <class Blktype>
void
CacheSet<Blktype>::setTREEDown(Blktype *blk)
{
    int i = assoc - 1;
    for(;i >= 0;--i)
        if(blks[i] == blk)
            break;
    if(i == -1)
        return;
    int index = assoc / 2;
    int last = 0;
    int li = 2;
    do{
        ////std::cout<<"i / index"<<i / index<<std::endl;
        MRU[last] = i / index;
        // //std::cout<<"down! assoc:"<<assoc<<" index:"<<index<<" i:"<<i<<" last:"<<last<<std::endl;
        ////std::cout<<"change MRU[last] down to: "<<MRU[last]<<std::endl;
        last = 2 * (last - li / 2 + 1) + i / index + li - 1;
        li *= 2;
        i -= (i / index) * index;
        index /= 2;
    }while(li <= assoc);
    // //std::cout<<"down MRU:"<<std::endl;
    // for(i = 0; i < assoc; i++)
    //     //std::cout<<MRU[i]<<" ";
    // //std::cout<<std::endl;
}

template <class Blktype>
void
CacheSet<Blktype>::setTREEUp(Blktype *blk)
{
    int i = assoc - 1;
    for(;i >= 0;--i)
        if(blks[i] == blk)
            break;
    if(i == -1)
        return;
    int index = assoc / 2;
    int last = 0;
    int li = 2;
    do{
        //td::cout<<"i / index"<<i / index<<"~i / index"<<~(i / index)<<std::endl;
        MRU[last] = 1 - (i / index);
        ////std::cout<<"up! assoc:"<<assoc<<" index:"<<index<<" i:"<<i<<" last:"<<last<<std::endl;
        ////std::cout<<"change MRU[last] up to: "<<MRU[last]<<std::endl;
        last = 2 * (last - li / 2 + 1) + i / index + li - 1;
        li *= 2;
        i -= (i / index) * index;
        index /= 2;
    }while(li <= assoc);
    // //std::cout<<"up MRU:"<<std::endl;
    // for(i = 0; i < assoc; i++)
    //     //std::cout<<MRU[i]<<" ";
    // //std::cout<<std::endl;
}

template <class Blktype>
int
CacheSet<Blktype>::findTREE()
{
    int li = 2;
    int last = 0;
    ////std::cout<<"assoc:"<<assoc<<std::endl;
    while(li < assoc)
    {
        ////std::cout<<"last:"<<last<<" MRU[last]:"<<MRU[last]<<" li:"<<li<<" 2 * (last - li / 2 + 1) + MRU[last] + li - 1 = "<<2 * (last - li / 2 + 1) + MRU[last] + li - 1<<std::endl;
        last = 2 * (last - li / 2 + 1) + MRU[last] + li - 1;
        li *= 2;
    }
    ////std::cout<<"result:"<<2 * (last - li / 2 + 1) + MRU[last]<<" assoc:"<<assoc<<" last:"<<last<<std::endl;
    return 2 * (last - li / 2 + 1) + MRU[last];
}

template <class Blktype>
void
CacheSet<Blktype>::hit2q(Blktype *blk)
{
    //std::cout<<"begin2q"<<std::endl;
    int i = assoc - 1;
    for(;i >= 0;--i)
        if(blks[i] == blk)
            break;
    if(i == -1)
        return;
    node *tmp = Am.front;
    if(Am.front)
    {
        if(tmp -> val == i)
        {
            Am.front = tmp -> next;
            Am.rear -> next = tmp;
            Am.rear = tmp;
            if(!Am.front)
                Am.front = tmp;
        }
        else
            while(tmp -> next && tmp -> next != Am.rear)
            {
                if(tmp -> next -> val == i)
                {
                    Am.rear -> next = tmp -> next;
                    Am.rear = tmp -> next;
                    if(!Am.front)
                        Am.front = tmp -> next;
                    tmp -> next = tmp -> next -> next;
                    break;
                }
                tmp = tmp -> next;
            }
    }
    if(A1.front)
    {
        tmp = A1.front;
        if(tmp -> val == i)
        {
            //std::cout<<"368: A1.front = "<<(tmp->next == NULL)<<" and num is "<<A1.num<<std::endl;
            A1.front = tmp -> next;
            if(A1.front == NULL)
                A1.rear = NULL;
            if(Am.rear)
                Am.rear -> next = tmp;
            Am.rear = tmp;
            if(!Am.front)
                Am.front = tmp;
            A1.num--;
            Am.num++;
        }
        else
        {
            while(tmp -> next && tmp -> next != A1.rear)
            {
                if(tmp -> next -> val == i)
                {
                    if(Am.rear)
                        Am.rear -> next = tmp -> next;
                    Am.rear = tmp -> next;
                    if(!Am.front)
                        Am.front = tmp -> next;
                    tmp -> next = tmp -> next -> next;
                    A1.num--;
                    Am.num++;
                    break;
                }
                tmp = tmp -> next;
            }
            if(A1.rear -> val == i)
            {
                if(Am.rear)
                    Am.rear -> next = A1.rear;
                Am.rear = A1.rear;
                if(!Am.front)
                    Am.front = A1.rear;
                A1.rear = tmp;
                if(!A1.front)
                    A1.front = tmp;
                tmp -> next = NULL;
                A1.num--;
                Am.num++;
            }
        }
    }
    //std::cout<<"end2q"<<std::endl;
}

template <class Blktype>
void
CacheSet<Blktype>::miss2q(Blktype *blk)
{
    //std::cout<<"beginmiss2q"<<std::endl;
    int i = assoc - 1;
    for(;i >= 0;--i)
        if(blks[i] == blk)
            break;
    if(i == -1)
        return;
    node *tmp, *del = NULL;
    bool finddel = false;
    if(A1.front)
    {
        tmp = A1.front;
        del = tmp -> next;
        if(tmp -> val == i)
        {
            //std::cout<<"423: A1.front = "<<(del == NULL)<<" and num is "<<A1.num<<std::endl;
            A1.front = del;
            if(A1.front == NULL)
                A1.rear = NULL;
            del = tmp;
            A1.num--;
            finddel = true;
        }
        else
            while(del)
            {
                if(del -> val == i)
                {
                    tmp -> next = del -> next;
                    finddel = true;
                    A1.num--;
                    break;
                }
                tmp = del;
                del = del -> next;
            }
    }
    if(!finddel && Am.front)
    {
        tmp = Am.front;
        del = tmp -> next;
        if(tmp -> val == i)
        {
            Am.front = del;
            del = tmp;
            Am.num--;
            finddel = true;
        }
        else
            while(del)
            {
                if(del -> val == i)
                {
                    tmp -> next = del -> next;
                    Am.num--;
                    finddel = true;
                    break;
                }
                tmp = del;
                del = del -> next;
            }
    }
    if(finddel)
    {
        if(A1.num >= A1thres)
        {
            del -> next = A1.front;
            //std::cout<<"473: A1.front = "<<(del == NULL)<<" and num is "<<A1.num<<std::endl;
            A1.front = del;
            if(A1.front == NULL)
                A1.rear = NULL;
            A1.num++;
            //std::cout<<"miss2q A1.num++ num is "<<A1.num<<std::endl;
        }
        else
        {
            del -> next = Am.front;
            Am.front = del;
            Am.num++;
        }
    }
    //std::cout<<"endmiss2q"<<std::endl;
}

template <class Blktype>
int
CacheSet<Blktype>::find2q()
{
    //std::cout<<"beginfind2q"<<std::endl;
    int index = 0;
    if(A1.num >= A1thres)
    {
        //std::cout<<"in A1: "<<A1.num<<" "<<A1thres<<std::endl;
        //std::cout<<"A1.front is "<<(A1.front == NULL)<<std::endl;
        //std::cout<<"A1.rear is "<<(A1.rear == NULL)<<std::endl;
        //std::cout<<"A1.front "<<A1.front -> val<<" next is "<<(A1.front -> next == NULL)<<std::endl;
        index = A1.front -> val;
        //std::cout<<"500: A1.front = "<<(A1.front -> next == NULL)<<" and num is "<<A1.num<<std::endl;
        A1.front = A1.front -> next;
        if(A1.front == NULL)
                A1.rear = NULL;
        A1.num--;
    }
    else if(Am.front)
    {
        index = Am.front -> val;
        Am.front = Am.front -> next;
        Am.num--;
    }
    //std::cout<<"endfind2q"<<std::endl;
    return index;
}

template <class Blktype>
void
CacheSet<Blktype>::insert2q(Blktype *blk)
{
    //std::cout<<"beginmins2q"<<std::endl;
    int i = assoc - 1;
    for(;i >= 0;--i)
        if(blks[i] == blk)
            break;
    if(i == -1)
        return;
    node *newnode = new node;
    newnode -> val = i;
    if(A1.rear)
        A1.rear -> next = newnode;
    else
        A1.front = newnode;
    A1.rear = newnode;
    A1.num++;
    //std::cout<<"miss2q A1.num++ num is "<<A1.num<<std::endl;
    //std::cout<<"endins2q"<<std::endl;
}

#endif
