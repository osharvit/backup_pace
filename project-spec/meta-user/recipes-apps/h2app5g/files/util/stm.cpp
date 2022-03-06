#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>      // std::stringstream
#include "stm.h"

stm::stm():
    numstate(1),
    prevstate(0),
    currstate(0),
    nextstate(0)
{
    this->p1state_handler = new struct _statehndler[this->numstate];

    for(size_t kk = 0; kk < this->numstate; kk++){
        this->install_state_handler(kk, &stm::default_entry, &stm::default_state, &stm::default_exit);
    }

    set_stm_id(0);

    state_changed_callback = NULL;
    state_changed_argument = NULL;
}

stm::stm(size_t n_state):
    numstate(n_state),
    prevstate(0),
    currstate(0),
    nextstate(0)
{
    this->p1state_handler = new struct _statehndler[this->numstate];

    for(size_t kk = 0; kk < this->numstate; kk++){
        this->install_state_handler(kk, &stm::default_entry, &stm::default_state, &stm::default_exit);
    }

    set_stm_id(0);

    state_changed_callback = NULL;
    state_changed_argument = NULL;
}

stm::~stm()
{
    if(this->p1state_handler != NULL) {
        delete [] this->p1state_handler;
    }
}

void stm::default_entry()
{
}

size_t stm::default_state(void *p1arg, int event)
{
    (void)(p1arg);  // to avoid of unused warning
    (void)(event);  // to avoid of unused warning

    return(currstate);
}

void stm::default_exit()
{
}

size_t stm::get_curr_state() const
{
    return(this->currstate);
}

size_t stm::get_next_state() const
{
    return(this->nextstate);
}

void stm::install_state_handler(size_t state,
                                void (stm::*entry_func)(),
                                size_t (stm::*state_func)(void *, int),
                                void (stm::*exit_func)())
{
    if(state < this->numstate) {
        this->p1state_handler[state].entry_func = static_cast<void (stm::*)()>(entry_func);
        this->p1state_handler[state].state_func = static_cast<size_t (stm::*)(void *, int)>(state_func);
        this->p1state_handler[state].exit_func  = static_cast<void (stm::*)()>(exit_func);
    }
} // end of stm::install_state_handler(...)

void stm::install_state_handler(size_t state, size_t (stm::*state_func)(void *, int))
{
    if(state < this->numstate) {
        this->p1state_handler[state].state_func = static_cast<size_t (stm::*)(void *, int)>(state_func);
    }
} // end of stm::install_state_handler(...)

size_t stm::dispatch_event(void *p1arg, int event)
{
    transit_state(nextstate, false);

    return((this->*p1state_handler[this->currstate].state_func)(p1arg, event));
} // end of stm::dispatch_event(...)

void stm::transit_state(size_t next_state, bool reentry)
{
    if((next_state == this->currstate) && (reentry == false)) return;

    (this->*p1state_handler[this->currstate].exit_func)();

    if((state_changed_callback != NULL) && (state_changed_argument != NULL)){
        state_changed_callback(state_changed_argument, stm_id, this->currstate, next_state);
    }
    this->prevstate = this->currstate;
    this->nextstate = this->currstate = next_state;

    (this->*p1state_handler[this->nextstate].entry_func)();
} // end of  stm::transit_state(...)

void stm::set_stm_id(size_t id)
{
    stm_id = id;
}

void stm::install_state_changed_callback(void (*callback)(void *, size_t, size_t, size_t), void *argument)
{
    this->state_changed_callback = callback;
    this->state_changed_argument = argument;
} // end of void stm::install_state_changed_callback()
