//
//  immutable_ref_value.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 31/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "immutable_ref_value.h"

#include "quark.h"

#include <string>



struct smurf_impl_t {
	public: smurf_impl_t() :
		_a(1.2f),
		_name("default smurf")
	{
	}
	public: smurf_impl_t(float a, std::string name) :
		_a(a),
		_name(name)
	{
		QUARK_ASSERT(check_invariant2());
	}

	public: ~smurf_impl_t(){
		QUARK_ASSERT(check_invariant2());
	}

	public: bool check_invariant2() const {
		QUARK_ASSERT(_name.size() > 0);
		return true;
	}

	public: bool operator==(const smurf_impl_t& rhs) const{
		QUARK_ASSERT(check_invariant2());

		return _a == rhs._a && _name == rhs._name;
	}

	float _a;
	std::string _name;
};


QUARK_UNIT_TESTQ("make_immutable_ref()", "Basic construction"){
	const auto a = immutable_ref_value_t<smurf_impl_t>();
	QUARK_TEST_VERIFY(a.check_invariant());
}


QUARK_UNIT_TESTQ("make_immutable_ref()", "Basic construction"){
	const auto a = make_immutable_ref<smurf_impl_t>(12.4f, "Hungry-Smurf");

	QUARK_TEST_VERIFY(a.check_invariant());
	QUARK_UT_VERIFY(a->_a == 12.4f);
	QUARK_UT_VERIFY(a->_name == "Hungry-Smurf");
}

QUARK_UNIT_TESTQ("make_immutable_ref()", "operator==() -- EQUAL by ptr comparison shortcut"){
	const auto a = make_immutable_ref<smurf_impl_t>(12.4f, "Hungry-Smurf");
	const auto b = a;

	QUARK_TEST_VERIFY(a == b);
}

QUARK_UNIT_TESTQ("make_immutable_ref()", "operator==()"){
	const auto a = make_immutable_ref<smurf_impl_t>(12.4f, "Hungry-Smurf");
	const auto b = make_immutable_ref<smurf_impl_t>(12.4f, "Fuller");

	QUARK_TEST_VERIFY(!(a == b));
}

QUARK_UNIT_TESTQ("make_immutable_ref()", "operator==() -- equal BY VALUE"){
	const auto a = make_immutable_ref<smurf_impl_t>(12.4f, "Hungry-Smurf");
	const auto b = make_immutable_ref<smurf_impl_t>(12.4f, "Hungry-Smurf");

	QUARK_TEST_VERIFY(a == b);
}

QUARK_UNIT_TESTQ("make_immutable_ref()", "Test using std::string"){
	const auto a = make_immutable_ref<std::string>("Hello, world!");

	QUARK_TEST_VERIFY(a.check_invariant());
	QUARK_UT_VERIFY(*a == "Hello, world!");

	QUARK_UT_VERIFY(a->size() == 13);
}




QUARK_UNIT_TESTQ("make_immutable_value()", "Basic construction"){
	const auto a = make_immutable_value<smurf_impl_t>();
	QUARK_TEST_VERIFY(a.check_invariant());
}


QUARK_UNIT_TESTQ("make_immutable_value()", "Basic construction"){
	const auto a = make_immutable_value<smurf_impl_t>(12.4f, "Hungry-Smurf");

	QUARK_TEST_VERIFY(a.check_invariant());
	QUARK_UT_VERIFY(a->_a == 12.4f);
	QUARK_UT_VERIFY(a->_name == "Hungry-Smurf");
}

QUARK_UNIT_TESTQ("make_immutable_value()", "operator==()"){
	const auto a = make_immutable_value<smurf_impl_t>(12.4f, "Hungry-Smurf");
	const auto b = make_immutable_value<smurf_impl_t>(12.4f, "Fuller");

	QUARK_TEST_VERIFY(!(a == b));
}

QUARK_UNIT_TESTQ("make_immutable_value()", "operator==() -- equal BY VALUE"){
	const auto a = make_immutable_value<smurf_impl_t>(12.4f, "Hungry-Smurf");
	const auto b = make_immutable_value<smurf_impl_t>(12.4f, "Hungry-Smurf");

	QUARK_TEST_VERIFY(a == b);
}

QUARK_UNIT_TESTQ("make_immutable_value()", "Test using std::string"){
	const auto a = make_immutable_value<std::string>("Hello, world!");

	QUARK_TEST_VERIFY(a.check_invariant());
	QUARK_UT_VERIFY(*a == "Hello, world!");
}




