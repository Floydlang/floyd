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

	struct Value;
	struct TValueType;

	struct IFunctionContext;
	struct TFunctionValue;
	struct TStaticFunctionType;

	struct TCompositeValue;
	struct TStaticCompositeType;

	struct TOrderedValue;
	struct TStaticOrderedType;

	struct Runtime;


	/////////////////////////////////////////		Functions

	/*
		Support for custom function-types.
	*/

	const int kMaxFunctionArgs = 6;

	typedef Value (*CFunctionPtr)(const IFunctionContext& context, const Value args[], std::size_t argCount);
	typedef Value (*TooboxFunctionPtr)(const IFunctionContext& context, const Value args[], std::size_t argCount);


	struct IFunctionContext {
		virtual ~IFunctionContext(){};

		virtual void* IFunctionContext_GetToolbox(uint32_t toolboxMagic) = 0;
		virtual TooboxFunctionPtr IFunctionContext_GetFunction(const std::string& functionName) = 0;


		virtual Runtime& IFunctionContext_GetRuntime() = 0;
	};




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
		kComposite,

		kOrdered
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

		public: bool CheckInvariant() const{
			return true;
		}


		///////////////////////////////		State
			EType _type = EType::kNull;

			//	Pairs of member-name + member-type.
			std::vector<std::pair<std::string, TValueType> > _more;
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

	struct Value {
		public: bool CheckInvariant() const;
		public: EType GetType() const;
		public: TValueType GetValueType() const;
		public: std::string GetTypeString() const;


		///////////////////		Internals

		friend Value MakeNull();
		friend bool IsNull(const Value& value);

		friend Value MakeFloat(float value);
		friend bool IsFloat(const Value& value);
		friend float GetFloat(const Value& value);

		friend Value MakeString(const std::string& value);
		friend bool IsString(const Value& value);
		friend std::string GetString(const Value& value);

		friend Value MakeFunction(const Runtime& runtime, const TValueType& type);
		friend bool IsFunction(const Value& value);
		friend CFunctionPtr GetFunction(const Value& value);
		friend const TTypeDefinition& GetFunctionSignature(const Value& value);
		friend Value CallFunction(std::shared_ptr<Floyd::Runtime> runtime, const Value& value, const std::vector<Value>& args);

		friend Value MakeComposite(const Runtime& runtime, const TValueType& type);
		friend bool IsComposite(const Value& value);
		friend const Value& GetCompositeMember(const Value& composite, const std::string& member);
		friend const Value Assoc(const Value& composite, const std::string& member, const Value& newValue);

		friend Value MakeOrdered(const Runtime& runtime, const TValueType& type);
		friend bool IsOrdered(const Value& value);
		friend const Value& GetOrderedMember(const Value& ordered, const std::string& member);


		///////////////////		State
			private: EType _type = EType::kNull;
			private: float _asFloat = 0.0f;
			private: std::string _asString = "";

			private: std::shared_ptr<const TCompositeValue> _asComposite;
			private: std::shared_ptr<const TFunctionValue> _asFunction;

//			private: std::shared_ptr<const TSeq<Value>> _asSeq;
			private: std::shared_ptr<const TOrderedValue> _asOrdered;
//			private: std::shared_ptr<const TUnordered<std::string, Value>> _asUnordered;
	};




	/////////////////////////////////////////		TValueType

	/*
		Specifies the exact type of a value, even custom types.
		References state in the Runtime - depends implicitly on the Runtime-object.

		### put pts to TStaticCompositeType etc in here, not ID. Fast without lookup.
	*/

	struct TValueType {
		TValueType(){
		}

		TValueType(EType basicType) :
			_type(basicType)
		{
		}

		TValueType(EType refType, int customTypeID) :
			_type(refType),
			_customTypeID(customTypeID)
		{
		}

		public: bool CheckInvariant() const{
			return true;
		}

		public: TValueType& operator=(const TValueType& other) = default;
		public: bool operator==(const TValueType& other){
			return _type == other._type && _customTypeID == other._customTypeID;
		}


		///////////////////		State
		EType _type = EType::kNull;
		int _customTypeID = -1;
	};





	/////////////////////////////////////////		TFunctionValue

	/*
		Holds the value of a custom function.

		??? Something is weird here. The name + types of a function makes up its *value*.
	*/

	struct TFunctionValue {
		std::shared_ptr<TStaticFunctionType> _type;

		//	Vector with all members, keyed on memeber name string.
//		std::vector<std::pair<std::string, Value> > _members;
	};



	/////////////////////////////////////////		TStaticFunctionType


	struct TStaticFunctionType {
		TTypeDefinition _signature;
		CFunctionPtr _f = nullptr;
		int _id = 0;
	};





	/////////////////////////////////////////		TCompositeValue

	/*
		Holds the value of a custom composite.
	*/

	struct TCompositeValue {
		std::shared_ptr<TStaticCompositeType> _type;

		//	Vector with all members, keyed on memeber name string.
		std::vector<std::pair<std::string, Value> > _memberValues;
	};



	/////////////////////////////////////////		TStaticCompositeType


	/*
		Completely describes a custom composite-type. The type needs to be defined before program is run.
	*/

	struct TStaticCompositeType {

		//	Use 32-bit hash of signature string instead.
//		int _id;

		//	Contains types and names of all members.
		TTypeDefinition _signature;

		Value _checkInvariant;
		int _id = 0;
	};



	/////////////////////////////////////////		TOrderedValue


	struct TOrderedValue {
		std::shared_ptr<TStaticOrderedType> _type;
		std::vector<Value> _values;
	};


	/////////////////////////////////////////		TStaticOrderedType


	struct TStaticOrderedType {
		//	Use 32-bit hash of signature string instead.
//		int _id;

		//	Contains types and names of all members.
		TTypeDefinition _signature;
		int _id = 0;
	};







	const int kNoStaticTypeID = -1;


	/////////////////////////////////////////		Runtime


	/*
		Tracks all static information, like types and typedefs etc.
		Must exist before you can run simulation.
	*/


	//### Rename to "Runtime". The other object shold be called "Model".
	//### Make a special phase where you can define statics *then* construct the Runtime.
	struct Runtime {
		public: Runtime();
		public: bool CheckInvariant() const;

		public: TValueType DefineFunction(const TTypeDefinition& type, CFunctionPtr f);
		public: const std::shared_ptr<TStaticFunctionType> LookupFunctionType(const TValueType& type) const;

		public: TooboxFunctionPtr LookupFunction(const std::string& functionName);


		public: TValueType DefineComposite(const TTypeDefinition& type, const Value& checkInvariant);
		public: const std::shared_ptr<TStaticCompositeType> LookupCompositeType(const TValueType& type) const;


		public: TValueType DefineOrdered(const TTypeDefinition& type);
		public: const std::shared_ptr<TStaticOrderedType> LookupOrderedType(const TValueType& type) const;



		////////////////////////////		State

		private: int _functionTypeIDGenerator = 10000;
		private: std::unordered_map<int, std::shared_ptr<TStaticFunctionType> > _functionTypes;

		private: std::unordered_map<int, std::shared_ptr<TStaticCompositeType> > _compositeTypes;
		private: int _compositeTypeIDGenerator = 20000;

		private: std::unordered_map<int, std::shared_ptr<TStaticOrderedType> > _orderedTypes;
		private: int _orderedTypeIDGenerator = 30000;
	};


	std::shared_ptr<Runtime> MakeRuntime();



	/////////////////////////////////////////		Basic types

	Value MakeDefaultValue(const TValueType& type);


	Value MakeNull();
	bool IsNull(const Value& value);


	Value MakeFloat(float value);
	bool IsFloat(const Value& value);
	float GetFloat(const Value& value);


	Value MakeString(const std::string& value);
	bool IsString(const Value& value);
	std::string GetString(const Value& value);



	/////////////////////////////////////////		Function


	Value MakeFunction(const Runtime& runtime, const TValueType& type);
	bool IsFunction(const Value& value);
	CFunctionPtr GetFunction(const Value& value);
	const TTypeDefinition& GetFunctionSignature(const Value& value);

	//	Arguments must match those of the function or assert.
	Value CallFunction(std::shared_ptr<Floyd::Runtime> runtime, const Value& value, const std::vector<Value>& args);


	/////////////////////////////////////////		Composite


	Value MakeComposite(const Runtime& runtime, const TValueType& type);
	bool IsComposite(const Value& value);
	const Value& GetCompositeMember(const Value& composite, const std::string& member);
	const Value Assoc(const Value& composite, const std::string& member, const Value& newValue);

//	const TValueType& GetCompositeType(const Value& value);
//	const TTypeDefinition& GetCompositeDef(const TValueType& type);


	Value MakeOrdered(const Runtime& runtime, const TValueType& type);
	bool IsOrdered(const Value& value);
	const Value& GetOrderedMember(const Value& ordered, const std::string& member);
}

#endif /* defined(__Floyd__FloydType__) */
