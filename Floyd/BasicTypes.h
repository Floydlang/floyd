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



////////////////////////		Basic types



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
	kType_ValueObjectRef,
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
	public: CTableRecord(const std::map<std::string, VValue>& iValues);
	public: bool CheckInvariant() const;


	///////////////////////		State.
		public: std::map<std::string, VValue> fValues;
		public: long fRefCount;
};




////////////////////////		VTableRef




class VTableRef {
	public: VTableRef(const VTableRef& iOther);
	public: VTableRef& operator=(const VTableRef& iOther);
	public: ~VTableRef();
	public: bool CheckInvariant() const;
	public: void Swap(VTableRef& ioOther);

	public: void Set(const std::string& iKey, const VValue& iValue);
	public: VValue Get(const std::string& iKey) const;
	public: long GetSize() const;

	//	MZ: Will increment reference counter inside record.
	public: VTableRef(CTableRecord* iRecord);


	friend bool operator==(const VTableRef& iA, const VTableRef& iB);

	///////////////////////		State.
		private: CTableRecord* fRecord;
};

bool operator==(const VTableRef& iA, const VTableRef& iB);



////////////////////////		VValueObjectMeta




class VValueObjectMeta {
	public: VValueObjectMeta(const std::vector<VMemberMeta>& iMemberMetas);
	public: VValueObjectMeta(const VValueObjectMeta& iOther);
	public: VValueObjectMeta& operator=(const VValueObjectMeta& iOther);
	public: void Swap(VValueObjectMeta& ioOther);
	public: bool CheckInvariant() const;
	public: long CountMembers() const;
	public: const VMemberMeta& GetMember(long iIndex) const;

	friend bool operator==(const VValueObjectMeta& iA, const VValueObjectMeta& iB);

	///////////////////////		State.
		private: std::vector<VMemberMeta> fMemberMetas;
};

bool operator==(const VValueObjectMeta& iA, const VValueObjectMeta& iB);







////////////////////////		TValueObjectState



class TValueObjectState {
	public: TValueObjectState(const VValueObjectMeta& iMeta, const std::vector<VValue>& iValues);
	public: TValueObjectState(const TValueObjectState& iOther);
	private: TValueObjectState& operator=(const TValueObjectState& iOther);
	public: ~TValueObjectState();
	public: bool CheckInvariant() const;


	///////////////////////		State.
		public: const VValueObjectMeta* fMeta;
		public: const std::vector<VValue> fValues;
};

bool operator==(const TValueObjectState& iA, const TValueObjectState& iB);
inline bool operator!=(const TValueObjectState& iA, const TValueObjectState& iB){	return !(iA == iB); };




////////////////////////		CValueObjectRecord




class CValueObjectRecord {
	public: CValueObjectRecord(const TValueObjectState& iState);
	public: bool CheckInvariant() const;

	private: CValueObjectRecord(const CValueObjectRecord& iOther);
	private: CValueObjectRecord& operator=(const CValueObjectRecord& iOther);

	///////////////////////		State.
		public: TValueObjectState fState;
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
		int64_t fInt;
//		double fFloat;
//		int32_t fEnum;
		std::string fMachineStringRef;
		std::shared_ptr<VTableRef> fTableRef;
		std::shared_ptr<VValueObjectRef> fValueObjectRef;
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
	public: static VValue MakeInt(int64_t iInt);
	public: int64_t GetInt() const;

	public: static VValue MakeMachineString(const std::string& iMachineString);
	public: std::string GetMachineString() const;

	public: bool IsTableRef() const;
	public: static VValue MakeTableRef(const VTableRef& iTableRef);
	public: VTableRef GetTableRef() const;

	public: bool IsValueObjectRef() const;
	public: static VValue MakeValueObjectRef(const VValueObjectRef& iValueObjectRef);
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
	public: std::vector<VValueObjectMeta> GetValueObjectMetaCopy() const;
	public: bool CheckInvariant() const;

	friend CStaticDefinition* MakeStaticDefinition(const CStaticRegitrat& iRegistrat);

	///////////////////////		State.
		private: std::vector<VValueObjectMeta> fValueObjectMetas;
};



//////////////////////////			CRuntime




class CRuntime {
	public: ~CRuntime();
	public: bool CheckInvariant() const;

	private: CRuntime& operator=(const CRuntime& iOther);
	private: CRuntime(const CRuntime& iOther);

	public: VValueObjectRef MakeValueObject(const TValueObjectState& iState);
	public: VValueObjectRef MakeValueObject(const VValueObjectMeta& iType, const std::vector<VValue>& iValues);

	public: VTableRef MakeEmptyTable();

	private: void FlushUnusedValueObjects();

	public: CRuntime(const CStaticDefinition& iStaticDefinition);


	///////////////////////		State.
		public: const CStaticDefinition* fStaticDefinition;

		//### Use hash of TValueObjectState as the key!
		public: std::vector<std::shared_ptr<CValueObjectRecord> > fValueObjectRecords;

		public: std::vector<std::shared_ptr<CTableRecord> > fTableRecords;
};




//////////////////////////			Free functions




CStaticDefinition* MakeStaticDefinition(const CStaticRegitrat& iRegistrat);

CRuntime* MakeRuntime(const CStaticDefinition& iStaticDefinition);





#endif /* defined(__Floyd__BasicTypes__) */
