//
//  variable_length_quantity.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-22.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef variable_length_quantity_hpp
#define variable_length_quantity_hpp

#include <vector>

uint32_t unpack_vlq(const uint8_t data[]);
std::vector<uint8_t> pack_vlq(uint32_t v);


#endif /* variable_length_quantity_hpp */
