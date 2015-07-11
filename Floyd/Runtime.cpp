

//
//  Runtime.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 05/07/15.
//  Copyright (c) 2015 Marcus Zetterquist. All rights reserved.
//

#include "Quark.h"

#include "Runtime.h"

using namespace std;


//	### Intern & RC any bigger values.



/////////////////////////////////////////		WireOutput



WireOutput::WireOutput(FunctionPart* ownerFunctionPart) :
	_ownerFunctionPart(ownerFunctionPart)
{
	ASSERT(ownerFunctionPart != nullptr);

	ASSERT(CheckInvariant());
}

WireOutput::WireOutput(OutputPinPart* ownerOutputPinPart) :
	_ownerOutputPinPart(ownerOutputPinPart)
{
	ASSERT(ownerOutputPinPart != nullptr);

	ASSERT(CheckInvariant());
}

WireOutput::WireOutput(InputPinPart* ownerInputPinPart) :
	_ownerInputPinPart(ownerInputPinPart)
{
	ASSERT(ownerInputPinPart != nullptr);

	ASSERT(CheckInvariant());
}

WireOutput::WireOutput(ConstantPart* ownerConstantPart) :
	_ownerConstantPart(ownerConstantPart)
{
	ASSERT(ownerConstantPart != nullptr);

	ASSERT(CheckInvariant());
}

bool WireOutput::CheckInvariant() const{
	if(_ownerFunctionPart != nullptr){
//		ASSERT(_ownerFunctionPart == nullptr);
		ASSERT(_ownerOutputPinPart == nullptr);
		ASSERT(_ownerInputPinPart == nullptr);
		ASSERT(_ownerConstantPart == nullptr);
	}
	else if(_ownerOutputPinPart != nullptr){
		ASSERT(_ownerFunctionPart == nullptr);
//		ASSERT(_ownerOutputPinPart == nullptr);
		ASSERT(_ownerInputPinPart == nullptr);
		ASSERT(_ownerConstantPart == nullptr);
	}
	else if(_ownerInputPinPart != nullptr){
		ASSERT(_ownerFunctionPart == nullptr);
		ASSERT(_ownerOutputPinPart == nullptr);
//		ASSERT(_ownerInputPinPart == nullptr);
		ASSERT(_ownerConstantPart == nullptr);
	}
	else if(_ownerConstantPart != nullptr){
		ASSERT(_ownerFunctionPart == nullptr);
		ASSERT(_ownerOutputPinPart == nullptr);
		ASSERT(_ownerInputPinPart == nullptr);
//		ASSERT(_ownerConstantPart == nullptr);
	}
	else{
	}

	return true;
}


/////////////////////////////////////////		ConstantPart


ConstantPart::ConstantPart(const FloydDT& value) :
	_value(value),
	_output(this)
{
	ASSERT(value.CheckInvariant());

	ASSERT(CheckInvariant());
}


/////////////////////////////////////////		FunctionPart


FunctionPart::FunctionPart() :
	_output(this)
{

	ASSERT(CheckInvariant());
}











shared_ptr<FunctionPart> MakeFunctionPart(const FloydDT& f){
	ASSERT(f.CheckInvariant());
	ASSERT(IsFunction(f));

	auto result = shared_ptr<FunctionPart>(new FunctionPart());
	result->_function = f;
//	result->_output._ownerFunctionPart = result.get();
	for(auto i: f._asFunction->_signature._args){
		auto input = WireInput(i.second, i.first);
		result->_inputs.push_back(input);
	}
	return result;
}


shared_ptr<ConstantPart> MakeConstantPart(const FloydDT& value){
	ASSERT(value.CheckInvariant());

	auto result = shared_ptr<ConstantPart>(new ConstantPart(value));
	return result;
}

//	### Put wiring in separate datastructure, not in pin instances.

void Connect(WireInput& dest, WireOutput& source){
	ASSERT(dest.CheckInvariant());
	ASSERT(source.CheckInvariant());

	source._connectedTo = &dest;
	dest._connectedTo = &source;
}



namespace {

	FloydDT GenerateValue(const WireOutput& output){
		ASSERT(output.CheckInvariant());

		if(output._ownerFunctionPart != nullptr){
//			const TFunctionSignature& functionSignature = output._ownerFunctionPart->_functionDef->_signature;

			//	Read all our inputs to get arguments to call function part.
			vector<FloydDT> argValues;
			for(auto i: output._ownerFunctionPart->_inputs){
				const auto v = i._connectedTo == nullptr ? MakeNull() : GetValue(*i._connectedTo);
				argValues.push_back(v);
			}
			const auto functionResult = CallFunction(output._ownerFunctionPart->_function, argValues);
			return functionResult;
		}
		else if(output._ownerOutputPinPart != nullptr){
			const auto nextOutput = output._ownerOutputPinPart->_input._connectedTo;
			const auto v = nextOutput == nullptr ? MakeNull() : GetValue(*nextOutput);
			return v;
		}
		else if(output._ownerInputPinPart != nullptr){
			const auto nextOutput = output._ownerInputPinPart->_input._connectedTo;
			const auto v = nextOutput == nullptr ? MakeNull() : GetValue(*nextOutput);
			return v;
		}
		else if(output._ownerConstantPart != nullptr){
			return output._ownerConstantPart->_value;
		}
		else{
			ASSERT(false);
			return MakeNull();
		}
	}

}

FloydDT GetValue(const WireOutput& output){
	auto r = GenerateValue(output);
	return r;
}



shared_ptr<InputPinPart> MakeInputPin(const TTypeSignature& type, const string& label){
	auto result = shared_ptr<InputPinPart>(new InputPinPart(type, label));
	result->_label = label;
	return result;
}

shared_ptr<OutputPinPart> MakeOutputPin(const TTypeSignature& type, const string& label){
	auto result = shared_ptr<OutputPinPart>(new OutputPinPart(type, label));
	result->_label = label;
//	result->_output._ownerOutputPinPart = result.get();
	return result;
}









shared_ptr<SimulationRuntime> MakeRuntime(const string& /*json*/){
	return shared_ptr<SimulationRuntime>(new SimulationRuntime());
}



/////////////////////////////////////////		SimulationRuntime



SimulationRuntime::SimulationRuntime(){
}

map<string, shared_ptr<InputPinPart> > SimulationRuntime::GetInputPins(){
	return _inputPins;
}

map<string, shared_ptr<OutputPinPart> > SimulationRuntime::GetOutputPins(){
	return _outputPins;
}






/////////////////////////////////////////		Tests




namespace {


	float ExampleFunction1(float a, float b, string s){
		return s == "*" ? a * b : a + b;
	}

	FloydDT ExampleFunction1_Glue(const FloydDT args[], size_t argCount){
		ASSERT(args != nullptr);
		ASSERT(argCount == 3);
		ASSERT(args[0]._type == FloydDTType::kFloat);
		ASSERT(args[1]._type == FloydDTType::kFloat);
		ASSERT(args[2]._type == FloydDTType::kString);

		const float a = args[0]._asFloat;
		const float b = args[1]._asFloat;
		const string s = args[2]._asString;
		const float r = ExampleFunction1(a, b, s);

		FloydDT result = MakeFloat(r);
		return result;
	}




	const string kTest2 = "hjsdf";

	const string kTest3 =
	"{"
	"	version: 2"
	"}";




	FloydDT MakeFunction1(){
		TFunctionSignature signature;
		signature._args.push_back(
			pair<string, TTypeSignature>("a", MakeSignature("<float>"))
		);
		signature._args.push_back(
			pair<string, TTypeSignature>("b", MakeSignature("<float>"))
		);
		signature._args.push_back(
			pair<string, TTypeSignature>("s", MakeSignature("<string>"))
		);

		signature._returnType = MakeSignature("<float>");

		FunctionDef def;
		def._functionPtr = ExampleFunction1_Glue;
		def._signature = signature;

		const FloydDT result = MakeFunction(def);
		return result;
	}


	void ProveWorks__MakeFunctionPart__1Function__ValidFunctionPart(){
		auto r = MakeFunctionPart(MakeFunction1());

		UT_VERIFY(r->CheckInvariant());
		UT_VERIFY(IsFunction(r->_function));
		UT_VERIFY(r->_inputs.size() == 3);
		UT_VERIFY(r->_output.CheckInvariant());
	}


		//	### add clock -auto clock = inputs["clock"];
	shared_ptr<SimulationRuntime> MakeExample1Runtime(){
		auto r = MakeRuntime(kTest2);

		//	Create all external pins.
		r->_inputPins["a"] = MakeInputPin(MakeSignature("<float>"), "a");
		r->_inputPins["b"] = MakeInputPin(MakeSignature("<float>"), "b");
		r->_inputPins["s"] = MakeInputPin(MakeSignature("<string>"), "s");
		r->_outputPins["result"] = MakeOutputPin(MakeSignature("<float>"), "result");
		r->_functionParts["f1"] = MakeFunctionPart(MakeFunction1());

		//	Connect all internal wires.
		Connect(r->_functionParts["f1"]->_inputs[0], r->_inputPins["a"]->_output);
		Connect(r->_functionParts["f1"]->_inputs[1], r->_inputPins["b"]->_output);
		Connect(r->_functionParts["f1"]->_inputs[2], r->_inputPins["s"]->_output);
		Connect(r->_outputPins["result"]->_input, r->_functionParts["f1"]->_output);

		ASSERT(r->CheckInvariant());
		return r;
	}


	void ProveWorks__GetValue__MinimalSimulation1Function__OutputIs6(){
		auto r = MakeExample1Runtime();

		auto constantA = MakeConstantPart(MakeFloat(3.0f));
		auto constantB = MakeConstantPart(MakeFloat(2.0f));
		auto constantS = MakeConstantPart(MakeString("*"));

		//	Make test rig for the simulation.
		auto inputPinA = r->GetInputPins()["a"];
		auto inputPinB = r->GetInputPins()["b"];
		auto inputPinS = r->GetInputPins()["s"];
		auto resultOutputPin = r->GetOutputPins()["result"];
		Connect(inputPinA->_input, constantA->_output);
		Connect(inputPinB->_input, constantB->_output);
		Connect(inputPinS->_input, constantS->_output);

		//	Read "result" output pin. This should cause simulation to run all through.
		FloydDT result = GetValue(resultOutputPin->_output);

		ASSERT(GetFloat(result) == 6.0f);
	}

}


void test(){
	RunFloydTypeTests();

	ProveWorks__MakeFunctionPart__1Function__ValidFunctionPart();
	ProveWorks__GetValue__MinimalSimulation1Function__OutputIs6();
}

