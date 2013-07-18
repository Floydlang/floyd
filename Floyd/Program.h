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



struct CEvaluationNode;



//////////////////////////			TPin


/**
	@brief a one-value socket where you can connect a wire.
	If fNode is NULL the fCurrentValue is the constant in fCurrentValue. 
*/

class TPin {
	public: TPin() :
		fSourceNode(-1)
	{
	};

	VValue fCurrentValue;
	long fSourceNode;
};



//////////////////////////			CEvaluationNode



/**
	@brief a one-value socket where you can connect a wire.
	If fNode is NULL the fCurrentValue is the constant in fCurrentValue. 
*/


class CEvaluationNode {
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

	public: ~CEvaluationNode();
	public: bool CheckInvariant() const;


	public: static CEvaluationNode MakeConditional(const TPin iInputPins[3], EComparison comparison);
	public: static CEvaluationNode MakeAdd(const TPin iInputPins[3]);

	///////////////////////		Internals.
		private: CEvaluationNode(ENodeType iType);

	///////////////////////		State.
		public: TPin fOutputPin;
		public: std::vector<TPin> fInputPins;

		public: ENodeType fType;

		public: EComparison fComparison;
		public: CEvaluationNode* fPerEntryExpression;
		public: long fGenerationID;
};



//////////////////////////			CProgram



class CProgram {
	public: CProgram(CRuntime& iRuntime, const std::vector<CEvaluationNode>& iNodes);
	public: ~CProgram();
	public: bool CheckInvariant() const;
	public: void Evaluate();

	private: void EvaluateNodeRecursively(long iNodeIndex, long iGenerationID);
	private: void TouchPin(long iNodeIndex, TPin& iPin, long iGenerationID);

	///////////////////////		State.
		public: CRuntime* fRuntime;
		public: std::vector<CEvaluationNode> fNodes;
		public: long fGenerationID;
};




#endif /* defined(__Floyd__Program__) */
