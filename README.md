# tft_tearfree
hack to make cheap tft displays without Vsync pin exposed tear-free


It speeds up refresh rate in even frame and slows down in odd frame, while keeping up regular interval of updates.

this way display refresh rate stays synchronized to dma updates even without vblank signal . 

as it is bit flimsy to set up , it's just best to do it by trial and error and for this helper function is provided.
pressing keys 1-7 selects parameters
1,2,3 - even frame timing parameters
4,5,6 - odd frame timing parameters
7 is the expected refesh rate the system converges to. 

The longer refresh rate the more time we get for drawing and dma
when one goes down as low as 33hz the display might flicker on it's own , especially with solid colors displayed.

ST7789 and ST7735 have different registers with slightly different range (st7789 has more range by one bit depth)
idle and normal mode registers are swapped, porch control register is an extra register for st7789 (but only for normal mode...)
and the names of registers are different (FRMCTL/FRMCTRL... dyslexics beware)

in general it works quite well . 
the code is using esp32 rtos tasking but it's not really nessesary, if you use chip without multitasking os 
merely call the graphing functions from just before the waiting for dma to finish . 
Of course this will reduce the amount of time the graphing routine can run and will not allow dma/multicore features, but for small things it's good enough.

