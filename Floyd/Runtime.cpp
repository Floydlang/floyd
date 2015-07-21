//
//  FloydType.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 09/07/15.
//  Copyright (c) 2015 Marcus Zetterquist. All rights reserved.
//

#include "cpp_extension.h"

#include "Runtime.h"

#include <string>


using namespace std;




	#if false
		### Idea how to encode Value-type efficently.
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






namespace {

	bool IsWellformedFunction(const std::string& s){
		if(s.length() > std::string("<>()").length()){
			return true;
		}
		else{
			return false;
		}
	}

	bool IsWellformedComposite(const std::string& s){
		return s.length() == 123344;
	}

	bool IsWellformedSignatureString(const std::string& s){
		if(s == "<null>"){
			return true;
		}
		else if(s == "<float>"){
			return true;
		}
		else if(s == "<string>"){
			return true;
		}
		else if(IsWellformedFunction(s)){
			return true;
		}
		else if(IsWellformedComposite(s)){
			return true;
		}
		else{
			return false;
		}
	}

}

#if false

TTypeSignatureString MakeSignature(const std::string& s){
	ASSERT(IsWellformedSignatureString(s));

	return TTypeSignatureString(s);
}
#endif


/*
TTypeSignatureHash HashSignature(const TTypeSignature& s){
	uint32_t badHash = 1234;

	for(auto c: s._s){
		badHash += c;
	}
	TTypeSignatureHash result;
	result._hash = badHash;
	return result;
}
*/



namespace {

/*	std::pair<std::string, std::string> GetTypeToken(const std::string s){
		
	}
*/

}

#if false
TTypeDefinition TypeSignatureFromString(const TTypeSignatureString& s){
	ASSERT(s.CheckInvariant());

	const auto s2 = s._s;
	if(s2 == "<null>"){
		return TTypeDefinition(EType::kNull);
	}
	else if(s2 == "<float>"){
		return TTypeDefinition(EType::kFloat);
	}
	else if(s2 == "<string>"){
		return TTypeDefinition(EType::kString);
	}
	else{
		if(s2[0] == '<'){
		}
	}
	return TTypeDefinition(EType::kNull);
}




UNIT_TEST("", "TypeSignatureFromString", "<null>", "kNull"){
	const auto a = TypeSignatureFromString(TTypeSignatureString("<null>"));
	UT_VERIFY(a._type == EType::kNull);
	UT_VERIFY(a._more.empty());
}

#endif






bool Floyd::Value::CheckInvariant() const{
	//	Use NAN for no-float?

	if(_type == EType::kNull){
		ASSERT(_asFloat == 0.0f);
		ASSERT(_asString == "");
		ASSERT(_asFunction.get() == nullptr);
		ASSERT(_asComposite.get() == nullptr);
		ASSERT(_asOrdered.get() == nullptr);
	}
	else if(_type == EType::kFloat){
//		ASSERT(_asFloat == 0.0f);
		ASSERT(_asString == "");
		ASSERT(_asFunction.get() == nullptr);
		ASSERT(_asComposite.get() == nullptr);
		ASSERT(_asOrdered.get() == nullptr);
	}
	else if(_type == EType::kString){
		ASSERT(_asFloat == 0.0f);
//		ASSERT(_asString == "");
		ASSERT(_asFunction.get() == nullptr);
		ASSERT(_asComposite.get() == nullptr);
		ASSERT(_asOrdered.get() == nullptr);
	}
	else if(_type == EType::kFunction){
		ASSERT(_asFloat == 0.0f);
		ASSERT(_asString == "");
		ASSERT(_asFunction.get() != nullptr);
		ASSERT(_asComposite.get() == nullptr);
		ASSERT(_asOrdered.get() == nullptr);

		ASSERT(_asFunction->_type != nullptr);
	}
	else if(_type == EType::kComposite){
		ASSERT(_asFloat == 0.0f);
		ASSERT(_asString == "");
		ASSERT(_asFunction.get() == nullptr);
		ASSERT(_asComposite.get() != nullptr);
		ASSERT(_asOrdered.get() == nullptr);

		ASSERT(_asComposite->_type != nullptr);
	}
	else if(_type == EType::kOrdered){
		ASSERT(_asFloat == 0.0f);
		ASSERT(_asString == "");
		ASSERT(_asFunction.get() == nullptr);
		ASSERT(_asComposite.get() == nullptr);
		ASSERT(_asOrdered.get() != nullptr);
	}
	else{
		ASSERT(false);
	}
	return true;
}


std::string Floyd::Value::GetTypeString() const{
	ASSERT(CheckInvariant());

	if(_type == kNull){
		return "<null>";
	}
	else if(_type == kFloat){
		return "<float>";
	}

	else if(_type == kString){
		return "<string>";
	}
	else if(_type == kFunction){
		return "<xxxx---function>";
	}
	else{
		ASSERT(false);
		return "???";
	}
}




Floyd::Value Floyd::MakeNull(){
	Value result;
	result._type = EType::kNull;

	ASSERT(result.CheckInvariant());
	return result;
}

bool Floyd::IsNull(const Value& value){
	ASSERT(value.CheckInvariant());

	return value._type == kNull;
}




Floyd::Value Floyd::MakeFloat(float value){
	Value result;
	result._type = EType::kFloat;
	result._asFloat = value;

	ASSERT(result.CheckInvariant());
	return result;
}

bool Floyd::IsFloat(const Value& value){
	ASSERT(value.CheckInvariant());

	return value._type == kFloat;
}

float Floyd::GetFloat(const Value& value){
	ASSERT(value.CheckInvariant());
	ASSERT(IsFloat(value));

	return value._asFloat;
}




Floyd::Value Floyd::MakeString(const std::string& value){
	Value result;
	result._type = EType::kString;
	result._asString = value;

	ASSERT(result.CheckInvariant());
	return result;
}


bool Floyd::IsString(const Value& value){
	ASSERT(value.CheckInvariant());

	return value._type == kString;
}

std::string Floyd::GetString(const Value& value){
	ASSERT(value.CheckInvariant());
	ASSERT(IsString(value));

	return value._asString;
}




Floyd::Value Floyd::MakeFunction(const Runtime& runtime, const TValueType& type){
	ASSERT(runtime.CheckInvariant());
	ASSERT(type.CheckInvariant());
	ASSERT(type._type == EType::kFunction);

	const auto t = runtime.LookupFunctionType(type);
	auto v = std::shared_ptr<TFunctionValue>(new TFunctionValue());
	v->_type = t;

	Value result;
	result._type = EType::kFunction;
	result._asFunction = v;

	ASSERT(result.CheckInvariant());
	return result;
}


bool Floyd::IsFunction(const Value& value){
	ASSERT(value.CheckInvariant());

	return value._type == kFunction;
}

Floyd::CFunctionPtr Floyd::GetFunction(const Value& value){
	ASSERT(value.CheckInvariant());
	ASSERT(IsFunction(value));

	return value._asFunction->_type->_f;
}

const Floyd::TTypeDefinition& Floyd::GetFunctionSignature(const Value& value){
	ASSERT(value.CheckInvariant());
	ASSERT(IsFunction(value));

	return value._asFunction->_type->_signature;
}

Floyd::Value Floyd::CallFunction(const Value& value, const std::vector<Value>& args){
	ASSERT(value.CheckInvariant());
#if ASSERT_ON
	for(auto a: args){
		ASSERT(a.CheckInvariant());
	}
	ASSERT(args.size() == value._asFunction->_signature._more.size() - 1);
#endif

	const TTypeDefinition& functionTypeSignature = value._asFunction->_type->_signature;

	std::vector<Value> argValues;
	int index = 0;
	for(auto i: args){
		//	Make sure the type of the argument is correct.
		auto a = i.GetType();
		auto b = functionTypeSignature._more[index + 1].second._type;
		ASSERT(a == b);

		argValues.push_back(i);

		index++;
	}

	// check that types match!

	Value dummy[1];
	Value functionResult = value._asFunction->_type->_f(
		argValues.empty() ? &dummy[0] : argValues.data(),
		argValues.size()
	);


	ASSERT(functionResult.GetType() == functionTypeSignature._more[0].second._type);

	return functionResult;
}



Floyd::Value Floyd::MakeDefaultValue(const Floyd::TValueType& type){
	if(type._type == EType::kNull){
		return MakeNull();
	}
	else if(type._type == EType::kFloat){
		return MakeFloat(0.0f);
	}
	else if(type._type == EType::kString){
		return MakeString("");
	}
	else{
		ASSERT(false);
	}
}





Floyd::Value Floyd::MakeComposite(const Runtime& runtime, const TValueType& type){
	ASSERT(runtime.CheckInvariant());
	ASSERT(type.CheckInvariant());
	ASSERT(type._type == EType::kComposite);

	const auto t = runtime.LookupCompositeType(type);
	auto v = std::shared_ptr<TCompositeValue>(new TCompositeValue());
	v->_type = t;
	for(const auto it: t->_signature._more){
		const auto& memberType = it.second;
		const Value defaultValue = MakeDefaultValue(memberType);
		v->_memberValues.push_back(
			std::pair<string, Value>(it.first, defaultValue)
		);
	}

	Value result;
	result._type = EType::kComposite;
	result._asComposite = v;

	ASSERT(result.CheckInvariant());
	return result;
}

bool Floyd::IsComposite(const Value& value){
	ASSERT(value.CheckInvariant());

	return value._type == Floyd::kComposite;
}

const Floyd::Value& Floyd::GetCompositeMember(const Value& composite, const std::string& member){
	ASSERT(IsComposite(composite));
	ASSERT(!member.empty());

std::shared_ptr<const TCompositeValue> temp = composite._asComposite;
/*
composite._asComposite->_memberValues[0].first = "x";
*/

	for(const auto& it: composite._asComposite->_memberValues){
		if(it.first == member){
			return it.second;
		}
	}

	//	Unknown member.
	throw std::runtime_error("");
}

const Floyd::Value Floyd::Assoc(const Value& composite, const std::string& member, const Value& newValue){
	ASSERT(IsComposite(composite));
	ASSERT(!member.empty());
	ASSERT(newValue.CheckInvariant());

	auto v = shared_ptr<TCompositeValue>(new TCompositeValue());
	{
		v->_type = composite._asComposite->_type;
		v->_memberValues = composite._asComposite->_memberValues;

		auto it = v->_memberValues.begin();
		while(it != v->_memberValues.end() && it->first != member){
			it++;
		}
		if(it == v->_memberValues.end()){
			//	Unknown member.
			throw std::runtime_error("");
		}
		it->second = newValue;
	}

	Value result;
	result._type = EType::kComposite;
	result._asComposite = v;

	ASSERT(result.CheckInvariant());
	return result;
}


namespace {

	Floyd::TValueType DefineNoteComposite(Floyd::Runtime& runtime){
		using namespace Floyd;
		auto type = TTypeDefinition(EType::kComposite);
		type._more.push_back(std::pair<std::string, TValueType>("note_number", EType::kFloat));
		type._more.push_back(std::pair<std::string, TValueType>("velocity", EType::kFloat));
		const TValueType result = runtime.DefineComposite(type, MakeNull());
		return result;
	}
}



UNIT_TEST("Runtime", "Composite", "BasicUsage", ""){
	using namespace Floyd;
	Runtime runtime;

	auto type = TTypeDefinition(EType::kComposite);
	type._more.push_back(std::pair<std::string, TValueType>("note_number", EType::kString));
	type._more.push_back(std::pair<std::string, TValueType>("velocity", EType::kString));

	const TValueType kNoteCompositeType = runtime.DefineComposite(type, MakeNull());
	const auto a = MakeComposite(runtime, kNoteCompositeType);

	UT_VERIFY(IsComposite(a));
}

UNIT_TEST("Composite", "GetCompositeMember", "Composite with two floats", "Has correct members"){
	using namespace Floyd;
	Runtime runtime;

	const auto noteType = DefineNoteComposite(runtime);
	const Value c = MakeComposite(runtime, noteType);

	const Value noteNumber = GetCompositeMember(c, "note_number");
	const Value velocity = GetCompositeMember(c, "velocity");

	UT_VERIFY(GetFloat(noteNumber) == 0.0f);
	UT_VERIFY(GetFloat(velocity) == 0.0f);
}


UNIT_TEST("Composite", "Assoc", "Replace member", "Read back cahange"){
	using namespace Floyd;
	Runtime runtime;

	const auto noteType = DefineNoteComposite(runtime);
	const Value c0 = MakeComposite(runtime, noteType);

	UT_VERIFY(GetFloat(GetCompositeMember(c0, "note_number")) == 0.0f);
	UT_VERIFY(GetFloat(GetCompositeMember(c0, "velocity")) == 0.0f);

	const Value c1 = Assoc(c0, "note_number", MakeFloat(3.14f));

	UT_VERIFY(GetFloat(GetCompositeMember(c0, "note_number")) == 0.0f);
	UT_VERIFY(GetFloat(GetCompositeMember(c0, "velocity")) == 0.0f);
	UT_VERIFY(GetFloat(GetCompositeMember(c1, "note_number")) == 3.14f);
	UT_VERIFY(GetFloat(GetCompositeMember(c1, "velocity")) == 0.0f);
}





/////////////////////////////////////////		Ordered




Floyd::Value Floyd::MakeOrdered(const Runtime& runtime, const TValueType& type){
	ASSERT(runtime.CheckInvariant());
	ASSERT(type.CheckInvariant());
	ASSERT(type._type == EType::kOrdered);

	const auto t = runtime.LookupOrderedType(type);
	auto v = std::shared_ptr<TOrderedValue>(new TOrderedValue());
	v->_type = t;

	Value result;
	result._type = EType::kOrdered;
	result._asOrdered = v;

	ASSERT(result.CheckInvariant());
	return result;
}

bool Floyd::IsOrdered(const Value& value){
	ASSERT(value.CheckInvariant());

	return value._type == Floyd::kOrdered;
}

const Floyd::Value& Floyd::GetOrderedMember(const Value& ordered, const std::string& member){
	ASSERT(IsOrdered(ordered));
	ASSERT(!member.empty());

	std::shared_ptr<const TOrderedValue> temp = ordered._asOrdered;

/*
	for(const auto& it: composite._asComposite->_memberValues){
		if(it.first == member){
			return it.second;
		}
	}
*/

	//	Unknown member.
	throw std::runtime_error("");
}




UNIT_TEST("Runtime", "DefineOrdered", "BasicUsage", ""){
	using namespace Floyd;
	Runtime runtime;

	auto type = TTypeDefinition(EType::kOrdered);
	type._more.push_back(std::pair<std::string, TValueType>("", EType::kFloat));

	const TValueType t = runtime.DefineOrdered(type);
	const auto a = MakeOrdered(runtime, t);

	UT_VERIFY(IsOrdered(a));
}




/////////////////////////////////////////		Runtime



Floyd::Runtime::Runtime(){
	ASSERT(CheckInvariant());
}

bool Floyd::Runtime::CheckInvariant() const{
	return true;
}


//??? Check for duplicates.
Floyd::TValueType Floyd::Runtime::DefineFunction(const TTypeDefinition& type, CFunctionPtr f){
	ASSERT(CheckInvariant());
	ASSERT(type.CheckInvariant());
	ASSERT(f != nullptr);

	const int id = _functionTypeIDGenerator++;

	auto b = std::shared_ptr<TStaticFunctionType>(new TStaticFunctionType());
	b->_signature = type;
	b->_f = f;
	_functionTypes[id] = b;

	ASSERT(CheckInvariant());

	TValueType result;
	result._type = EType::kFunction;
	result._customTypeID = id;

	return result;
}

const std::shared_ptr<Floyd::TStaticFunctionType> Floyd::Runtime::LookupFunctionType(const TValueType& type) const{
	ASSERT(type._type == EType::kFunction);

	const auto it = _functionTypes.find(type._customTypeID);
	ASSERT(it != _functionTypes.end());

	return it->second;
}






//??? Check for duplicates.
Floyd::TValueType Floyd::Runtime::DefineComposite(const TTypeDefinition& type, const Value& checkInvariant){
	ASSERT(CheckInvariant());
	ASSERT(type.CheckInvariant());

	const int id = _compositeTypeIDGenerator++;

	auto b = std::shared_ptr<TStaticCompositeType>(new TStaticCompositeType());
	b->_signature = type;
	b->_checkInvariant = checkInvariant;
	_compositeTypes[id] = b;

	ASSERT(CheckInvariant());

	TValueType result;
	result._type = EType::kComposite;
	result._customTypeID = id;

	return result;
}

const std::shared_ptr<Floyd::TStaticCompositeType> Floyd::Runtime::LookupCompositeType(const TValueType& type) const{
	ASSERT(type._type == EType::kComposite);

	const auto it = _compositeTypes.find(type._customTypeID);
	ASSERT(it != _compositeTypes.end());

	return it->second;
}








//??? Check for duplicates.
Floyd::TValueType Floyd::Runtime::DefineOrdered(const TTypeDefinition& type){
	ASSERT(CheckInvariant());
	ASSERT(type.CheckInvariant());

	const int id = _orderedTypeIDGenerator++;

	auto b = std::shared_ptr<TStaticOrderedType>(new TStaticOrderedType());
	b->_signature = type;
	_orderedTypes[id] = b;

	ASSERT(CheckInvariant());

	TValueType result;
	result._type = EType::kOrdered;
	result._customTypeID = id;

	return result;
}

const std::shared_ptr<Floyd::TStaticOrderedType> Floyd::Runtime::LookupOrderedType(const TValueType& type) const{
	ASSERT(type._type == EType::kOrdered);

	const auto it = _orderedTypes.find(type._customTypeID);
	ASSERT(it != _orderedTypes.end());

	return it->second;
}








/////////////////////////////////////////		Test functions



using namespace Floyd;


namespace {
	float ExampleFunction1(float a, float b, std::string s){
		return s == "*" ? a * b : a + b;
	}

	Value ExampleFunction1_Glue(const Value args[], std::size_t argCount){
		ASSERT(args != nullptr);
		ASSERT(argCount == 3);
		ASSERT(IsFloat(args[0]));
		ASSERT(IsFloat(args[1]));
		ASSERT(IsString(args[2]));

		const float a = GetFloat(args[0]);
		const float b = GetFloat(args[1]);
		const std::string s = GetString(args[2]);
		const float r = ExampleFunction1(a, b, s);

		Value result = MakeFloat(r);
		return result;
	}

	Value MakeFunction1(){
		TTypeDefinition typeDef(EType::kFunction);
		typeDef._more.push_back(std::pair<std::string, TValueType>("", EType::kFloat));
		typeDef._more.push_back(std::pair<std::string, TValueType>("a", EType::kFloat));
		typeDef._more.push_back(std::pair<std::string, TValueType>("b", EType::kFloat));
		typeDef._more.push_back(std::pair<std::string, TValueType>("c", EType::kString));

		Runtime runtime;
		const auto type = runtime.DefineFunction(typeDef, ExampleFunction1_Glue);

		const Value result = MakeFunction(runtime, type);
		return result;
	}

	void ProveWorks__MakeFunction__SimpleFunction__CorrectValue(){
		TTypeDefinition typeDef(EType::kFunction);
		typeDef._more.push_back(std::pair<std::string, TValueType>("", EType::kFloat));

		Runtime runtime;
		const auto type = runtime.DefineFunction(typeDef, ExampleFunction1_Glue);

		Value result = MakeFunction(runtime, type);
		UT_VERIFY(IsFunction(result));
		UT_VERIFY(GetFunction(result) == ExampleFunction1_Glue);
	}

	void ProveWorks__CallFunction__SimpleFunction__CorrectReturn(){
		const Value f = MakeFunction1();

		UT_VERIFY(IsFunction(f));
		UT_VERIFY(GetFunction(f) == ExampleFunction1_Glue);

		std::vector<Value> args;
		args.push_back(MakeFloat(2.0f));
		args.push_back(MakeFloat(3.0f));
		args.push_back(MakeString("*"));
		auto r = CallFunction(f, args);
		UT_VERIFY(IsFloat(r));
		UT_VERIFY(GetFloat(r) == 6.0f);
	}

}





/////////////////////////////////////////		Test TSeq




namespace {

	void ProveWorks__TSeq_DefaultConstructor__Basic__NoAssert(){
		auto a = TSeq<int>();
		UT_VERIFY(a.CheckInvariant());
	}

	TSeq<int> MakeTestSeq3(){
		const int kTest[] = {	88, 90, 100 };
		const auto a = TSeq<int>(std::vector<int>(&kTest[0], &kTest[3]));
		ASSERT(a.Count() == 3);

		const auto b = a.ToVector();
		ASSERT(b[0] == 88);
		ASSERT(b[1] == 90);
		ASSERT(b[2] == 100);

		return a;
	}

	void ProveWorks__TSeq_First__ThreeItems__8(){
		auto a = MakeTestSeq3();
		auto b = a.First();
		UT_VERIFY(b == 88);
	}

	void ProveWorks__TSeq_Rest__ThreeItems__TwoItems(){
		const auto a = MakeTestSeq3();
		const auto b = a.Rest();

		UT_VERIFY(b->Count() == 2);
		const auto c = b->ToVector();
		ASSERT(c[0] == 90);
		ASSERT(c[1] == 100);
	}

	void ProveWorks__TSeq_Rest__ReadAllItems__ResultIsEmpty(){
		const auto a = MakeTestSeq3();
		const auto b = a.Rest();
		const auto c = b->Rest();
		const auto d = c->Rest();
		UT_VERIFY(d->Count() == 0);
	}

}




/////////////////////////////////////////		Test TOrdered



namespace {

	void ProveWorks__TOrdered_DefaultConstructor__Basic__NoAssert(){
		auto a = TOrdered<int>();
		UT_VERIFY(a.CheckInvariant());
	}

	void ProveWorks__TOrdered_Assoc__OnEmptyCollection__OneItem(){
		auto a = TOrdered<int>();
		auto b = a.Assoc(0, 13);

		UT_VERIFY(a.Count() == 0);
		UT_VERIFY(b->Count() == 1);
		UT_VERIFY(b->AtIndex(0) == 13);
	}

	void ProveWorks__TOrdered_Assoc__AppendThree__ThreeItems(){
		auto a = TOrdered<int>();
		auto b = a.Assoc(0, 8);
		auto c = b->Assoc(1, 9);
		auto d = c->Assoc(2, 10);

		UT_VERIFY(a.Count() == 0);

		UT_VERIFY(b->Count() == 1);
		UT_VERIFY(b->AtIndex(0) == 8);

		UT_VERIFY(c->Count() == 2);
		UT_VERIFY(c->AtIndex(0) == 8);
		UT_VERIFY(c->AtIndex(1) == 9);

		UT_VERIFY(d->Count() == 3);
		UT_VERIFY(d->AtIndex(0) == 8);
		UT_VERIFY(d->AtIndex(1) == 9);
		UT_VERIFY(d->AtIndex(2) == 10);
	}

	std::shared_ptr<const TOrdered<int>> MakeTestOrdered3(){
		auto a = TOrdered<int>();
		auto b = a.Assoc(0, 8);
		b = b->Assoc(1, 9);
		b = b->Assoc(2, 10);
		return b;
	}

	void ProveWorks__TOrdered_Assoc__ReplaceItem__Correct(){
		auto a = MakeTestOrdered3();
		auto b = a->Assoc(1, 99);

		UT_VERIFY(a->Count() == 3);
		UT_VERIFY(a->AtIndex(1) == 9);
		UT_VERIFY(b->Count() == 3);
		UT_VERIFY(b->AtIndex(1)== 99);
	}

}



/////////////////////////////////////////		Test TUnordered



namespace {

	void ProveWorks__TUnordered_DefaultConstructor__Basic__NoAssert(){
		auto a = TUnordered<string, int>();
		UT_VERIFY(a.CheckInvariant());
	}

	void ProveWorks__TUnordered_Assoc__OnEmptyCollection__OneItem(){
		auto a = TUnordered<string, int>();
		auto b = a.Assoc("three", 13);

		UT_VERIFY(a.Count() == 0);
		UT_VERIFY(b->Count() == 1);
		UT_VERIFY(b->AtKey("three") == 13);
	}

	void ProveWorks__TUnordered_Assoc__AppendThree__ThreeItems(){
		auto a = TUnordered<string, int>();
		auto b = a.Assoc("zero", 8);
		auto c = b->Assoc("one", 9);
		auto d = c->Assoc("two", 10);

		UT_VERIFY(a.Count() == 0);

		UT_VERIFY(b->Count() == 1);
		UT_VERIFY(b->AtKey("zero") == 8);

		UT_VERIFY(c->Count() == 2);
		UT_VERIFY(c->AtKey("zero") == 8);
		UT_VERIFY(c->AtKey("one") == 9);

		UT_VERIFY(d->Count() == 3);
		UT_VERIFY(d->AtKey("zero") == 8);
		UT_VERIFY(d->AtKey("one") == 9);
		UT_VERIFY(d->AtKey("two") == 10);
	}

	std::shared_ptr<const TUnordered<string, int>> MakeTestUnordered3(){
		auto a = TUnordered<string, int>();
		auto b = a.Assoc("zero", 8);
		b = b->Assoc("one", 9);
		b = b->Assoc("two", 10);
		return b;
	}

	void ProveWorks__TUnordered_Assoc__ReplaceItem__Correct(){
		auto a = MakeTestUnordered3();
		auto b = a->Assoc("one", 99);

		UT_VERIFY(a->Count() == 3);
		UT_VERIFY(a->AtKey("one") == 9);
		UT_VERIFY(b->Count() == 3);
		UT_VERIFY(b->AtKey("one")== 99);
	}
}



UNIT_TEST("FloydType", "RunFloydTypeTests()", "", ""){
	ProveWorks__MakeFunction__SimpleFunction__CorrectValue();
	ProveWorks__CallFunction__SimpleFunction__CorrectReturn();

	ProveWorks__TSeq_DefaultConstructor__Basic__NoAssert();
	ProveWorks__TSeq_First__ThreeItems__8();
	ProveWorks__TSeq_Rest__ThreeItems__TwoItems();
	ProveWorks__TSeq_Rest__ReadAllItems__ResultIsEmpty();

	ProveWorks__TOrdered_DefaultConstructor__Basic__NoAssert();
	ProveWorks__TOrdered_Assoc__OnEmptyCollection__OneItem();
	ProveWorks__TOrdered_Assoc__AppendThree__ThreeItems();
	ProveWorks__TOrdered_Assoc__ReplaceItem__Correct();

	ProveWorks__TUnordered_DefaultConstructor__Basic__NoAssert();
	ProveWorks__TUnordered_Assoc__OnEmptyCollection__OneItem();
	ProveWorks__TUnordered_Assoc__AppendThree__ThreeItems();
	ProveWorks__TUnordered_Assoc__ReplaceItem__Correct();
}

