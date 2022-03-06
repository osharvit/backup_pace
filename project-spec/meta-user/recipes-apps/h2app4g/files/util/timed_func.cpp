#include "num_def.h"    // NOTE: this "num_def.h" should always come before other header files.
#include "timed_func.h"

// static instantiation
void  (*timed_func::mp1upperwmcallback)(void *) = NULL;
void   *timed_func::mp1upperwmarg = NULL;

timed_func::timed_func():
    mp1mutex(NULL),
    item_count(0),
    upperwmthreshold(LIST_SIZE)
{
    this->mp1upperwmcallback = defaultfunction;
    this->mp1upperwmarg = NULL;

    for(size_t kk = 0; kk < LIST_SIZE; kk++){
        this->mp1funclist[kk].p1func = defaultfunction;
        this->mp1funclist[kk].p1arg = NULL;
        this->mp1funclist[kk].offset = 0;
        this->mp1funclist[kk].interval = 1;
        this->mp1funclist[kk].enable = true;
    }
}   // end of timed_func::timed_func()

timed_func::~timed_func() {}

size_t timed_func::get_count() const
{
    mutex_lock  lock(mp1mutex); // NOTE: constructor and destructor call pthread_mutex_lock() and pthread_mutex_unlock(), respectively.

    return(item_count);
}

bool timed_func::is_time(size_t tick, size_t idx) const
{
    bool result = false;

    mutex_lock  lock(mp1mutex); // NOTE: constructor and destructor call pthread_mutex_lock() and pthread_mutex_unlock(), respectively.

    if(this->mp1funclist[idx].enable) {
        result = (this->mp1funclist[idx].offset == (tick % std::max(this->mp1funclist[idx].interval, (size_t)1)))? true: false;
    }

    return(result);
}

void timed_func::exec_item(size_t idx)
{
    (*mp1funclist[idx].p1func)(mp1funclist[idx].p1arg);
} // end of void timed_func::exec_item(...)

void timed_func::defaultfunction(void *p1arg)
{
    (void)(p1arg);    // to avoid of warning

    if(g_debug_mask & (1UL << MASKBIT_CALL)){
        TRACE0();
    }
} // end of void timed_func::defaultfunction(...)

void timed_func::install_mutex(void *p1arg)
{
    this->mp1mutex = p1arg;
} // end of timed_func::install_mutex(...)

void timed_func::install_upper_watermark(size_t threshold, void (*p1func)(void *), void *p1arg)
{
    this->upperwmthreshold = threshold;
    if(p1func != NULL) {
        this->mp1upperwmcallback = p1func;
        this->mp1upperwmarg = p1arg;
    }
    else {
        this->mp1upperwmcallback = defaultfunction;
        this->mp1upperwmarg = NULL;
    }
} // end of timed_func::install_upper_watermark(...)

int timed_func::add_item(void (*p1func)(void *), void *p1arg, int offset, int interval, bool override)
{
    int k = 0;
    int result = -1;

    if(item_count >= LIST_SIZE) {
        TRACE3(std::cerr, timed_func, 0) << "error: list is full" << item_count << std::endl;
        return(-1);
    }

    mutex_lock  lock(mp1mutex); // NOTE: constructor and destructor call pthread_mutex_lock() and pthread_mutex_unlock(), respectively.

    // find if it is an existing item
    for(k = 0; k < item_count; k++) {
        if((mp1funclist[k].p1func == p1func) && (mp1funclist[k].p1arg == p1arg))
            break;
    }
    if((k < item_count) && (!override)){
        TRACE3(std::cerr, timed_func, 0) << "error: already exist as " << k << std::endl;
    }
    else {
        mp1funclist[k].p1func = p1func;
        mp1funclist[k].p1arg = p1arg;
        mp1funclist[k].offset = offset;
        mp1funclist[k].interval = interval;
        mp1funclist[k].enable = true;

        if(k < item_count){
            result = item_count;
        }
        else {
            result = ++item_count;

            if(item_count == upperwmthreshold) {
                mp1upperwmcallback((void *)&item_count);
            }
        }
    }

    return(result);
} // end of timed_func::add_item(...)

int timed_func::del_item(void (*p1func)(void *), void *p1arg)
{
    int result = -1;
    int k;

    if(item_count <= 0) {
        TRACE3(std::cerr, timed_func, 0) << "list is empty" << std::endl;
        return(-1);
    }

    mutex_lock  lock(this->mp1mutex); // NOTE: constructor and destructor call pthread_mutex_lock() and pthread_mutex_unlock(), respectively.

    // find if it is an existing item
    for(k = 0; k < item_count; k++) {
        if((mp1funclist[k].p1func == p1func) && (mp1funclist[k].p1arg == p1arg))
           break;
    }
    if(k < item_count) {
        // shift
        for( ; k < (item_count - 1); k++) {
            mp1funclist[k].p1func   = mp1funclist[k+1].p1func;
            mp1funclist[k].p1arg    = mp1funclist[k+1].p1arg;
            mp1funclist[k].offset   = mp1funclist[k+1].offset;
            mp1funclist[k].interval = mp1funclist[k+1].interval;
            mp1funclist[k].enable   = mp1funclist[k+1].enable;
        }
        for( ; k < item_count; k++) {
            mp1funclist[k].p1func   = defaultfunction;
            mp1funclist[k].p1arg    = NULL;
            mp1funclist[k].offset   = 0;
            mp1funclist[k].interval = 1;
        }
        result = --item_count;
    }
    else {
        result = -1;
    }

    return(result);
} // end of timed_func::del_item(...)
