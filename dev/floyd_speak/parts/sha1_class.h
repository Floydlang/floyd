//
//  sha1_class.h
//  Permafrost
//
//  Created by Marcus Zetterquist on 2014-01-16.
//  Copyright (c) 2014 Marcus Zetterquist. All rights reserved.
//

#ifndef __Permafrost__sha1_class__
#define __Permafrost__sha1_class__

#include <cstdint>
#include <cstddef>
#include <string>
#include "quark.h"





/**
sha1 hash
https://code.google.com/p/smallsha1/downloads/detail?name=sha1.zip&can=2&q=
*/



////////////////////////			TSHA1



struct TSHA1 {
	public: static const std::size_t kHashSize = 20;

	public: TSHA1();
	public: TSHA1(const std::uint8_t iData[kHashSize]);
	public: TSHA1(const TSHA1& iOther);
	public: TSHA1& operator=(const TSHA1& iOther);
	public: void Swap(TSHA1& ioOther);

	public: bool IsNil() const;
	public: const std::uint8_t* GetPtr() const;
	public: bool CheckInvariant() const;
	public: bool operator==(const TSHA1& iOther) const;
	public: bool operator!=(const TSHA1& iOther) const { return !(*this == iOther);};

	friend bool operator<(const TSHA1& iLHS, const TSHA1& iRHS);

	///////////////////		State
		 std::uint8_t fHash[kHashSize];
};

bool operator<(const TSHA1& iLHS, const TSHA1& iRHS);


////////////////////////			Free


//	strlen("sha1 = ");
const std::size_t kSHA1Prefix = 7;

//	iData can be nullptr if size is 0.
TSHA1 CalcSHA1(const std::uint8_t iData[], std::size_t iSize);

TSHA1 CalcSHA1(const std::string& iS);

/**
	String is always exactly 40 characters long.
	"1234567890123456789012345678901234567890"
*/
static const size_t kSHA1StringSize = TSHA1::kHashSize * 2;

std::string SHA1ToStringPlain(const TSHA1& iSHA1);
TSHA1 SHA1FromStringPlain(const std::string& iString);


/**
	"sha1 = 1234567890123456789012345678901234567890"
*/
bool IsValidTaggedSHA1(const std::string& iString);
std::string SHA1ToStringTagged(const TSHA1& iSHA1);
TSHA1 SHA1FromStringTagged(const std::string& iString);


//const TSHA1 CalcSHA1Vec(const immutable_vector<std::uint8_t>& iData);



#endif /* defined(__Permafrost__sha1_class__) */
