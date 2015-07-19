//
//  FloydType.h
//  Floyd
//
//  Created by Marcus Zetterquist on 09/07/15.
//  Copyright (c) 2015 Marcus Zetterquist. All rights reserved.

/*
	Floyd's types and containers and the runtime to support them.
*/


#ifndef __Floyd__FloydType__
#define __Floyd__FloydType__

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>


namespace Floyd {

	struct FloydDT;
	struct TValueType;
	struct TCompositeValue;
	struct Runtime;
	struct TStaticCompositeType;


	/////////////////////////////////////////		EType
	
	/*
		All base types in Floyd. Basics like "float" and "string", but also the base type for custom composites etc.
		The order in the enum is the *normalized* order of the types through out the system.
	*/


	enum EType {
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
		String version of TTypeDefinition


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



	/////////////////////////////////////////		TTypeDefinition

	/*
		Defines a *custom* Floyd type.
		You can define composites, containers holding specify data types.

		Not used to define built-in basics, like "float".
	*/


	struct TTypeDefinition {
		TTypeDefinition(){
		}

		TTypeDefinition(EType type) :
			_type(type)
		{
		}


		EType _type = EType::kNull;

		//	Pairs of member-name + member-type.
		std::vector<std::pair<std::string, TValueType> > _more;
	};




	/////////////////////////////////////////		Function

	/*
		Support for custom function-types.
	*/

	const int kMaxFunctionArgs = 6;

	typedef FloydDT (*CFunctionPtr)(const FloydDT args[], std::size_t argCount);

	//	Function signature = string in format
	//		FloydType
	//	### Make optimized calling signatures with different sets of C arguments.


	struct FunctionDef {
		TTypeDefinition _signature;
		CFunctionPtr _functionPtr;
	};




	/////////////////////////////////////////		TSeq - Collection


	/*
		Support for custom seq-collections.
	*/

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
		EType _valueType;
	};

	struct TOrderedTypeInstance {
		EType _valueType;
	};

	struct TUnorderedTypeInstance {
		EType _valueType;
	};



	/////////////////////////////////////////		TOrdered - Collection

	/*
		Support for custom ordered-collections.
	*/

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



	/////////////////////////////////////////		TUnordered - Collection

	/*
		Support for custom unordered collections.
	*/

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




	/////////////////////////////////////////		Value

	/*
		This a dynamic value type that can hold any types of values in Floyd's simulation.
		This C++ type is *never* exposed to client programs, only used internally by Floyd.

		All values are immutable.

		In the floyd simulation, values are static, but in the host they are dynamic.

		Also holds every new data type, like custom composites.

		Depending on the type of value, the value is inline in the Value object itself or interned.



		### Attempt to make this struct 16 bytes big on 64-bit architectures.

		### Maybe possible to squeeze down to 8 bytes, if we can detect int64, float64 from 64bit word.
		### Make some rare bitpattern of int64/float64 actual cause them to use a second memory block?
	*/

	struct FloydDT {
		public: bool CheckInvariant() const;
		public: EType GetType() const{
			return _type;
		}
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
		friend const TTypeDefinition& GetFunctionSignature(const FloydDT& value);
		friend FloydDT CallFunction(const FloydDT& value, const std::vector<FloydDT>& args);

		friend FloydDT MakeComposite(const Runtime& runtime, const TValueType& type);


		///////////////////		State
			private: EType _type = EType::kNull;
			private: float _asFloat = 0.0f;
			private: std::string _asString = "";

			private: std::shared_ptr<TCompositeValue> _asComposite;
			private: std::shared_ptr<FunctionDef> _asFunction;

			private: std::shared_ptr<TSeq<FloydDT>> _asSeq;
			private: std::shared_ptr<TOrdered<FloydDT>> _asOrdered;
			private: std::shared_ptr<TUnordered<std::string, FloydDT>> _asUnordered;
	};




	/////////////////////////////////////////		TValueType

	/*
		Specifies the exact type of a value, even custom types.
		References state in the Runtime - depends implicitly on the Runtime-object.
	*/

	struct TValueType {
		TValueType(){
		}

		TValueType(EType basicType) :
			_type(basicType)
		{
		}


		EType _type = EType::kNull;
		int _customTypeID = -1;
	};




	/////////////////////////////////////////		TCompositeValue

	/*
		Holds the value of a custom composite.
	*/

	struct TCompositeValue {
		TStaticCompositeType* _type;

		//	Vector with all members, keyed on memeber name string.
		std::vector<std::pair<std::string, FloydDT> > _members;
	};

	FloydDT MakeComposite(const Runtime& runtime, const TValueType& type);




	/////////////////////////////////////////		TStaticCompositeType


	/*
		Completely describes a custom composite-type. The type needs to be defined before program is run.
	*/

	struct TStaticCompositeType {

		//	Use 32-bit hash of signature string instead.
		int _id;

		//	Contains types and names of all members.
		TTypeDefinition _signature;

		FloydDT _checkInvariant;
	};




	/////////////////////////////////////////		Runtime


	/*
		Tracks all static information, like types and typedefs etc.
		Must exist before you can run simulation.
	*/


	//### Rename to "Runtime". The other object shold be called "Model".
	struct Runtime {
		public: Runtime();
		public: bool CheckInvariant() const;

		public: TValueType DefineComposite(const std::string& signature, const TTypeDefinition& type, const FloydDT& checkInvariant);

#if false
		public: int SignatureToID(const TTypeSignatureSpec& s){
			//		const auto it = _staticCompositeTypes.find(s._s);
			//		return it == _staticCompositeTypes.end() ? -1 : it->second->_id;
			return 666;
		}
#endif

		public: const std::shared_ptr<TStaticCompositeType> LookupCompositeType(const TValueType& type) const{
			ASSERT(type._type == EType::kComposite);

			for(const auto it: _staticCompositeTypes){
				if(it.second->_id == type._customTypeID){
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
	const TTypeDefinition& GetFunctionSignature(const FloydDT& value);

	//	Arguments must match those of the function or assert.
	FloydDT CallFunction(const FloydDT& value, const std::vector<FloydDT>& args);



	FloydDT MakeSeq();
	FloydDT MakeOrdered();
	FloydDT MakeUnordered();

}

#endif /* defined(__Floyd__FloydType__) */
