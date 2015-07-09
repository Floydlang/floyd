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


/*
	These are the micro-ops of the simulation. No UI-logic or interactive stuff here.
*/

struct InputPinInstance;

//	An actual concrete pin in the simulation.
struct OutputPinInstance {
	OutputPinInstance() :
		_connectedTo(nullptr)
	{}


	std::string _label;

	InputPinInstance* _connectedTo;
};

struct InputPinInstance {
	InputPinInstance() :
		_connectedTo(nullptr)
	{}

	std::string _label;

	OutputPinInstance* _connectedTo;
};

std::shared_ptr<InputPinInstance> MakeInputPin(const std::string& label);
std::shared_ptr<OutputPinInstance> MakeOutputPin(const std::string& label);



struct WireInstance {
	OutputPinInstance* _sourcePin;
	InputPinInstance* _destPin;
};


struct ConstantPart {
	FloydDT _value;
	OutputPinInstance _outputPin;
};

//	A function, positioned in the simulation.
struct FunctionPart {
	FunctionDef* _function;
	std::map<int, InputPinInstance> _inputs;
	OutputPinInstance _output;
};

struct BoardInstance {
	std::map<int, OutputPinInstance> _boardOutputPins;
	std::map<int, InputPinInstance> _boardInputPins;
	std::map<int, WireInstance> _wires;
	std::map<int, FunctionPart> _functionInstances;
};






ConstantPart MakeConstantPart(const FloydDT& value);
void Connect(InputPinInstance& dest, OutputPinInstance& source);
FloydDT GetValue(const OutputPinInstance& /*output*/);




void test();


/////////////////////////////////////////		SimulationRuntime


struct SimulationRuntime {
	SimulationRuntime();

	//	Returns the input pins exposed by this board.
	std::map<std::string, std::shared_ptr<InputPinInstance> > GetInputs();

	//	Returns the output pins exposed by this board.
	std::map<std::string, std::shared_ptr<OutputPinInstance> > GetOutputs();

	//////////////////////////		State
	std::map<std::string, FunctionDef> _functions;
	std::map<std::string, std::shared_ptr<InputPinInstance> > _inputs;
	std::map<std::string, std::shared_ptr<OutputPinInstance> > _outputs;
};

std::shared_ptr<SimulationRuntime> MakeRuntime(const std::string& json);



#endif /* defined(__Floyd__Runtime__) */
