
// This style encapsulates Calculator and allows us to chose alter implementation without affecting any clients: decide which functions are member's don't affect clients at all

class Calculator {
	private: float _acc;
	private: float _prevAcc;

	private: Calculator(float acc, float prevAcc) :
		_acc(acc),
		_prevAcc(prevAcc)
	{
	}
	private: Calculator(const Calculator& current, float newValue){
		return Calculator(newValue, current._acc)
	}

	friend Calculator add(const Calculator& a, float v);
	friend Calculator makeCalculator();
}

Calculator makeCalculator(){
}

Calculator add(const Calculator& a, float v){
	return Calculator(a, v + a._acc)
}

Calculator sub(const Calculator& a, float v){
	return add(a, -v)
}

