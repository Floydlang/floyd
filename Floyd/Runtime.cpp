//
//  Runtime.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 05/07/15.
//  Copyright (c) 2015 Marcus Zetterquist. All rights reserved.
//

#include "Quark.h"

#include "Runtime.h"


//	### Intern and RC bigger values.


ConstantPart MakeConstantPart(const FloydDT& value){
	ConstantPart result;
	result._value = value;
	return result;
}

//	### Put wiring in separate datastructure, not in pin instances.

void Connect(InputPinInstance& dest, OutputPinInstance& source){
	source._connectedTo = &dest;
	dest._connectedTo = &source;
}

FloydDT GetValue(const OutputPinInstance& output){
	return output._connectedTo == nullptr ? MakeNull() : output._connectedTo->GetValue();
}





std::shared_ptr<InputPinInstance> MakeInputPin(const std::string& label){
	auto result = std::shared_ptr<InputPinInstance>(new InputPinInstance());
	result->_label = label;
	return result;
}

std::shared_ptr<OutputPinInstance> MakeOutputPin(const std::string& label){
	auto result = std::shared_ptr<OutputPinInstance>(new OutputPinInstance());
	result->_label = label;
	return result;
}









std::shared_ptr<SimulationRuntime> MakeRuntime(const std::string& json){
	return std::shared_ptr<SimulationRuntime>(new SimulationRuntime());
}


/////////////////////////////////////////		SimulationRuntime


SimulationRuntime::SimulationRuntime(){
}

std::map<std::string, std::shared_ptr<InputPinInstance> > SimulationRuntime::GetInputs(){
	return _inputs;
}

std::map<std::string, std::shared_ptr<OutputPinInstance> > SimulationRuntime::GetOutputs(){
	return _outputs;
}






/////////////////////////////////////////




namespace {


	float ExampleFunction1(float a, float b, std::string s){
		return s == "*" ? a * b : a + b;
	}

	FloydDT ExampleFunction1_Glue(const FloydDT args[], int argCount){
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




	const std::string kTest2 = "hjsdf";

	const std::string kTest3 =
	"{"
	"	version: 2"
	"}";


	void ProveWorks_BasicTest(){
		auto r = MakeRuntime(kTest2);
		r->_inputs["a"] = MakeInputPin("a");
		r->_inputs["b"] = MakeInputPin("b");
		r->_outputs["result"] = MakeOutputPin("result");
		auto inputs = r->GetInputs();
		auto outputs = r->GetOutputs();

		auto a = MakeConstantPart(MakeFloat(3.0f));
		auto b = MakeConstantPart(MakeFloat(4.0f));

	//	auto clock = inputs["clock"];
		Connect(*inputs["a"], a._outputPin);
		Connect(*inputs["b"], b._outputPin);

		FloydDT result = GetValue(*outputs["result"]);

		ASSERT(IsNull(result));
	}

}


void test(){
	RuntFloydTypeTests();
	ProveWorks_BasicTest();
}

