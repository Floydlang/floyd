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

struct FloydDT;
struct TCompositeValue;
struct FloydBasicRuntime;
struct TStaticCompositeType;



//	This is the normalized order of the types.
enum FloydDTType {
	kNull = 1,
//	kBool,
	kFloat,
	kString,
	kFunction,
	kComposite

/*
	Exception

	Seq
	Ordered
	Unordered
*/

};




/////////////////////////////////////////		Type Signatures



/*

	"<null>"										null
//	"<bool>"										bool
	"<float>"										float
	"<string>"										string

	"<float>(<string>, <float>)"					function returning float, with string and float arguments

	"{ <string> a, <string> b, <float> c }"			composite with three NAMED members.

	"<string>[]"									seq with strings. No key.

	"<float>[<int>]"								ordered with floats, array access using int.

	"<float>{<int>}"								unordered with floats, keyed on ints.
	"<float>{<string>}"								unordered with floats, keyed on strings.

	"{ <float>, <int>, <string> }"					tuple with float, int, string. Always access with index.

	"<float>/<int>/<string>"						tagged union.
*/





struct TTypeSignatureString {
	TTypeSignatureString(){
	}
	explicit TTypeSignatureString(const std::string& s) :
		_s(s)
	{
	}
	TTypeSignatureString(const TTypeSignatureString& other)=default;

	bool CheckInvariant() const {
		ASSERT(!_s.empty());
		return true;
	}

	std::string _s;
};

//	### use hash of string for speed.




//	Input string must be wellformed, normalized format.
TTypeSignatureString MakeSignature(const std::string& s);



struct TFunctionSignature {
	TTypeSignatureString _returnType;
	std::vector<std::pair<std::string, TTypeSignatureString>> _args;
};

std::vector<TFunctionSignature> UnpackFunctionSignature(const TTypeSignatureString& s);


struct TTypeSignatureSpec {
	TTypeSignatureSpec(FloydDTType type) :
		_type(type)
	{
	}


	FloydDTType _type = FloydDTType::kNull;
	
	std::vector<TTypeSignatureString> _more;
};


TTypeSignatureSpec TypeSignatureFromString(const TTypeSignatureString& s);











/////////////////////////////////////////		FloydDT



/*
	This a dynamic value type that can hold all types of values in Floyd's simulation.
	This data type is *never* exposed to client programs, only used internally by Floyd.

	All values are immutable.

	In the floyd simulation, values are static, but in the host they are dynamic.

	Also holds every new data type, like composites.
*/


//### Allow member names to be part of composite's signature? I think no.





/////////////////////////////////////////		Function



const int kMaxFunctionArgs = 6;

typedef FloydDT (*CFunctionPtr)(const FloydDT args[], std::size_t argCount);

//	Function signature = string in format
//		FloydType
//	### Make optimized calling signatures with different sets of C arguments.

struct FunctionDef {
	TFunctionSignature _signature;
	CFunctionPtr _functionPtr;
};




/////////////////////////////////////////		TSeq




template <typename V> class TSeq {
	public: TSeq(){
		ASSERT(CheckInvariant());
	}

	public: TSeq(const std::vector<V>& v) :
		_vector(v)
	{
		ASSERT(CheckInvariant());
	}

	public: std::size_t Count() const {
		return _vector.size();
	}

	public: bool CheckInvariant() const{
		return true;
	}


	///////////////////////////////		Seq

	public: V First() const {
		ASSERT(CheckInvariant());
		ASSERT(!_vector.empty());
		return _vector[0];
	}

	public: std::shared_ptr<const TSeq<V> > Rest() const {
		ASSERT(CheckInvariant());
		ASSERT(!_vector.empty());

		auto v = std::vector<V>(&_vector[1], &_vector[_vector.size()]);
		auto result = std::shared_ptr<const TSeq<V> >(new TSeq<V>(v));
		ASSERT(result->Count() == Count() - 1);
		return result;
	}


	public: std::vector<V> ToVector() const{
		ASSERT(CheckInvariant());

		return _vector;
	}

	///////////////////////////////		State
	private: const std::vector<V> _vector;
};


struct TSeqTypeInstance {
	FloydDTType _valueType;
};

struct TOrderedTypeInstance {
	FloydDTType _valueType;
};

struct TUnorderedTypeInstance {
	FloydDTType _valueType;
};



/////////////////////////////////////////		TOrdered



template <typename V> class TOrdered {
	public: TOrdered(){
		ASSERT(CheckInvariant());
	}

	public: TOrdered(const std::vector<V>& v) :
		_vector(v)
	{
		ASSERT(CheckInvariant());
	}

	public: std::size_t Count() const {
		return _vector.size();
	}

	public: bool CheckInvariant() const{
		return true;
	}


	///////////////////////////////		Order

	public: const V& AtIndex(std::size_t idx) const {
		ASSERT(CheckInvariant());
		ASSERT(idx < _vector.size());

		return _vector[idx];
	}

	//### Interning?
	public: std::shared_ptr<const TOrdered<V> > Assoc(std::size_t idx, const V& value) const {
		ASSERT(CheckInvariant());
		ASSERT(idx <= _vector.size());

		//	Append just after end of vector?
		if(idx == _vector.size()){
			auto v = _vector;
			v.push_back(value);
			auto result = std::shared_ptr<const TOrdered<V> >(new TOrdered<V>(v));
			ASSERT(result->Count() == Count() + 1);
			return result;
		}

		//	Overwrite existing index.
		else{
			auto v = _vector;
			v[idx] = value;
			auto result = std::shared_ptr<const TOrdered<V> >(new TOrdered<V>(v));
			ASSERT(result->Count() == Count());
			return result;
		}
	}


	///////////////////////////////		State
	private: const std::vector<V> _vector;
};


template <typename V> TOrdered<V> Append(const TOrdered<V>& a, const TOrdered<V>& b){
	ASSERT(a.CheckInvariant());
	ASSERT(b.CheckInvariant());

	TOrdered<V> result = a + b;
	return result;
}


/////////////////////////////////////////		TUnordered


template <typename K, typename V> class TUnordered {
	public: TUnordered(){
		ASSERT(CheckInvariant());
	}

	public: TUnordered(const std::unordered_map<K, V>& v) :
		_map(v)
	{
		ASSERT(CheckInvariant());
	}

	public: std::size_t Count() const {
		return _map.size();
	}

	public: bool CheckInvariant() const{
		return true;
	}


	///////////////////////////////		Order

	public: V AtKey(const K& key) const {
		ASSERT(CheckInvariant());
		ASSERT(Exists(key));

		auto it = _map.find(key);
		ASSERT(it != _map.end());
		return it->second;
	}

	public: bool Exists(const K& key) const {
		ASSERT(CheckInvariant());

		const auto it = _map.find(key);
		return it != _map.end();
	}

	public: std::shared_ptr<const TUnordered<K, V> > Assoc(const K& key, const V& value) const {
		ASSERT(CheckInvariant());

		auto v = _map;
		v[key] = value;
		const auto result = std::shared_ptr<const TUnordered<K, V> >(new TUnordered<K, V>(v));
		return result;
	}


	///////////////////////////////		State
	private: const std::unordered_map<K, V> _map;
};




/////////////////////////////////////////		FloydDT

/*
	Make this 16 bytes big on 64-bit architectures.

	### Maybe possible to squeeze down to 8 bytes, if we can detect int64, float64 from 64bit word.
	### Make some rare bitpattern of int64/float64 actual cause them to use a second memory block?

	Depending on the type of value, the value is inline in FloydDT or interned.
*/


#if false

	/*
		0 -- 7
		8 bit: base-type
		---------------------
		0 = null [inlined value]
		5 = bool [inlined value]
		1 = float64 bit [inlined value]
		4 = int64 [inlined value]
		2 = string [inlined value] (1 + 6 bytes + 8)
		3 = string [interned ID]
		8 = function [custom types] [
		9 = enum
		10 = exception
		11 = composite
		12 = tuple
		13 = seq
		14 = ordered
		15 = tagged_union
		16 -- 255 = reserved

		8 -- 31		(256 -- 2^32) custom types = 16M types.

		32 -- 64	32bits A
	*/
	uint64_t _param0;
	uint64_t _param1;
#endif


struct FloydDT {
	public: bool CheckInvariant() const;
	public: std::string GetTypeString() const;


	///////////////////		Internals

	friend FloydDT MakeNull();
	friend bool IsNull(const FloydDT& value);

	friend FloydDT MakeFloat(float value);
	friend bool IsFloat(const FloydDT& value);
	friend float GetFloat(const FloydDT& value);

	friend FloydDT MakeString(const std::string& value);
	friend bool IsString(const FloydDT& value);
	friend std::string GetString(const FloydDT& value);

	friend FloydDT MakeFunction(const FunctionDef& f);
	friend bool IsFunction(const FloydDT& value);
	friend CFunctionPtr GetFunction(const FloydDT& value);
	friend const TFunctionSignature& GetFunctionSignature(const FloydDT& value);
	friend FloydDT CallFunction(const FloydDT& value, const std::vector<FloydDT>& args);

	friend FloydDT MakeComposite(const FloydBasicRuntime& runtime, int compositeTypeID);


	///////////////////		State
		private: FloydDTType _type = FloydDTType::kNull;
		private: float _asFloat = 0.0f;
		private: std::string _asString = "";

		private: std::shared_ptr<TCompositeValue> _asComposite;
		private: std::shared_ptr<FunctionDef> _asFunction;

		private: std::shared_ptr<TSeq<FloydDT>> _asSeq;
		private: std::shared_ptr<TOrdered<FloydDT>> _asOrdered;
		private: std::shared_ptr<TUnordered<std::string, FloydDT>> _asUnordered;
};





/////////////////////////////////////////		TCompositeValue




struct TCompositeValue {
	TStaticCompositeType* _type;

	//	Vector with all members, keyed on memeber name string.
	std::vector<std::pair<std::string, FloydDT> > _members;
};





/////////////////////////////////////////		TStaticCompositeType


/*
	Completely describes a type of composite. The type needs to be defined before program is run.
*/

struct TStaticCompositeType {

	//	Use 32-bit hash of signature string instead.
	int _id;

	//	Contains types and names of all members.
	TTypeSignatureString _signature;

	FloydDT _checkInvariant;
};





/////////////////////////////////////////		TStaticCompositeType


/*
	Tracks all static information, like types and typedefs etc.
	Must exist before you can run simulation.
*/


//### Rename to "Runtime". The other object shold be called "Model".
struct FloydBasicRuntime {
	public: FloydBasicRuntime();
	public: bool CheckInvariant() const;

	public: int DefineComposite(const std::string& signature, const FloydDT& checkInvariant);


	public: int SignatureToID(const TTypeSignatureString& s){
		const auto it = _staticCompositeTypes.find(s._s);
		return it == _staticCompositeTypes.end() ? -1 : it->second->_id;
	}

	public: const std::shared_ptr<TStaticCompositeType> LookupID(int id) const{
		for(const auto it: _staticCompositeTypes){
			if(it.second->_id == id){
				return it.second;
			}
		}
		return nullptr;
	}



	////////////////////////////		State

	//	### faster to key on ID.
	std::unordered_map<std::string, std::shared_ptr<TStaticCompositeType> > _staticCompositeTypes;
	int _idGenerator = 0;
};







FloydDT MakeComposite(const FloydBasicRuntime& runtime, int compositeTypeID);






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
const TFunctionSignature& GetFunctionSignature(const FloydDT& value);

//	Arguments must match those of the function or assert.
FloydDT CallFunction(const FloydDT& value, const std::vector<FloydDT>& args);




FloydDT MakeSeq();
FloydDT MakeOrdered();
FloydDT MakeUnordered();


#endif /* defined(__Floyd__FloydType__) */
