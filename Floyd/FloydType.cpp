//
//  FloydType.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 09/07/15.
//  Copyright (c) 2015 Marcus Zetterquist. All rights reserved.
//

#include "Quark.h"

#include "FloydType.h"

#include <string>





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


TTypeSignature MakeSignature(const std::string& s){
	ASSERT(IsWellformedSignatureString(s));

	TTypeSignature result;
	result._s = s;
	return result;
}

TTypeSignatureHash HashSignature(const TTypeSignature& s){
	uint32_t badHash = 1234;

	for(auto c: s._s){
		badHash += c;
	}
	TTypeSignatureHash result;
	result._hash = badHash;
	return result;
}


/*
std::vector<TTypeSignature> UnpackCompositeSignature(const TTypeSignature& s){
}
*/



bool FloydDT::CheckInvariant() const{
	//	Use NAN for no-float?

	if(_type == FloydDTType::kNull){
		ASSERT(_asFloat == 0.0f);
		ASSERT(_asString == "");
		ASSERT(_asFunction.get() == nullptr);
		ASSERT(_asComposite.get() == nullptr);
	}
	else if(_type == FloydDTType::kFloat){
//		ASSERT(_asFloat == 0.0f);
		ASSERT(_asString == "");
		ASSERT(_asFunction.get() == nullptr);
		ASSERT(_asComposite.get() == nullptr);
	}
	else if(_type == FloydDTType::kString){
		ASSERT(_asFloat == 0.0f);
//		ASSERT(_asString == "");
		ASSERT(_asFunction.get() == nullptr);
		ASSERT(_asComposite.get() == nullptr);
	}
	else if(_type == FloydDTType::kFunction){
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

std::string FloydDT::GetType() const{
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




FloydDT MakeNull(){
	FloydDT result;
	result._type = FloydDTType::kNull;

	ASSERT(result.CheckInvariant());
	return result;
}

bool IsNull(const FloydDT& value){
	ASSERT(value.CheckInvariant());

	return value._type == kNull;
}




FloydDT MakeFloat(float value){
	FloydDT result;
	result._type = FloydDTType::kFloat;
	result._asFloat = value;

	ASSERT(result.CheckInvariant());
	return result;
}

bool IsFloat(const FloydDT& value){
	ASSERT(value.CheckInvariant());

	return value._type == kFloat;
}

float GetFloat(const FloydDT& value){
	ASSERT(value.CheckInvariant());
	ASSERT(IsFloat(value));

	return value._asFloat;
}




FloydDT MakeString(const std::string& value){
	FloydDT result;
	result._type = FloydDTType::kString;
	result._asString = value;

	ASSERT(result.CheckInvariant());
	return result;
}


bool IsString(const FloydDT& value){
	ASSERT(value.CheckInvariant());

	return value._type == kString;
}

std::string GetString(const FloydDT& value){
	ASSERT(value.CheckInvariant());
	ASSERT(IsString(value));

	return value._asString;
}




FloydDT MakeFunction(const FunctionDef& f){
	FloydDT result;
	result._type = FloydDTType::kFunction;
	result._asFunction = std::shared_ptr<FunctionDef>(new FunctionDef(f));

	ASSERT(result.CheckInvariant());
	return result;
}


bool IsFunction(const FloydDT& value){
	ASSERT(value.CheckInvariant());

	return value._type == kFunction;
}

CFunctionPtr GetFunction(const FloydDT& value){
	ASSERT(value.CheckInvariant());
	ASSERT(IsFunction(value));

	return value._asFunction->_functionPtr;
}

FloydDT CallFunction(const FloydDT& value, const std::vector<FloydDT>& args){
	ASSERT(value.CheckInvariant());
#if ASSERT_ON
	for(auto a: args){
		ASSERT(a.CheckInvariant());
	}
#endif

	const TFunctionSignature& functionTypeSignature = value._asFunction->_signature;

	std::vector<FloydDT> argValues;
	int index = 0;
	for(auto i: args){
		//	Make sure the type of the argument is correct.
		auto a = i.GetType();
		auto b = functionTypeSignature._args[index].second._s;
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


	ASSERT(functionResult.GetType() == functionTypeSignature._returnType._s);

	return functionResult;
}




namespace {

	float ExampleFunction1(float a, float b, std::string s){
		return s == "*" ? a * b : a + b;
	}

	FloydDT ExampleFunction1_Glue(const FloydDT args[], std::size_t argCount){
		ASSERT(args != nullptr);
		ASSERT(argCount == 3);
		ASSERT(args[0]._type == FloydDTType::kFloat);
		ASSERT(args[1]._type == FloydDTType::kFloat);
		ASSERT(args[2]._type == FloydDTType::kString);

		const float a = args[0]._asFloat;
		const float b = args[1]._asFloat;
		const std::string s = args[2]._asString;
		const float r = ExampleFunction1(a, b, s);

		FloydDT result = MakeFloat(r);
		return result;
	}

	FloydDT MakeFunction1(){
		TFunctionSignature signature;
		signature._args.push_back(
			std::pair<std::string, TTypeSignature>("a", MakeSignature("<float>"))
		);
		signature._args.push_back(
			std::pair<std::string, TTypeSignature>("b", MakeSignature("<float>"))
		);
		signature._args.push_back(
			std::pair<std::string, TTypeSignature>("s", MakeSignature("<string>"))
		);

		signature._returnType = MakeSignature("<float>");

		FunctionDef def;
		def._functionPtr = ExampleFunction1_Glue;
		def._signature = signature;

		const FloydDT result = MakeFunction(def);
		return result;
	}



	void ProveWorks__MakeFunction__SimpleFunction__CorrectFloydDT(){
		FunctionDef def;

		TFunctionSignature signature;
		signature._returnType = MakeSignature("<float>");
		signature._args.push_back(
			std::pair<std::string, TTypeSignature>("a", MakeSignature("<float>"))
		);
		def._functionPtr = ExampleFunction1_Glue;
		def._signature = signature;

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


void RunFloydTypeTests(){
	ProveWorks__MakeFunction__SimpleFunction__CorrectFloydDT();
	ProveWorks__CallFunction__SimpleFunction__CorrectReturn();
}





