/*
 * cart_factory.h
 *
 *  Created on: Aug 7, 2014
 *      Author: Dale
 */

#ifndef CART_FACTORY_H_
#define CART_FACTORY_H_

#include <string>
#include "mappers/cart.h"

class CartFactory
{
public:
	Cart* getCartridge(std::string filename);
};


#endif /* CART_FACTORY_H_ */
