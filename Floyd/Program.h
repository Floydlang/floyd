//
//  Program.h
//  Floyd
//
//  Created by Marcus Zetterquist on 2013-07-18.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#ifndef __Floyd__Program__
#define __Floyd__Program__



#include <vector>
#include "BasicTypes.h"



class CEvalNode;
class CProgram;


//////////////////////////			TPin


/**
	@brief This is a one-value socket where you can connect a wire.
	If fNode is NULL the fCurrentValue is the constant in fCurrentValue. 
*/

class TPin {
	public: TPin();
	public: TPin(const VValue& iCurrentValue);
	public: TPin(const VValue& iCurrentValue, long iSourceNode);
	public: bool CheckInvariant() const;
	public: VValue GetValue() const;
	public: void StoreValue(const VValue& iValue);


	private: VValue fCurrentValue;
	public: long fSourceNode;
};



//////////////////////////			CEvalNode



/**
	@brief This is a node in the evaluation graph that has inputs + output + operation.
*/


class CEvalNode {
	public: enum EComparison {
		kComparison_QuestionIsTrue,
		kComparison_QuestionIsFalse,

		kComparison_ALessThanB,
		kComparison_ALessOrEqualB,
		kComparison_ABiggerThanB,
		kComparison_ABiggerOrEqualToB,

		kComparison_AEqualToB
	};

	public: enum ENodeType {
		kNodeType_Switcher,

		/**
			input[0] = question (or nil)
			input[1] = a
			input[2] = b
			r = (question ? a : b)
			r = (a == b ? a : b)
			r = (a > b ? a : b)
		*/
 		kNodeType_Conditional,

		kNodeType_Looper,

		kNodeType_FindValue,

		/**
			Adds all inputs.
		*/
		kNodeType_Add,

		kNodeType_Multiply
	};

	public: ~CEvalNode();
	public: bool CheckInvariant() const;

	public: static CEvalNode MakeLooper(const TPin& iRange,
		const TPin& iStartValue,
		const TPin& iContext,
		const VExpressionRef& iExpressionRef);

	public: static CEvalNode MakeConditional(EComparison comparison, const TPin iQuestion, const TPin iA, const TPin iB);
	public: static CEvalNode MakeConditional(EComparison comparison, const TPin iInputPins[3]);

	public: static CEvalNode MakeAdd(const TPin& iA, const TPin& iB, const TPin& iC);
	public: static CEvalNode MakeAdd(const TPin iInputPins[3]);

	///////////////////////		Internals.
		private: CEvalNode(ENodeType iType);

	///////////////////////		State.
		public: TPin fOutputPin;
		public: std::vector<TPin> fInputPins;

		public: ENodeType fType;

		public: EComparison fComparison;
		public: std::shared_ptr<VExpressionRef> fExpressionRef;
		public: long fGenerationID;
};



//////////////////////////			CProgram



class CProgram {
	public: CProgram(CRuntime& iRuntime, const std::vector<CEvalNode>& iNodes);
	public: ~CProgram();
	public: bool CheckInvariant() const;
	public: void Evaluate();

	private: void EvaluateNodeRecursively(long iNodeIndex, long iGenerationID);
	private: void EvaluatePin(long iNodeIndex, TPin& iPin, long iGenerationID);

	///////////////////////		State.
		public: CRuntime* fRuntime;
		public: std::vector<CEvalNode> fNodes;
		public: long fGenerationID;
};




#endif /* defined(__Floyd__Program__) */
