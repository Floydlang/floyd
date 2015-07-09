


//	DataType = sha1 of (reverse domain of organisation + module-name + class-name + version-string)
class FloydDataType {
	string sha1;
};


class Param {
	FloydDataType type;
	string doc;
	ContractRange range;
	string name;
};

class FloydFunction {
	string name;
	string module;
	void (*f)();
	vector<Param> outputs;
	vector<Input> inputs;
};


//	Static design of a circuit.
class FloydCircuit {
	const vector<FloydCircuit> circuits
	vector<Output> outputs;
	vector<Input> inputs;
};

//	
class CircuitState {
	
};


class FloydRuntime {
	

};



class FunctionContext {
	//	Assign once.
	vector<FloydDataType> localVariables;

	vector<FloydDataType> inputs
	vector<FloydDataType> result;
}