#ifndef __STM_H__
#define __STM_H__

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>      // std::stringstream

class stm
{
    public:
        stm();
        stm(size_t n_state);
        virtual ~stm();

        void install_state_handler(size_t state,
                                   void (stm::*entry_func)(),
                                   size_t (stm::*state_func)(void *, int),
                                   void (stm::*exit_func)());
        void install_state_handler(size_t state, size_t (stm::*state_func)(void *, int));

        void install_state_changed_callback(void (*callback)(void *, size_t, size_t, size_t), void *argument);

        size_t get_curr_state() const;
        size_t get_next_state() const;

        size_t dispatch_event(void *p1arg, int event);
        void transit_state(size_t next_state, bool reentry=true);
        void set_stm_id(size_t id);

    protected:
        void default_entry();
        size_t default_state(void *p1arg, int event);
        void default_exit();

    protected:
        struct _statehndler {
            void (stm::*entry_func)();
            size_t (stm::*state_func)(void *, int);
            void (stm::*exit_func)();
        }*p1state_handler;

        size_t numstate;
        size_t prevstate;
        size_t currstate;
        size_t nextstate;

        size_t stm_id;

        void (*state_changed_callback)(void *state_changed_argument, size_t stm_id, size_t curr_state, size_t next_state);
        void  *state_changed_argument;
};

#endif // __STM_H__
