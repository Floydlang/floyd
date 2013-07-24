//
//  Program.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2013-07-18.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "Quark.h"

#include "Program.h"





////////////////////////			TPin



TPin::TPin() :
	fSourceNode(-1)
{
	ASSERT(CheckInvariant());
}

TPin::TPin(const VValue& iCurrentValue) :
	fCurrentValue(iCurrentValue),
	fSourceNode(-1)
{
	ASSERT(iCurrentValue.CheckInvariant());
	ASSERT(CheckInvariant());
}

TPin::TPin(const VValue& iCurrentValue, long iSourceNode) :
	fCurrentValue(iCurrentValue),
	fSourceNode(iSourceNode)
{
	ASSERT(iCurrentValue.CheckInvariant());
	ASSERT(CheckInvariant());
}

bool TPin::CheckInvariant() const{
	ASSERT(this != NULL);

	ASSERT(fCurrentValue.CheckInvariant());
	return true;
}

VValue TPin::GetValue() const{
	ASSERT(CheckInvariant());

	return fCurrentValue;
}

void TPin::StoreValue(const VValue& iValue){
	ASSERT(CheckInvariant());
	ASSERT(iValue.CheckInvariant());

	fCurrentValue = iValue;

	ASSERT(CheckInvariant());
}



//////////////////////////			CEvalNode




CEvalNode::CEvalNode(ENodeType iType) :
	fType(iType),
	fComparison(kComparison_ABiggerOrEqualToB),
	fGenerationID(-1)
{

	ASSERT(CheckInvariant());
}

CEvalNode::~CEvalNode(){
	ASSERT(CheckInvariant());
}

bool CEvalNode::CheckInvariant() const{
	ASSERT(this != NULL);
	return true;
}

CEvalNode CEvalNode::MakeConditional(EComparison comparison, const TPin iQuestion, const TPin iA, const TPin iB){
	CEvalNode result(kNodeType_Conditional);
	result.fComparison = comparison;
	result.fInputPins.push_back(iQuestion);
	result.fInputPins.push_back(iA);
	result.fInputPins.push_back(iB);
	ASSERT(result.CheckInvariant());
	return result;
}

CEvalNode CEvalNode::MakeConditional(EComparison comparison, const TPin iInputPins[3]){
	CEvalNode result(kNodeType_Conditional);
	result.fComparison = comparison;
	result.fInputPins = std::vector<TPin>(&iInputPins[0], &iInputPins[3]);
	ASSERT(result.CheckInvariant());
	return result;
}

CEvalNode CEvalNode::MakeLooper(const TPin& iRange,
	const TPin& iStartValue,
	const TPin& iContext,
	const VExpressionRef& iExpressionRef)
{
	CEvalNode result(kNodeType_Looper);
	result.fInputPins.push_back(iRange);
	result.fInputPins.push_back(iStartValue);
	result.fInputPins.push_back(iContext);
	result.fExpressionRef.reset(new VExpressionRef(iExpressionRef));
	ASSERT(result.CheckInvariant());
	return result;
}

CEvalNode CEvalNode::MakeAdd(const TPin& iA, const TPin& iB, const TPin& iC){
	CEvalNode result(kNodeType_Add);
	result.fInputPins.push_back(iA);
	result.fInputPins.push_back(iB);
	result.fInputPins.push_back(iC);
	ASSERT(result.CheckInvariant());
	return result;
}

CEvalNode CEvalNode::MakeAdd(const TPin iInputPins[3]){
	CEvalNode result(kNodeType_Add);
	result.fInputPins = std::vector<TPin>(&iInputPins[0], &iInputPins[3]);
	ASSERT(result.CheckInvariant());
	return result;
}



//////////////////////////			CProgram



CProgram::CProgram(CRuntime& iRuntime, const std::vector<CEvalNode>& iNodes) :
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

void CProgram::EvaluatePin(long iNodeIndex, TPin& iPin, long iGenerationID){
	if(iPin.fSourceNode != -1){
		EvaluateNodeRecursively(iPin.fSourceNode, iGenerationID);
		VValue newValue = fNodes[iPin.fSourceNode].fOutputPin.GetValue();
		iPin.StoreValue(newValue);
	}
}

bool IsTrue(const VValue& iValue){
	if(iValue.IsNil()){
		return false;
	}
	else if(iValue.IsInt()){
		return iValue.GetInt() == 1 ? true : false;
	}
	else if(iValue.IsTableRef()){
		return false;
	}
	else if(iValue.IsValueObjectRef()){
		return false;
	}
	else{
		return false;
		ASSERT(false);
	}
}

void CProgram::EvaluateNodeRecursively(long iNodeIndex, long iGenerationID){
	CEvalNode& node = fNodes[iNodeIndex];

	if(node.fGenerationID != iGenerationID){
		VValue newValue;

		if(node.fType == CEvalNode::kNodeType_Conditional){
			if(node.fComparison == CEvalNode::kComparison_QuestionIsTrue){
				EvaluatePin(iNodeIndex, node.fInputPins[0], iGenerationID);
				EvaluatePin(iNodeIndex, node.fInputPins[1], iGenerationID);
				EvaluatePin(iNodeIndex, node.fInputPins[2], iGenerationID);
				if(IsTrue(node.fInputPins[0].GetValue())){
					newValue = node.fInputPins[1].GetValue();
				}
				else{
					newValue = node.fInputPins[2].GetValue();
				}
			}

			else if (node.fComparison == CEvalNode::kComparison_QuestionIsFalse){
				ASSERT(false);
			}
			else if (node.fComparison == CEvalNode::kComparison_ALessThanB){
				ASSERT(false);
			}
			else if (node.fComparison == CEvalNode::kComparison_ALessOrEqualB){
				ASSERT(false);
			}
			else if (node.fComparison == CEvalNode::kComparison_ABiggerThanB){
				ASSERT(false);
			}
			else if (node.fComparison == CEvalNode::kComparison_ABiggerOrEqualToB){
				ASSERT(false);
			}
			else if (node.fComparison == CEvalNode::kComparison_AEqualToB){
				ASSERT(false);
			}
			else{
				ASSERT(false);
			}
		}
		else if(node.fType == CEvalNode::kNodeType_Looper){
			EvaluatePin(iNodeIndex, node.fInputPins[0], iGenerationID);
			EvaluatePin(iNodeIndex, node.fInputPins[1], iGenerationID);
			EvaluatePin(iNodeIndex, node.fInputPins[2], iGenerationID);
			EvaluatePin(iNodeIndex, node.fInputPins[3], iGenerationID);

			VValue range = node.fInputPins[0].GetValue();
			VValue startValue = node.fInputPins[1].GetValue();
			VValue context = node.fInputPins[2].GetValue();
			VValue function = node.fInputPins[3].GetValue();
			VValue v = startValue;
			if(function.IsNil() || range.GetRange().IsEmpty()){
			}
			else{
			}
			newValue = v;
		}
		else if(node.fType == CEvalNode::kNodeType_Add){
			TInteger sum = 0;
			for(long pinIndex = 0 ; pinIndex < node.fInputPins.size() ; pinIndex++){
				EvaluatePin(iNodeIndex, node.fInputPins[pinIndex], iGenerationID);
				VValue v = node.fInputPins[pinIndex].GetValue();
				if(v.IsInt()){
					TInteger a = v.GetInt();
					sum += a;
				}
			}
			newValue = VValue::MakeInt(sum);
		}
		else{
			ASSERT(false);
		}
		node.fOutputPin.StoreValue(newValue);
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


