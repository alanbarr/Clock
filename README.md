#VFD Clock for MSP430 Launchpad

## Fork!
This is a fork of [cubeberg's GIT repository for his MSP430 Launchpad IV-18
VFD clock](https://github.com/cubeberg/Clock).

## Hardware Notes:
The VFD clock booster pack is available for purchase as:
* [A full kit from the 43oh store.](
   http://store.43oh.com/index.php?route=product/product&manufacturer_id=12&product_id=95)

* [A bare PCB from the 43oh store.](
   http://store.43oh.com/index.php?route=product/product&manufacturer_id=12&product_id=101)

* [A bare PCB on Tindie.](
   https://www.tindie.com/products/cubeberg/iv-18-vfd-clock-bare-pcb-launchpad-booster/)

Documentation, assembly instructions, etc. can be found here:
http://forum.43oh.com/topic/2487-siv-18-vfd-clock-booster-pack/

## TODO
* Get temperature working with mspgcc.
* Some general tidying.
* Do something about build warnings in time.h regarding 2D array.
* Look into splitting up main.c into smaller files.
* Shift logic out of ISR's to normal code.
* Find out why warning: multi-character character constant [-Wmultichar] was
  not being given for the degree? symbol in translateChar before it was moved.
* Remove override screen in favour of x amount of extra screens.
* Change first day of week to Monday? or make it configurable.
* Change date format to dd/mm/yy or make it configurable.
