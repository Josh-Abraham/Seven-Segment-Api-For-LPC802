/****************************************************************
 * 																*
 *						Seven Segment API						*
 * 			  		 Written By: Joshua Abraham					*
 * 			Some code originally from Dr.James Smith			*
 * 			 				Version: 1.0.0 			 			*
 * 		    			Written: April 13th, 2020				*
 * 							Board Used: OM4000					*
 * 																*
 ****************************************************************/

#include "LPC802.h"
#include "clock_config.h"
#include "seven_segment.h"
#include "stdbool.h"
#include "ctype.h"


#define MRT_REPEAT		(0)
#define MRT_CHAN0		(0)
#define MRT_CHAN1		(1)

// 7 Segment GPIO Pin outputs
int segments[7] = {-1, -1, -1, -1, -1, -1, -1};

//decimalPoint GPIO locations
int dp = -1;
// Disabled to Start
bool enableDP = false;

// Pin Assignments of digits enabled, also controls which digits if not all are used
int digits[4] = {-1, -1, -1 , -1};

// Type is either common anode (0) or common cathode (1)
// NOTE: This System was designed on a common cathode display
int sevenSegType = 1;

// Char Sequence to be used as the input
char chrSequence[4] = "    ";
int currentDigit = -1;
int currentClock = -1;
// Refresh rate for the display
int cycleRate = -1;


// Count Down Clock
int startCount = -1;
int currentCount = -1;
// Normalized -> count within 9999 & 0000 limit but the counter can exceed this
int normalizedCount = -1;
// Either 'Up' or 'Down'
char countDirection[4] = "";
// Speed at which counting happens -> how often the interrupt is triggered
int countRate = -1;
// The increment for the counter
int countIncrement = 0;
// WHich clock is used for the counter
int countClock = -1;

bool pauseCounter = false;
bool enableCountStopValue = true;
// A preset stop value to end the counting sequence, if not enabled, the counter wraps around
int countStopValue;

// Carousel Component -> allows for larger segments of text
// Transition clock is the clock for the carousel motion & rate determines speed
int transitionClock = -1;
int transitionRate = -1;
int transitionIndex = -1;
// How long the sentence/characters being displayed is
int carouselSequenceLength = -1;
char carouselSequence[] = "";
// Continuous carousel or one-shot
bool enableContinousCycle = false;
bool carouselOverflow = false;
bool pauseCarouselTransition = false;

// Slider Component -> slides a new set of 4 characters onto the screen
char sliderSequence[] = "";
int sliderSequenceLength = -1;
bool pauseSliderTransition = false;
int sliderTransitionIndex = -1;


/************************************************************************************************
 * 																								*
 * 								Seven Segment Configuration Functions							*
 * 						Used for initial setup and re-setup of the display 						*
 * 				Sets Digits, Segments and Decimal Points and their respective GPIOs				*
 * 																								*
 ************************************************************************************************/

/*
 * Function:  setupSevenSegment
 * --------------------
 * Sets up the GPIO Assignments
 * This function is called internally whenever the Digit GPIOs or Segment GPIOs change
 *
 * Return: no return
 */
void setupSevenSegment() {
	SYSCON->SYSAHBCLKCTRL0 |= (SYSCON_SYSAHBCLKCTRL0_GPIO0_MASK); // GPIO is on
	// Loop through the digits, turning them on if set to a good value (not -1)
	for (int i = 0; i < 4; i++) {
		if (digits[i] != -1) {
			GPIO->DIRSET[0] = (1UL<<digits[i]);
			if (sevenSegType == 1) {
				GPIO->SET[0] = (1UL<<digits[i]); // Set the Digit bit i to 1
			} else {
				GPIO->CLR[0] = (1UL<<digits[i]); // Set the Digit bit i to 0
			}
		}
	}

	// Set the direction of the gpios for the segments, allows for either an enable bit or not
	for(int i = 0; i < 7; i++) {
		GPIO->DIRSET[0] = (1UL<<segments[i]);
	}
}


/*
 * Function:  digitGPIOSetup
 * --------------------
 * Sets up the GPIO Assignments for the 4 GPIOs to be used for digits
 * Internally calls setupSevenSegment
 *
 * channels: list of up to 4 GPIO pins to be used for the digits of the display
 *
 * Return: no return
 */
void digitGPIOSetup(int channels[]) {
	for (int i = 0; i < 4; i++) {
		digits[i] = channels[i];
	}
	setupSevenSegment();
}


/*
 * Function:  sevenSegmentGPIOSetup
 * --------------------
 * Sets up the GPIO Assignments for the 4 GPIOs to be used for digits
 * Internally calls setupSevenSegment
 *
 * segs: list of up to 7 GPIO pins to be used for the display (Should be ordered from Segment A .. Segment G)
 *
 * Return: no return
 */
void sevenSegmentGPIOSetup(int segs[]) {
	for (int i = 0; i < 7; i++) {
		segments[i] = segs[i];
	}
	setupSevenSegment();
}

/*
 * Function:  setSevenSegmentType
 * --------------------
 * Sets up the type of seven segment to be used
 *
 * type: 0 = common anode, otherwise common cathode
 *
 * Return: no return
 */
void setSevenSegmentType(int type) {
	if (type == 0) {
		sevenSegType = type;
	} else {
		type = 1;
	}
}

/*
 * Function:  enableDecimalSegment
 * --------------------
 * Enable or disable the Decimal Segment
 *
 * decimalSegment: GPIO Pin for decimal point, -1 means disabled
 *
 * Return: no return
 */
void enableDecimalSegment(int decimalSegment) {
	dp = decimalSegment;
	enableDP = true;
	GPIO->DIRSET[0] = (1UL<<dp);
}

/*
 * Function: sevenSegmentFullSetup
 * --------------------
 * Do a full reset/setup rather than just portions
 *
 * decimalSegment: GPIO Pin for decimal point, -1 means disabled
 *
 * Return: no return
 */
void sevenSegmentFullSetup(int channels[], int segs[], int decimalSegment) {
	for (int i = 0; i < 4; i++) {
		digits[i] = channels[i];
	}
	for (int i = 0; i < 7; i++) {
		segments[i] = segs[i];
	}
	dp = decimalSegment;
	setupSevenSegment();
}


/*
 * Function: clearDigits
 * --------------------
 * Clear the content being displayed
 * Used internally for switching between digits
 *
 *
 * Return: no return
 */
void clearDigits() {
	for (int i = 0; i < 4; i++) {
		if (digits[i] != -1) {
			if (sevenSegType == 1) {
				GPIO->SET[0] = (1UL<<digits[i]); // Set the Digit bit i to 1
			} else {
				GPIO->CLR[0] = (1UL<<digits[i]); // Set the Digit bit i to 0
			}
		}
	}
}

/*
 * Function: enableDigits
 * --------------------
 * If only 1 character is being displayed then there is no need to loop so all digits are turned on
 * If a User wishes to display 0000, or 8888, or ---- for example
 *
 *
 * Return: no return
 */
void enableDigits() {
	for (int i = 0; i < 4; i++) {
		if (digits[i] != -1) {
			if (sevenSegType == 1) {
				GPIO->CLR[0] = (1UL<<digits[i]); // Set the Digit bit i to 0
			} else {
				GPIO->SET[0] = (1UL<<digits[i]); // Set the Digit bit i to 1
			}
		}
	}
}

/*
 * Function: enableDigit
 * --------------------
 * Pick the next digit in the sequence to enable & disable the rest
 * Function used internally for swapping currently displayed digit
 *
 * digitPlace: the current digit(0..3) that needs to be updated
 *
 * Return: no return
 */
void enableDigit(int digitPlace) {
	if (digits[digitPlace] != -1) {
		if (sevenSegType == 1) {
			GPIO->CLR[0] = (1UL<<digits[digitPlace]); // Set the Digit bit i to 0

		} else {
			GPIO->SET[0] = (1UL<<digits[digitPlace]); // Set the Digit bit i to 1
		}
	}

}

/*
 * Function: displayDP
 * --------------------
 * Display the decimal point or not
 * Function used internally to determine if the decimal point should be displayed
 *
 * Return: no return
 */
void displayDP() {
	if (enableDP) {
		if (sevenSegType == 1) {
			GPIO->SET[0] = (1UL<<dp); // Turn on decimal point Segment
		} else {
			GPIO->CLR[0] = (1UL<<dp); // Turn on decimal point Segment
		}
	} else {
		if (sevenSegType == 1) {
			GPIO->CLR[0] = (1UL<<dp); // Turn off decimal point Segment
		} else {
			GPIO->SET[0] = (1UL<<dp); // Turn off decimal point Segment
		}
	}
}

/*
 * Function: displayValue
 * --------------------
 * Display a single value from binary sequence of 7 segments
 * Function used internally to determine which binary value to display
 * Calls displayDP internally
 *
 * binaryValue: binary representation in 7-segment format wishing to be displayed
 *
 * Return: no return
 */
void displayValue(int binaryValue) {
	int segmentCount = 7;
	for(int i = 0; i < segmentCount; i++) {
		int bit = (binaryValue >> i) & 1;
		if ((bit == 1 && sevenSegType == 1) || (bit == 0 && sevenSegType == 0)) {
			GPIO->SET[0] = (1UL<<segments[i]); // Turn on Segment
		} else {
			GPIO->CLR[0] = (1UL<<segments[i]); // Turn on Segment
		}
	}

	displayDP();
}


/*
 * Function: toggleDecimalPoint
 * --------------------
 * Turn on/off the decimal point
 * Calls displayDP internally
 *
 * Return: no return
 */
void toggleDecimalPoint() {
	enableDP = !enableDP;
	displayDP();
}


/*
 * Function: setDecimalPoint
 * --------------------
 *  Turn on the decimal point
 * Calls displayDP internally
 *
 * Return: no return
 */
void setDecimalPoint() {
	enableDP = true;
	displayDP();
}

/*
 * Function: clearDecimalPoint
 * --------------------
 * Turn off the decimal point
 *
 * Return: no return
 */
void clearDecimalPoint() {
	enableDP = false;
	displayDP();
}


/************************************************************************************************
 * 																								*
 * 										Non-Timer Configurations								*
 * 									Allow for single input displays								*
 * 																								*
 ************************************************************************************************/

/*
 * Function: displaySingleCharacter
 * --------------------
 * If only 1 character is being displayed then there is no need to use a timer
 * If a User wishes to display 0000, or 8888, or ---- for example
 * Calls clearDigits and enableDigits internally
 *
 * inputChar: Single character to display on all active digit displays
 *
 * Return: no return
 */
void displaySingleCharacter(char inputChar) {
	clearDigits();
	char upperCaseChar = toupper(inputChar);
	int size = sizeof(inputCharacterOptions)/sizeof(inputCharacterOptions[0]);//Method
	for (int i = 0; i < 4; i++) {
		chrSequence[3-i] = inputChar;
	}
	for (int i = 0; i < size; i++) {
		if (upperCaseChar == inputCharacterOptions[i]) {
			displayValue(displayOptions[i]);
		}
	}
	enableDigits();
}

/*
 * Function: displaySingleInt
 * --------------------
 * Wrapper Function to allow integers to be displayed as well
 * Mainly for clarity of use for developers
 * If only 1 character is being displayed then there is no need to loop
 * If a User wishes to display 0000, or 8888 for example
 * Calls displaySingleCharacter internally
 *
 * inputNum: Single integer to display on all active digit displays
 *
 * Return: no return
 */
void displaySingleInt(int inputNum) {
	char characterValue = inputNum +'0';
	displaySingleCharacter(characterValue);
}


/************************************************************************************************
 * 																								*
 * 										Timer Configurations 									*
 * 				Clocks are used internally for various tasks within 7-segment displays			*
 * 		Users can choose which clocks they wish to use simply by specifying it as a parameter	*											*
 * 		Clock Setup handles all clocks used (refresh, counter, and carousel/slider transition	*
 * 							Clock Choices: SysTick, WKT, MRT0, MRT1, CTIMER0					*
 * 																								*
 ************************************************************************************************/

/*
 * Function: SysTick_Configuration_Seven_Segment
 * --------------------
 * Enables the SysTick Timer for use
 * Function called internally
 *
 * Return: no return
 */
void SysTick_Configuration_Seven_Segment() {
	__disable_irq();
	NVIC_DisableIRQ(SysTick_IRQn); // turn off the SysTick interrupt.
	if (currentClock == 0) {
		SysTick_Config(cycleRate);
	} else if (countClock == 0) {
		SysTick_Config(countRate);
	} else if (transitionClock == 0) {
		SysTick_Config(transitionRate);
	}

	NVIC_EnableIRQ(SysTick_IRQn); // SysTick IRQs are on.
	__enable_irq();
}

/*
 * Function: WKT_Configuration_Seven_Segment
 * --------------------
 * Enables the WakeUp Timer for use continuously
 * Note that the WKT uses the Low Power Oscillator
 * Function called internally
 *
 * Return: no return
 */
void WKT_Configuration_Seven_Segment() {
	__disable_irq();
	SYSCON->SYSAHBCLKCTRL0 |= (SYSCON_SYSAHBCLKCTRL0_WKT_MASK);

	NVIC_DisableIRQ(WKT_IRQn); // turn off the WKT interrupt.
	SYSCON->PDRUNCFG &= ~(SYSCON_PDRUNCFG_LPOSC_PD_MASK);
	SYSCON->LPOSCCLKEN |= (SYSCON_LPOSCCLKEN_WKT_MASK);
	SYSCON->PRESETCTRL0 &= ~(SYSCON_PRESETCTRL0_WKT_RST_N_MASK);
	SYSCON->PRESETCTRL0 |= (SYSCON_PRESETCTRL0_WKT_RST_N_MASK);
	WKT->CTRL = WKT_CTRL_CLKSEL_MASK;

	if (currentClock == 1) {
		WKT->COUNT = cycleRate;
	} else if (countClock == 1) {
		WKT->COUNT = countRate;
	} else if (transitionClock == 1) {
		WKT->COUNT = transitionRate;
	}


	NVIC_EnableIRQ(WKT_IRQn);
	__enable_irq();
}


/*
 * Function: MRT_Configuration_Seven_Segment
 * --------------------
 * Enables the MultiRate Timer for use
 * Can use 1 or 2 channels depending on actions chosen
 * Function called internally
 *
 * Return: no return
 */
void MRT_Configuration_Seven_Segment(int channel) {
	__disable_irq(); // turn off globally
	NVIC_DisableIRQ(MRT0_IRQn);
	SYSCON->SYSAHBCLKCTRL0 |= (SYSCON_SYSAHBCLKCTRL0_MRT_MASK);

	SYSCON->PRESETCTRL0 &= ~(0x400); 	//~(SYSCON_PRESETCTRL0_MRT_RST_N_MASK);
	SYSCON->PRESETCTRL0 |= 0x400; 		//(SYSCON_PRESETCTRL0_WKT_RST_N_MASK);
	if (currentClock == 2 && countClock == 3) {
		MRT0->CHANNEL[0].CTRL = (MRT_REPEAT << MRT_CHANNEL_CTRL_MODE_SHIFT | MRT_CHANNEL_CTRL_INTEN_MASK);
		MRT0->CHANNEL[1].CTRL = (MRT_REPEAT << MRT_CHANNEL_CTRL_MODE_SHIFT | MRT_CHANNEL_CTRL_INTEN_MASK);
		MRT0->CHANNEL[0].INTVAL = cycleRate | (MRT_CHANNEL_INTVAL_LOAD_MASK);
		MRT0->CHANNEL[1].INTVAL = countRate | (MRT_CHANNEL_INTVAL_LOAD_MASK);
	} else if (currentClock == 3 && countClock == 2) {
		MRT0->CHANNEL[0].CTRL = (MRT_REPEAT << MRT_CHANNEL_CTRL_MODE_SHIFT | MRT_CHANNEL_CTRL_INTEN_MASK);
		MRT0->CHANNEL[1].CTRL = (MRT_REPEAT << MRT_CHANNEL_CTRL_MODE_SHIFT | MRT_CHANNEL_CTRL_INTEN_MASK);
		MRT0->CHANNEL[1].INTVAL = cycleRate | (MRT_CHANNEL_INTVAL_LOAD_MASK);
		MRT0->CHANNEL[0].INTVAL = countRate | (MRT_CHANNEL_INTVAL_LOAD_MASK);
	} else {
		int tempCount = 0;
		if (channel == 0 && currentClock == 2) {
			tempCount = cycleRate;
		} else if (channel == 1 && currentClock == 3) {
			tempCount = cycleRate;
		} else if (channel == 0 && countClock == 2) {
			tempCount = countRate;
		} else if (channel == 1 && countClock == 3) {
			tempCount = countRate;
		} else if (transitionClock == 2 || transitionClock == 3) {
			tempCount = transitionRate;
		}
		MRT0->CHANNEL[channel].CTRL = (MRT_REPEAT << MRT_CHANNEL_CTRL_MODE_SHIFT | MRT_CHANNEL_CTRL_INTEN_MASK);
		MRT0->CHANNEL[channel].INTVAL = tempCount | (MRT_CHANNEL_INTVAL_LOAD_MASK);
	}

	NVIC_EnableIRQ(MRT0_IRQn);
	__enable_irq();
}

/*
 * Function: CTIMER_Configuration_Seven_Segment
 * --------------------
 * Enables the CTimer0 for use
 * Function called internally
 *
 * Return: no return
 */
void CTIMER_Configuration_Seven_Segment() {
	__disable_irq(); // turn off globally
	 NVIC_DisableIRQ(CTIMER0_IRQn);
	 SYSCON->SYSAHBCLKCTRL0 |= (SYSCON_SYSAHBCLKCTRL0_CTIMER0_MASK);
	 SYSCON->PRESETCTRL0 &= ~(SYSCON_PRESETCTRL0_CTIMER0_RST_N_MASK); // Reset
	 SYSCON->PRESETCTRL0 |= (SYSCON_PRESETCTRL0_CTIMER0_RST_N_MASK); // clear the reset.
	 // Match Channel 0 and generate IRQ
	 CTIMER0->MCR |= CTIMER_IR_MR0INT_MASK; // interrupt on Ch 0 match
	 if (currentClock == 4) {
		CTIMER0->MR[0] = cycleRate;
	} else if (countClock == 4) {
		CTIMER0->MR[0] = countRate;
	} else if (transitionClock == 4) {
		CTIMER0->MR[0] = transitionRate;
	}

	 CTIMER0->PR = (0); // PR = 0: Divide by 1 of APB clock, No Scaling
	 CTIMER0->TCR |= CTIMER_TCR_CEN_MASK;
	 NVIC_EnableIRQ(CTIMER0_IRQn);
	 __enable_irq(); // global
}


/****************************************************************************************************
 * 																									*
 *									Timer Based Functions											*
 * 			These functions use at least 1 internal clock of the users choice						*
 *	Functions include: displaying 4 characters simultaneously, displaying 4 numbers simultaneously,	*
 *				a counting display, a carousel display, and a slider display						*
 * 																									*
 ****************************************************************************************************/


/*
 * Function: display4Characters
 * --------------------
 * Function used to display 4 characters continuously on the 7 segment display
 * This function calls the timer configurations internally
 *
 * inputSequence: Takes 4 character input
 * clockType: from the list "SysTick", "WKT", "MRT0", "MRT1", "CTIMER0" as input
 * refreshRate: how fast the 4 characters are cycled through on the 7-segment display
 *
 * Return: no return
 */
void display4Characters(char inputSequence[], char clockType[], int refreshRate) {
	for (int i = 0; i < 4; i++) {
		chrSequence[3-i] = inputSequence[i];
	}
	cycleRate = refreshRate;
	if (strcmp(clockType, "SysTick") == 0) {
		currentClock = 0;
		SysTick_Configuration_Seven_Segment();
	} else if (strcmp(clockType, "WKT") == 0) {
		currentClock = 1;
		WKT_Configuration_Seven_Segment();
	} else if (strcmp(clockType, "MRT0") == 0) {
		currentClock = 2;
		MRT_Configuration_Seven_Segment(MRT_CHAN0);
	}  else if (strcmp(clockType, "MRT1") == 0) {
		currentClock = 3;
		MRT_Configuration_Seven_Segment(MRT_CHAN1);
	} else if (strcmp(clockType, "CTIMER0") == 0) {
		currentClock = 4;
		CTIMER_Configuration_Seven_Segment();
	}

	currentDigit = 0;
}


/*
 * Function: display4Numbers
 * --------------------
 * Function used to display 4 numbers continuously on the 7 segment display
 * This function calls the timer configurations internally
 *
 * inputSequence: Takes 4 character input
 * clockType: from the list "SysTick", "WKT", "MRT0", "MRT1", "CTIMER0" as input
 * refreshRate: how fast the 4 numbers are cycled through on the 7-segment display
 *
 * Return: no return
 */
void display4Numbers(int inputNumber, char clockType[], int refreshRate) {
	int shifter = 1000;
	int number = inputNumber;
	char inputSequence[4] = "    ";
	for (int i = 0; i < 4; i++) {
		inputSequence[i] = (number -(number % shifter))/shifter +'0';
		number = (number % shifter);
		shifter = shifter/10;
	}
	display4Characters(inputSequence, clockType, refreshRate);
}


/*
 * Function: setupSevenSegmentCounter
 * --------------------
 * Function used to display a counter on the 7 segment display
 * This function calls the 2 timers configurations internally
 * Calling the respective interrupts will refresh the display and update the counter
 *
 * clockStart: Starting time for the display
 * clockType: from the list "SysTick", "WKT", "MRT0", "MRT1", "CTIMER0" as input
 * newCountDirection: "UP" or "DOWN"
 * newCountIncrement: counter increment/decrement depending on direction (negatives are allowed)
 * newStopValue: early stop point for counting
 * enableStopValue: true or false
 * newCountRate: Speed of counting progression
 * refreshClock: from the list: "SysTick", "WKT", "MRT0", "MRT1", "CTIMER0"
 * refreshRate: how fast the numbers are cycled through on the 7-segment display
 * NOTE: counterClock & refreshClock cannot be the same clock
 *
 * Return: no return
 */
void setupSevenSegmentCounter(int clockStart, char counterClock[],  char newCountDirection[],
		int newCountIncrement, int newStopValue, bool enableStopValue, int newCountRate, char refreshClock[], int refreshRate) {

	if (strcmp(counterClock, refreshClock) != 0) {
		startCount = clockStart;
		normalizedCount = clockStart;
		currentCount = clockStart;

		if (strcmp(newCountDirection, "UP") == 0) {
			strncpy(countDirection, "UP", 4);
		} else {
			strncpy(countDirection, "DOWN", 4);
		}

		countStopValue = newStopValue;
		enableCountStopValue = enableStopValue;
		countIncrement = newCountIncrement;
		countRate = newCountRate;

		if (strcmp(counterClock, "SysTick") == 0) {
			countClock = 0;
			SysTick_Configuration_Seven_Segment();
		} else if (strcmp(counterClock, "WKT") == 0) {
			countClock = 1;
			WKT_Configuration_Seven_Segment();
		} else if (strcmp(counterClock, "MRT0") == 0) {
			countClock = 2;
			MRT_Configuration_Seven_Segment(MRT_CHAN0);
		}  else if (strcmp(counterClock, "MRT1") == 0) {
			countClock = 3;
			MRT_Configuration_Seven_Segment(MRT_CHAN1);
		} else if (strcmp(counterClock, "CTIMER0") == 0) {
			countClock = 4;
			CTIMER_Configuration_Seven_Segment();
		}

		display4Numbers(clockStart, refreshClock, refreshRate);
	}
}


/*
 * Function: sevenSegmentDisplayTextCarousel
 * --------------------
 * Function used to display a carousel of characters on the 7 segment display
 * This function calls the 2 timers configurations internally
 * Calling the respective interrupts will refresh the display and update the transition of the carousel
 * Creating a carousel sequence of text (loops on the display counter clockwise)
 *
 * characterSequence: As long of a sequence as desired of characters ("Hello World")
 * sequenceLength: Length of Sequence
 * newTransitionClock: from the list "SysTick", "WKT", "MRT0", "MRT1", "CTIMER0" as input
 * transitionSpeed: Speed of carousel motion
 * newEnableContinousCycle: true implies the coaursel will cycle, false implies a one shot action
 * newEnablePadding: Pad the sequence with a blank start and end screen
 * refreshClock: from the list: "SysTick", "WKT", "MRT0", "MRT1", "CTIMER0"
 * refreshRate: speed of cycling through digits
 * NOTE: newTransitionClock & refreshClock cannot be the same clock
 *
 * Return: no return
 */
void sevenSegmentDisplayTextCarousel(char characterSequence[], int sequenceLength, char newTransitionClock[],
		int transitionSpeed, bool newEnableContinousCycle, bool newEnablePadding, char refreshClock[], int refreshRate) {
	if (strcmp(newTransitionClock, refreshClock) != 0) {
		carouselSequenceLength = sequenceLength;
		int padding = 0;
		// 4 Character Padding
		if (newEnablePadding) {
			for (int i = 0; i < 4; i++) {
				carouselSequence[i] = ' ';
			}
			padding = 4;
			carouselSequenceLength = carouselSequenceLength + 3;
		}
		for (int i = 0; i < sequenceLength; i++) {
			carouselSequence[i + padding] = characterSequence[i];
		}

		if (newEnablePadding && !newEnableContinousCycle) {
			for (int i = 0; i < 4; i++) {
				carouselSequence[4 + sequenceLength + i] = ' ';
			}
			carouselSequenceLength = carouselSequenceLength + 5;
		}
		// Single Character Padding
		if (newEnableContinousCycle & !newEnablePadding) {
			carouselSequence[sequenceLength] = ' ';
		}
		transitionRate = transitionSpeed;
		cycleRate = refreshRate;
		enableContinousCycle = newEnableContinousCycle;
		pauseCarouselTransition = false;

		if (strcmp(newTransitionClock, "SysTick") == 0) {
			transitionClock = 0;
			SysTick_Configuration_Seven_Segment();
		} else if (strcmp(newTransitionClock, "WKT") == 0) {
			transitionClock = 1;
			WKT_Configuration_Seven_Segment();
		} else if (strcmp(newTransitionClock, "MRT0") == 0) {
			transitionClock = 2;
			MRT_Configuration_Seven_Segment(MRT_CHAN0);
		}  else if (strcmp(newTransitionClock, "MRT1") == 0) {
			transitionClock = 3;
			MRT_Configuration_Seven_Segment(MRT_CHAN1);
		} else if (strcmp(newTransitionClock, "CTIMER0") == 0) {
			transitionClock = 4;
			CTIMER_Configuration_Seven_Segment();
		}

		transitionIndex = -1;
		char firstScreen[] = {carouselSequence[0], carouselSequence[1], carouselSequence[2], carouselSequence[3]};
		display4Characters(firstScreen, refreshClock, refreshRate);
	}
}



/*
 * Function: sevenSegmentDisplayTextSlider
 * --------------------
 * Creating a slider for text to display 4 characters at a time and switch between character sets
 * This function calls the 2 timers configurations internally
 * Calling the respective interrupts will refresh the display and update the transition of the slider
 *
 * characterSequence: As long of a sequence as desired of characters ("Hello World")
 * sequenceLength: Length of Sequence
 * newTransitionClock: from the list "SysTick", "WKT", "MRT0", "MRT1", "CTIMER0" as input
 * transitionSpeed: Speed of slider swapping
 * newEnableContinousCycle: true implies the slider will cycle, false implies a one shot action
 * newEnablePadding: Pad the sequence with a blank start and end screen
 * ignoreSingleSpaces: True or False, sequences with single spaces will have the spaces ignored
 * double spaces are not affected
 * Sample input and output of using the IgnoreSingleSpaces "A B C D  E F  G" -> "ABCD  EF  G"
 * refreshClock: from the list: "SysTick", "WKT", "MRT0", "MRT1", "CTIMER0"
 * refreshRate: speed of cycling through digits
 * NOTE: newTransitionClock & refreshClock cannot be the same clock
 *
 * Return: no return
 */
void sevenSegmentDisplayTextSlider(char characterSequence[], int sequenceLength, char newTransitionClock[],
		int transitionSpeed, bool newEnableContinousCycle, bool newEnablePadding, bool ignoreSingleSpaces, char refreshClock[], int refreshRate) {
	if (strcmp(newTransitionClock, refreshClock) != 0) {
		sliderSequenceLength = sequenceLength;
		int padding = 0;
		// 4 Character Padding
		if (newEnablePadding) {
			for (int i = 0; i < 4; i++) {
				sliderSequence[i] = ' ';
			}
			padding = 4;
			sliderSequenceLength = sliderSequenceLength + 4;
		}
		int spaceAdjuster = 0;
		for (int i = 0; i < sequenceLength; i++) {
			if(ignoreSingleSpaces) {
				if(i == 0 && characterSequence[i] == ' ' && characterSequence[i + 1] != ' ') {
					spaceAdjuster = spaceAdjuster + 1;
				 	sliderSequenceLength = sliderSequenceLength - 1;
				} else if (i == (sequenceLength-1) && characterSequence[i] == ' ' && characterSequence[i - 1] != ' ') {
					spaceAdjuster = spaceAdjuster + 1;
					sliderSequenceLength = sliderSequenceLength - 1;
				} else if (characterSequence[i] == ' ' && characterSequence[i - 1] != ' ' && characterSequence[i + 1] != ' ') {
					spaceAdjuster = spaceAdjuster + 1;
					sliderSequenceLength = sliderSequenceLength - 1;
				} else {
					sliderSequence[i + padding - spaceAdjuster] = characterSequence[i];
				}
			} else {
				sliderSequence[i + padding] = characterSequence[i];
			}

		}

		for (int i =0; i < 4 - ((sequenceLength - spaceAdjuster) % 4); i++) {
			if ((sequenceLength - spaceAdjuster) % 4 != 0) {
				sliderSequence[sequenceLength + padding - spaceAdjuster + i] = ' ';
			}
		}
		if ((sequenceLength - spaceAdjuster) % 4 != 0) {
			sliderSequenceLength = sliderSequenceLength + (4 - ((sequenceLength - spaceAdjuster) % 4));
		}


		transitionRate = transitionSpeed;
		cycleRate = refreshRate;
		enableContinousCycle = newEnableContinousCycle;
		pauseSliderTransition = false;

		if (strcmp(newTransitionClock, "SysTick") == 0) {
			transitionClock = 0;
			SysTick_Configuration_Seven_Segment();
		} else if (strcmp(newTransitionClock, "WKT") == 0) {
			transitionClock = 1;
			WKT_Configuration_Seven_Segment();
		} else if (strcmp(newTransitionClock, "MRT0") == 0) {
			transitionClock = 2;
			MRT_Configuration_Seven_Segment(MRT_CHAN0);
		}  else if (strcmp(newTransitionClock, "MRT1") == 0) {
			transitionClock = 3;
			MRT_Configuration_Seven_Segment(MRT_CHAN1);
		} else if (strcmp(newTransitionClock, "CTIMER0") == 0) {
			transitionClock = 4;
			CTIMER_Configuration_Seven_Segment();
		}

		sliderTransitionIndex = 0;
		char firstScreen[] = {sliderSequence[0], sliderSequence[1], sliderSequence[2], sliderSequence[3]};
		display4Characters(firstScreen, refreshClock, refreshRate);
	}
}

/************************************************************************************************
 * 																								*
 *									Timer Interrupt Functions									*
 * 			These functions should be called in the respective timer interrupt routine			*
 *			Allows flexibility to still use other logic within the interrupt as desired			*
 * 																								*
 ************************************************************************************************/


/*
 * Function: display4CharactersInterrupt
 * --------------------
 * Used to swap between digits quickly on the display
 * Should be called in the interrupt from the timer associated with the refreshRate
 * Used when display4Characters is used
 *
 *
 * Return: no return
 */
void display4CharactersInterrupt() {
	clearDigits();
	char upperCaseChar = toupper(chrSequence[currentDigit]);
	int size = sizeof(inputCharacterOptions)/sizeof(inputCharacterOptions[0]);//Method

	for (int i = 0; i < size; i++) {
		if (upperCaseChar == inputCharacterOptions[i]) {
			displayValue(displayOptions[i]);
		}
	}
	enableDigit(currentDigit);
	currentDigit = currentDigit + 1;
	if (currentDigit == 4) {
		currentDigit = 0;
	}

	// Handle Specific Details for Each Clock
	// 0 == SysTick, 1 == WKT, 2 == MRT0, 3 = MRT1, 4 = CTIMER0
	if (currentClock == 1) {
		WKT->COUNT = cycleRate;
	} else if (currentClock == 4) {
		 CTIMER0->TCR |= CTIMER_TCR_CRST_MASK; // set bit 1 to 1
		 CTIMER0->TCR &= ~(CTIMER_TCR_CRST_MASK); // clear bit 1 to 0
	}

}


/*
 * Function: display4NumbersInterrupt
 * --------------------
 * Used to swap between digits quickly on the display
 * Should be called in the interrupt from the timer associated with the refreshRate
 * Just a Wrapper Function For Clarity
 * Called when digits are used, in reality display4CharactersInterrupt could be called instead
 *
 * Used when display4Numbers is used
 *
 *
 * Return: no return
 */
void display4NumbersInterrupt() {
	display4CharactersInterrupt();
}


/*
 * Function: displayCarouselInterrupt
 * --------------------
 * Used to swap between digits quickly on the display
 * Should be called in the interrupt from the timer associated with the refreshRate
 * Just a Wrapper Function For Clarity
 * Called when the carousel is used, in reality display4CharactersInterrupt could be called instead
 *
 * Used when sevenSegmentDisplayTextCarousel is used
 *
 *
 * Return: no return
 */
void displayCarouselInterrupt() {
	display4CharactersInterrupt();
}


/*
 * Function: displaySliderInterrupt
 * --------------------
 * Used to swap between digits quickly on the display
 * Should be called in the interrupt from the timer associated with the refreshRate
 * Just a Wrapper Function For Clarity
 * Called when the slider is used, in reality display4CharactersInterrupt could be called instead
 *
 * Used when sevenSegmentDisplayTextSlider is used
 *
 *
 * Return: no return
 */
void displaySliderInterrupt() {
	display4CharactersInterrupt();
}



/*
 * Function: updateSevenSegmentCounterInterrupt
 * --------------------
 * Interrupt for the counter mechanism if the seven segment display is used as a counter
 * Updates the count for the 7-segment
 * This function should be called within the interrupt that handles clock incrementing/decrementing
 *
 * Used when setupSevenSegmentCounter is used, where the clock associated with counting was passing in to counterClock
 *
 *
 * Return: no return
 */
void updateSevenSegmentCounterInterrupt() {
	if (pauseCounter == false) {
		if (strcmp(countDirection, "UP") == 0) {
			currentCount = currentCount + countIncrement;
			normalizedCount = normalizedCount + countIncrement;
		} else {
			currentCount = currentCount - countIncrement;
			normalizedCount = normalizedCount - countIncrement;
		}

		if (normalizedCount > 9999) {
			normalizedCount = normalizedCount - 10000;
		} else if(normalizedCount < 0) {
			normalizedCount = 10000 + normalizedCount;
		}

		if(currentCount == countStopValue && enableCountStopValue == true) {
			pauseCounter = true;
		}

		int shifter = 1000;
		int number = normalizedCount;
		for (int i = 0; i < 4; i++) {
			chrSequence[3-i] = (number -(number % shifter))/shifter +'0';
			number = (number % shifter);
			shifter = shifter/10;
		}
	}

	if (countClock == 1) {
		WKT->COUNT = countRate;
	} else if (countClock == 4) {
		 CTIMER0->TCR |= CTIMER_TCR_CRST_MASK; // set bit 1 to 1
		 CTIMER0->TCR &= ~(CTIMER_TCR_CRST_MASK); // clear bit 1 to 0
	}
}


/*
 * Function: sevenSegmentCarouselInterrupt
 * --------------------
 * Interrupt for the carousel mechanism if the seven segment display is used as a carousel
 * Updates the currently displayed characters
 * This function should be called within the interrupt that handles carousel transitions
 *
 * Used when sevenSegmentDisplayTextCarousel is used, where the clock associated with transitions
 * was passing into newTransitionClock
 *
 *
 * Return: no return
 */
void sevenSegmentCarouselInterrupt() {
	if (!carouselOverflow && !pauseCarouselTransition){
		transitionIndex = transitionIndex + 1;
	}

	int wrapAround = 0;
	for (int i = 0; i < 4; i++) {
		if (i + transitionIndex > carouselSequenceLength) {
			chrSequence[3-i] = carouselSequence[wrapAround];
			wrapAround = wrapAround + 1;
		} else {
			chrSequence[3-i] = carouselSequence[i + transitionIndex];
		}

	}

	if (transitionIndex == carouselSequenceLength && enableContinousCycle) {
		transitionIndex = -1;
	} else if (transitionIndex == (carouselSequenceLength-4) && !enableContinousCycle) {
		carouselOverflow = true;
	}

	if (transitionClock == 1) {
		WKT->COUNT = transitionRate;
	} else if (transitionClock == 4) {
		 CTIMER0->TCR |= CTIMER_TCR_CRST_MASK; // set bit 1 to 1
		 CTIMER0->TCR &= ~(CTIMER_TCR_CRST_MASK); // clear bit 1 to 0
	}
}



/*
 * Function: sevenSegmentSliderInterrupt
 * --------------------
 * Interrupt for the slider mechanism if the seven segment display is used as a slider
 * to swap between sets of 4 characters
 * Updates the currently displayed characters
 * This function should be called within the interrupt that handles slider transitions
 *
 * Used when sevenSegmentDisplayTextSlider is used, where the clock associated with transitions
 * was passing into newTransitionClock
 *
 *
 * Return: no return
 */
void sevenSegmentSliderInterrupt() {

	for (int i = 0; i < 4; i++) {
		chrSequence[3-i] = sliderSequence[i + sliderTransitionIndex];
	}

	if (!pauseSliderTransition){
		sliderTransitionIndex = sliderTransitionIndex + 4;
	}
	if (sliderTransitionIndex > (sliderSequenceLength - 4) && enableContinousCycle) {
		sliderTransitionIndex = 0;
	} else if (sliderTransitionIndex > (sliderSequenceLength - 4)) {
		sliderTransitionIndex = sliderTransitionIndex - 4;
		pauseSliderTransition = true;
	}

	if (transitionClock == 1) {
		WKT->COUNT = transitionRate;
	} else if (transitionClock == 4) {
		 CTIMER0->TCR |= CTIMER_TCR_CRST_MASK; // set bit 1 to 1
		 CTIMER0->TCR &= ~(CTIMER_TCR_CRST_MASK); // clear bit 1 to 0
	}
}



/************************************************************************************************
 * 																								*
 *							Seven Segment Counter Helper Functions								*
 * 		These functions can be used to make changes to the counter in real time if in use		*
 * 																								*
 ************************************************************************************************/


/*
 * Function: getSevenSegmentDisplayCount
 * --------------------
 * Getter for the current normalized (displayed) count
 * Normalized implies the currently displayed count value (between 0000 and 9999)
 * The actual count value may be larger as the counter wraps around when it hits the edge values
 * To get the actual count value use getSevenSegmentTotalCount instead
 *
 * Return: normalizedCount
 */
int getSevenSegmentDisplayCount() {
	return normalizedCount;
}

/*
 * Function: getSevenSegmentTotalCount
 * --------------------
 *  Getter for the actual count
 *  If overflow within the counter has happened and the counter wraps around
 *  the display value may differ from the actual count
 * To get the normalized (currently displayed) count value use getSevenSegmentDisplayCount instead
 *
 * Return: currentCount
 */
int getSevenSegmentTotalCount() {
	return currentCount;
}

/*
 * Function: toggleSevenSegmentCounterPause
 * --------------------
 *  Toggle for the counter to pause or run
 *
 *
 * Return: no return
 */
void toggleSevenSegmentCounterPause() {
	pauseCounter = !pauseCounter;
}

/*
 * Function: pauseSevenSegmentCounter
 * --------------------
 *  Pause the counter
 *
 *
 * Return: no return
 */
void pauseSevenSegmentCounter() {
	pauseCounter = true;
}

/*
 * Function: runSevenSegmentCounter
 * --------------------
 * Continue running the counter
 *
 *
 * Return: no return
 */
void runSevenSegmentCounter() {
	pauseCounter = false;
}

/*
 * Function: resetSevenSegmentCount
 * --------------------
 * Resets the counter to the original value
 * Will also restart the counting sequence
 *
 *
 * Return: no return
 */
void resetSevenSegmentCount() {
	currentCount = startCount;
	normalizedCount = startCount;
	pauseCounter = false;
}

/*
 * Function: setSevenSegmentCount
 * --------------------
 * Change the count value to a new number
 * Will update the displayed and normalized value
 * Will also restart the counter, doing this may cause a need to update the users stopValue
 *
 * newCount: new count value to be displayed and stored
 *
 * Return: no return
 */
void setSevenSegmentCount(int newCount) {
	startCount = newCount;
	currentCount = newCount;
	normalizedCount = newCount;
	pauseCounter = false;
}

/*
 * Function: updateSevenSegmentIncrementer
 * --------------------
 * Change the count increment/decrement value to a new number
 *
 * newIncrement: an integer value for changing the increment or decrement amount
 *
 * Return: no return
 */
void updateSevenSegmentIncrementer(int newIncrement) {
	countIncrement = newIncrement;
}

/*
 * Function: changeSevenSegmentCountDirection
 * --------------------
 * Change the count direction to either "UP" or "DOWN"
 * If an incorrect string is entered, no change will be applied
 *
 * newDirection: either "UP' or "DOWN" as required
 *
 *
 * Return: no return
 */
void changeSevenSegmentCountDirection(char newDirection[]) {
	if (strcmp(newDirection, "UP") == 0) {
		strncpy(countDirection, "UP", 4);
	} else if (strcmp(newDirection, "DOWN") == 0) {
		strncpy(countDirection, "DOWN", 4);
	}
}


/*
 * Function: setCountStopValue
 * --------------------
 * Change the stop value for the counter
 * Will also enable the stop condition automatically
 * Will also continue the counter if previously paused, but will pause the counter
 * if the new stop value equals the current count
 *
 * newStopValue: the new integer value for the counter to stop at
 *
 *
 * Return: no return
 */
void setCountStopValue(int newStopValue) {
	enableCountStopValue = true;
	countStopValue = newStopValue;
	if (currentCount != countStopValue) {
		pauseCounter = false;
	} else {
		pauseCounter = true;
	}
}

/*
 * Function: clearCountStopValue
 * --------------------
 * Change the stop value for the counter
 * Will also enable the stop condition automatically
 * Will also continue the counter if previously paused, but will pause the counter
 * if the new stop value equals the current count
 *
 * newStopValue: the new integer value for the counter to stop at
 *
 *
 * Return: no return
 */
void clearCountStopValue(){
	enableCountStopValue = false;
	countStopValue = 0;
	pauseCounter = false;
}


/************************************************************************************************
 * 																								*
 *							Seven Segment Carousel Helper Functions								*
 * 		These functions can be used to make changes to the carousel in real time if in use		*
 * 																								*
 ************************************************************************************************/


/*
 * Function: togglePauseSevenSegmentDisplayCarousel
 * --------------------
 *  Toggle the pause on the carousel motion
 *
 *
 * Return: no return
 */
void togglePauseSevenSegmentDisplayCarousel() {
	pauseCarouselTransition = !pauseCarouselTransition;
}

/*
 * Function: pauseSevenSegmentDisplayCarousel
 * --------------------
 *  Pause the carousel motion
 *
 *
 * Return: no return
 */
void pauseSevenSegmentDisplayCarousel() {
	pauseCarouselTransition = true;
}

/*
 * Function: runSevenSegmentDisplayCarousel
 * --------------------
 *  Continue the carousel motion if paused
 *
 *
 * Return: no return
 */
void runSevenSegmentDisplayCarousel() {
	pauseCarouselTransition = false;
}

/*
 * Function: restartSevenSegmentDisplayCarousel
 * --------------------
 *  Start carousel from beginning sequence again
 *  Removes the pause on the carousel if any
 *
 *
 * Return: no return
 */
void restartSevenSegmentDisplayCarousel() {
	transitionIndex = -1;
	carouselOverflow = false;
	pauseCarouselTransition = false;
}



/************************************************************************************************
 * 																								*
 *							Seven Segment Slider Helper Functions								*
 * 		These functions can be used to make changes to the slider in real time if in use		*
 * 																								*
 ************************************************************************************************/

/*
 * Function: pauseSevenSegmentDisplaySlider
 * --------------------
 * Pause slider animation for the display
 *
 *
 * Return: no return
 */
void pauseSevenSegmentDisplaySlider() {
	pauseSliderTransition = true;
}


/*
 * Function: togglePauseSevenSegmentDisplaySlider
 * --------------------
 * Toggle whether the slider animation is running or not for the display
 *
 *
 * Return: no return
 */
void togglePauseSevenSegmentDisplaySlider() {
	pauseSliderTransition = !pauseSliderTransition;
}

/*
 * Function: runSevenSegmentDisplaySlider
 * --------------------
 * Run slider animation for the display
 *
 *
 * Return: no return
 */
void runSevenSegmentDisplaySlider() {
	pauseSliderTransition = false;
}


/*
 * Function: restartSevenSegmentDisplaySlider
 * --------------------
 * Restart slider animation for the display
 * Will also run the display if previously paused
 *
 *
 * Return: no return
 */
void restartSevenSegmentDisplaySlider() {
	sliderTransitionIndex = 0;
	pauseSliderTransition = false;
}
