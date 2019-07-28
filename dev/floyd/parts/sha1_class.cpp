//
//  sha1_class.cpp
//  Permafrost
//
//  Created by Marcus Zetterquist on 2014-01-16.
//  Copyright (c) 2014 Marcus Zetterquist. All rights reserved.
//

#include "sha1_class.h"


#include "quark.h"
#include "sha1.h"

#ifdef __APPLE__
     #include <CommonCrypto/CommonDigest.h>
#endif


#define ASSERT(x) QUARK_ASSERT(x)
#define UNIT_TEST(a, b, c, d) QUARK_UNIT_TEST(a, b, c, d)
#define UT_VERIFY(x) QUARK_UT_VERIFY(x)
#define TRACE(x) QUARK_TRACE(x)


///////////////////////////////////////		TSHA1


TSHA1::TSHA1(){
	for(auto& i : fHash){
		 i = 0x00;
	}

	ASSERT(CheckInvariant());
}

TSHA1::TSHA1(const std::uint8_t iData[kHashSize]){
	ASSERT(iData != nullptr);

	std::memcpy(fHash, iData, kHashSize);

	ASSERT(CheckInvariant());
}


TSHA1::TSHA1(const TSHA1& iOther){
	ASSERT(iOther.CheckInvariant());

	std::memcpy(&fHash[0], &iOther.fHash[0], kHashSize);
	ASSERT(CheckInvariant());
}

TSHA1& TSHA1::operator=(const TSHA1& iOther){
	ASSERT(CheckInvariant());
	ASSERT(iOther.CheckInvariant());

	TSHA1 temp(iOther);
	temp.Swap(*this);

	ASSERT(CheckInvariant());
	return *this;
}

void TSHA1::Swap(TSHA1& ioOther){
	ASSERT(CheckInvariant());
	ASSERT(ioOther.CheckInvariant());

	std::uint8_t temp[kHashSize];
	std::memcpy(temp, fHash, kHashSize);
	std::memcpy(fHash, ioOther.fHash, kHashSize);
	std::memcpy(ioOther.fHash, temp, kHashSize);

	ASSERT(CheckInvariant());
	ASSERT(ioOther.CheckInvariant());
}

bool TSHA1::CheckInvariant() const{
	return true;
}

bool TSHA1::IsNil() const{
	ASSERT(CheckInvariant());

	for(auto& i : fHash){
		if(i != 0x00){
			return false;
		}
	}
	return true;
}

const std::uint8_t* TSHA1::GetPtr() const{
	ASSERT(CheckInvariant());

	return &fHash[0];
}

bool TSHA1::operator==(const TSHA1& iOther) const{
	ASSERT(CheckInvariant());
	ASSERT(iOther.CheckInvariant());

	for(std::size_t i = 0 ; i < kHashSize ; i++){
		if(fHash[i] != iOther.fHash[i]){
			return false;
		}
	}
	return true;
}


bool operator<(const TSHA1& iLHS, const TSHA1& iRHS){
	ASSERT(iLHS.CheckInvariant());
	ASSERT(iRHS.CheckInvariant());

	int result = std::memcmp(&iLHS.fHash[0], &iRHS.fHash[0], TSHA1::kHashSize);
	return result < 0 ? true : false;
}








TSHA1 CalcSHA1(const std::uint8_t iData[], std::size_t iSize){
	ASSERT(iData != nullptr || iSize == 0);

#ifndef __APPLE_
	TSHA1 hash;
    sha1::calc(iData, static_cast<const int>(iSize), hash.fHash);
#else
	ASSERT(CC_SHA1_DIGEST_LENGTH == TSHA1::kHashSize);
	std::uint8_t buffer[TSHA1::kHashSize];
	CC_SHA1(iData,  static_cast<const int>(iSize), buffer);
	TSHA1 hash(buffer);
	 //CC_LONG
#endif

	ASSERT(hash.CheckInvariant());
	return hash;
}

TSHA1 CalcSHA1(const std::string& iS){
	auto ptr = reinterpret_cast<const std::uint8_t*>(iS.c_str());
	return CalcSHA1(ptr, iS.size());
}



std::string SHA1ToStringPlain(const TSHA1& iSHA1){
	ASSERT(iSHA1.CheckInvariant());

	char s[41];
	sha1::toHexString(iSHA1.GetPtr(), s);
	std::string hex(s);
//	TRACE(hex);


	ASSERT(SHA1FromStringPlain(s) == iSHA1);

	return hex;
}

namespace {
	uint8_t HexCharToVal(char iChar){
		ASSERT((iChar >= '0' && iChar <= '9') || (iChar >= 'a' && iChar <= 'f'));

		if(iChar >= '0' && iChar <= '9'){
			return iChar - '0';
		}
		else if(iChar >= 'a' && iChar <= 'f'){
			return iChar - 'a' + 10;
		}
		else{
			ASSERT(false);
			return 0;
		}
	}
}
TSHA1 SHA1FromStringPlain(const std::string& iString){
	ASSERT(iString.size() == kSHA1StringSize);

	std::uint8_t buffer[TSHA1::kHashSize];
	for(int i = 0 ; i < 20 ; i++){
		uint8_t high4Bit = HexCharToVal(iString[i * 2 + 0]);
		uint8_t low4Bit = HexCharToVal(iString[i * 2 + 1]);

		uint32_t ch = (high4Bit << 4) | low4Bit;
		buffer[i] = static_cast<uint8_t>(ch);
	}

	TSHA1 result(buffer);
	ASSERT(result.CheckInvariant());
	return result;
}



bool IsValidTaggedSHA1(const std::string& iString){
	if(iString.size() != kSHA1Prefix + 40){
		return false;
	}
	std::string prefix(&iString[0], &iString[kSHA1Prefix]);
	if(prefix != "sha1 = "){
		return false;
	}
	std::string plain = iString.substr(kSHA1Prefix);
	for(auto i: plain){
		if((i >= '0' && i <= '9') || (i >= 'a' && i <= 'f')){
		}
		else{
			return false;
		}
	}
	return true;
}

std::string SHA1ToStringTagged(const TSHA1& iSHA1){
	ASSERT(iSHA1.CheckInvariant());

	std::string result = "sha1 = " + SHA1ToStringPlain(iSHA1);
	ASSERT(IsValidTaggedSHA1(result));
	return result;
}

TSHA1 SHA1FromStringTagged(const std::string& iString){
	ASSERT(IsValidTaggedSHA1(iString));

	std::string s(&iString[kSHA1Prefix], &iString[iString.size()]);
	TSHA1 result = SHA1FromStringPlain(s);
	return result;
}




/*
	const TSHA1 CalcSHA1Vec(const immutable_vector<std::uint8_t>& iData){
		ASSERT(iData.check_invariant());

		TSHA1 result = ::CalcSHA1(iData.data(), iData.size());
		return result;
	}
*/


///////////////////////////////////////		Unit tests


namespace {
	const uint8_t kTestData[] = {
		0x12, 0x00, 0x9f, 0x45, 0x22, 0x1a, 0x1b, 0x12, 0xff, 0xfe,
		0x00, 0xf1, 0xbf, 0xbb, 0xfb, 0x00, 0x00, 0x01, 0x02, 0xff
	};

	const uint8_t kTestDataHash[] = {
		0xd5, 0x12, 0x3d, 0xa1, 0x3b, 0xcb, 0xa2, 0x14, 0xf2, 0xd1,
		0x11, 0x61, 0x6b, 0xae, 0xd3, 0x49, 0xa6, 0x8b, 0x07, 0xb3
	};

/*
	const uint8_t kHashNoData[] = {
		0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55,
		0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09,
	};
*/
	const uint8_t kZeroHash[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};

}


///////////////////////////////		sha1::calc()



UNIT_TEST("sha1", "sha1()", "20 bytes", "valid hash"){
	std::uint8_t buffer[TSHA1::kHashSize];
    sha1::calc(kTestData, TSHA1::kHashSize, buffer);
	UT_VERIFY(std::memcmp(buffer, kTestDataHash, TSHA1::kHashSize) == 0);
}


///////////////////////////////		sha1::toHexString()


UNIT_TEST("sha1", "sha1()", "20 bytes", "valid hash"){
	TSHA1 hash = CalcSHA1(kTestData, 20);

	char s[41];
	sha1::toHexString(hash.GetPtr(), s);
	std::string hex(s);
	TRACE(hex);
}


namespace {
	TSHA1 MakeRamp(){
		std::uint8_t buffer[TSHA1::kHashSize];
		for(std::size_t i = 0 ; i < TSHA1::kHashSize ; i++){
			buffer[i] = static_cast<uint8_t>(10 + i);
		}
		return TSHA1(buffer);
	}
}


///////////////////////////////		TSHA1::TSHA1()


UNIT_TEST("sha1", "TSHA1::TSHA1()", "equal", "all zeros"){
	TSHA1 test;
	UT_VERIFY(test == TSHA1(kZeroHash));
}


///////////////////////////////		TSHA1::TSHA1(const TSHA1& iOther)


UNIT_TEST("sha1", "TSHA1::TSHA1(const TSHA1& iOther)", "copy", "same byte contents"){
	TSHA1 test = MakeRamp();
	TSHA1 copy(test);
	UT_VERIFY(test == copy);
}



///////////////////////////////		TSHA1::operator=()


UNIT_TEST("sha1", "TSHA1::TSHA1(const TSHA1& iOther)", "copy", "same byte contents"){
	TSHA1 test = MakeRamp();

	TSHA1 copy;
	copy = test;
	UT_VERIFY(test == copy);
}



///////////////////////////////		TSHA1::operator==()



UNIT_TEST("sha1", "TSHA1::operator==()", "equal", "true"){
	TSHA1 test = CalcSHA1("my message");
	TSHA1 test2 = test;

	UT_VERIFY(test == test2);
}

UNIT_TEST("sha1", "TSHA1::operator==()", "equal", "true"){
	TSHA1 test = CalcSHA1("my message");
	TSHA1 test2;

	UT_VERIFY(!(test == test2));
}




///////////////////////////////		TSHA1::operator<()



UNIT_TEST("sha1", "TSHA1::operator<()", "equal", "false"){
	TSHA1 test = CalcSHA1("my message");
	TSHA1 test2 = test;

	UT_VERIFY(!(test < test2));
}

UNIT_TEST("sha1", "TSHA1::operator<()", "forced a < b", "true"){
	TSHA1 base = CalcSHA1("my message");

	std::uint8_t buffer[TSHA1::kHashSize];
	memcpy(buffer, base.GetPtr(), TSHA1::kHashSize);

	buffer[0] = 1;
	TSHA1 a(buffer);

	buffer[0] = 20;
	TSHA1 b(buffer);

	UT_VERIFY(a < b);
}

UNIT_TEST("sha1", "TSHA1::operator<()", "forced different b < a", "false"){
	TSHA1 base = CalcSHA1("my message");

	std::uint8_t buffer[TSHA1::kHashSize];
	memcpy(buffer, base.GetPtr(), TSHA1::kHashSize);

	buffer[0] = 1;
	TSHA1 a(buffer);

	buffer[0] = 20;
	TSHA1 b(buffer);

	UT_VERIFY(!(b < a));
}



///////////////////////////////		IsValidTaggedSHA1()



UNIT_TEST("sha1", "IsValidTaggedSHA1()", "Valid, zero sha1", "OK"){
	UT_VERIFY(IsValidTaggedSHA1("sha1 = 0000000000000000000000000000000000000000"));
}

UNIT_TEST("sha1", "IsValidTaggedSHA1()", "Valid, zero sha1, missing tag", "fail"){
	UT_VERIFY(!IsValidTaggedSHA1("0000000000000000000000000000000000000000"));
}

UNIT_TEST("sha1", "IsValidTaggedSHA1()", "Valid tag, too long hash", "fail"){
	UT_VERIFY(!IsValidTaggedSHA1("sha1 = 00000000000000000000000000000000000000001"));
}

UNIT_TEST("sha1", "IsValidTaggedSHA1()", "Valid tag, too short hash", "fail"){
	UT_VERIFY(!IsValidTaggedSHA1("sha1 = 000000000000000000000000000000000000000"));
}

UNIT_TEST("sha1", "IsValidTaggedSHA1()", "Valid tag, legal numbers", "OK"){
	UT_VERIFY(IsValidTaggedSHA1("sha1 = 0123456789abcdef0123456789abcdef01234567"));
}

UNIT_TEST("sha1", "IsValidTaggedSHA1()", "Valid, illegal char", "fail"){
	UT_VERIFY(!IsValidTaggedSHA1("sha1 = 000000000000000000000000000000000000000h"));
}


/*
///////////////////////////////		CalcSHA1Vec()



UNIT_TEST("sha1", "CalcSHA1Vec()", "kTestData", "correct hash"){
	std::vector<std::uint8_t> v(&kTestData[0], &kTestData[20]);
	TSHA1 hash = CalcSHA1Vec(v);
	UT_VERIFY(hash == TSHA1(kTestDataHash));
}

UNIT_TEST("sha1", "CalcSHA1Vec()", "empty input array", "correct hash"){
	TSHA1 hash = CalcSHA1Vec(immutable_vector<std::uint8_t>());
	UT_VERIFY(hash == TSHA1(kHashNoData));
}
*/



