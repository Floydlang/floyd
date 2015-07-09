//
//  BasicTypes.h
//  Floyd
//
//  Created by Marcus Zetterquist on 2013-07-04.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#ifndef __Floyd__BasicTypes__
#define __Floyd__BasicTypes__

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>



class CStaticDefinition;
class VValue;
class VRange;
class CRuntime;
class VExpressionRef;

typedef int64_t TInteger;



////////////////////////		Basic types

/*
	Unified number
	Composite XYZ (open, static type)
	Tagged union XYZ
	Typedef<T> XYZ
	Vector<T>
	Map<T>
	A Function
	Tuple
	sha1
	enum

	new_type <name> <type>
*/

enum EBasicType {
	kBasicType_Nil	=	4,
//	kBasicType_Boolean,
	kBasicType_Int,
//	kBasicType_Float,
//	kBasicType_Enum,
	kBasicType_MachineStringRef
//	kBasicType_HumanStringRef,
};



////////////////////////		Complex types


enum EValueType {
	kType_Nil	=	4,
//		kType_Boolean,
	kType_Int,
//		kType_Float,
//		kType_Enum,
	kType_MachineStringRef,

	kType_TableRef,
	kType_Range,
	kType_ValueObjectRef,
	kType_ExpressionRef,
//	kType_MotherboardRef,
//	kType_InterfaceRef
};



////////////////////////		VMemberMeta



class VMemberMeta {
	public: VMemberMeta(const std::string& iKey, EValueType iType);
	public: bool CheckInvariant() const;


	///////////////////////		State.
		public: std::string fKey;
		public: EValueType fType;
};

bool operator==(const VMemberMeta& iA, const VMemberMeta& iB);




////////////////////////		CTableRecord




class CTableRecord {
	public: CTableRecord(CRuntime& iRuntime, const std::map<std::string, VValue>& iValues);
	public: bool CheckInvariant() const;


	///////////////////////		State.
		public: std::map<std::string, VValue> fValues;
		public: long fRefCount;
		public: CRuntime* fRuntime;
};




////////////////////////		VTableRef



class VTableRef {
	public: VTableRef(const VTableRef& iOther);
	public: VTableRef& operator=(const VTableRef& iOther);
	public: ~VTableRef();
	public: bool CheckInvariant() const;
	public: void Swap(VTableRef& ioOther);

	//	Returns a modified table. The original table is unmodified.
	public: VTableRef SetCopy(const std::string& iKey, const VValue& iValue) const;
	public: VValue Get(const std::string& iKey) const;
	public: const VValue operator[](const std::string& iKey) const;

	public: long GetSize() const;
	public: VRange GetRange() const;

	//	MZ: Will increment reference counter inside record.
	public: VTableRef(CTableRecord* iRecord);

	public: CTableRecord& GetRecord() const;

	friend bool operator==(const VTableRef& iA, const VTableRef& iB);

	///////////////////////		State.
		private: CTableRecord* fRecord;
};

bool operator==(const VTableRef& iA, const VTableRef& iB);





////////////////////////		VExpressionRef




class VExpressionRef {
//	public: VExpressionRef();

//	CProgram* fProgram;
//	CEvalNode* fOutputNode;
};





////////////////////////		VValueObjectMeta




class VValueObjectMeta {
	public: VValueObjectMeta(const std::string& iTypeName, const std::vector<VMemberMeta>& iMemberMetas);
	public: VValueObjectMeta(const VValueObjectMeta& iOther);
	public: VValueObjectMeta& operator=(const VValueObjectMeta& iOther);
	public: void Swap(VValueObjectMeta& ioOther);
	public: bool CheckInvariant() const;

	public: std::string GetTypeName() const;
	public: long CountMembers() const;
	public: const VMemberMeta& GetMember(long iIndex) const;

	friend bool operator==(const VValueObjectMeta& iA, const VValueObjectMeta& iB);

	///////////////////////		State.
		private: std::string fTypeName;
		private: std::vector<VMemberMeta> fMemberMetas;
};

inline bool operator!=(const VValueObjectMeta& iA, const VValueObjectMeta& iB){	return !(iA == iB); }
bool operator==(const VValueObjectMeta& iA, const VValueObjectMeta& iB);




typedef std::pair<const VValueObjectMeta&, const std::vector<VValue> > TValueObjectState;



////////////////////////		CValueObjectRecord




class CValueObjectRecord {
	public: CValueObjectRecord(const VValueObjectMeta& iMeta, const std::vector<VValue>& iValues);
	public: bool CheckInvariant() const;

	private: CValueObjectRecord(const CValueObjectRecord& iOther);
	private: CValueObjectRecord& operator=(const CValueObjectRecord& iOther);

	///////////////////////		State.
		public: const VValueObjectMeta& fMeta;
		public: const std::vector<VValue> fValues;
		public: long fRefCount;
};



////////////////////////		VValueObjectRef



class VValueObjectRef {
	public: VValueObjectRef(const VValueObjectRef& iOther);
	public: ~VValueObjectRef();
	public: VValueObjectRef& operator=(const VValueObjectRef& iOther);
	public: void Swap(VValueObjectRef& ioOther);
	public: bool CheckInvariant() const;

	public: VValue GetMember(const std::string& iKey) const;

	//	MZ: Will increment reference counter inside record.
	public: VValueObjectRef(CValueObjectRecord* iRecord);

	///////////////////////		State.
		private: CValueObjectRecord* fRecord;
};




////////////////////////		VValue




struct TInternalValue {
	TInternalValue();
	TInternalValue(const TInternalValue& iOther);
	TInternalValue& operator=(const TInternalValue& iOther);
	void Swap(TInternalValue& ioOther);
	public: bool CheckInvariant() const;

	///////////////////////		State.
//		bool fBoolean;
		TInteger fInt;
//		double fFloat;
//		int32_t fEnum;
		std::string fMachineStringRef;
		std::shared_ptr<VTableRef> fTableRef;
		std::shared_ptr<VRange> fRange;
		std::shared_ptr<VValueObjectRef> fValueObjectRef;
		std::shared_ptr<VExpressionRef> fExpressionRef;
//		VMotherboardRef fMotherboardRef;
//		VInterfaceRef fInterfaceRef;
};


//	Can hold any floyd object.
class VValue {
	public: VValue();
	public: VValue(const VValue& iOther);
	public: VValue& operator=(const VValue& iOther);
	public: ~VValue();
	public: void Swap(VValue& ioOther);
	public: bool CheckInvariant() const;

	public: EValueType GetType() const;

	public: bool IsNil() const;

	public: bool IsInt() const;
	public: static VValue MakeInt(TInteger iInt);
	public: VValue(TInteger iInt);
	public: TInteger GetInt() const;

	public: static VValue MakeMachineString(const std::string& iMachineString);
	public: VValue(const std::string& iMachineString);
	public: std::string GetMachineString() const;

	public: bool IsTableRef() const;
	public: static VValue MakeTableRef(const VTableRef& iTableRef);
	public: VValue(const VTableRef& iTableRef);
	public: VTableRef GetTableRef() const;

	public: bool IsRange() const;
	public: static VValue MakeRange(const VRange& iRange);
	public: VValue(const VRange& iRange);
	public: VRange GetRange() const;

	public: bool IsValueObjectRef() const;
	public: static VValue MakeValueObjectRef(const VValueObjectRef& iValueObjectRef);
	public: VValue(const VValueObjectRef& iValueObjectRef);
	public: VValueObjectRef GetValueObjectRef() const;

//	public: VValue MakeMotherboardRef(const VMotherboardRef& iMotherboardRef);
//	public: VMotherboardRef GetMotherboardRef() const;

//	public: VValue MakeInterfaceRef(const VInterfaceRef& iInterfaceRef);
//	public: VInterfaceRef GetInterfaceRef() const;


	////////////////////		State.
		private: EValueType fType;
		private: TInternalValue fInternalValue;
};

bool operator==(const VValue& iA, const VValue& iB);
inline bool operator!=(const VValue& iA, const VValue& iB){	return !(iA == iB);	};





////////////////////////		VRange


/**	@brief The way to iterate over a container.
	@details
		Is always const - can never modify container.
		Owns the container - you can dispose of all other references to the container.
		The order of the elements depends on the container type. A table will return items in key-sort-order.
*/


class VRange {
	public: bool CheckInvariant() const;

//	public: std::size_t GetSize() const;
	public: bool IsEmpty() const;
	public: void PopFront();
	public: const VValue& GetFront() const;

	public: VRange(const VTableRef& iTableRef,
		std::map<std::string, VValue>::const_iterator& iBegin,
		std::map<std::string, VValue>::const_iterator& iEnd);

	///////////////////////		State.
		public: const VTableRef fTableRef;
		public: std::map<std::string, VValue>::const_iterator fBegin;
		public: std::map<std::string, VValue>::const_iterator fEnd;
};




//////////////////////////			CStaticRegitrat




class CStaticRegitrat {
	public: void Register(const VValueObjectMeta& iMeta);
	public: bool CheckInvariant() const;


	friend CStaticDefinition* MakeStaticDefinition(const CStaticRegitrat& iRegistrat);

	///////////////////////		State.
		private: std::vector<VValueObjectMeta> fValueObjectMetas;
};




//////////////////////////			CStaticDefinition



class CStaticDefinition {
	public: const VValueObjectMeta* LookupValueObjectMetaFromIndex(long iMetaIndex) const;
	public: const VValueObjectMeta* LookupValueObjectMetaFromName(const std::string& iTypeName) const;
	public: std::vector<VValueObjectMeta> GetValueObjectMetaCopy() const;


	public: bool CheckInvariant() const;

	friend CStaticDefinition* MakeStaticDefinition(const CStaticRegitrat& iRegistrat);

	///////////////////////		State.
		private: std::vector<VValueObjectMeta> fValueObjectMetas;
};




//////////////////////////			CRuntime




class CRuntime {
	public: CRuntime(const CStaticDefinition& iStaticDefinition);
	public: ~CRuntime();
	public: bool CheckInvariant() const;

	private: CRuntime& operator=(const CRuntime& iOther);
	private: CRuntime(const CRuntime& iOther);

	public: const VValueObjectMeta& LookupValueObjectType(const std::string& iTypeName) const;
	public: VValueObjectRef MakeValueObject(const VValueObjectMeta& iType, const std::vector<VValue>& iValues);

	//	MZ: Will reuse existing empty table, if any.
	public: VTableRef MakeEmptyTable();

	//	MZ: Will reuse existing identical table, if any.
	public: VTableRef MakeTableWithValues(const std::map<std::string, VValue>& iValues);

	private: void FlushUnusedValueObjects();


	///////////////////////		State.
		private: const CStaticDefinition* fStaticDefinition;
		public: std::vector<std::shared_ptr<CValueObjectRecord> > fValueObjectRecords;
		public: std::vector<std::shared_ptr<CTableRecord> > fTableRecords;
};




//////////////////////////			Free functions



CStaticDefinition* MakeStaticDefinition(const CStaticRegitrat& iRegistrat);

CRuntime* MakeRuntime(const CStaticDefinition& iStaticDefinition);





#endif /* defined(__Floyd__BasicTypes__) */
