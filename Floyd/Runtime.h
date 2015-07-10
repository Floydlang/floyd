//
//  Runtime.h
//  Floyd
//
//  Created by Marcus Zetterquist on 05/07/15.
//  Copyright (c) 2015 Marcus Zetterquist. All rights reserved.
//

#ifndef __Floyd__Runtime__
#define __Floyd__Runtime__

#include "FloydType.h"
#include <string>
/*
#include <vector>
#include <map>
#include <memory>

struct TComposite;
struct FloydDT;
*/
struct WireInput;
struct WireOutput;
struct OutputPinPart;
struct InputPinPart;
struct FunctionPart;
struct ConstantPart;

/*
//	May generate many output values, stored
class IOutputGenerator {
	public: virtual ~IOutputGenerator(){};
	public: virtual FloydDT IOutputGenerator_Render() = 0;
};
*/

/*
	These are the micro-ops of the simulation. No UI-logic or interactive stuff here.
*/




struct WireInput {
	public: WireInput(const TTypeSignature& type, const std::string& label) :
		_type(type),
		_label(label)
	{
	}
	public: bool CheckInvariant() const{
		return true;
	}


	std::string _label;
	TTypeSignature _type;
	WireOutput* _connectedTo = nullptr;
};


//	Generates values.
struct WireOutput {
	public: bool CheckInvariant() const;


	//	Which node consumes our output?
	WireInput* _connectedTo = nullptr;

	FunctionPart* _ownerFunctionPart = nullptr;
	OutputPinPart* _ownerOutputPinPart = nullptr;
	ConstantPart* _ownerConstantPart = nullptr;


//	IOutputGenerator* _generator;
//	FloydDT _outputValue;
};




//	An actual concrete pin in the simulation.
struct OutputPinPart {
	OutputPinPart(const TTypeSignature& type, const std::string& label) :
		_label(label),
		_output(),
		_input(type, label)
	{
	}


	std::string _label;
	WireOutput _output;
	WireInput _input;
};

struct InputPinPart {
	InputPinPart(const TTypeSignature& type, const std::string& label) :
		_label(label),
		_output(),
		_input(type, label)
	{
	}

	std::string _label;
	WireOutput _output;
	WireInput _input;
};


std::shared_ptr<InputPinPart> MakeInputPin(const TTypeSignature& type, const std::string& label);
std::shared_ptr<OutputPinPart> MakeOutputPin(const TTypeSignature& type, const std::string& label);



struct WireInstance {
	OutputPinPart* _sourcePin = nullptr;
	InputPinPart* _destPin = nullptr;
};


struct ConstantPart {
	FloydDT _value;
	WireOutput _output;
};

//	A function, positioned in the simulation.
struct FunctionPart {
	FloydDT _function;
//	FunctionDef* _functionDef;
	std::vector<WireInput> _inputs;
	WireOutput _output;
};

struct BoardInstance {
	std::map<int, OutputPinPart> _boardOutputPins;
	std::map<int, InputPinPart> _boardInputPins;
	std::map<int, WireInstance> _wires;
	std::map<int, FunctionPart> _functionParts;
};




std::shared_ptr<FunctionPart> MakeFunctionPart(const FloydDT& f);
ConstantPart MakeConstantPart(const FloydDT& value);

void Connect(WireInput& dest, WireOutput& source);

FloydDT GetValue(const WireOutput& output);




void test();


/////////////////////////////////////////		SimulationRuntime


struct SimulationRuntime {
	SimulationRuntime();
	bool CheckInvariant() const {
		return true;
	}

	//	Returns the input pins exposed by this board.
	std::map<std::string, std::shared_ptr<InputPinPart> > GetInputPins();

	//	Returns the output pins exposed by this board.
	std::map<std::string, std::shared_ptr<OutputPinPart> > GetOutputPins();

	//////////////////////////		State
	std::map<std::string, std::shared_ptr<FunctionPart> > _functionParts;
	std::map<std::string, std::shared_ptr<InputPinPart> > _inputPins;
	std::map<std::string, std::shared_ptr<OutputPinPart> > _outputPins;
};

std::shared_ptr<SimulationRuntime> MakeRuntime(const std::string& json);



#endif /* defined(__Floyd__Runtime__) */
