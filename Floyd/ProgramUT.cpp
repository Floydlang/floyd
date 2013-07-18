//
//  ProgramUT.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2013-07-18.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "ProgramUT.h"

#include "Quark.h"
#include "BasicTypes.h"
#include "Program.h"


namespace {



		struct TEmptyRuntimeFixture {
			public: TEmptyRuntimeFixture(){
				CStaticRegitrat registrat;
				fStaticDefinition.reset(MakeStaticDefinition(registrat));
				fRuntime.reset(MakeRuntime(*fStaticDefinition.get()));
			}

			std::auto_ptr<CStaticDefinition> fStaticDefinition;
			std::auto_ptr<CRuntime> fRuntime;
		};


		CEvaluationNode MakeConditional1(){
			TPin inputs[3];
			inputs[0].fCurrentValue = VValue::MakeInt(3);
			inputs[1].fCurrentValue = VValue::MakeInt(4);
			inputs[2].fCurrentValue = VValue::MakeInt(5);
			CEvaluationNode a = CEvaluationNode::MakeConditional(inputs, CEvaluationNode::kComparison_QuestionIsTrue);
			return a;
		}

		CEvaluationNode MakeAdd1(){
			TPin inputs[3];
			inputs[0].fCurrentValue = VValue::MakeInt(3);
			inputs[1].fCurrentValue = VValue::MakeInt(4);
			inputs[2].fCurrentValue = VValue::MakeInt(5);
			CEvaluationNode a = CEvaluationNode::MakeAdd(inputs);
			return a;
		}

		CEvaluationNode MakeAdd2(){
			TPin inputs[3];
			inputs[0].fCurrentValue = VValue::MakeInt(6);
			inputs[1].fCurrentValue = VValue::MakeInt(7);
			inputs[2].fCurrentValue = VValue::MakeInt(8);
			CEvaluationNode a = CEvaluationNode::MakeAdd(inputs);
			return a;
		}




	void ProveWorks_CEvaluationNode_MakeConditional__Plain__ConstructsOK(){
		CEvaluationNode a = MakeConditional1();

		ASSERT(a.fOutputPin.fSourceNode == -1);
		ASSERT(a.fInputPins.size() == 3);
		ASSERT(a.fInputPins[0].fCurrentValue.GetInt() == 3);
		ASSERT(a.fInputPins[0].fSourceNode == -1);
		ASSERT(a.fInputPins[1].fCurrentValue.GetInt() == 4);
		ASSERT(a.fInputPins[1].fSourceNode == -1);
		ASSERT(a.fInputPins[2].fCurrentValue.GetInt() == 5);
		ASSERT(a.fInputPins[2].fSourceNode == -1);
		ASSERT(a.fType == CEvaluationNode::kNodeType_Conditional);
		ASSERT(a.fComparison == CEvaluationNode::kComparison_QuestionIsTrue);
		ASSERT(a.fPerEntryExpression == NULL);
		ASSERT(a.fGenerationID == -1);
	}

	void ProveWorks_CEvaluationNode_MakeAdd__Plain__ConstructsOK(){
		CEvaluationNode a = MakeAdd1();

		ASSERT(a.fOutputPin.fSourceNode == -1);
		ASSERT(a.fInputPins.size() == 3);
		ASSERT(a.fInputPins[0].fCurrentValue.GetInt() == 3);
		ASSERT(a.fInputPins[0].fSourceNode == -1);
		ASSERT(a.fInputPins[1].fCurrentValue.GetInt() == 4);
		ASSERT(a.fInputPins[1].fSourceNode == -1);
		ASSERT(a.fInputPins[2].fCurrentValue.GetInt() == 5);
		ASSERT(a.fInputPins[2].fSourceNode == -1);
		ASSERT(a.fType == CEvaluationNode::kNodeType_Add);
		ASSERT(a.fComparison == CEvaluationNode::kComparison_ABiggerOrEqualToB);
		ASSERT(a.fPerEntryExpression == NULL);
		ASSERT(a.fGenerationID == -1);
	}




	void ProveWorks_CProgram_Constructor__1Node__ConstructsOK(){
		TEmptyRuntimeFixture f;

		std::vector<CEvaluationNode> nodes;
		nodes.push_back(MakeAdd1());
		CProgram p(*f.fRuntime.get(), nodes);
		ASSERT(p.fNodes.size() == 1);
		ASSERT(p.fGenerationID == 1);
	}




	void ProveWorks_CProgram_Evaluate__1Node__CorrectOutputValue(){
		TEmptyRuntimeFixture f;

		std::vector<CEvaluationNode> nodes;
		nodes.push_back(MakeAdd1());
		CProgram p(*f.fRuntime.get(), nodes);

		ASSERT(p.fNodes[0].fOutputPin.fCurrentValue.IsNil());
		p.Evaluate();
		ASSERT(p.fNodes[0].fOutputPin.fCurrentValue.GetInt() == 12);
	}

	void ProveWorks_CProgram_Evaluate__2NodeSeparate__CorrectOutputValues(){
		TEmptyRuntimeFixture f;

		std::vector<CEvaluationNode> nodes;
		nodes.push_back(MakeAdd1());
		nodes.push_back(MakeAdd2());
		CProgram p(*f.fRuntime.get(), nodes);

		p.Evaluate();
		ASSERT(p.fNodes[0].fOutputPin.fCurrentValue.GetInt() == 12);
		ASSERT(p.fNodes[1].fOutputPin.fCurrentValue.GetInt() == 21);
	}

	void ProveWorks_CProgram_Evaluate__2NodeWithDependencies__CorrectOutputValues(){
		TEmptyRuntimeFixture f;

		std::vector<CEvaluationNode> nodes;
		CEvaluationNode node1 = MakeAdd1();
		CEvaluationNode node2 = MakeAdd2();

		node1.fInputPins[0].fSourceNode = 1;

		nodes.push_back(node1);
		nodes.push_back(node2);

		CProgram p(*f.fRuntime.get(), nodes);

		p.Evaluate();
		ASSERT(p.fNodes[0].fOutputPin.fCurrentValue.GetInt() == 21 + 9);
		ASSERT(p.fNodes[1].fOutputPin.fCurrentValue.GetInt() == 21);
	}
}


void TestProgram(){
	ProveWorks_CEvaluationNode_MakeConditional__Plain__ConstructsOK();
	ProveWorks_CEvaluationNode_MakeAdd__Plain__ConstructsOK();
	ProveWorks_CProgram_Constructor__1Node__ConstructsOK();

	ProveWorks_CProgram_Evaluate__1Node__CorrectOutputValue();
	ProveWorks_CProgram_Evaluate__2NodeSeparate__CorrectOutputValues();
	ProveWorks_CProgram_Evaluate__2NodeWithDependencies__CorrectOutputValues();
}


