#ifndef _ANT_CARRIER_H_
#define _ANT_CARRIER_H_

#include "num_def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include "stm.h"
#include "itc_msg.h"    // struct playback_struct;

class ant_carrier //: public stm
{
    public:
        ant_carrier();
        virtual ~ant_carrier();

    public:
        int ant;
        playback_struct *p1pl;
        section_struct  *p2sc[NUM_SECTION_PER_CARRIER];

};

#endif // _ANT_CARRIER_H_
