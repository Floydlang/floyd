//
//  Program.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2013-07-18.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "Quark.h"

#include "Program.h"





//////////////////////////			CEvaluationNode




CEvaluationNode::CEvaluationNode(ENodeType iType) :
	fType(iType),
	fComparison(kComparison_ABiggerOrEqualToB),
	fPerEntryExpression(NULL),
	fGenerationID(-1)
{

	ASSERT(CheckInvariant());
}

CEvaluationNode::~CEvaluationNode(){
	ASSERT(CheckInvariant());
}

bool CEvaluationNode::CheckInvariant() const{
	ASSERT(this != NULL);
	return true;
}

CEvaluationNode CEvaluationNode::MakeConditional(const TPin iInputPins[3], EComparison comparison){
	CEvaluationNode result(kNodeType_Conditional);
	result.fComparison = comparison;
	result.fInputPins = std::vector<TPin>(&iInputPins[0], &iInputPins[3]);
	ASSERT(result.CheckInvariant());
	return result;
}

CEvaluationNode CEvaluationNode::MakeAdd(const TPin iInputPins[3]){
	CEvaluationNode result(kNodeType_Add);
	result.fInputPins = std::vector<TPin>(&iInputPins[0], &iInputPins[3]);
	ASSERT(result.CheckInvariant());
	return result;
}



//////////////////////////			CProgram



CProgram::CProgram(CRuntime& iRuntime, const std::vector<CEvaluationNode>& iNodes) :
	fRuntime(&iRuntime),
	fNodes(iNodes),
	fGenerationID(1)
{
	ASSERT(CheckInvariant());
}

CProgram::~CProgram(){
	ASSERT(CheckInvariant());
}

bool CProgram::CheckInvariant() const{
	ASSERT(this != NULL);

	for(auto it = fNodes.begin() ; it != fNodes.end() ; it++){
		ASSERT(it->CheckInvariant());
	}

	return true;
}

void CProgram::TouchPin(long iNodeIndex, TPin& iPin, long iGenerationID){
	if(iPin.fSourceNode != -1){
		EvaluateNodeRecursively(iPin.fSourceNode, iGenerationID);
		iPin.fCurrentValue = fNodes[iPin.fSourceNode].fOutputPin.fCurrentValue;
	}
}

void CProgram::EvaluateNodeRecursively(long iNodeIndex, long iGenerationID){
	CEvaluationNode& node = fNodes[iNodeIndex];

	if(node.fGenerationID != iGenerationID){
		if(node.fType == CEvaluationNode::kNodeType_Conditional){
		}
		else if(node.fType == CEvaluationNode::kNodeType_Add){
			TInteger sum = 0;
			for(long pinIndex = 0 ; pinIndex < node.fInputPins.size() ; pinIndex++){
				TouchPin(iNodeIndex, node.fInputPins[pinIndex], iGenerationID);
				VValue v = node.fInputPins[pinIndex].fCurrentValue;
				if(v.IsInt()){
					TInteger a = v.GetInt();
					sum += a;
				}
			}
			node.fOutputPin.fCurrentValue = VValue::MakeInt(sum);
		}
		else{
			ASSERT(false);
		}
		node.fGenerationID = iGenerationID;
	}
}

void CProgram::Evaluate(){
	ASSERT(CheckInvariant());

	fGenerationID++;

	//	MZ: Execute *all* nodes.
	for(long nodeIndex = 0 ; nodeIndex < fNodes.size() ; nodeIndex++){
		EvaluateNodeRecursively(nodeIndex, fGenerationID);
	}
}


