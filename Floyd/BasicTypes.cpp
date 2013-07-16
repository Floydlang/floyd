//
//  BasicTypes.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2013-07-04.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "BasicTypes.h"

#include <cassert>

#include "Quark.h"






TInternalValue::TInternalValue() :
	fInt(666)
{
	ASSERT(CheckInvariant());
}

TInternalValue::TInternalValue(const TInternalValue& iOther) :
	fInt(iOther.fInt),
	fMachineStringRef(iOther.fMachineStringRef),
	fTableRef(iOther.fTableRef),
	fValueObjectRef(iOther.fValueObjectRef)
//	fMotherboardRef(iOther.fMotherboardRef),
//	fInterfaceRef(iOther.fInterfaceRef)
{

	ASSERT(CheckInvariant());
}

TInternalValue& TInternalValue::operator=(const TInternalValue& iOther){
	ASSERT(CheckInvariant());
	ASSERT(iOther.CheckInvariant());

	TInternalValue temp = iOther;

	temp.Swap(*this);

	ASSERT(CheckInvariant());
	ASSERT(iOther.CheckInvariant());

	return *this;
}

void TInternalValue::Swap(TInternalValue& ioOther){
	ASSERT(CheckInvariant());
	ASSERT(ioOther.CheckInvariant());

	std::swap(fInt, ioOther.fInt);
	fMachineStringRef.swap(ioOther.fMachineStringRef);
	fTableRef.swap(ioOther.fTableRef);
	fValueObjectRef.swap(ioOther.fValueObjectRef);

	ASSERT(CheckInvariant());
	ASSERT(ioOther.CheckInvariant());
}

bool TInternalValue::CheckInvariant() const{
	ASSERT(this != NULL);

	return true;
}




////////////////////////		VValue




VValue::VValue() :
	fType(kType_Nil)
{
	ASSERT(CheckInvariant());
}

VValue::VValue(const VValue& iOther) :
	fType(iOther.fType),
	fInternalValue(iOther.fInternalValue)
{
	ASSERT(iOther.CheckInvariant());

	ASSERT(CheckInvariant());
}

VValue& VValue::operator=(const VValue& iOther){
	ASSERT(CheckInvariant());
	ASSERT(iOther.CheckInvariant());

	VValue temp(iOther);
	temp.Swap(*this);

	ASSERT(CheckInvariant());
	ASSERT(iOther.CheckInvariant());

	return *this;
}

VValue::~VValue(){
	ASSERT(CheckInvariant());
}

void VValue::Swap(VValue& ioOther){
	ASSERT(CheckInvariant());
	ASSERT(ioOther.CheckInvariant());

	std::swap(fType, ioOther.fType);
	fInternalValue.Swap(ioOther.fInternalValue);

	ASSERT(CheckInvariant());
	ASSERT(ioOther.CheckInvariant());
}

bool VValue::CheckInvariant() const{
	ASSERT(this != NULL);

	if(fType == kType_Nil){
	}
	else if(fType == kType_Int){
	}
	else if(fType == kType_MachineStringRef){
	}
	else if(fType == kType_TableRef){
		//### MZ: MOre!
		ASSERT(fInternalValue.fTableRef.get() != NULL);
	}
	else if(fType == kType_ValueObjectRef){
		//### MZ: MOre!
		ASSERT(fInternalValue.fValueObjectRef.get() != NULL);
	}
	else{
		ASSERT(false);
	}

	return true;
}

EValueType VValue::GetType() const{
	ASSERT(CheckInvariant());

	return fType;
}

bool VValue::IsNil() const{
	ASSERT(CheckInvariant());

	return GetType() == kType_Nil;
}


bool VValue::IsInt() const {
	ASSERT(CheckInvariant());

	return GetType() == kType_Int;
}

VValue VValue::MakeInt(int64_t iInt){
	VValue temp;
	temp.fInternalValue.fInt = iInt;
	temp.fType = kType_Int;

	ASSERT(temp.CheckInvariant());
	return temp;
}

int64_t VValue::GetInt() const{
	ASSERT(CheckInvariant());
	ASSERT(fType == kType_Int);

	return fInternalValue.fInt;
}

VValue VValue::MakeMachineString(const std::string& iMachineString){
	VValue temp;
	temp.fInternalValue.fMachineStringRef = iMachineString;
	temp.fType = kType_MachineStringRef;

	ASSERT(temp.CheckInvariant());
	return temp;
}

std::string VValue::GetMachineString() const{
	ASSERT(CheckInvariant());
	ASSERT(fType == kType_MachineStringRef);

	return fInternalValue.fMachineStringRef;
}


bool VValue::IsTableRef() const{
	ASSERT(CheckInvariant());

	return fType == kType_TableRef;
}

VValue VValue::MakeTableRef(const VTableRef& iTableRef){
	ASSERT(iTableRef.CheckInvariant());

	std::shared_ptr<VTableRef> temp(new VTableRef(iTableRef));

	VValue result;
	result.fInternalValue.fTableRef = temp;
	result.fType = kType_TableRef;

	ASSERT(result.CheckInvariant());
	return result;
}

VTableRef VValue::GetTableRef() const{
	ASSERT(CheckInvariant());
	ASSERT(fType == kType_TableRef);

	return *fInternalValue.fTableRef.get();
}




bool VValue::IsValueObjectRef() const{
	ASSERT(CheckInvariant());

	return fType == kType_ValueObjectRef;
}

VValue VValue::MakeValueObjectRef(const VValueObjectRef& iValueObjectRef){
	ASSERT(iValueObjectRef.CheckInvariant());

	std::shared_ptr<VValueObjectRef> temp(new VValueObjectRef(iValueObjectRef));

	VValue result;
	result.fInternalValue.fValueObjectRef = temp;
	result.fType = kType_ValueObjectRef;

	ASSERT(result.CheckInvariant());
	return result;
}

VValueObjectRef VValue::GetValueObjectRef() const{
	ASSERT(CheckInvariant());
	ASSERT(fType == kType_ValueObjectRef);

	return *fInternalValue.fValueObjectRef.get();
}

/*
VValue VValue::MakeMotherboardRef(const VMotherboardRef& iMotherboardRef){
	VValue temp;
	temp.fInternalValue.fMotherboardRef = iMotherboardRef;
	temp.fType = kType_MotherboardRef;

	ASSERT(temp.CheckInvariant());
	return temp;
}

VMotherboardRef VValue::GetMotherboardRef() const{
	ASSERT(CheckInvariant());
	ASSERT(fType == kType_MotherboardRef);

	return fInternalValue.fMotherboardRef;
}
*/


bool operator==(const VValue& iA, const VValue& iB){
	ASSERT(iA.CheckInvariant());
	ASSERT(iB.CheckInvariant());

	if(iA.GetType() != iB.GetType()){
		return false;
	}

	if(iA.GetType() == kType_Nil){
		return true;
	}
	else if(iA.GetType() == kType_Int){
		if(iA.GetInt() != iB.GetInt()){
			return false;
		}
	}
	else if(iA.GetType() == kType_MachineStringRef){
		if(iA.GetMachineString() != iB.GetMachineString()){
			return false;
		}
	}
	else if(iA.GetType() == kType_MachineStringRef){
		if(iA.GetMachineString() != iB.GetMachineString()){
			return false;
		}
	}
	return true;
}





////////////////////////		VMemberMeta



VMemberMeta::VMemberMeta(const std::string& iKey, EValueType iType) :
	fKey(iKey),
	fType(iType)
{
	ASSERT(iKey.size() > 0);

	ASSERT(CheckInvariant());
}

bool VMemberMeta::CheckInvariant() const{
	ASSERT(this != NULL);
	ASSERT(fKey.size() > 0);

	return true;
}

bool operator==(const VMemberMeta& iA, const VMemberMeta& iB){
	ASSERT(iA.CheckInvariant());
	ASSERT(iB.CheckInvariant());

	if(iA.fKey != iB.fKey){
		return false;
	}
	if(iA.fType != iB.fType){
		return false;
	}
	return true;
}






////////////////////////		CTableRecord




CTableRecord::CTableRecord(const std::map<std::string, VValue>& iValues) :
	fValues(iValues),
	fRefCount(0)
{
}

bool CTableRecord::CheckInvariant() const{
	ASSERT(this != NULL);

	return true;
}




////////////////////////		VTableRef



//	MZ: Will increment reference counter inside record.
VTableRef::VTableRef(CTableRecord* iRecord) :
	fRecord(iRecord)
{
	fRecord->fRefCount++;

	ASSERT(CheckInvariant());
}

VTableRef::VTableRef(const VTableRef& iOther) :
	fRecord(iOther.fRecord)
{
	ASSERT(iOther.CheckInvariant());

	fRecord->fRefCount++;

	ASSERT(CheckInvariant());
}

VTableRef& VTableRef::operator=(const VTableRef& iOther){
	ASSERT(CheckInvariant());
	ASSERT(iOther.CheckInvariant());

	VTableRef temp(iOther);
	temp.Swap(*this);

	return *this;
}


VTableRef::~VTableRef(){
	ASSERT(CheckInvariant());

	fRecord->fRefCount--;
}

bool VTableRef::CheckInvariant() const{
	ASSERT(this != NULL);

	ASSERT(fRecord != NULL);
	ASSERT(fRecord->CheckInvariant());
	ASSERT(fRecord->fRefCount >= 1);

	return true;
}

void VTableRef::Swap(VTableRef& ioOther){
	ASSERT(CheckInvariant());
	ASSERT(ioOther.CheckInvariant());

	std::swap(fRecord, ioOther.fRecord);

	ASSERT(CheckInvariant());
	ASSERT(ioOther.CheckInvariant());
}

void VTableRef::Set(const std::string& iKey, const VValue& iValue){
	ASSERT(CheckInvariant());
	ASSERT(iKey.size() > 0);
	ASSERT(iValue.CheckInvariant());

	fRecord->fValues[iKey] = iValue;

	ASSERT(CheckInvariant());
}

VValue VTableRef::Get(const std::string& iKey) const{
	ASSERT(CheckInvariant());
	ASSERT(iKey.size() > 0);

	VValue result = fRecord->fValues[iKey];

	return result;
}

long VTableRef::GetSize() const{
	ASSERT(CheckInvariant());

	return fRecord->fValues.size();
}


bool operator==(const VTableRef& iA, const VTableRef& iB){
	ASSERT(iA.CheckInvariant());
	ASSERT(iB.CheckInvariant());

	if(iA.fRecord == iB.fRecord){
		return true;
	}
	return false;
}



////////////////////////		VValueObjectMeta



bool operator==(const VValueObjectMeta& iA, const VValueObjectMeta& iB){
	ASSERT(iA.CheckInvariant());
	ASSERT(iB.CheckInvariant());

	return iA.fMemberMetas == iB.fMemberMetas;
}

VValueObjectMeta::VValueObjectMeta(const std::vector<VMemberMeta>& iMemberMetas) :
	fMemberMetas(iMemberMetas)
{
#if DEBUG
	for(auto it = iMemberMetas.begin() ; it != iMemberMetas.end() ; it++){
		ASSERT(it->fKey.size() > 0);
	}
#endif

	ASSERT(CheckInvariant());
}

VValueObjectMeta::VValueObjectMeta(const VValueObjectMeta& iOther) :
	fMemberMetas(iOther.fMemberMetas)
{
	ASSERT(iOther.CheckInvariant());

	ASSERT(CheckInvariant());
}

VValueObjectMeta& VValueObjectMeta::operator=(const VValueObjectMeta& iOther){
	ASSERT(CheckInvariant());
	ASSERT(iOther.CheckInvariant());

	VValueObjectMeta temp(iOther);
	temp.Swap(*this);

	ASSERT(CheckInvariant());
	ASSERT(iOther.CheckInvariant());
	return *this;
}

void VValueObjectMeta::Swap(VValueObjectMeta& ioOther){
	ASSERT(CheckInvariant());
	ASSERT(ioOther.CheckInvariant());

	fMemberMetas.swap(ioOther.fMemberMetas);

	ASSERT(CheckInvariant());
	ASSERT(ioOther.CheckInvariant());
}

bool VValueObjectMeta::CheckInvariant() const{
	ASSERT(this != NULL);

	for(auto it = fMemberMetas.begin() ; it != fMemberMetas.end() ; it++){
		ASSERT(it->fKey.size() > 0);
	}

	return true;
}

long VValueObjectMeta::CountMembers() const{
	ASSERT(CheckInvariant());

	return fMemberMetas.size();
}

const VMemberMeta& VValueObjectMeta::GetMember(long iIndex) const{
	ASSERT(CheckInvariant());

	return fMemberMetas[iIndex];
}




////////////////////////		TValueObjectState




TValueObjectState::TValueObjectState(const VValueObjectMeta& iMeta, const std::vector<VValue>& iValues) :
	fMeta(&iMeta),
	fValues(iValues)
{
	ASSERT(iMeta.CheckInvariant());

	ASSERT(iValues.size() == iMeta.CountMembers());

#if DEBUG
	for(long i = 0 ; i < iValues.size() ; i++){
		ASSERT(iValues[i].CheckInvariant());
		ASSERT(iValues[i].GetType() == iMeta.GetMember(i).fType);
	}
#endif

	ASSERT(CheckInvariant());
}

TValueObjectState::TValueObjectState(const TValueObjectState& iOther) :
	fMeta(iOther.fMeta),
	fValues(iOther.fValues)
{
	ASSERT(iOther.CheckInvariant());

	ASSERT(CheckInvariant());
}

TValueObjectState::~TValueObjectState(){
	ASSERT(CheckInvariant());
}

bool TValueObjectState::CheckInvariant() const{
	ASSERT(this != NULL);
	ASSERT(fMeta != NULL);
	ASSERT(fMeta->CheckInvariant());

	ASSERT(fValues.size() == fMeta->CountMembers());

	for(long i = 0 ; i < fValues.size() ; i++){
		ASSERT(fValues[i].CheckInvariant());
		ASSERT(fValues[i].GetType() == fMeta->GetMember(i).fType);
	}

	return true;
}

/*
bool operator<(const TValueObjectState& iA, const TValueObjectState& iB){
	ASSERT(iA.CheckInvariant());
	ASSERT(iB.CheckInvariant());

	if(iA.fMeta < iB.fMeta){
		return true;
	}
	else if(iA.fValues.size() < iB.fValues.size()){
		return true;
	}
	else{
		return false;
	}
}
*/
bool operator==(const TValueObjectState& iA, const TValueObjectState& iB){
	ASSERT(iA.CheckInvariant());
	ASSERT(iB.CheckInvariant());

	if(iA.fMeta != iB.fMeta){
		return false;
	}

	if(iA.fValues.size() != iB.fValues.size()){
		return false;
	}

	for(long i = 0 ; i < iA.fValues.size() ; i++){
		if(iA.fValues[i] != iB.fValues[i]){
			return false;
		}
	}

	return true;
}


////////////////////////		CValueObjectRecord



CValueObjectRecord::CValueObjectRecord(const TValueObjectState& iState) :
	fState(iState),
	fRefCount(0)
{
	ASSERT(iState.CheckInvariant());

	ASSERT(CheckInvariant());
}

bool CValueObjectRecord::CheckInvariant() const{
	ASSERT(this != NULL);
	ASSERT(fRefCount >= 0);
	ASSERT(fState.CheckInvariant());

	return true;
}


////////////////////////		VValueObjectRef





VValueObjectRef::VValueObjectRef(const VValueObjectRef& iOther)
:
	fRecord(iOther.fRecord)
{
	ASSERT(iOther.CheckInvariant());
	ASSERT(CheckInvariant());

	fRecord->fRefCount++;

	ASSERT(CheckInvariant());
}

VValueObjectRef::~VValueObjectRef(){
	ASSERT(CheckInvariant());

	fRecord->fRefCount--;
}

bool VValueObjectRef::CheckInvariant() const{
	ASSERT(this != NULL);
	ASSERT(fRecord != NULL);
	ASSERT(fRecord->fRefCount >= 1);
	return true;
}

VValueObjectRef& VValueObjectRef::operator=(const VValueObjectRef& iOther){
	ASSERT(CheckInvariant());
	ASSERT(iOther.CheckInvariant());

	VValueObjectRef temp = iOther;
	temp.Swap(*this);

	ASSERT(CheckInvariant());
	ASSERT(iOther.CheckInvariant());

	return *this;
}

void VValueObjectRef::Swap(VValueObjectRef& ioOther){
	ASSERT(CheckInvariant());
	ASSERT(ioOther.CheckInvariant());

	std::swap(fRecord, ioOther.fRecord);

	ASSERT(CheckInvariant());
	ASSERT(ioOther.CheckInvariant());
}

VValueObjectRef::VValueObjectRef(CValueObjectRecord* iRecord) :
	fRecord(iRecord)
{
	fRecord->fRefCount++;

	ASSERT(CheckInvariant());
}

VValue VValueObjectRef::GetMember(const std::string& iKey) const{
	ASSERT(CheckInvariant());
	ASSERT(iKey.size() > 0);

	long memberIndex = 0;
	long memberCount = fRecord->fState.fMeta->CountMembers();
	while(memberIndex < memberCount && fRecord->fState.fMeta->GetMember(memberIndex).fKey != iKey){
		memberIndex++;
	}

	VValue result;
	if(memberIndex == memberCount){
		ASSERT(false);
	}
	else{
		result = fRecord->fState.fValues[memberIndex];
	}
	ASSERT(result.CheckInvariant());

	return result;
}



//////////////////////////			CStaticRegitrat



void CStaticRegitrat::Register(const VValueObjectMeta& iMeta){
	ASSERT(CheckInvariant());
	ASSERT(iMeta.CheckInvariant());

	fValueObjectMetas.push_back(iMeta);

	ASSERT(CheckInvariant());
}

bool CStaticRegitrat::CheckInvariant() const{
	ASSERT(this != NULL);

	return true;
}


//////////////////////////			CStaticDefinition


const VValueObjectMeta* CStaticDefinition::LookupValueObjectMetaFromIndex(long iMetaIndex) const{
	ASSERT(CheckInvariant());
	ASSERT(iMetaIndex >= 0);
	ASSERT(iMetaIndex < fValueObjectMetas.size());

	return &fValueObjectMetas[iMetaIndex];
}

std::vector<VValueObjectMeta> CStaticDefinition::GetValueObjectMetaCopy() const{
	ASSERT(CheckInvariant());

	return fValueObjectMetas;
}

bool CStaticDefinition::CheckInvariant() const{
	ASSERT(this != NULL);

	return true;
}







//////////////////////////			CRuntime




CRuntime::~CRuntime(){
	ASSERT(CheckInvariant());

	FlushUnusedValueObjects();

	ASSERT(fValueObjectRecords.empty());
}



bool CRuntime::CheckInvariant() const{
	ASSERT(this != NULL);

	for(auto it = fValueObjectRecords.begin() ; it != fValueObjectRecords.end() ; it++){
		CValueObjectRecord& record = *it->get();
		ASSERT(record.CheckInvariant());
	}
	return true;
}

CRuntime::CRuntime(const CStaticDefinition& iStaticDefinition) :
	fStaticDefinition(&iStaticDefinition)
{
	ASSERT(iStaticDefinition.CheckInvariant());

	ASSERT(CheckInvariant());
}

VValueObjectRef CRuntime::MakeValueObject(const TValueObjectState& iState){
	ASSERT(CheckInvariant());
	ASSERT(iState.CheckInvariant());

	VValueObjectRef result = MakeValueObject(*iState.fMeta, iState.fValues);
	ASSERT(result.CheckInvariant());

	ASSERT(CheckInvariant());
	return result;
}

VValueObjectRef CRuntime::MakeValueObject(const VValueObjectMeta& iType, const std::vector<VValue>& iValues){
	ASSERT(CheckInvariant());
	ASSERT(iType.CheckInvariant());

	TValueObjectState state(iType, iValues);

	CValueObjectRecord* record = NULL;

	auto it = fValueObjectRecords.begin();
	while(it != fValueObjectRecords.end() && (*it)->fState != state){
		 it++;
	}

	//	This exact value object does not already exist to be reused - allocate one!
	if(it == fValueObjectRecords.end()){
		std::shared_ptr<CValueObjectRecord> r(new CValueObjectRecord(state));
		record = r.get();
		fValueObjectRecords.push_back(r);
	}

	//	Reuse existing value object.
	else{
		record = it->get();
	}

	ASSERT(record != NULL);
	VValueObjectRef result(record);
	ASSERT(result.CheckInvariant());

	ASSERT(CheckInvariant());

	return result;
}

VTableRef CRuntime::MakeEmptyTable(){
	ASSERT(CheckInvariant());

	std::shared_ptr<CTableRecord> record(new CTableRecord(std::map<std::string, VValue>()));
	fTableRecords.push_back(record);

	VTableRef result(record.get());
	ASSERT(result.CheckInvariant());

	ASSERT(CheckInvariant());

	return result;


}


void CRuntime::FlushUnusedValueObjects(){
	ASSERT(CheckInvariant());

	auto it = fValueObjectRecords.begin();
	while(it != fValueObjectRecords.end()){
		if((*it)->fRefCount == 0){
			it = fValueObjectRecords.erase(it);
		}
	}

	ASSERT(CheckInvariant());
}




//////////////////////////			Free functions




CStaticDefinition* MakeStaticDefinition(const CStaticRegitrat& iRegistrat){
	ASSERT(iRegistrat.CheckInvariant());

	std::auto_ptr<CStaticDefinition> result(new CStaticDefinition());

	result->fValueObjectMetas = iRegistrat.fValueObjectMetas;
	return result.release();
}


CRuntime* MakeRuntime(const CStaticDefinition& iStaticDefinition){
	ASSERT(iStaticDefinition.CheckInvariant());

	std::auto_ptr<CRuntime> result(new CRuntime(iStaticDefinition));

	return result.release();
}





