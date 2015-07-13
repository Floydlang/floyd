//
//  FloydType.h
//  Floyd
//
//  Created by Marcus Zetterquist on 09/07/15.
//  Copyright (c) 2015 Marcus Zetterquist. All rights reserved.
//

#ifndef __Floyd__FloydType__
#define __Floyd__FloydType__

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>

struct TComposite;
struct FloydDT;




/////////////////////////////////////////		FloydDT



/*
	This a dynamic value type that can hold all types of values in Floyd's simulation.
	This data type is *never* exposed to client programs, only used internally by Floyd.

	All values are immutable.

	In the floyd simumation, values are static, but in the host they are dynamic.

	Also holds every new data type, like composites.

	###	Use a unique ID for each interneted object-type = smaller than object. Goal is FloydDT == 8 bytes on 64-bit systems.

	Signatures: these are unique names for a specific type. All types: built-in, custom composites, function etc.
	You can also give a type a specific name, which makes it its own uniqu type.

	Definition											Explanation
	---------------------------------------------------	------------------------------------------------------------
	"<null>"											null
	"<float>"											float
	"<string>"											string
	"<float>(<string>, <float>)"						function returning float, with string and float arguments
	"{ <string>, <string>, <float> }"					composite with three unnamed members.

	"meters <float>"									float with a new name
	"triangle { <float> a, <float> b, <float> c }"		a named composite
	"<float> MySineF(<float>)"							a named function

	The signature strings are in a normalized format and must appear exactly as above, whitespace and all.

	There is an alternative, faster encoding of the signature: a 32bit hash of the signature string.

	### Add map, vector etc.


	[] = vector
	{} = composite
	{} = map
	() = tuple
	<> = tagged_union

	f() = function
*/


//### Allow member names to be part of composite's signature? I think no.



struct TTypeSignature {
	std::string _s;
};

struct TTypeSignatureHash {
	uint32_t _hash;
};



//	Input string must be wellformed, normalized format.
TTypeSignature MakeSignature(const std::string& s);

TTypeSignatureHash HashSignature(const TTypeSignature& s);

//	Returns the members of a composite.
std::vector<TTypeSignature> UnpackCompositeSignature(const TTypeSignature& s);


struct TFunctionSignature {
	TTypeSignature _returnType;
	std::vector<std::pair<std::string, TTypeSignature>> _args;
};

std::vector<TFunctionSignature> UnpackFunctionSignature(const TTypeSignature& s);



/////////////////////////////////////////		Composite


//??? Invariant-check must be part of composite signature too!


typedef bool (*CompositeCheckInvariant)(const TComposite& value);

struct TCompositeDef {
	std::map<std::string, FloydDT> _members;
	CompositeCheckInvariant _checkInvariant;
};



/////////////////////////////////////////		Function


const int kMaxFunctionArgs = 6;

typedef FloydDT (*CFunctionPtr)(const FloydDT args[], std::size_t argCount);

//	Function signature = string in format
//		FloydType
//	### Make optimized calling signatures with different sets of C arguments.

struct FunctionDef {
	TFunctionSignature _signature;
//	TTypeSignature _signature;
	CFunctionPtr _functionPtr;
};




/////////////////////////////////////////		TGenericContainer



template <typename V> class TOrdered {
	public: TOrdered(){
		UT_VERIFY(CheckInvariant());
	}

	public: std::size_t Count() const {
		return _vector.size();
	}

	public: bool CheckInvariant() const{
		return true;
	}


	///////////////////////////////		Seq

	public: V First() const {
		UT_VERIFY(CheckInvariant());
		ASSERT(!_vector.empty());
		return _vector[0];
	}

	public: TOrdered<V> Rest() const {
		UT_VERIFY(CheckInvariant());
		ASSERT(!_vector.empty());

		TOrdered<V> result;
		auto temp = std::vector<V>(&_vector[1], &_vector[_vector.size()]);
		result._vector.swap(temp);
		ASSERT(result.CheckInvariant());
		ASSERT(result.Count() == Count() - 1);
		return result;
	}



	///////////////////////////////		Order

	public: const V& operator[](std::size_t idx) const {
		UT_VERIFY(CheckInvariant());
		ASSERT(idx < _vector.size());

		return _vector[idx];
	};


	public: const TOrdered<V> Assoc(std::size_t idx, V value) const {
		UT_VERIFY(CheckInvariant());
		ASSERT(idx <= _vector.size());

		//	Append just after end of vector?
		if(idx == _vector.size()){
			TOrdered<V> result;
			result._vector = _vector;
			result._vector.push_back(value);

			ASSERT(result.CheckInvariant());
			ASSERT(result.Count() == Count() + 1);
			return result;
		}

		//	Overwrite existing index.
		else{
			TOrdered<V> result;
			result._vector = _vector;
			result._vector[idx] = value;

			ASSERT(result.CheckInvariant());
			ASSERT(result.Count() == Count());
			return result;
		}
	};



	///////////////////////////////		State
	private: std::vector<V> _vector;
};


template <typename V> TOrdered<V> Append(const TOrdered<V>& a, const TOrdered<V>& b){
	ASSERT(a.CheckInvariant());
	ASSERT(b.CheckInvariant());

	TOrdered<V> result = a + b;
	return result;
}




/////////////////////////////////////////		FloydDT



//	This is the normalized order of the types.
enum FloydDTType {
	kNull = 1,
	kFloat,
	kString,
	kFunction
//	,
//	kComposite,
//	kMapKV
};

struct FloydDT {
	public: bool CheckInvariant() const;
	public: std::string GetType() const;


	///////////////////		State
		FloydDTType _type = FloydDTType::kNull;
		float _asFloat = 0.0f;
		std::string _asString = "";

		std::shared_ptr<TComposite> _asComposite;
		std::shared_ptr<FunctionDef> _asFunction;
};


/////////////////////////////////////////		Functions


FloydDT MakeNull();
bool IsNull(const FloydDT& value);


FloydDT MakeFloat(float value);
bool IsFloat(const FloydDT& value);
float GetFloat(const FloydDT& value);


FloydDT MakeString(const std::string& value);
bool IsString(const FloydDT& value);
std::string GetString(const FloydDT& value);


FloydDT MakeFunction(const FunctionDef& f);
bool IsFunction(const FloydDT& value);
CFunctionPtr GetFunction(const FloydDT& value);

//	Arguments must match those of the function or assert.
FloydDT CallFunction(const FloydDT& value, const std::vector<FloydDT>& args);

FloydDT MakeComposite(const TCompositeDef& c);



void RunFloydTypeTests();

#endif /* defined(__Floyd__FloydType__) */
