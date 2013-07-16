//
//  main.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2013-07-04.
//  Copyright (c) 2013 Marcus Zetterquist. All rights reserved.
//

#include "Quark.h"
#include "BasicTypes.h"
#include "BasicTypesUT.h"

#include <iostream>


namespace {
	void Example1(){
		CStaticRegitrat registrat;

		//	Register a new type of value object.
		{
			std::vector<VMemberMeta> members;
			members.push_back(VMemberMeta("x", kType_Int));
			members.push_back(VMemberMeta("y", kType_Int));
			members.push_back(VMemberMeta("z", kType_Int));
			registrat.Register(VValueObjectMeta(members));
		}

		std::auto_ptr<CStaticDefinition> staticDefinition(MakeStaticDefinition(registrat));
		std::auto_ptr<CRuntime> runtime(MakeRuntime(*staticDefinition.get()));

		const VValueObjectMeta* type = runtime->fStaticDefinition->LookupValueObjectMetaFromIndex(0);

		std::vector<VValue> members;
		members.push_back(VValue::MakeInt(1));
		members.push_back(VValue::MakeInt(2));
		members.push_back(VValue::MakeInt(3));

		VValueObjectRef ref1 = runtime->MakeValueObject(*type, members);
		VValueObjectRef ref2 = runtime->MakeValueObject(*type, members);
		VValueObjectRef ref3 = runtime->MakeValueObject(*type, members);

		VValueObjectRef ref4 = ref1;



		VTableRef tableRef = runtime->MakeEmptyTable();
		tableRef.Set("a", VValue::MakeInt(4000));
		tableRef.Set("b", VValue::MakeInt(5000));

		ASSERT(tableRef.Get("a").GetInt() == 4000);
		ASSERT(tableRef.Get("b").GetInt() == 5000);
	}
}



int main(int argc, const char * argv[]){
	std::cout << "Start tests!\n";

	try{
		TestBasicTypes();

		Example1();

		std::cout << "Success!\n";
	}
	catch(...){
		std::cout << "Failure!\n";
		return -1;
	}

    return 0;
}

