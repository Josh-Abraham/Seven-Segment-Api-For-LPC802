#include "LPC802.h"
#include "clock_config.h"
#include "seven_segment.h"

// NOTE: Example is for a Common Cathode Seven Segment
// To change to anode uncomment setSevenSegmentType

#define MRT_GFLAG0		(0)
#define MRT_GFLAG1		(1)
#define GPIO16			(16)	//Decimal Point
int count = 0;
int messageDisplay = 0;
void clockSetup() {
	SYSCON->MAINCLKSEL = (0x0 << SYSCON_MAINCLKSEL_SEL_SHIFT);
	SYSCON->MAINCLKUEN &= ~(0x1);
	SYSCON->MAINCLKUEN |= 0x1;
	BOARD_BootClockFRO18M(); // 18 MHz clock
}

void SysTick_Configuration() {
	__disable_irq();
	NVIC_DisableIRQ(SysTick_IRQn); // turn off the SysTick interrupt.
	SysTick_Config(4000000);
	NVIC_EnableIRQ(SysTick_IRQn); // SysTick IRQs are on.
	__enable_irq();
}


void SysTick_Handler(void) {
	count++;
	if (count == 30) {
		pauseSevenSegmentDisplayCarousel();
	} else if (count == 35) {
		sevenSegmentDisplayTextSlider("EEC53215i5  fun", 15, "WKT", 850000, true, true, false, "MRT0", 55000);
		messageDisplay = 1;
	}
}

void WKT_IRQHandler(void) {
	WKT->CTRL |= WKT_CTRL_ALARMFLAG_MASK;
	if (messageDisplay == 0) {
		sevenSegmentCarouselInterrupt();
	} else {
		sevenSegmentSliderInterrupt();
	}
}

void MRT0_IRQHandler(void) {
	if (MRT0->IRQ_FLAG & (1 << MRT_GFLAG0)) {
		MRT0->CHANNEL[0].STAT = MRT_CHANNEL_STAT_INTFLAG_MASK; // CLEAR FLAG
		if (messageDisplay == 1) {
			displaySliderInterrupt();
		}
	} else if (MRT0->IRQ_FLAG & (1 << MRT_GFLAG1)) {
		MRT0->CHANNEL[1].STAT = MRT_CHANNEL_STAT_INTFLAG_MASK; // CLEAR FLAG
		if (messageDisplay == 0) {
			displayCarouselInterrupt();
		}
	}
}

int main(void) {
	clockSetup();
	SysTick_Configuration();
	// May need to adjust which pins you are using
	int channels[] = {11, 13, 1, 10};
	digitGPIOSetup(channels);
	int segs[] = {0, 4, 9, 7, 17, 8, 12};
	sevenSegmentGPIOSetup(segs);
	enableDecimalSegment(GPIO16);
	clearDecimalPoint();
	// setSevenSegmentType(0);
	sevenSegmentDisplayTextCarousel("12345678", 8, "WKT", 550000, true, true, "MRT1", 55000);

    while(1) {
    	asm("NOP");
    }
    return 0 ;
}
