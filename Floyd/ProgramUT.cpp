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


		CEvalNode MakeConditional1(){
			TPin inputs[3] = {	VValue(3), VValue(4), VValue(5) };
			CEvalNode a = CEvalNode::MakeConditional(CEvalNode::kComparison_QuestionIsTrue, inputs);
			return a;
		}

		CEvalNode MakeAdd1(){
			TPin inputs[3] = {	VValue(3), VValue(4), VValue(5) };
			CEvalNode a = CEvalNode::MakeAdd(inputs);
			return a;
		}

		CEvalNode MakeAdd2(){
			TPin inputs[3] = {	VValue(6), VValue(7), VValue(8) };
			CEvalNode a = CEvalNode::MakeAdd(inputs);
			return a;
		}




	void ProveWorks_CEvalNode_MakeConditional__Plain__ConstructsOK(){
		CEvalNode a = MakeConditional1();

		ASSERT(a.fOutputPin.fSourceNode == -1);
		ASSERT(a.fInputPins.size() == 3);
		ASSERT(a.fInputPins[0].GetValue().GetInt() == 3);
		ASSERT(a.fInputPins[0].fSourceNode == -1);
		ASSERT(a.fInputPins[1].GetValue().GetInt() == 4);
		ASSERT(a.fInputPins[1].fSourceNode == -1);
		ASSERT(a.fInputPins[2].GetValue().GetInt() == 5);
		ASSERT(a.fInputPins[2].fSourceNode == -1);
		ASSERT(a.fType == CEvalNode::kNodeType_Conditional);
		ASSERT(a.fComparison == CEvalNode::kComparison_QuestionIsTrue);
		ASSERT(!a.fExpressionRef);
		ASSERT(a.fGenerationID == -1);
	}

	void ProveWorks_CEvalNode_MakeAdd__Plain__ConstructsOK(){
		CEvalNode a = MakeAdd1();

		ASSERT(a.fOutputPin.fSourceNode == -1);
		ASSERT(a.fInputPins.size() == 3);
		ASSERT(a.fInputPins[0].GetValue().GetInt() == 3);
		ASSERT(a.fInputPins[0].fSourceNode == -1);
		ASSERT(a.fInputPins[1].GetValue().GetInt() == 4);
		ASSERT(a.fInputPins[1].fSourceNode == -1);
		ASSERT(a.fInputPins[2].GetValue().GetInt() == 5);
		ASSERT(a.fInputPins[2].fSourceNode == -1);
		ASSERT(a.fType == CEvalNode::kNodeType_Add);
		ASSERT(a.fComparison == CEvalNode::kComparison_ABiggerOrEqualToB);
		ASSERT(!a.fExpressionRef);
		ASSERT(a.fGenerationID == -1);
	}




	void ProveWorks_CProgram_Constructor__1Node__ConstructsOK(){
		TEmptyRuntimeFixture f;

		std::vector<CEvalNode> nodes;
		nodes.push_back(MakeAdd1());
		CProgram p(*f.fRuntime.get(), nodes);
		ASSERT(p.fNodes.size() == 1);
		ASSERT(p.fGenerationID == 1);
	}




	void ProveWorks_CProgram_Evaluate__1Node__CorrectOutputValue(){
		TEmptyRuntimeFixture f;

		std::vector<CEvalNode> nodes;
		nodes.push_back(MakeAdd1());
		CProgram p(*f.fRuntime.get(), nodes);

		ASSERT(p.fNodes[0].fOutputPin.GetValue().IsNil());
		p.Evaluate();
		ASSERT(p.fNodes[0].fOutputPin.GetValue().GetInt() == 12);
	}

	void ProveWorks_CProgram_Evaluate__2NodeSeparate__CorrectOutputValues(){
		TEmptyRuntimeFixture f;

		std::vector<CEvalNode> nodes;
		nodes.push_back(MakeAdd1());
		nodes.push_back(MakeAdd2());
		CProgram p(*f.fRuntime.get(), nodes);

		p.Evaluate();
		ASSERT(p.fNodes[0].fOutputPin.GetValue().GetInt() == 12);
		ASSERT(p.fNodes[1].fOutputPin.GetValue().GetInt() == 21);
	}

	void ProveWorks_CProgram_Evaluate__2NodeWithDependencies__CorrectOutputValues(){
		TEmptyRuntimeFixture f;

		std::vector<CEvalNode> nodes;
		nodes.push_back(MakeAdd1());
		nodes.push_back(MakeAdd2());
		nodes[0].fInputPins[0].fSourceNode = 1;
		CProgram p(*f.fRuntime.get(), nodes);

		p.Evaluate();
		ASSERT(p.fNodes[0].fOutputPin.GetValue().GetInt() == 21 + 9);
		ASSERT(p.fNodes[1].fOutputPin.GetValue().GetInt() == 21);
	}



	void ProveWorks_CEvalNode_MakeConditional__Evaluate_True__BecomesA(){
		TEmptyRuntimeFixture f;

		std::vector<CEvalNode> nodes;

		nodes.push_back(CEvalNode::MakeConditional(CEvalNode::kComparison_QuestionIsTrue, VValue(1), VValue(4), VValue(5)));

		CProgram p(*f.fRuntime.get(), nodes);

		p.Evaluate();
		ASSERT(p.fNodes[0].fOutputPin.GetValue().GetInt() == 4);
	}

	void ProveWorks_CEvalNode_MakeConditional__Evaluate_False__BecomesB(){
		TEmptyRuntimeFixture f;

		std::vector<CEvalNode> nodes;

		nodes.push_back(CEvalNode::MakeConditional(CEvalNode::kComparison_QuestionIsTrue, VValue(3), VValue(4), VValue(5)));

		CProgram p(*f.fRuntime.get(), nodes);

		p.Evaluate();
		ASSERT(p.fNodes[0].fOutputPin.GetValue().GetInt() == 5);
	}

	void ProveWorks_CEvalNode_MakeLooper__BasicConstruction__ConstructionOK(){
		TEmptyRuntimeFixture f;

		std::map<std::string, VValue> values;
		values["1"] = 1000;
		values["2"] = 2000;
		values["3"] = 3000;
		values["4"] = 4000;
		VTableRef table = f.fRuntime->MakeTableWithValues(values);

		std::vector<CEvalNode> nodes;

		VRange range = table.GetRange();
		VExpressionRef expressionRef;

//		nodes.push_back(CEvalNode::MakeLooper(VValue(range), VValue(3), VValue(4), expressionRef));
/*CEvalNode CEvalNode::MakeLooper(const TPin& iRange,
	const TPin& iStartValue,
	const TPin& iContext,
	const TPin& iFunction)*/

		CProgram p(*f.fRuntime.get(), nodes);

		p.Evaluate();
//		ASSERT(p.fNodes[0].fOutputPin.GetValue().GetInt() == 5);
	}



}


void TestProgram(){
	ProveWorks_CEvalNode_MakeConditional__Plain__ConstructsOK();
	ProveWorks_CEvalNode_MakeAdd__Plain__ConstructsOK();
	ProveWorks_CProgram_Constructor__1Node__ConstructsOK();

	ProveWorks_CProgram_Evaluate__1Node__CorrectOutputValue();
	ProveWorks_CProgram_Evaluate__2NodeSeparate__CorrectOutputValues();
	ProveWorks_CProgram_Evaluate__2NodeWithDependencies__CorrectOutputValues();

	ProveWorks_CEvalNode_MakeConditional__Evaluate_True__BecomesA();
	ProveWorks_CEvalNode_MakeConditional__Evaluate_False__BecomesB();

	ProveWorks_CEvalNode_MakeLooper__BasicConstruction__ConstructionOK();
}


