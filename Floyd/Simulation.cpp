//
//  Runtime.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 05/07/15.
//  Copyright (c) 2015 Marcus Zetterquist. All rights reserved.
//

#include "cpp_extension.h"

#include "Simulation.h"

using namespace std;





/////////////////////////////////////////		WireOutput



Floyd::WireOutput::WireOutput(FunctionPart* ownerFunctionPart) :
	_ownerFunctionPart(ownerFunctionPart)
{
	ASSERT(ownerFunctionPart != nullptr);

	ASSERT(CheckInvariant());
}

Floyd::WireOutput::WireOutput(OutputPinPart* ownerOutputPinPart) :
	_ownerOutputPinPart(ownerOutputPinPart)
{
	ASSERT(ownerOutputPinPart != nullptr);

	ASSERT(CheckInvariant());
}

Floyd::WireOutput::WireOutput(InputPinPart* ownerInputPinPart) :
	_ownerInputPinPart(ownerInputPinPart)
{
	ASSERT(ownerInputPinPart != nullptr);

	ASSERT(CheckInvariant());
}

Floyd::WireOutput::WireOutput(ConstantPart* ownerConstantPart) :
	_ownerConstantPart(ownerConstantPart)
{
	ASSERT(ownerConstantPart != nullptr);

	ASSERT(CheckInvariant());
}

bool Floyd::WireOutput::CheckInvariant() const{
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


Floyd::ConstantPart::ConstantPart(const Value& value) :
	_value(value),
	_output(this)
{
	ASSERT(value.CheckInvariant());

	ASSERT(CheckInvariant());
}


/////////////////////////////////////////		FunctionPart


Floyd::FunctionPart::FunctionPart() :
	_output(this)
{

	ASSERT(CheckInvariant());
}











shared_ptr<Floyd::FunctionPart> Floyd::MakeFunctionPart(const Value& f){
	ASSERT(f.CheckInvariant());
	ASSERT(IsFunction(f));

	auto result = shared_ptr<FunctionPart>(new FunctionPart());
	result->_function = f;
//	result->_output._ownerFunctionPart = result.get();
	const auto signature = GetFunctionSignature(f);
	for(auto i = 1 ; i < signature._more.size() ; i++){
		const auto a = signature._more[i];
		auto input = WireInput(a.second, a.first);
		result->_inputs.push_back(input);
	}
	return result;
}


shared_ptr<Floyd::ConstantPart> Floyd::MakeConstantPart(const Value& value){
	ASSERT(value.CheckInvariant());

	auto result = shared_ptr<Floyd::ConstantPart>(new ConstantPart(value));
	return result;
}

//	### Put wiring in separate datastructure, not in pin instances.

void Floyd::Connect(WireInput& dest, WireOutput& source){
	ASSERT(dest.CheckInvariant());
	ASSERT(source.CheckInvariant());

	source._connectedTo = &dest;
	dest._connectedTo = &source;
}



namespace {

	Floyd::Value GenerateValue(Floyd::Simulation& simulation,const Floyd::WireOutput& output){
		ASSERT(simulation.CheckInvariant());
		ASSERT(output.CheckInvariant());

		using namespace Floyd;

		if(output._ownerFunctionPart != nullptr){
//			const TFunctionSignature& functionSignature = output._ownerFunctionPart->_functionDef->_signature;

			//	Read all our inputs to get arguments to call function part.
			vector<Value> argValues;
			for(auto i: output._ownerFunctionPart->_inputs){
				const auto v = i._connectedTo == nullptr ? MakeNull() : GetValue(simulation, *i._connectedTo);
				argValues.push_back(v);
			}
			const auto functionResult = CallFunction(simulation.GetRuntime(), output._ownerFunctionPart->_function, argValues);
			return functionResult;
		}
		else if(output._ownerOutputPinPart != nullptr){
			const auto nextOutput = output._ownerOutputPinPart->_input._connectedTo;
			const auto v = nextOutput == nullptr ? MakeNull() : GetValue(simulation, *nextOutput);
			return v;
		}
		else if(output._ownerInputPinPart != nullptr){
			const auto nextOutput = output._ownerInputPinPart->_input._connectedTo;
			const auto v = nextOutput == nullptr ? MakeNull() : GetValue(simulation, *nextOutput);
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

Floyd::Value Floyd::GetValue(Floyd::Simulation& simulation, const WireOutput& output){
	ASSERT(simulation.CheckInvariant());
	ASSERT(output.CheckInvariant());

	auto r = GenerateValue(simulation, output);
	return r;
}



shared_ptr<Floyd::InputPinPart> Floyd::MakeInputPin(const TValueType& type, const string& label){
	auto result = shared_ptr<InputPinPart>(new InputPinPart(type, label));
	result->_label = label;
	return result;
}

shared_ptr<Floyd::OutputPinPart> Floyd::MakeOutputPin(const TValueType& type, const string& label){
	auto result = shared_ptr<OutputPinPart>(new OutputPinPart(type, label));
	result->_label = label;
//	result->_output._ownerOutputPinPart = result.get();
	return result;
}







shared_ptr<Floyd::Simulation> Floyd::MakeSimulation(std::shared_ptr<Runtime> runtime, const string& /*json*/){
	return shared_ptr<Simulation>(new Simulation(runtime));
}



/////////////////////////////////////////		Simulation



Floyd::Simulation::Simulation(std::shared_ptr<Runtime> runtime) :
	_runtime(runtime)
{
	ASSERT(runtime->CheckInvariant());

	ASSERT(CheckInvariant());
}

bool Floyd::Simulation::CheckInvariant() const {
	ASSERT(_runtime && _runtime->CheckInvariant());

	return true;
}

map<string, shared_ptr<Floyd::InputPinPart> > Floyd::Simulation::GetInputPins(){
	ASSERT(CheckInvariant());

	return _inputPins;
}

map<string, shared_ptr<Floyd::OutputPinPart> > Floyd::Simulation::GetOutputPins(){
	ASSERT(CheckInvariant());

	return _outputPins;
}


std::shared_ptr<Floyd::Runtime> Floyd::Simulation::GetRuntime(){
	ASSERT(CheckInvariant());

	return _runtime;
}




/////////////////////////////////////////		Tests

using namespace Floyd;



namespace {

	float ExampleFunction1(float a, float b, string s){
		return s == "*" ? a * b : a + b;
	}

	Value ExampleFunction1_Glue(const IFunctionContext& context, const Value args[], size_t argCount){
		ASSERT(args != nullptr);
		ASSERT(argCount == 3);
		ASSERT(IsFloat(args[0]));
		ASSERT(IsFloat(args[1]));
		ASSERT(IsString(args[2]));

		const float a = GetFloat(args[0]);
		const float b = GetFloat(args[1]);
		const string s = GetString(args[2]);
		const float r = ExampleFunction1(a, b, s);

		const Value result = MakeFloat(r);
		return result;
	}


	const string kTest2 = "hjsdf";

	const string kTest3 =
	"{"
	"	version: 2"
	"}";


	Value MakeFunction1(){
		TTypeDefinition typeDef(EType::kFunction);
		typeDef._more.push_back(std::pair<std::string, TValueType>("", EType::kFloat));
		typeDef._more.push_back(std::pair<std::string, TValueType>("a", EType::kFloat));
		typeDef._more.push_back(std::pair<std::string, TValueType>("b", EType::kFloat));
		typeDef._more.push_back(std::pair<std::string, TValueType>("s", EType::kString));

		auto runtime = Runtime();
		const auto result = runtime.DefineFunction("F1", typeDef, ExampleFunction1_Glue);
		return result;
	}

		//	### add clock -auto clock = inputs["clock"];
	shared_ptr<Simulation> MakeExample1Simulation(){
		auto runtime = shared_ptr<Runtime>(new Runtime());
		auto r = MakeSimulation(runtime, kTest2);

		//	Create all external pins.
		r->_inputPins["a"] = MakeInputPin(TValueType(EType::kFloat), "a");
		r->_inputPins["b"] = MakeInputPin(TValueType(EType::kFloat), "b");
		r->_inputPins["s"] = MakeInputPin(TValueType(EType::kString), "s");
		r->_outputPins["result"] = MakeOutputPin(TValueType(EType::kFloat), "result");
		r->_functionParts["f1"] = MakeFunctionPart(MakeFunction1());

		//	Connect all internal wires.
		Connect(r->_functionParts["f1"]->_inputs[0], r->_inputPins["a"]->_output);
		Connect(r->_functionParts["f1"]->_inputs[1], r->_inputPins["b"]->_output);
		Connect(r->_functionParts["f1"]->_inputs[2], r->_inputPins["s"]->_output);
		Connect(r->_outputPins["result"]->_input, r->_functionParts["f1"]->_output);

		ASSERT(r->CheckInvariant());
		return r;
	}


}


UNIT_TEST("Runtime", "MakeFunctionPart", "1Function", "ValidFunctionPart"){
	const auto r = MakeFunctionPart(MakeFunction1());

	UT_VERIFY(r->CheckInvariant());
	UT_VERIFY(IsFunction(r->_function));
	UT_VERIFY(r->_inputs.size() == 3);
	UT_VERIFY(r->_output.CheckInvariant());
}

UNIT_TEST("Runtime", "GetValue", "MinimalSimulation1Function", "OutputIs6"){
	auto r = MakeExample1Simulation();

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
	Value result = GetValue(*r, resultOutputPin->_output);

	ASSERT(GetFloat(result) == 6.0f);
}







/////////////////////////////////////////		Test scenario 1






namespace {

	Value fx(const IFunctionContext& /*context*/, const Value args[], size_t argCount){
		ASSERT(args != nullptr);
		ASSERT(argCount == 3);
		ASSERT(IsFloat(args[0]));
		ASSERT(IsFloat(args[1]));
		ASSERT(IsString(args[2]));

		const float a = GetFloat(args[0]);
		const float b = GetFloat(args[1]);
		const string s = GetString(args[2]);
		const float r = s == "*" ? a * b : a + b;
		const Value result = MakeFloat(r);
		return result;
	}
}

UNIT_TEST("Runtime", "", "Scenario 1", ""){
	auto runtime = shared_ptr<Runtime>(new Runtime());
	auto simulation = MakeSimulation(runtime, kTest2);

	//	Build the board.
	{
		TTypeDefinition typeDef(EType::kFunction);
		typeDef._more.push_back(std::pair<std::string, TValueType>("", EType::kFloat));
		typeDef._more.push_back(std::pair<std::string, TValueType>("a", EType::kFloat));
		typeDef._more.push_back(std::pair<std::string, TValueType>("b", EType::kFloat));
		typeDef._more.push_back(std::pair<std::string, TValueType>("s", EType::kString));
		const auto fxValue = runtime->DefineFunction("fx", typeDef, fx);

		simulation->_functionParts["f1"] = MakeFunctionPart(fxValue);

		//	Create all external pins.
		simulation->_inputPins["a"] = MakeInputPin(TValueType(EType::kFloat), "a");
		simulation->_inputPins["b"] = MakeInputPin(TValueType(EType::kFloat), "b");
		simulation->_inputPins["s"] = MakeInputPin(TValueType(EType::kString), "s");
		simulation->_outputPins["result"] = MakeOutputPin(TValueType(EType::kFloat), "result");

		//	Connect all internal wires.
		Connect(simulation->_functionParts["f1"]->_inputs[0], simulation->_inputPins["a"]->_output);
		Connect(simulation->_functionParts["f1"]->_inputs[1], simulation->_inputPins["b"]->_output);
		Connect(simulation->_functionParts["f1"]->_inputs[2], simulation->_inputPins["s"]->_output);
		Connect(simulation->_outputPins["result"]->_input, simulation->_functionParts["f1"]->_output);
	}

	//	Make test rig for the simulation.
	auto constantA = MakeConstantPart(MakeFloat(3.0f));
	auto constantB = MakeConstantPart(MakeFloat(2.0f));
	auto constantS = MakeConstantPart(MakeString("*"));

	auto inputPinA = simulation->GetInputPins()["a"];
	auto inputPinB = simulation->GetInputPins()["b"];
	auto inputPinS = simulation->GetInputPins()["s"];
	auto resultOutputPin = simulation->GetOutputPins()["result"];
	Connect(inputPinA->_input, constantA->_output);
	Connect(inputPinB->_input, constantB->_output);
	Connect(inputPinS->_input, constantS->_output);

	//	Read "result" output pin. This should cause simulation to run all through.
	Value result = GetValue(*simulation, resultOutputPin->_output);

	ASSERT(GetFloat(result) == 6.0f);
}

