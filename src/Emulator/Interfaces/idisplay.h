/*
 * Display.h
 *
 *  Created on: Oct 19, 2014
 *      Author: Dale
 */

#ifndef SRC_EMULATOR_INTERFACES_DISPLAY_H_
#define SRC_EMULATOR_INTERFACES_DISPLAY_H_

class IDisplay
{
public:
    virtual void NextPixel(uint32_t pixel) = 0;
    virtual ~IDisplay() {}
};



#endif /* SRC_EMULATOR_INTERFACES_DISPLAY_H_ */
