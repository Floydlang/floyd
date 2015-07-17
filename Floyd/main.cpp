//
//  main.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2013-07-04.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "cpp_extension.h"
#include "BasicTypes.h"
#include "BasicTypesUT.h"

#include "ProgramUT.h"

#include "Runtime.h"
#include "Experiments.h"

#include <iostream>



UNIT_TEST("Example1","", "", ""){
	CStaticRegitrat registrat;

	//	Register a new type of value object.
	{
		std::vector<VMemberMeta> members;
		members.push_back(VMemberMeta("x", kType_Int));
		members.push_back(VMemberMeta("y", kType_Int));
		members.push_back(VMemberMeta("z", kType_Int));
		registrat.Register(VValueObjectMeta("Point3D", members));
	}

	std::auto_ptr<CStaticDefinition> staticDefinition(MakeStaticDefinition(registrat));
	std::auto_ptr<CRuntime> runtime(MakeRuntime(*staticDefinition.get()));

	const VValueObjectMeta& point3DType = runtime->LookupValueObjectType("Point3D");

	std::vector<VValue> members;
	members.push_back(VValue::MakeInt(1));
	members.push_back(VValue::MakeInt(2));
	members.push_back(VValue::MakeInt(3));

	VValueObjectRef ref1 = runtime->MakeValueObject(point3DType, members);
	VValueObjectRef ref2 = runtime->MakeValueObject(point3DType, members);
	VValueObjectRef ref3 = runtime->MakeValueObject(point3DType, members);

	VValueObjectRef ref4 = ref1;


	VTableRef tableRef = runtime->MakeEmptyTable();
	tableRef = tableRef.SetCopy("a", VValue::MakeInt(4000));
	tableRef = tableRef.SetCopy("b", VValue::MakeInt(5000));

	UT_VERIFY(tableRef["a"].GetInt() == 4000);
	UT_VERIFY(tableRef["b"].GetInt() == 5000);
}


UNIT_TEST("Example2","", "", ""){
	CStaticRegitrat registrat;

	//	Register a new type of value object.
	{
		std::vector<VMemberMeta> members;
		members.push_back(VMemberMeta("x", kType_Int));
		members.push_back(VMemberMeta("y", kType_Int));
		members.push_back(VMemberMeta("z", kType_Int));
		registrat.Register(VValueObjectMeta("Point3D", members));
	}

	std::auto_ptr<CStaticDefinition> staticDefinition(MakeStaticDefinition(registrat));
	std::auto_ptr<CRuntime> runtime(MakeRuntime(*staticDefinition.get()));

	const VValueObjectMeta& point3DType = runtime->LookupValueObjectType("Point3D");

	std::vector<VValue> members;
	members.push_back(VValue::MakeInt(1));
	members.push_back(VValue::MakeInt(2));
	members.push_back(VValue::MakeInt(3));

	VValueObjectRef ref1 = runtime->MakeValueObject(point3DType, members);
	VValueObjectRef ref2 = runtime->MakeValueObject(point3DType, members);
	VValueObjectRef ref3 = runtime->MakeValueObject(point3DType, members);

	VValueObjectRef ref4 = ref1;


	VTableRef tableRef = runtime->MakeEmptyTable();
	tableRef = tableRef.SetCopy("a", VValue::MakeInt(4000));
	tableRef = tableRef.SetCopy("b", VValue::MakeInt(5000));

	ASSERT(tableRef["a"].GetInt() == 4000);
	ASSERT(tableRef["b"].GetInt() == 5000);
}

int main(int /*argc*/, const char* /*argv*/[]){
	TDefaultRuntime runtime("");
	SetRuntime(&runtime);
	run_tests();

	try {
		TRACE("Hello, World 3!");
	}
	catch(...){
		TRACE("Error");
		return -1;
	}

    return 0;
}



