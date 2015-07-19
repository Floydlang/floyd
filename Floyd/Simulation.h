//
//  Runtime.h
//  Floyd
//
//  Created by Marcus Zetterquist on 05/07/15.
//  Copyright (c) 2015 Marcus Zetterquist. All rights reserved.
//

/*
	The Simulation lets you make part-instances and wire them together, then run the simulation like a hardware circuit.
	It uses the Runtime to store state and types.

	Parts are things like "pins", function-parts, CHIP-parts etc. They all have inputs and outputs you
	can wire together.

	Uses a pull-concept.
*/

#ifndef __Floyd__Runtime__
#define __Floyd__Runtime__

#include "Runtime.h"
#include <string>



namespace Floyd {

	struct TComposite;
	struct Value;
	struct WireInput;
	struct WireOutput;
	struct OutputPinPart;
	struct InputPinPart;
	struct FunctionPart;
	struct ConstantPart;



	/////////////////////////////////////////		WireInput

	/*
		This is an input of a part, where you can connect a wire ot an output.
	*/
	struct WireInput {
		public: WireInput(const TValueType& type, const std::string& label) :
			_type(type),
			_label(label)
		{
		}
		public: bool CheckInvariant() const{
			return true;
		}


		std::string _label;
		TValueType _type;
		WireOutput* _connectedTo = nullptr;
	};



	/////////////////////////////////////////		WireOutput


	/*
		This is a small output of a part, where you can connect a wire to an input.
	*/

	struct WireOutput {
		public: WireOutput(FunctionPart* ownerFunctionPart);
		public: WireOutput(OutputPinPart* ownerOutputPinPart);
		public: WireOutput(InputPinPart* ownerInputPinPart);
		public: WireOutput(ConstantPart* ownerConstantPart);
		public: bool CheckInvariant() const;


		//	Which node consumes our output?
		WireInput* _connectedTo = nullptr;

		FunctionPart* _ownerFunctionPart = nullptr;
		OutputPinPart* _ownerOutputPinPart = nullptr;
		InputPinPart* _ownerInputPinPart = nullptr;
		ConstantPart* _ownerConstantPart = nullptr;
	};



	/////////////////////////////////////////		OutputPinPart


	/*
		Output pin exposed by the simulation towards the outside world.
	*/
	struct OutputPinPart {
		OutputPinPart(const TValueType& type, const std::string& label) :
			_label(label),
			_output(this),
			_input(type, label)
		{
		}


		std::string _label;
		WireOutput _output;
		WireInput _input;
	};



	/////////////////////////////////////////		InputPinPart


	/*
		Input pin exposed by the simulation towards the outside world.
	*/
	struct InputPinPart {
		InputPinPart(const TValueType& type, const std::string& label) :
			_label(label),
			_output(this),
			_input(type, label)
		{
		}

		std::string _label;
		WireOutput _output;
		WireInput _input;
	};


	std::shared_ptr<InputPinPart> MakeInputPin(const TValueType& type, const std::string& label);
	std::shared_ptr<OutputPinPart> MakeOutputPin(const TValueType& type, const std::string& label);



/*
	/////////////////////////////////////////		WireInstance


	struct WireInstance {
		OutputPinPart* _sourcePin = nullptr;
		InputPinPart* _destPin = nullptr;
	};
*/

	/////////////////////////////////////////		ConstantPart

	/*
		A part containing a constant value. It cannot change at runtime.
	*/
	struct ConstantPart {
		ConstantPart(const Value& value);
		public: bool CheckInvariant() const{
			return true;
		}

		Value _value;
		WireOutput _output;
	};


	/////////////////////////////////////////		FunctionPart

	/*
		A function wired up in the simulation. Each input argument becomes an input, the output becomes an output.
		If you need to same function in several different places in the simulation, just make more FunctionParts.
	*/
	struct FunctionPart {
		public: FunctionPart();
		public: bool CheckInvariant() const{
			return true;
		}


		Value _function;
		std::vector<WireInput> _inputs;
		WireOutput _output;
	};


	/////////////////////////////////////////		BoardInstance

	/*
		TBD: A simulation should be able to hold nested boards.
	*/
/*
	struct BoardInstance {
		std::map<int, OutputPinPart> _boardOutputPins;
		std::map<int, InputPinPart> _boardInputPins;
//		std::map<int, WireInstance> _wires;
		std::map<int, FunctionPart> _functionParts;
	};
*/


	std::shared_ptr<FunctionPart> MakeFunctionPart(const Value& f);
	std::shared_ptr<ConstantPart> MakeConstantPart(const Value& value);

	void Connect(WireInput& dest, WireOutput& source);

	Value GetValue(const WireOutput& output);



	/////////////////////////////////////////		Simulation

	/*
		A simulation instance.
		It has no outside dependencies, except the Runtime that holds values and types.
	*/

	struct Simulation {
		Simulation(std::shared_ptr<Runtime> runtime);
		bool CheckInvariant() const;

		//	Returns the input pins exposed by this board.
		std::map<std::string, std::shared_ptr<InputPinPart> > GetInputPins();

		//	Returns the output pins exposed by this board.
		std::map<std::string, std::shared_ptr<OutputPinPart> > GetOutputPins();


		//////////////////////////		State
		std::shared_ptr<Runtime> _runtime;
		std::map<std::string, std::shared_ptr<FunctionPart> > _functionParts;
		std::map<std::string, std::shared_ptr<InputPinPart> > _inputPins;
		std::map<std::string, std::shared_ptr<OutputPinPart> > _outputPins;
	};

	std::shared_ptr<Simulation> MakeSimulation(std::shared_ptr<Runtime> runtime, const std::string& json);

}


#endif /* defined(__Floyd__Runtime__) */
