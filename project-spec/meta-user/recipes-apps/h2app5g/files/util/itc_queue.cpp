#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include "itc_msg.h"
#include "mutex_lock.h"   // mutex_lock
#include "itc_queue.h"      // itc_queue

itc_queue::itc_queue()
{
    this->p1deq = NULL;
    this->p1enq = (itc_queue::elem *)this;
    this->num_elements  = 0;

    this->p1mutex = NULL;
}

itc_queue::itc_queue(itc_queue::elem *p1elem, void *p1arg, int num, int size)
{
    uint8_t *p1body = (uint8_t *)p1arg;

    this->p1deq = NULL;
    this->p1enq = (itc_queue::elem *)this;
    this->num_elements  = 0;

    this->p1mutex   = NULL;



    for(int kk = 0; kk < num; kk++) {
        this->link(&p1elem[kk], (void *)p1body);
        this->put(&p1elem[kk]);
        p1body += size;
    }
}

itc_queue::~itc_queue() {}

void itc_queue::put(itc_queue::elem *p1elem)
{
    // 1) case for num_elements = 0 
    //      this
    //      +-----------+                   
    //  +-->|      p1deq|----->NULL
    //  |   |           |                   
    //  |   |           |                   
    //  |   |      p1enq|--+                
    //  |   +-----------+  |                
    //  |                  |
    //  +------------------+
    //
    //  
    // 2) after this->put(p1elem_1)
    //      this                p1elem_1
    //      +-----------+       +-----------+
    //      |      p1deq|--+--->|     p1next|------>NULL
    //      |           |  |    |     p1body|
    //      |           |  |    |     p1comp|
    //      |      p1enq|--+    |     p1pool|
    //      +-----------+       +-----------+
    //                         
    // 3) after this->put(p1elem_2)
    //      this                p1elem_1            p1elem_2
    //      +-----------+       +-----------+       +-----------+
    //      |      p1deq|------>|     p1next|--+--->|     p1next|------>NULL
    //      |           |       |     p1body|  |    |     p1body|
    //      |           |       |     p1comp|  |    |     p1comp|
    //      |      p1enq|--+    |     p1pool|  |    |     p1pool|
    //      +-----------+  |    +-----------+  |    +-----------+
    //                     |                   |
    //                     +-------------------+
    mutex_lock  lock(this->p1mutex);


    if(this->num_elements <= 0) {
        this->p1deq = p1elem;
        this->p1enq = p1elem;   // update enqueue pointer
        this->p1enq->p1next = NULL;
    }
    else {
        this->p1enq->p1next = p1elem;
        this->p1enq         = p1elem;   // update enqueue pointer
        this->p1enq->p1next = NULL;
    }

    this->num_elements++;
} // end of itc_queue::put(itc_queue::elem *p1elem)

itc_queue::elem * itc_queue::get()
{
    // 1) case for num_elements = 2 
    //      this                p1elem_1            p1elem_2 
    //      +-----------+       +-----------+       +-----------+
    //      |      p1deq|------>|     p1next|--+--->|     p1next|----->NULL
    //      |           |       |     p1body|  |    |     p1body|
    //      |           |       |     p1comp|  |    |     p1comp|
    //      |      p1enq|--+    |     p1pool|  |    |     p1pool|
    //      +-----------+  |    +-----------+  |    +-----------+
    //                     |                   | 
    //                     +-------------------+ 
    //  
    // 2) after this->get()
    //                          p1elem_1                     
    //                          +-----------+                    
    //                          |     p1next|                    
    //                          |     p1body|                    
    //                          |     p1comp|                    
    //                          |     p1pool|                    
    //                          +-----------+                    
    //                                                            
    //      this                                    p1elem_2
    //      +-----------+                           +-----------+
    //      |      p1deq|----------------------+--->|     p1next|----->NULL
    //      |           |                      |    |     p1body|
    //      |           |                      |    |     p1comp|
    //      |      p1enq|--+                   |    |     p1pool|
    //      +-----------+  |                   |    +-----------+
    //                     |                   | 
    //                     +-------------------+ 
    //  
    // 3) after this->get()
    //                                              p1elem_2 
    //                                              +-----------+                    
    //                                              |     p1next|                    
    //                                              |     p1body|                    
    //                                              |     p1comp|                    
    //                                              |     p1pool|                    
    //                                              +-----------+                    
    //                                                            
    //      this                                              
    //      +-----------+                                        
    //      |      p1deq|----------------------+---------------------->NULL
    //      |           |                      |                 
    //      |           |                      |                 
    //      |      p1enq|--+                   |                 
    //      +-----------+  |                   |                 
    //                     |                   | 
    //                     +-------------------+ 

    itc_queue::elem *p1elem = NULL;

    mutex_lock  lock(this->p1mutex);

    if((this->p1deq != NULL) && (this->num_elements > 0)){
        if(this->p1deq == this->p1enq) {    // case for empty
            this->p1enq = (itc_queue::elem *)this;
        }

        p1elem = this->p1deq;
        this->p1deq = this->p1deq->p1next;

        this->num_elements--;
    }
    else {
        p1elem = NULL;
    }

    return(p1elem);
}

int itc_queue::count() const
{
    return(this->num_elements);
}

void itc_queue::link(itc_queue::elem *p1elem, void *p1arg)
{
    p1elem->p1next = NULL;
    p1elem->p1body = p1arg;
    p1elem->p1comp = NULL;
    p1elem->p1pool = this;
}

void itc_queue::set_name(const std::string &name)
{
    this->name = name;
}

void itc_queue::install_mutex(void *p1arg)
{
    this->p1mutex = p1arg;
}

void itc_queue::flush()
{
    mutex_lock  lock(this->p1mutex);

    for( ; this->count() > 0; ) {
        itc_queue::elem *p1elem;

        if((p1elem = this->get()) != NULL) {
            // loop#0)
            //       p1elem              p1next_comp 
            //      +-----------+       +-----------+
            //      |     p1next|       |     p1next|
            //      |     p1body|       |     p1body|
            //      |     p1comp|------>|     p1comp|----->NULL
            //      |     p1pool|       |     p1pool|
            //      +-----------+       +-----------+
            //
            // loop#1)
            //                           p1elem           p1next_comp 
            //      +-----------+       +-----------+
            //      |     p1next|       |     p1next|
            //      |     p1body|       |     p1body|
            //      |     p1comp|->NULL |     p1comp|----->NULL
            //      |     p1pool|       |     p1pool|
            //      +-----------+       +-----------+
            //                       
            //                       
            for( ; p1elem != NULL; ) {
                itc_queue::elem *p1next_comp = p1elem->p1comp;  // for next
                p1elem->p1comp = NULL;                          // clear current p1elem
                p1elem->p1pool->put(p1elem);                    // push back the current p1elem to pool
                p1elem = p1next_comp;                           // swap
            }
        }
        else {
            break;
        }
    }
}

void itc_queue::dump()
{
    int cnt = this->count();
    if(cnt > 0) {
        itc_queue::elem *p1elem = this->p1deq;
        for(int ll = 0; ll < cnt; ll++) {
            itc_msg *p1msg = (itc_msg *)p1elem->p1body;
            std::cout << ", ("<< p1msg->hdr.msgid << "," << p1msg->hdr.msglen << ")";
            p1elem = this->p1deq->p1next;
        }
        std::cout << std::endl;
    }
}

