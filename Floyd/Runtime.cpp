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






bool Floyd::FloydDT::CheckInvariant() const{
	//	Use NAN for no-float?

	if(_type == EType::kNull){
		ASSERT(_asFloat == 0.0f);
		ASSERT(_asString == "");
		ASSERT(_asFunction.get() == nullptr);
		ASSERT(_asComposite.get() == nullptr);
	}
	else if(_type == EType::kFloat){
//		ASSERT(_asFloat == 0.0f);
		ASSERT(_asString == "");
		ASSERT(_asFunction.get() == nullptr);
		ASSERT(_asComposite.get() == nullptr);
	}
	else if(_type == EType::kString){
		ASSERT(_asFloat == 0.0f);
//		ASSERT(_asString == "");
		ASSERT(_asFunction.get() == nullptr);
		ASSERT(_asComposite.get() == nullptr);
	}
	else if(_type == EType::kFunction){
		ASSERT(_asFloat == 0.0f);
		ASSERT(_asString == "");
		ASSERT(_asFunction.get() != nullptr);
//		ASSERT(_asFunction->_signature == 3);
		ASSERT(_asFunction->_functionPtr != nullptr);

		ASSERT(_asComposite.get() == nullptr);
	}
	else{
		ASSERT(false);
	}
	return true;
}


std::string Floyd::FloydDT::GetTypeString() const{
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




Floyd::FloydDT Floyd::MakeNull(){
	FloydDT result;
	result._type = EType::kNull;

	ASSERT(result.CheckInvariant());
	return result;
}

bool Floyd::IsNull(const FloydDT& value){
	ASSERT(value.CheckInvariant());

	return value._type == kNull;
}




Floyd::FloydDT Floyd::MakeFloat(float value){
	FloydDT result;
	result._type = EType::kFloat;
	result._asFloat = value;

	ASSERT(result.CheckInvariant());
	return result;
}

bool Floyd::IsFloat(const FloydDT& value){
	ASSERT(value.CheckInvariant());

	return value._type == kFloat;
}

float Floyd::GetFloat(const FloydDT& value){
	ASSERT(value.CheckInvariant());
	ASSERT(IsFloat(value));

	return value._asFloat;
}




Floyd::FloydDT Floyd::MakeString(const std::string& value){
	FloydDT result;
	result._type = EType::kString;
	result._asString = value;

	ASSERT(result.CheckInvariant());
	return result;
}


bool Floyd::IsString(const FloydDT& value){
	ASSERT(value.CheckInvariant());

	return value._type == kString;
}

std::string Floyd::GetString(const FloydDT& value){
	ASSERT(value.CheckInvariant());
	ASSERT(IsString(value));

	return value._asString;
}




Floyd::FloydDT Floyd::MakeFunction(const FunctionDef& f){
	FloydDT result;
	result._type = EType::kFunction;
	result._asFunction = std::shared_ptr<FunctionDef>(new FunctionDef(f));

	ASSERT(result.CheckInvariant());
	return result;
}


bool Floyd::IsFunction(const FloydDT& value){
	ASSERT(value.CheckInvariant());

	return value._type == kFunction;
}

Floyd::CFunctionPtr Floyd::GetFunction(const FloydDT& value){
	ASSERT(value.CheckInvariant());
	ASSERT(IsFunction(value));

	return value._asFunction->_functionPtr;
}

const Floyd::TTypeDefinition& Floyd::GetFunctionSignature(const FloydDT& value){
	ASSERT(value.CheckInvariant());
	ASSERT(IsFunction(value));

	return value._asFunction->_signature;
}

Floyd::FloydDT Floyd::CallFunction(const FloydDT& value, const std::vector<FloydDT>& args){
	ASSERT(value.CheckInvariant());
#if ASSERT_ON
	for(auto a: args){
		ASSERT(a.CheckInvariant());
	}
	ASSERT(args.size() == value._asFunction->_signature._more.size() - 1);
#endif

	const TTypeDefinition& functionTypeSignature = value._asFunction->_signature;

	std::vector<FloydDT> argValues;
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

	FloydDT dummy[1];
	FloydDT functionResult = value._asFunction->_functionPtr(
		argValues.empty() ? &dummy[0] : argValues.data(),
		argValues.size()
	);


	ASSERT(functionResult.GetType() == functionTypeSignature._more[0].second._type);

	return functionResult;
}






Floyd::FloydDT Floyd::MakeComposite(const Runtime& runtime, const TValueType& type){
	const auto def = runtime.LookupCompositeType(type);
	ASSERT(def != nullptr);

	auto c = shared_ptr<TCompositeValue>(new TCompositeValue);
//	c->_def = def;
//???
//	for(const auto m: def->_)
//	c->_members.

	FloydDT result;
	result._type = EType::kComposite;
	result._asComposite = c;
	return result;

}



/////////////////////////////////////////		Runtime



Floyd::Runtime::Runtime(){
	ASSERT(CheckInvariant());
}

bool Floyd::Runtime::CheckInvariant() const{
	return true;
}


Floyd::TValueType Floyd::Runtime::DefineComposite(const std::string& signature, const TTypeDefinition& type, const FloydDT& checkInvariant){
	ASSERT(CheckInvariant());

//	const TTypeSignatureString s(signature);
	const int id = _idGenerator++;

	auto b = std::shared_ptr<TStaticCompositeType>(new TStaticCompositeType());
	b->_id = id;
	b->_signature = type;
	b->_checkInvariant = checkInvariant;
	_staticCompositeTypes[signature] = b;

	ASSERT(CheckInvariant());

	TValueType result;
	result._type = EType::kComposite;
	result._customTypeID = id;

	return result;
}









using namespace Floyd;




/////////////////////////////////////////		Test functions


namespace {
	float ExampleFunction1(float a, float b, std::string s){
		return s == "*" ? a * b : a + b;
	}

	FloydDT ExampleFunction1_Glue(const FloydDT args[], std::size_t argCount){
		ASSERT(args != nullptr);
		ASSERT(argCount == 3);
		ASSERT(IsFloat(args[0]));
		ASSERT(IsFloat(args[1]));
		ASSERT(IsString(args[2]));

		const float a = GetFloat(args[0]);
		const float b = GetFloat(args[1]);
		const std::string s = GetString(args[2]);
		const float r = ExampleFunction1(a, b, s);

		FloydDT result = MakeFloat(r);
		return result;
	}

	FloydDT MakeFunction1(){
		TTypeDefinition type(EType::kFunction);
		type._more.push_back(std::pair<std::string, TValueType>("", EType::kFloat));

		type._more.push_back(std::pair<std::string, TValueType>("a", EType::kFloat));
		type._more.push_back(std::pair<std::string, TValueType>("b", EType::kFloat));
		type._more.push_back(std::pair<std::string, TValueType>("c", EType::kString));

		FunctionDef def;
		def._functionPtr = ExampleFunction1_Glue;
		def._signature = type;

		const FloydDT result = MakeFunction(def);
		return result;
	}

	void ProveWorks__MakeFunction__SimpleFunction__CorrectFloydDT(){
		TTypeDefinition type(EType::kFunction);
		type._more.push_back(std::pair<std::string, TValueType>("", EType::kFloat));

		FunctionDef def;
		def._functionPtr = ExampleFunction1_Glue;
		def._signature = type;
		FloydDT result = MakeFunction(def);
		UT_VERIFY(IsFunction(result));
		UT_VERIFY(GetFunction(result) == ExampleFunction1_Glue);
	}

	void ProveWorks__CallFunction__SimpleFunction__CorrectReturn(){
		const FloydDT f = MakeFunction1();

		UT_VERIFY(IsFunction(f));
		UT_VERIFY(GetFunction(f) == ExampleFunction1_Glue);

		std::vector<FloydDT> args;
		args.push_back(MakeFloat(2.0f));
		args.push_back(MakeFloat(3.0f));
		args.push_back(MakeString("*"));
		auto r = CallFunction(f, args);
		UT_VERIFY(IsFloat(r));
		UT_VERIFY(GetFloat(r) == 6.0f);
	}

}


#if true
UNIT_TEST("Runtime", "Composite", "BasicUsage", ""){
	Runtime basic;

	auto type = TTypeDefinition(EType::kComposite);
	type._more.push_back(std::pair<std::string, TValueType>("note_number", EType::kString));
	type._more.push_back(std::pair<std::string, TValueType>("velocity", EType::kString));

	const TValueType kNoteCompositeType = basic.DefineComposite("{ <string>, <string> }", type, MakeNull());
	const auto a = MakeComposite(basic, kNoteCompositeType);
}
#endif



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
	ProveWorks__MakeFunction__SimpleFunction__CorrectFloydDT();
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

