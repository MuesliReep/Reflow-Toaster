#ifndef PROFILE_H_
#define PROFILE_H_


#include "Profile.h"

enum FLOW_STATE {
  PREHEAT, SOAK, REFLOW,DWELL,COOLDOWN
};

FLOW_STATE flow_state;
	
Profile::Profile() {
  flow_state = PREHEAT;
}

Profile::~Profile() {
}

float Profile::getTemp(int time) {
    
   float target;
    
   switch(flow_state) {
     case PREHEAT:   //Pre-heat to 125 with max 1.5C/Sec
       target = (float)time * 1.5;
       if (time > 100)
         flow_state = SOAK;
       break;
     case SOAK:      //Increase temp to 170.0 in 30 sec and hold for a total of 120 sec
       target = 170.0;
       if (time > 220)
         flow_state = REFLOW;
       break;
     case REFLOW:    //Increase temp to 235C in 45 sec
       target = 235.0;
       if (time > 265)
         flow_state = DWELL;
       break;
     case DWELL:     //Hold temperature for 30 sec
       target = 235.0;
       if (time > 295)
         flow_state = COOLDOWN;
       break;
     case COOLDOWN:  //Stop and let the oven cooldown with a max 3C/Sec
       target = 0;
       break;
     default:
       target = 0;
       break;
   }
   
   return target;
}

#endif /* PROFILE_H_ */
