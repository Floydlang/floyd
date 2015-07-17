//
//  BasicTypesUT.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2013-07-16.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "cpp_extension.h"
#include "BasicTypesUT.h"

#include "BasicTypes.h"


namespace {



	void ProveWorks_VValue_DefaultConstructor__None__ResultIsNil(){
		VValue a;
		ASSERT(a.IsNil());
	}

	void ProveWorks_VValue_MakeInt__Normal__ResultIsCorrectInt(){
		VValue a = VValue::MakeInt(1001);
		ASSERT(a.IsInt());
		ASSERT(a.GetInt() == 1001);
	}

	void ProveWorks_VValue_CopyConstructor__Int__ResultIsCorrectInt(){
		VValue a = VValue::MakeInt(1001);
		VValue b(a);
		ASSERT(b.GetInt() == 1001);
	}

	void ProveWorks_VValue_OperatorEqual__Int__ResultIsCorrectInt(){
		VValue a = VValue::MakeInt(1001);
		VValue b;
		b = a;
		ASSERT(b.GetInt() == 1001);
	}

	void ProveWorks_MakeStaticDefinition__Empty__ResultIsEmpty(){
		CStaticRegitrat registrat;
		std::auto_ptr<CStaticDefinition> def(MakeStaticDefinition(registrat));
		ASSERT(def.get() != NULL);
		ASSERT(def->GetValueObjectMetaCopy().empty());
	}



		VValueObjectMeta MakeValueObjectMeta1(){
			std::vector<VMemberMeta> memberMetas;
			memberMetas.push_back(VMemberMeta("member_a", kType_Int));
			memberMetas.push_back(VMemberMeta("member_b", kType_MachineStringRef));
			memberMetas.push_back(VMemberMeta("member_c", kType_Int));
			memberMetas.push_back(VMemberMeta("member_d", kType_MachineStringRef));

			VValueObjectMeta result("type1", memberMetas);
			return result;
		}

		VValueObjectMeta MakeValueObjectMeta2(){
			std::vector<VMemberMeta > memberMetas;
			memberMetas.push_back(VMemberMeta("member_1", kType_MachineStringRef));
			memberMetas.push_back(VMemberMeta("member_2", kType_ValueObjectRef));
			memberMetas.push_back(VMemberMeta("member_3", kType_MachineStringRef));
			memberMetas.push_back(VMemberMeta("member_4", kType_Int));

			VValueObjectMeta result("type2", memberMetas);
			return result;
		}


	void ProveWorks_MakeStaticDefinition__OneValueObjectMeta__ResultCorrectEntry(){
		CStaticRegitrat registrat;
		registrat.Register(MakeValueObjectMeta1());
		std::auto_ptr<CStaticDefinition> def(MakeStaticDefinition(registrat));
		ASSERT(*def->LookupValueObjectMetaFromIndex(0) == MakeValueObjectMeta1());
	}

	void ProveWorks_MakeStaticDefinition__TwoValueObjectMeta__ResultCorrectEntries(){
		CStaticRegitrat registrat;
		registrat.Register(MakeValueObjectMeta1());
		registrat.Register(MakeValueObjectMeta2());
		std::auto_ptr<CStaticDefinition> def(MakeStaticDefinition(registrat));

		ASSERT(*def->LookupValueObjectMetaFromIndex(0) == MakeValueObjectMeta1());
		ASSERT(*def->LookupValueObjectMetaFromIndex(1) == MakeValueObjectMeta2());
	}



		struct TTestValueObjectsFixture {
			public: TTestValueObjectsFixture(){
				CStaticRegitrat registrat;
				registrat.Register(MakeValueObjectMeta1());
				registrat.Register(MakeValueObjectMeta2());

				fStaticDefinition.reset(MakeStaticDefinition(registrat));
				fRuntime.reset(MakeRuntime(*fStaticDefinition.get()));
			}

			std::auto_ptr<CStaticDefinition> fStaticDefinition;
			std::auto_ptr<CRuntime> fRuntime;
		};





	void ProveWorks_MakeRuntime__NoInstances__ResultsEmptyRuntime(){
		TTestValueObjectsFixture f;
		ASSERT(f.fRuntime->fValueObjectRecords.size() == 0);
	}


		VValueObjectRef MakeValueObjectType1ValueA(CRuntime& iRuntime){
			std::vector<VValue> values;
			values.push_back(VValue::MakeInt(1111));
			values.push_back(VValue::MakeMachineString("2222"));
			values.push_back(VValue::MakeInt(3333));
			values.push_back(VValue::MakeMachineString("4444"));
			TValueObjectState state(iRuntime.LookupValueObjectType("type1"), values);

			VValueObjectRef result = iRuntime.MakeValueObject(state.first, state.second);
			return result;
		}

		VValueObjectRef MakeValueObjectType1ValueB(CRuntime& iRuntime){
			std::vector<VValue> values;
			values.push_back(VValue::MakeInt(55));
			values.push_back(VValue::MakeMachineString("66"));
			values.push_back(VValue::MakeInt(77));
			values.push_back(VValue::MakeMachineString("88"));
			TValueObjectState state(iRuntime.LookupValueObjectType("type1"), values);

			VValueObjectRef result = iRuntime.MakeValueObject(state.first, state.second);
			return result;
		}



	void ProveWorks_MakeRuntime__OneValueObjectRef__ResultsCorrectValueObjectRef(){
		TTestValueObjectsFixture f;

		VValueObjectRef valueObjectRef = MakeValueObjectType1ValueA(*f.fRuntime.get());

		ASSERT(valueObjectRef.GetMember("member_a").GetInt() == 1111);
		ASSERT(valueObjectRef.GetMember("member_b").GetMachineString() == "2222");
		ASSERT(valueObjectRef.GetMember("member_c").GetInt() == 3333);
		ASSERT(valueObjectRef.GetMember("member_d").GetMachineString() == "4444");

		ASSERT(f.fRuntime->fValueObjectRecords.size() == 1);
		ASSERT(f.fRuntime->fValueObjectRecords[0]->fRefCount == 1);
	}

	void ProveWorks_MakeRuntime__TwoIdenticalValueObjectRef__ResultsCorrectValueObjectRefs(){
		TTestValueObjectsFixture f;

		VValueObjectRef valueObjectRefA = MakeValueObjectType1ValueA(*f.fRuntime.get());
		VValueObjectRef valueObjectRefB = MakeValueObjectType1ValueA(*f.fRuntime.get());

		ASSERT(valueObjectRefA.GetMember("member_a").GetInt() == 1111);
		ASSERT(valueObjectRefA.GetMember("member_b").GetMachineString() == "2222");
		ASSERT(valueObjectRefA.GetMember("member_c").GetInt() == 3333);
		ASSERT(valueObjectRefA.GetMember("member_d").GetMachineString() == "4444");

		ASSERT(valueObjectRefB.GetMember("member_a").GetInt() == 1111);
		ASSERT(valueObjectRefB.GetMember("member_b").GetMachineString() == "2222");
		ASSERT(valueObjectRefB.GetMember("member_c").GetInt() == 3333);
		ASSERT(valueObjectRefB.GetMember("member_d").GetMachineString() == "4444");

		ASSERT(f.fRuntime->fValueObjectRecords.size() == 1);
		ASSERT(f.fRuntime->fValueObjectRecords[0]->fRefCount == 2);
	}

	void ProveWorks_MakeRuntime__TwoDifferentValueObjectRef__ResultsCorrectValueObjectRefs(){
		TTestValueObjectsFixture f;

		VValueObjectRef valueObjectRefA = MakeValueObjectType1ValueA(*f.fRuntime.get());
		VValueObjectRef valueObjectRefB = MakeValueObjectType1ValueB(*f.fRuntime.get());

		ASSERT(valueObjectRefA.GetMember("member_a").GetInt() == 1111);
		ASSERT(valueObjectRefA.GetMember("member_b").GetMachineString() == "2222");
		ASSERT(valueObjectRefA.GetMember("member_c").GetInt() == 3333);
		ASSERT(valueObjectRefA.GetMember("member_d").GetMachineString() == "4444");

		ASSERT(valueObjectRefB.GetMember("member_a").GetInt() == 55);
		ASSERT(valueObjectRefB.GetMember("member_b").GetMachineString() == "66");
		ASSERT(valueObjectRefB.GetMember("member_c").GetInt() == 77);
		ASSERT(valueObjectRefB.GetMember("member_d").GetMachineString() == "88");

		ASSERT(f.fRuntime->fValueObjectRecords.size() == 2);
		ASSERT(f.fRuntime->fValueObjectRecords[0]->fRefCount == 1);
		ASSERT(f.fRuntime->fValueObjectRecords[1]->fRefCount == 1);
	}











		struct TTestTableFixture {
			public: TTestTableFixture(){
				CStaticRegitrat registrat;
				fStaticDefinition.reset(MakeStaticDefinition(registrat));
				fRuntime.reset(MakeRuntime(*fStaticDefinition.get()));
			}

			std::auto_ptr<CStaticDefinition> fStaticDefinition;
			std::auto_ptr<CRuntime> fRuntime;
		};


	void ProveWorks_MakeTable__EmptyTable__ConstructsEmpty(){
		TTestTableFixture f;

		VTableRef tableRef = f.fRuntime->MakeEmptyTable();

		ASSERT(tableRef.GetSize() == 0);

		ASSERT(f.fRuntime->fTableRecords.size() == 1);
		ASSERT(f.fRuntime->fTableRecords[0]->fRefCount == 1);
		ASSERT(f.fRuntime->fTableRecords[0]->fValues.empty());
	}

	void ProveWorks_Table_Set__InsertAndGet__CanReadBack(){
		TTestTableFixture f;

		VTableRef tableRef = f.fRuntime->MakeEmptyTable();
		tableRef = tableRef.SetCopy("test_10", VValue::MakeInt(12345));
		ASSERT(tableRef.Get("test_10").GetInt() == 12345);

//		ASSERT(f.fRuntime->fTableRecords.size() == 1);
//		ASSERT(f.fRuntime->fTableRecords[0]->fRefCount == 1);
//		ASSERT(f.fRuntime->fTableRecords[0]->fValues.size() == 1);
	}

	void ProveWorks_Table_GetSize__Empty__Returns0(){
		TTestTableFixture f;

		VTableRef tableRef = f.fRuntime->MakeEmptyTable();
		ASSERT(tableRef.GetSize() == 0);
	}

	void ProveWorks_Table_GetSize__3Entries__Returns3(){
		TTestTableFixture f;

		VTableRef tableRef = f.fRuntime->MakeEmptyTable();
		tableRef = tableRef.SetCopy("1", VValue::MakeInt(100));
		tableRef = tableRef.SetCopy("2", VValue::MakeInt(200));
		tableRef = tableRef.SetCopy("3", VValue::MakeInt(300));

		ASSERT(tableRef.GetSize() == 3);
	}

	void ProveWorks_Table_CopyConstructor__2Entries__CopyIsIdentical(){
		TTestTableFixture f;

		VTableRef a = f.fRuntime->MakeEmptyTable();
		a = a.SetCopy("1", VValue::MakeInt(1000));
		a = a.SetCopy("2", VValue::MakeInt(2000));

		VTableRef b(a);
		ASSERT(b.Get("1").GetInt() == 1000);
		ASSERT(b.Get("2").GetInt() == 2000);
	}

	void ProveWorks_Table_OperatorEquals__2Entries__CopyIsIdentical(){
		TTestTableFixture f;

		VTableRef a = f.fRuntime->MakeEmptyTable();
		a = a.SetCopy("1", VValue::MakeInt(10000));
		a = a.SetCopy("2", VValue::MakeInt(20000));

		VTableRef b = f.fRuntime->MakeEmptyTable();
		b = a;
		ASSERT(b.Get("1").GetInt() == 10000);
		ASSERT(b.Get("2").GetInt() == 20000);
	}


//	http://stackoverflow.com/questions/7410559/c-overloading-operators-based-on-the-side-of-assignment

	void ProveWorks_MakeRange__EntireTableRange__GetEntries(){
		TTestTableFixture f;

		VTableRef tableRef = f.fRuntime->MakeEmptyTable();
		tableRef = tableRef.SetCopy("1", 1000);
		tableRef = tableRef.SetCopy("2", 2000);
		tableRef = tableRef.SetCopy("3", 3000);
		tableRef = tableRef.SetCopy("4", 4000);

		VRange range = tableRef.GetRange();
		ASSERT(!range.IsEmpty())

		VValue x = range.GetFront();
		ASSERT(x == 1000);
		range.PopFront();
		ASSERT(!range.IsEmpty())

		x = range.GetFront();
		ASSERT(x == 2000);
		range.PopFront();
		ASSERT(!range.IsEmpty())

		x = range.GetFront();
		ASSERT(x == 3000);
		range.PopFront();
		ASSERT(!range.IsEmpty())

		x = range.GetFront();
		ASSERT(x == 4000);
		range.PopFront();

		ASSERT(range.IsEmpty());
	}




	void ProveWorks_VValue_MakeValueObjectRef__Simple__CanGetBackAgain(){
		TTestValueObjectsFixture f;

		VValueObjectRef a = MakeValueObjectType1ValueA(*f.fRuntime.get());

		VValue b = VValue::MakeValueObjectRef(a);
		ASSERT(b.IsValueObjectRef());
		VValueObjectRef c = b.GetValueObjectRef();

		ASSERT(c.GetMember("member_a").GetInt() == 1111);
		ASSERT(c.GetMember("member_b").GetMachineString() == "2222");
		ASSERT(c.GetMember("member_c").GetInt() == 3333);
		ASSERT(c.GetMember("member_d").GetMachineString() == "4444");
	}

	void ProveWorks_VValue_MakeTableRef__Simple__CanGetBackAgain(){
		TTestTableFixture f;

		VTableRef a = f.fRuntime->MakeEmptyTable();
		a = a.SetCopy("1", VValue::MakeInt(1000));
		a = a.SetCopy("2", VValue::MakeInt(2000));

		VValue b = VValue::MakeTableRef(a);
		ASSERT(b.IsTableRef());
		VTableRef c = b.GetTableRef();

		ASSERT(c == a);
	}




	void ProveWorks_VTableRef_Set__StoreValueObjectRef__CanGetBackAgain(){
		TTestValueObjectsFixture f;

		VTableRef tableRef = f.fRuntime->MakeEmptyTable();
		VValueObjectRef valueObjectRef = MakeValueObjectType1ValueA(*f.fRuntime.get());
		tableRef = tableRef.SetCopy("1", valueObjectRef);

		ASSERT(tableRef.Get("1").GetValueObjectRef().GetMember("member_b").GetMachineString() == "2222");
	}

	void ProveWorks_VTableRef_Set__StoreTableRef__CanGetBackAgain(){
		TTestValueObjectsFixture f;

		VTableRef tableRefA = f.fRuntime->MakeEmptyTable();
		tableRefA = tableRefA.SetCopy("one", VValue::MakeInt(8));
		tableRefA = tableRefA.SetCopy("two", VValue::MakeInt(9));

		VTableRef tableRefB = f.fRuntime->MakeEmptyTable();
		tableRefB = tableRefB.SetCopy("first", tableRefA);

		ASSERT(tableRefB.Get("first").GetTableRef().Get("one").GetInt() == 8);
		ASSERT(tableRefB.Get("first").GetTableRef().Get("two").GetInt() == 9);
	}

}

UNIT_TEST("BasicTypes", "TestBasicTypes()", "", ""){
	ProveWorks_VValue_DefaultConstructor__None__ResultIsNil();
	ProveWorks_VValue_MakeInt__Normal__ResultIsCorrectInt();
	ProveWorks_VValue_CopyConstructor__Int__ResultIsCorrectInt();
	ProveWorks_VValue_OperatorEqual__Int__ResultIsCorrectInt();

	ProveWorks_MakeStaticDefinition__Empty__ResultIsEmpty();
	ProveWorks_MakeStaticDefinition__OneValueObjectMeta__ResultCorrectEntry();
	ProveWorks_MakeStaticDefinition__TwoValueObjectMeta__ResultCorrectEntries();

	ProveWorks_MakeRuntime__NoInstances__ResultsEmptyRuntime();
	ProveWorks_MakeRuntime__OneValueObjectRef__ResultsCorrectValueObjectRef();
	ProveWorks_MakeRuntime__TwoIdenticalValueObjectRef__ResultsCorrectValueObjectRefs();
	ProveWorks_MakeRuntime__TwoDifferentValueObjectRef__ResultsCorrectValueObjectRefs();

	ProveWorks_MakeTable__EmptyTable__ConstructsEmpty();
	ProveWorks_Table_Set__InsertAndGet__CanReadBack();
	ProveWorks_Table_GetSize__Empty__Returns0();
	ProveWorks_Table_GetSize__3Entries__Returns3();
	ProveWorks_Table_CopyConstructor__2Entries__CopyIsIdentical();
	ProveWorks_Table_OperatorEquals__2Entries__CopyIsIdentical();

	ProveWorks_MakeRange__EntireTableRange__GetEntries();

	ProveWorks_VValue_MakeValueObjectRef__Simple__CanGetBackAgain();
	ProveWorks_VValue_MakeTableRef__Simple__CanGetBackAgain();

	ProveWorks_VTableRef_Set__StoreValueObjectRef__CanGetBackAgain();
	ProveWorks_VTableRef_Set__StoreTableRef__CanGetBackAgain();
}


