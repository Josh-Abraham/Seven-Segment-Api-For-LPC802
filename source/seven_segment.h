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

#ifndef SEVEN_SEGMENT_H_
#define SEVEN_SEGMENT_H_

// List of available characters for use
const static char inputCharacterOptions[]  = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
		'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		'U', 'V', 'W', 'X', 'Y', 'Z', '-', '=', '_', ' '};

// List of available characters for use within the library in binary form
const static int displayOptions[]  = {
	0b0111111, // 0
	0b0000110, // 1
	0b1011011, // 2
	0b1001111, // 3
	0b1100110, // 4
	0b1101101, // 5
	0b1111101, // 6
	0b0000111, // 7
	0b1111111, // 8
	0b1101111, // 9
	0b1110111, // A
	0b1111100, // b -> lower case
	0b0111001, // C
	0b1011110, // d -> lower case
	0b1111001, // E
	0b1110001, // F
	0b0111101, // G
	0b1110100, // h -> lower case
	0b0110000, // I
	0b0011110, // J
	0b1110101, // K
	0b0111000, // L
	0b1010101, // M -> Not a standard M
	0b1010100, // n -> lower case
	0b1011100, // o -> lower case
	0b1110011, // P
	0b1100111, // q -> lower case
	0b1010000, // r -> lower case
	0b0101101, // S -> without middle line
	0b1111000, // t -> lower case
	0b0111110, // U
	0b0011100, // v -> lower case
	0b1101010, // W -> Not a standard W
	0b1110110, // X
	0b1101110, // Y
	0b0011011, // Z -> with
	0b1000000, // -
	0b1001000, // =
	0b0001000, // _
	0b0000000, // _Space_
};

/************************************************************************************************
 * 																								*
 * 								Seven Segment Configuration Functions							*
 * 						Used for initial setup and re-setup of the display 						*
 * 				Sets Digits, Segments and Decimal Points and their respective GPIOs				*
 * 																								*
 ************************************************************************************************/

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
void digitGPIOSetup(int channels[]);


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
void sevenSegmentGPIOSetup(int segs[]);

/*
 * Function:  setSevenSegmentType
 * --------------------
 * Sets up the type of seven segment to be used
 *
 * type: 0 = common anode, otherwise common cathode
 *
 * Return: no return
 */
void setSevenSegmentType(int type);

/*
 * Function:  enableDecimalSegment
 * --------------------
 * Enable or disable the Decimal Segment
 *
 * decimalSegment: GPIO Pin for decimal point, -1 means disabled
 *
 * Return: no return
 */
void enableDecimalSegment(int decimalSegment);


/*
 * Function: sevenSegmentFullSetup
 * --------------------
 * Do a full reset/setup rather than just portions
 *
 * channels: list of up to 4 GPIO pins to be used for the digits of the display
 * segs: list of up to 7 GPIO pins to be used for the display (Should be ordered from Segment A .. Segment G)
 * decimalSegment: GPIO Pin for decimal point, -1 means disabled
 *
 * Return: no return
 */
void sevenSegmentFullSetup(int channels[], int segs[], int decimalSegment);


/*
 * Function: toggleDecimalPoint
 * --------------------
 * Turn on/off the decimal point
 * Calls displayDP internally
 *
 * Return: no return
 */
void toggleDecimalPoint();

/*
 * Function: setDecimalPoint
 * --------------------
 *  Turn on the decimal point
 * Calls displayDP internally
 *
 * Return: no return
 */
void setDecimalPoint();

/*
 * Function: clearDecimalPoint
 * --------------------
 * Turn off the decimal point
 *
 * Return: no return
 */
void clearDecimalPoint();


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
void displaySingleCharacter(char inputChar);

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
void displaySingleInt(int inputNum);



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
void display4Characters(char inputSequence[], char clockType[], int refreshRate);


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
void display4Numbers(int inputNumber, char clockType[], int refreshRate);


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
		int newCountIncrement, int newStopValue, bool enableStopValue, int newCountRate, char refreshClock[],
		int refreshRate);


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
		int transitionSpeed, bool newEnableContinousCycle, bool newEnablePadding, char refreshClock[], int refreshRate);

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
		int transitionSpeed, bool newEnableContinousCycle, bool newEnablePadding, bool ignoreSingleSpaces,
		char refreshClock[], int refreshRate);



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
void display4CharactersInterrupt(void);

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
void display4NumbersInterrupt();

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
void displayCarouselInterrupt();

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
void displaySliderInterrupt();

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
void updateSevenSegmentCounterInterrupt();

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
void sevenSegmentCarouselInterrupt();

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
void sevenSegmentSliderInterrupt();



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
int getSevenSegmentDisplayCount();

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
int getSevenSegmentTotalCount();

/*
 * Function: toggleSevenSegmentCounterPause
 * --------------------
 *  Toggle for the counter to pause or run
 *
 *
 * Return: no return
 */
void toggleSevenSegmentCounterPause();

/*
 * Function: pauseSevenSegmentCounter
 * --------------------
 *  Pause the counter
 *
 *
 * Return: no return
 */
void pauseSevenSegmentCounter();

/*
 * Function: runSevenSegmentCounter
 * --------------------
 * Continue running the counter
 *
 *
 * Return: no return
 */
void runSevenSegmentCounter();

/*
 * Function: resetSevenSegmentCount
 * --------------------
 * Resets the counter to the original value
 * Will also restart the counting sequence
 *
 *
 * Return: no return
 */
void resetSevenSegmentCount();

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
void setSevenSegmentCount(int newCount);

/*
 * Function: updateSevenSegmentIncrementer
 * --------------------
 * Change the count increment/decrement value to a new number
 *
 * newIncrement: an integer value for changing the increment or decrement amount
 *
 * Return: no return
 */
void updateSevenSegmentIncrementer(int newIncrement);


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
void changeSevenSegmentCountDirection(char newDirection[]);

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
void setCountStopValue(int newStopValue);

/*
 * Function: clearCountStopValue
 * --------------------
 * Clear the stop value for the counter
 * Will also disable the stop condition automatically
 * Will also continue the counter if previously paused
 *
 *
 *
 * Return: no return
 */
void clearCountStopValue();




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
void togglePauseSevenSegmentDisplayCarousel();

/*
 * Function: pauseSevenSegmentDisplayCarousel
 * --------------------
 *  Pause the carousel motion
 *
 *
 * Return: no return
 */
void pauseSevenSegmentDisplayCarousel();

/*
 * Function: runSevenSegmentDisplayCarousel
 * --------------------
 *  Continue the carousel motion if paused
 *
 *
 * Return: no return
 */
void runSevenSegmentDisplayCarousel();

/*
 * Function: restartSevenSegmentDisplayCarousel
 * --------------------
 *  Start carousel from beginning sequence again
 *  Removes the pause on the carousel if any
 *
 *
 * Return: no return
 */
void restartSevenSegmentDisplayCarousel();



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
void pauseSevenSegmentDisplaySlider();


/*
 * Function: togglePauseSevenSegmentDisplaySlider
 * --------------------
 * Toggle whether the slider animation is running or not for the display
 *
 *
 * Return: no return
 */
void togglePauseSevenSegmentDisplaySlider();


/*
 * Function: runSevenSegmentDisplaySlider
 * --------------------
 * Run slider animation for the display
 *
 *
 * Return: no return
 */
void runSevenSegmentDisplaySlider();


/*
 * Function: restartSevenSegmentDisplaySlider
 * --------------------
 * Restart slider animation for the display
 * Will also run the display if previously paused
 *
 *
 * Return: no return
 */
void restartSevenSegmentDisplaySlider();



#endif /* SEVEN_SEGMENT_H_ */
