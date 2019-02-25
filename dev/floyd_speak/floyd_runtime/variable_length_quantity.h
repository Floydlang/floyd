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

inline std::pair<uint32_t, size_t> unpack_vlq(const uint8_t data[]);
std::vector<uint8_t> pack_vlq(uint32_t v);



inline std::pair<uint32_t, size_t> unpack_vlq(const uint8_t data[]){
	const auto e0 = data[0];
	if((e0 & 0x80) == 0x00){
		return { e0, 1 };
	}

	const auto e1 = data[1];
	if((e1 & 0x80) == 0x00){
		return { ((e0 & 0x7f) << 7) | e1, 2 };
	}

	const auto e2 = data[2];
	if((e2 & 0x80) == 0x00){
		return { ((e0 & 0x7f) << 14) | ((e1 & 0x7f) << 7) | e2, 3 };
	}

	const auto e3 = data[3];
	if((e3 & 0x80) == 0x00){
		return { ((e0 & 0x7f) << 21) | ((e1 & 0x7f) << 14) | ((e2 & 0x7f) << 7) | e3, 4 };
	}

	const auto e4 = data[4];
	if((e4 & 0x80) == 0x00){
		return { ((e0 & 0x7f) << 28) | ((e1 & 0x7f) << 21) | ((e2 & 0x7f) << 14) | ((e3 & 0x7f) << 7) | e4, 5 };
	}
	throw std::exception();
}

inline uint32_t unpack_vlq2(const uint8_t data[], size_t& pos_io){
	const auto e0 = data[pos_io];
	if((e0 & 0x80) == 0x00){
		pos_io = pos_io + 1;
		return e0;
	}

	const auto e1 = data[pos_io + 1];
	if((e1 & 0x80) == 0x00){
		pos_io = pos_io + 2;
		return ((e0 & 0x7f) << 7) | e1;
	}

	const auto e2 = data[pos_io + 2];
	if((e2 & 0x80) == 0x00){
		pos_io = pos_io + 3;
		return ((e0 & 0x7f) << 14) | ((e1 & 0x7f) << 7) | e2;
	}

	const auto e3 = data[pos_io + 3];
	if((e3 & 0x80) == 0x00){
		pos_io = pos_io + 4;
		return ((e0 & 0x7f) << 21) | ((e1 & 0x7f) << 14) | ((e2 & 0x7f) << 7) | e3;
	}

	const auto e4 = data[pos_io + 4];
	if((e4 & 0x80) == 0x00){
		pos_io = pos_io + 5;
		return ((e0 & 0x7f) << 28) | ((e1 & 0x7f) << 21) | ((e2 & 0x7f) << 14) | ((e3 & 0x7f) << 7) | e4;
	}
	throw std::exception();
}


#endif /* variable_length_quantity_hpp */
