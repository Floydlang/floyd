//
//  variable_length_quantity.cpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-02-22.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "variable_length_quantity.h"

#include <vector>
#include "quark.h"


////////////////////////////////	REFERENCE IMPLEMENTATION


std::vector<uint8_t> WriteVarLen(unsigned long value)
{
   std::vector<uint8_t> result;
   unsigned long buffer;
   buffer = value & 0x7F;

   while ( (value >>= 7) )
   {
     buffer <<= 8;
     buffer |= ((value & 0x7F) | 0x80);
   }

   while (true)
   {
      result.push_back(buffer & 0xff);
//      putc(buffer, outfile);
      if (buffer & 0x80)
          buffer >>= 8;
      else
          break;
   }
   return result;
}

uint8_t getc2(const uint8_t data[], uint32_t& io_pos){
	const auto result = data[io_pos];
	io_pos++;
	return result;
}

unsigned long ReadVarLen(const uint8_t data[], uint32_t pos)
{
    unsigned long value;
    unsigned char c;

    if ( (value = getc2(data, pos)) & 0x80 )
    {
       value &= 0x7F;
       do
       {
         value = (value << 7) + ((c = getc2(data, pos)) & 0x7F);
       } while (c & 0x80);
    }

    return(value);
}

uint32_t unpack_vlq(const uint8_t data[]){
	const auto r = ReadVarLen(data, 0);
	return r;
}



const std::vector<std::pair<uint32_t, std::vector<uint8_t>>> test_data = {
	{ 0x00000000, { 0x00 } },
	{ 0x00000040, { 0x40 } },
	{ 0x0000007F, { 0x7F } },

	{ 0x00000080, { 0x81, 0x00 } },
	{ 0x00002000, { 0xC0, 0x00 } },
	{ 0x00003FFF, { 0xFF, 0x7F } },

	{ 0x00004000, { 0x81, 0x80, 0x00 } },
	{ 0x00100000, { 0xC0, 0x80, 0x00 } },
	{ 0x001FFFFF, { 0xFF, 0xFF, 0x7F } },

	{ 0x00200000, { 0x81, 0x80, 0x80, 0x00 } },
	{ 0x08000000, { 0xC0, 0x80, 0x80, 0x00 } },
	{ 0x0FFFFFFF, { 0xFF, 0xFF, 0xFF, 0x7F } },

	{ 0x1FFFFFFF, { 0x10, 0xFF, 0xFF, 0xFF, 0x7F } },
	{ 0x1FFFFFFF, { 0x10, 0xFF, 0xFF, 0xFF, 0x7F } }
};


/*
		BIT CACHE LINE
		|<-0---- 1------- 2------- 3------- 4------- 5------- 6------- 7----->|
		EEEEDDDD DDDCCCCC CCBBBBBB BAAAAAAA
*/

//http://midi.teragonaudio.com/tech/midifile/vari.htm
std::vector<uint8_t> pack_vlq(uint32_t v){
	const uint64_t a = (v & 0b00000000'00000000'00000000'01111111) >> 0;
	const uint64_t b = (v & 0b00000000'00000000'00111111'10000000) >> 7;
	const uint64_t c = (v & 0b00000000'00011111'11000000'00000000) >> 14;
	const uint64_t d = (v & 0b00001111'11100000'00000000'00000000) >> 21;
	const uint64_t e = (v & 0b11110000'00000000'00000000'00000000) >> 28;

	//	This is 5 * 8 = 40 bits.
	uint64_t r = (e << 32) | (d << 24) | (c << 16) | (b << 8) | (a << 0);

/*
		BIT CACHE LINE
		|<-0---- 1------- 2------- 3------- 4------- 5------- 6------- 7----->|
		1000EEEE 1DDDDDDD 1CCCCCCC 1BBBBBBB 1AAAAAAA
*/
	if(r < (1 << 8)){
		return { uint8_t(a & 0xff) };
	}
	else if(r < (1 << 16)){
		return { uint8_t((b & 0xff) | 0x80), uint8_t(a & 0xff) };
	}
	else if(r < (1 << 24)){
		return { uint8_t((c & 0xff) | 0x80), uint8_t((b & 0xff) | 0x80), uint8_t(a & 0xff) };
	}
	else if(r < (1L << 32)){
		return { uint8_t((d & 0xff) | 0x80), uint8_t((c & 0xff) | 0x80), uint8_t((b & 0xff) | 0x80), uint8_t(a & 0xff) };
	}
	else{
		return { uint8_t((e & 0xff) | 0x80), uint8_t((d & 0xff) | 0x80), uint8_t((c & 0xff) | 0x80), uint8_t((b & 0xff) | 0x80), uint8_t(a & 0xff) };
	}
}

//	Can be done without std::vector, like this:
std::pair<size_t, uint8_t[5]> pack_vlq2(uint32_t v){
	return {};
}


//	Wrapper that controls every result against reference implementation.
std::vector<uint8_t> pack_vlq__verified(uint32_t v){
	const auto a = pack_vlq(v);
	const auto b = WriteVarLen(v);

	QUARK_ASSERT(a == b);
	return a;
}

QUARK_UNIT_TEST("variable_length_quantity", "pack_vlq()", "", ""){
	std::vector<uint32_t> test_values;

	test_values.push_back(0);
	test_values.push_back(UINT_MAX);
	test_values.push_back(0b00000000'00000000'00000000'00000000);
	test_values.push_back(0b10101010'10101010'10101010'10101010);
	test_values.push_back(0b01010101'01010101'01010101'01010101);
	test_values.push_back(0b11111111'00000000'00000000'00000000);
	test_values.push_back(0b00000000'11111111'00000000'00000000);
	test_values.push_back(0b00000000'00000000'11111111'00000000);
	test_values.push_back(0b00000000'00000000'00000000'11111111);

	for(uint32_t v = 1 ; v < 32 ; v++){
		test_values.push_back(1 << v);
		test_values.push_back((1 << v) - 1);
		test_values.push_back((1 << v) + 1);
	}

	for(const auto e: test_values){
		pack_vlq__verified(e);
	}
}


//	Wrapper that controls every result against reference implementation.
uint32_t unpack_vlq__verified(const uint8_t data[]){
	const auto a = unpack_vlq(data);
	return a;
}

QUARK_UNIT_TEST("variable_length_quantity", "pack_vlq()", "", ""){
	QUARK_UT_VERIFY(unpack_vlq__verified(&test_data[0].second[0]) == test_data[0].first);
	QUARK_UT_VERIFY(unpack_vlq__verified(&test_data[1].second[0]) == test_data[1].first);
	QUARK_UT_VERIFY(unpack_vlq__verified(&test_data[2].second[0]) == test_data[2].first);

	QUARK_UT_VERIFY(unpack_vlq__verified(&test_data[3].second[0]) == test_data[3].first);
	QUARK_UT_VERIFY(unpack_vlq__verified(&test_data[4].second[0]) == test_data[4].first);
	QUARK_UT_VERIFY(unpack_vlq__verified(&test_data[5].second[0]) == test_data[5].first);

	QUARK_UT_VERIFY(unpack_vlq__verified(&test_data[6].second[0]) == test_data[6].first);
	QUARK_UT_VERIFY(unpack_vlq__verified(&test_data[7].second[0]) == test_data[7].first);
	QUARK_UT_VERIFY(unpack_vlq__verified(&test_data[8].second[0]) == test_data[8].first);

	QUARK_UT_VERIFY(unpack_vlq__verified(&test_data[9].second[0]) == test_data[9].first);
	QUARK_UT_VERIFY(unpack_vlq__verified(&test_data[10].second[0]) == test_data[10].first);
	QUARK_UT_VERIFY(unpack_vlq__verified(&test_data[11].second[0]) == test_data[11].first);
}


QUARK_UNIT_TEST("variable_length_quantity", "unpack_vlq()", "", ""){
	QUARK_UT_VERIFY(pack_vlq__verified(test_data[0].first) == test_data[0].second);
}

