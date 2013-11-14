#ifndef PROFILE_H_
#define PROFILE_H_


#include "Profile.h"

enum FLOW_STATE {
  PREHEAT, SOAK, REFLOW,DWELL,COOLDOWN
};

// The state enum
FLOW_STATE flow_state;

// The profile parameters, can be set in the constructor
float preheat;
float soak;
float peak;
float Tliq;

float preheatRamp;
float cooldownRamp;
	
float preheatTime;
float soakTime;
float dwellTime;
float reflowTime;

Profile::Profile() {
// Initial state
	flow_state = PREHEAT;

	// Profile parameters can be set here
	// The parameters are set for Sn/Pb and Sn/Pb/Ag solder paste

	// Profile temperature values in Celsius
	preheat	= 125; 			// Leave at 125C
	soak 	= 170; 			// Soak temp should be close to Tl
	peak 	= 220; 			// Max peak = 230C
	Tliq 	= 183; 			// liquidus temp ~180C

	// Profile ramp rates in deg. C per Sec
	preheatRamp 	= 1.5; 	// Max ramp = 2.5C/Sec
	cooldownRamp 	= 3.0; 	// Max ramp = 3.0C/Sec

	// Profile time values in Seconds
	preheatTime 	= preheat / preheatRamp;
	soakTime 		= 120; 	// Max soak time = 120S
	dwellTime 		= 30; 	// Max dwell time = 30S
	reflowTime 		= 65 - dwellTime - ((peak - Tliq) / cooldownRamp); // Max total time = 75Sec
}

Profile::~Profile() {
}

float Profile::getTemp(int time) {
    
	// The target temperature in C
	float target;

	switch (flow_state) {
	case PREHEAT: // Pre-heat to 125 with max 1.5C/Sec
		target = (float) ((float) time * 1.5);
		if (time > preheatTime)
			flow_state = SOAK;
		break;

	case SOAK: // Increase and hold temperature so all components are at the same temperature
		if (time < preheatTime + 15) // In first 10 seconds raise the temp to the soak temp
			target = preheat + ((soak - preheat) / 15) * (time - preheatTime);
		else if (time > preheatTime + soakTime - 10) // In last 10 seconds raise the temp to Tliq
			target = soak + ((Tliq - soak) / 10) * (time - (preheatTime + soakTime - 10));
		else //Set the temp  to soak teamp
			target = soak;

		if (time > preheatTime + soakTime)
			flow_state = REFLOW;
		break;

	case REFLOW: // Increase temperature to peak temperature, solder paste becomes liquidus
		target = Tliq + ((peak - Tliq) / reflowTime) * (time - (preheatTime + soakTime));
		if (time > preheatTime + soakTime + reflowTime)
			flow_state = DWELL;
		break;

	case DWELL: // Hold peak temperature
		target = peak;
		if (time > preheatTime + soakTime + reflowTime + dwellTime)
			flow_state = COOLDOWN;
		break;

	case COOLDOWN: // Stop and let the oven cooldown with a max 3C/Sec
		target = peak - cooldownRamp * ((float) time - (preheatTime + soakTime + reflowTime + dwellTime));
		if (target < 0)
			target = 0;
		break;

	default:
		target = 0;
		break;
	}

	return target;
}

#endif /* PROFILE_H_ */
