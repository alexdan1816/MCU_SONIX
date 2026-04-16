/*
struct tree
	main.c: initial and main loop function
	PFPA.c: unused
	GPIO.c: initial gpio pin
	 WDT.c: initial watchdog with 250ms reset while do nothing
 utility.c: delay
 segment.c: display segment tube.
 
 function:
	1. segment tube scan freuqency 1KHz, timer source from CT16B0 timer 1ms.
	2. time count down range 9999~0, each sencond minus 1.
*/