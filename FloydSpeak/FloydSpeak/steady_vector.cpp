/*
	Copyright 2015 Marcus Zetterquist

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.

	steady::vector<> is a persistent vector class for C++
*/

#include "steady_vector.h"

#include <algorithm>
#include <memory>
#include "quark.h"


//	Make local shortcut macros - shorter names
#define ASSERT(x) STEADY_ASSERT(x)
#define TRACE(x) STEADY_TRACE(x)
#define TRACE_SS(x) STEADY_TRACE_SS(x)
#define VERIFY(x) STEADY_TEST_VERIFY(x)
#define SCOPED_TRACE(x) STEADY_SCOPED_TRACE(x)



namespace steady {

using namespace internals;




////////////////////////////////////////////			Unit tests




////////////////////////////////////////////			count_to_depth()


QUARK_UNIT_TEST("", "count_to_depth()", "0", "-1"){
	VERIFY(count_to_depth(0) == 0);

	VERIFY(count_to_depth(1) == 1);
	VERIFY(count_to_depth(2) == 1);
	VERIFY(count_to_depth(3) == 1);

	VERIFY(count_to_depth(BRANCHING_FACTOR + 1) == 2);
	VERIFY(count_to_depth(BRANCHING_FACTOR * BRANCHING_FACTOR) == 2);

	VERIFY(count_to_depth(BRANCHING_FACTOR * BRANCHING_FACTOR + 1) == 3);
	VERIFY(count_to_depth(BRANCHING_FACTOR * BRANCHING_FACTOR * BRANCHING_FACTOR) == 3);
}


////////////////////////////////////////////			vector_size_to_shift()


QUARK_UNIT_TEST("", "vector_size_to_shift()", "", ""){
	VERIFY(vector_size_to_shift(0) == EMPTY_TREE_SHIFT);
	VERIFY(vector_size_to_shift(1) == LEAF_NODE_SHIFT);
	VERIFY(vector_size_to_shift(BRANCHING_FACTOR * 1) == LEAF_NODE_SHIFT);
	VERIFY(vector_size_to_shift(BRANCHING_FACTOR * 1 + 1) == LOWEST_LEVEL_INODE_SHIFT);
}


////////////////////////////////////////////			shift_to_max_size()


QUARK_UNIT_TEST("", "shift_to_max_size()", "", ""){
	VERIFY(shift_to_max_size(EMPTY_TREE_SHIFT) == 0);
	VERIFY(shift_to_max_size(LEAF_NODE_SHIFT) == BRANCHING_FACTOR * 1);
	VERIFY(shift_to_max_size(LOWEST_LEVEL_INODE_SHIFT) == BRANCHING_FACTOR * BRANCHING_FACTOR);
	VERIFY(shift_to_max_size(BRANCHING_FACTOR_SHIFT * 2) == BRANCHING_FACTOR * BRANCHING_FACTOR * BRANCHING_FACTOR);
}



////////////////////////////////////////////			Test how C++ works



void vector_test(const std::vector<int>& v){
}

QUARK_UNIT_TEST("std::vector<>", "auto convertion from initializer list", "", ""){
	std::vector<int> vi {1,2,3,4,5,6};

	vector_test(vi);

	vector_test(std::vector<int>{ 8, 9, 10});
	vector_test({ 8, 9, 10});
}


////////////////////////////////////////////			test_fixture<T>

/*
	Fixture class that you put on the stack in your unit tests.
	It makes sure the total count of leaf_node<T> and inode<T> are as expected - no leakage.
*/
template <class T>
struct test_fixture {
	test_fixture() :
		_scoped_tracer("test_fixture"),
		_inode_count(inode<T>::_debug_count),
		_leaf_count(leaf_node<T>::_debug_count)
	{
		TRACE_SS("inode count: " << _inode_count << " " << "Leaf node count: " << _leaf_count);
	}

	/*
		Use this constructor when you *expect* the number of nodes to have grown when test_fixture destructs.
	*/
	test_fixture(int inode_expected_count, int leaf_expected_count) :
		_scoped_tracer("test_fixture"),
		_inode_count(inode<T>::_debug_count),
		_leaf_count(leaf_node<T>::_debug_count),

		_inode_expected_count(inode_expected_count),
		_leaf_expected_count(leaf_expected_count)
	{
		TRACE_SS("inode count: " << _inode_count << " " << "Leaf node count: " << _leaf_count);
	}

	~test_fixture(){
		int inode_count = inode<T>::_debug_count;
		int leaf_count = leaf_node<T>::_debug_count;

		TRACE_SS("inode count: " << inode_count << " " << "Leaf node count: " << leaf_count);

		int inode_diff_count = inode_count - _inode_count;
		int leaf_expected_diff = leaf_count - _leaf_count;


		ASSERT(inode_diff_count == _inode_expected_count);
		ASSERT(leaf_expected_diff == _leaf_expected_count);
	}

	quark::scoped_trace _scoped_tracer;
	int _inode_count = 0;
	int _leaf_count = 0;

	int _inode_expected_count = 0;
	int _leaf_expected_count = 0;
};

QUARK_UNIT_TEST("", "test_fixture()", "no nodes", "no assert"){
	test_fixture<int> test;
	VERIFY(test._inode_count == 0);
	VERIFY(test._leaf_count == 0);
	VERIFY(test._inode_expected_count == 0);
	VERIFY(test._leaf_expected_count == 0);
}

#ifndef _MSC_VER	// SB: No initializer-list constructor defined for inode: a.reset(new idnode<int>({})); Does not compile in VS.
QUARK_UNIT_TEST("", "test_fixture()", "1 inode, 2 leaf nodes", "correct state (and no assert!)"){
	test_fixture<int> test;

	std::unique_ptr<inode<int>> a;
	std::unique_ptr<leaf_node<int>> b;
	std::unique_ptr<leaf_node<int>> c;
	{
		test_fixture<int> test(1, 2);

		a.reset(new inode<int>({}));
		b.reset(new leaf_node<int>());
		c.reset(new leaf_node<int>());

		VERIFY(test._inode_count == 0);
		VERIFY(test._leaf_count == 0);
		VERIFY(test._inode_expected_count == 1);
		VERIFY(test._leaf_expected_count == 2);
	}
}
#endif

/*
	Generates a ramp from _start_ incrementing by +1. _count_ items.
	Resulting vector is always _total_count_ big, padded with 0s.

	total_count: total_count >= count
*/
std::vector<int> generate_numbers(int start, int count, int total_count){
	ASSERT(count >= 0);
	ASSERT(total_count >= count);

	std::vector<int> a;
	int i = 0;
	while(i < count){
		a.push_back(start + i);
		i++;
	}
	while(i < total_count){
		a.push_back(0);
		i++;
	}
	return a;
}

QUARK_UNIT_TEST("", "generate_numbers()", "5 numbers", "correct vector"){
	const auto a = generate_numbers(8, 4, 7);
	VERIFY(a == (std::vector<int>{ 8, 9, 10, 11, 0, 0, 0 }));
}

std::array<int, BRANCHING_FACTOR> generate_leaves(int start, int count){
	const auto a = generate_numbers(start, count, BRANCHING_FACTOR);

	std::array<int, BRANCHING_FACTOR> result;
	for(int i = 0 ; i < BRANCHING_FACTOR ; i++){
		result[i] = a[i];
	}
	return result;
}


/*
	Construct a vector that uses 1 leaf node.

	leaf_node
		value 0
*/
vector<int> make_manual_vector1(){
	test_fixture<int> f(0, 1);

	node_ref<int> leaf = make_leaf_node<int>({ 7 });
	return vector<int>(leaf, 1, LEAF_NODE_SHIFT);
}

QUARK_UNIT_TEST("", "make_manual_vector1()", "", "correct nodes"){
	test_fixture<int> f;

	const auto a = make_manual_vector1();
	VERIFY(a.size() == 1);
	VERIFY(a.get_root().get_type() == node_type::leaf_node);
	VERIFY(a.get_root().get_leaf_node()->_rc == 1);
	VERIFY(a.get_root().get_leaf_node()->_values[0] == 7);
	for(int i = 1 ; i < BRANCHING_FACTOR ; i++){
		VERIFY(a.get_root().get_leaf_node()->_values[i] == 0);
	}
}


/*
	Construct a vector that uses 1 leaf node and two values.

	leaf_node
		value 0
		value 1
*/
vector<int> make_manual_vector2(){
	test_fixture<int> f(0, 1);

	node_ref<int> leaf = make_leaf_node<int>({	7, 8	});
	return vector<int>(leaf, 2, LEAF_NODE_SHIFT);
}

QUARK_UNIT_TEST("", "make_manual_vector2()", "", "correct nodes"){
	test_fixture<int> f;
	const auto a = make_manual_vector2();
	VERIFY(a.size() == 2);
	VERIFY(a.get_root().get_type() == node_type::leaf_node);
	VERIFY(a.get_root().get_leaf_node()->_rc == 1);
	VERIFY(a.get_root().get_leaf_node()->_values[0] == 7);
	VERIFY(a.get_root().get_leaf_node()->_values[1] == 8);
	VERIFY(a.get_root().get_leaf_node()->_values[2] == 0);
	VERIFY(a.get_root().get_leaf_node()->_values[3] == 0);
}


/*
	Construct a vector that uses 1 inode and 2 leaf nodes.

	inode
		leaf_node
			value 0
			value 1
			... full
		leaf_node
			value x
*/
vector<int> make_manual_vector_branchfactor_plus_1(){
	test_fixture<int> f(1, 2);
	node_ref<int> leaf0 = make_leaf_node<int>(generate_leaves(7, BRANCHING_FACTOR));
	node_ref<int> leaf1 = make_leaf_node<int>(generate_leaves(7 + BRANCHING_FACTOR, 1));
	std::vector<node_ref<int>> leafs = { leaf0, leaf1 };
	node_ref<int> inode = make_inode_from_vector(leafs);
	return vector<int>(inode, BRANCHING_FACTOR + 1, vector_size_to_shift(BRANCHING_FACTOR + 1));
}

QUARK_UNIT_TEST("", "make_manual_vector_branchfactor_plus_1()", "", "correct nodes"){
	test_fixture<int> f;

	const auto a = make_manual_vector_branchfactor_plus_1();
	VERIFY(a.size() == BRANCHING_FACTOR + 1);

	VERIFY(a.get_root().get_type() == node_type::inode);
	VERIFY(a.get_root().get_inode()->_rc == 1);
	VERIFY(a.get_root().get_inode()->count_children() == 2);
	VERIFY(a.get_root().get_inode()->get_child(0).get_type() == node_type::leaf_node);
	VERIFY(a.get_root().get_inode()->get_child(1).get_type() == node_type::leaf_node);

	const auto leaf0 = a.get_root().get_inode()->get_child_as_leaf_node(0);
	VERIFY(leaf0->_rc == 1);
	VERIFY(leaf0->_values == generate_leaves(7 + BRANCHING_FACTOR * 0, BRANCHING_FACTOR));

	const auto leaf1 = a.get_root().get_inode()->get_child_as_leaf_node(1);
	VERIFY(leaf1->_rc == 1);
	VERIFY(leaf1->_values == generate_leaves(7 + BRANCHING_FACTOR * 1, 1));
}

/*
	Construct a vector using 2 levels of inodes.

	inode
		inode
			leaf_node 0
				value 0
				value 1
				... full
			leaf node 1
				value 0
				value 1
				... full
			leaf node 2
				value 0
				value 1
				... full
			... full
		inode
			leaf_node
*/
vector<int> make_manual_vector_branchfactor_square_plus_1(){
	test_fixture<int> f(3, BRANCHING_FACTOR + 1);

	std::vector<node_ref<int>> leaves;
	for(int i = 0 ; i < BRANCHING_FACTOR ; i++){
		node_ref<int> leaf = make_leaf_node<int>(generate_leaves(1000 + BRANCHING_FACTOR * i, BRANCHING_FACTOR));
		leaves.push_back(leaf);
	}

	node_ref<int> extraLeaf = make_leaf_node<int>(generate_leaves(1000 + BRANCHING_FACTOR * BRANCHING_FACTOR + 0, 1));

	node_ref<int> inodeA = make_inode_from_vector<int>(leaves);
	node_ref<int> inodeB = make_inode_from_vector<int>({ extraLeaf });
	node_ref<int> rootInode = make_inode_from_vector<int>({ inodeA, inodeB });
	const size_t size = BRANCHING_FACTOR * BRANCHING_FACTOR + 1;
	return vector<int>(rootInode, size, vector_size_to_shift(size));
}

QUARK_UNIT_TEST("", "make_manual_vector_branchfactor_square_plus_1()", "", "correct nodes"){
	test_fixture<int> f;

	const auto a = make_manual_vector_branchfactor_square_plus_1();
	VERIFY(a.size() == BRANCHING_FACTOR * BRANCHING_FACTOR + 1);

	node_ref<int> rootINode = a.get_root();
	VERIFY(rootINode.get_type() == node_type::inode);
	VERIFY(rootINode.get_inode()->_rc == 2);
	VERIFY(rootINode.get_inode()->count_children() == 2);
	VERIFY(rootINode.get_inode()->get_child(0).get_type() == node_type::inode);
	VERIFY(rootINode.get_inode()->get_child(1).get_type() == node_type::inode);

	node_ref<int> inodeA = rootINode.get_inode()->get_child(0);
		VERIFY(inodeA.get_type() == node_type::inode);
		VERIFY(inodeA.get_inode()->_rc == 2);
		VERIFY(inodeA.get_inode()->count_children() == BRANCHING_FACTOR);
		for(int i = 0 ; i < BRANCHING_FACTOR ; i++){
			const auto leafNode = inodeA.get_inode()->get_child_as_leaf_node(i);
			VERIFY(leafNode->_rc == 1);
			VERIFY(leafNode->_values == generate_leaves(1000 + BRANCHING_FACTOR * i, BRANCHING_FACTOR));
		}

	node_ref<int> inodeB = rootINode.get_inode()->get_child(1);
		VERIFY(inodeB.get_type() == node_type::inode);
		VERIFY(inodeB.get_inode()->_rc == 2);
		VERIFY(inodeB.get_inode()->count_children() == 1);
		VERIFY(inodeB.get_inode()->get_child(0).get_type() == node_type::leaf_node);

		const auto leaf4 = inodeB.get_inode()->get_child_as_leaf_node(0);
		VERIFY(leaf4->_rc == 1);
		VERIFY(leaf4->_values == generate_leaves(1000 + BRANCHING_FACTOR * BRANCHING_FACTOR + 0, 1));
}


////////////////////////////////////////////		vector::vector()


QUARK_UNIT_TEST("vector", "vector()", "", "no_assert"){
	test_fixture<int> f;

	vector<int> v;
	v.trace_internals();
}


////////////////////////////////////////////		vector::operator[]


QUARK_UNIT_TEST("vector", "operator[]", "1 value", "read back"){
	test_fixture<int> f;

	const auto a = make_manual_vector1();
	VERIFY(a[0] == 7);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "operator[]", "Branchfactor + 1 values", "read back"){
	test_fixture<int> f;
	const auto a = make_manual_vector_branchfactor_plus_1();
	VERIFY(a[0] == 7);
	VERIFY(a[1] == 8);
	VERIFY(a[2] == 9);
	VERIFY(a[3] == 10);
	VERIFY(a[4] == 11);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "operator[]", "Branchfactor^2 + 1 values", "read back"){
	test_fixture<int> f;
	const auto a = make_manual_vector_branchfactor_square_plus_1();
	VERIFY(a[0] == 1000);
	VERIFY(a[1] == 1001);
	VERIFY(a[2] == 1002);
	VERIFY(a[3] == 1003);
	VERIFY(a[4] == 1004);
	VERIFY(a[5] == 1005);
	VERIFY(a[6] == 1006);
	VERIFY(a[7] == 1007);
	VERIFY(a[8] == 1008);
	VERIFY(a[9] == 1009);
	VERIFY(a[10] == 1010);
	VERIFY(a[11] == 1011);
	VERIFY(a[12] == 1012);
	VERIFY(a[13] == 1013);
	VERIFY(a[14] == 1014);
	VERIFY(a[15] == 1015);
	VERIFY(a[16] == 1016);
	a.trace_internals();
}


////////////////////////////////////////////		vector::store()


QUARK_UNIT_TEST("vector", "store()", "1 value", "read back"){
	test_fixture<int> f;
	const auto a = make_manual_vector1();
	const auto b = a.store(0, 1000);
	VERIFY(a[0] == 7);
	VERIFY(b[0] == 1000);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "store()", "5 value vector, replace #0", "read back"){
	test_fixture<int> f;
	const auto a = make_manual_vector_branchfactor_plus_1();
	const auto b = a.store(0, 1000);
	VERIFY(a[0] == 7);
	VERIFY(b[0] == 1000);
}

QUARK_UNIT_TEST("vector", "store()", "5 value vector, replace #4", "read back"){
	test_fixture<int> f;
	const auto a = make_manual_vector_branchfactor_plus_1();
	const auto b = a.store(4, 1000);
	VERIFY(a[0] == 7);
	VERIFY(b[4] == 1000);
}

QUARK_UNIT_TEST("vector", "store()", "17 value vector, replace bunch", "read back"){
	test_fixture<int> f;
	auto a = make_manual_vector_branchfactor_square_plus_1();
	a = a.store(4, 1004);
	a = a.store(5, 1005);
	a = a.store(0, 1000);
	a = a.store(16, 1016);
	a = a.store(10, 1010);

	VERIFY(a[0] == 1000);
	VERIFY(a[4] == 1004);
	VERIFY(a[5] == 1005);
	VERIFY(a[16] == 1016);
	VERIFY(a[10] == 1010);

	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "store()", "5 value vector, replace value 10000 times", "read back"){
	test_fixture<int> f;
	auto a = make_manual_vector_branchfactor_plus_1();

	for(int i = 0 ; i < 1000 ; i++){
		a = a.store(4, i);
	}
	VERIFY(a[4] == 999);

	a.trace_internals();
}


////////////////////////////////////////////		vector::push_back()


vector<int> push_back_n(int count, int value0){
//	test_fixture<int> f;
	vector<int> a;
	for(int i = 0 ; i < count ; i++){
		a = a.push_back(value0 + i);
	}
	return a;
}

void test_values(const vector<int>& vec, int value0){
	test_fixture<int> f;

	for(int i = 0 ; i < vec.size() ; i++){
		const auto value = vec[i];
		const auto expected = value0 + i;
		VERIFY(value == expected);
	}
}

QUARK_UNIT_TEST("vector", "push_back()", "one value => 1 leaf node", "read back"){
	test_fixture<int> f;
	const vector<int> a;
	const auto b = a.push_back(4);
	VERIFY(a.size() == 0);
	VERIFY(b.size() == 1);
	VERIFY(b[0] == 4);
}

QUARK_UNIT_TEST("vector", "push_back()", "two values => 1 leaf node", "read back"){
	test_fixture<int> f;
	const vector<int> a;
	const auto b = a.push_back(4);
	const auto c = b.push_back(9);

	VERIFY(a.size() == 0);

	VERIFY(b.size() == 1);
	VERIFY(b[0] == 4);

	VERIFY(c.size() == 2);
	VERIFY(c[0] == 4);
	VERIFY(c[1] == 9);
}

QUARK_UNIT_TEST("vector", "push_back()", "1 inode", "read back"){
	test_fixture<int> f;
	const auto count = BRANCHING_FACTOR + 1;
	vector<int> a = push_back_n(count, 1000);
	VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "push_back()", "1 inode + add leaf to last node", "read back all values"){
	test_fixture<int> f;
	const auto count = BRANCHING_FACTOR + 2;
	vector<int> a = push_back_n(count, 1000);
	VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "push_back()", "2-levels of inodes", "read back all values"){
	test_fixture<int> f;
	const auto count = BRANCHING_FACTOR * BRANCHING_FACTOR + 1;
	vector<int> a = push_back_n(count, 1000);
	VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "push_back()", "2-levels of inodes + add leaf-node to last node", "read back all values"){
	test_fixture<int> f;
	const auto count = BRANCHING_FACTOR * BRANCHING_FACTOR * 2;
	vector<int> a = push_back_n(count, 1000);
	VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}

QUARK_UNIT_TEST("vector", "push_back()", "3-levels of inodes + add leaf-node to last node", "read back all values"){
	test_fixture<int> f;
	const auto count = BRANCHING_FACTOR * BRANCHING_FACTOR * BRANCHING_FACTOR * 2;
	vector<int> a = push_back_n(count, 1000);
	VERIFY(a.size() == count);
	test_values(a, 1000);
	a.trace_internals();
}


////////////////////////////////////////////		vector::push_back(const std::vector<T>& values)


QUARK_UNIT_TEST("vector", "push_back()", "1-level of inodes + add leaf-node to last node", "read back all values"){
	test_fixture<int> f;
	const auto count = BRANCHING_FACTOR + 2;
	const auto data = generate_numbers(4, count, count);
	vector<int> a;
	a = a.push_back(data);
	VERIFY(a.to_vec() == data);
}


////////////////////////////////////////////		vector::push_back(const T values[], size_t count)


QUARK_UNIT_TEST("vector", "push_back()", "1-level of inodes + add leaf-node to last node", "read back all values"){
	test_fixture<int> f;
	const auto count = BRANCHING_FACTOR + 2;
	const auto data = generate_numbers(4, count, count);
	vector<int> a;
	a = a.push_back(&data[0], count);
	VERIFY(a.to_vec() == data);
}


////////////////////////////////////////////		vector::pop_back()


QUARK_UNIT_TEST("vector", "pop_back()", "basic", "correct result vector"){
	test_fixture<int> f;
	const auto data = generate_numbers(4, 50, 50);
	const auto data2 = std::vector<int>(&data[0], &data[data.size() - 1]);
	const auto a = vector<int>(data);
	const auto b = a.pop_back();
	VERIFY(b.to_vec() == data2);
}


////////////////////////////////////////////		vector::operator==()


QUARK_UNIT_TEST("vector", "operator==()", "empty vs empty", "true"){
	test_fixture<int> f;

	const vector<int> a;
	const vector<int> b;
	VERIFY(a == b);
}

QUARK_UNIT_TEST("vector", "operator==()", "empty vs 1", "false"){
	test_fixture<int> f;

	const vector<int> a;
	const vector<int> b{ 33 };
	VERIFY(!(a == b));
}

QUARK_UNIT_TEST("vector", "operator==()", "1000 vs 1000", "true"){
	test_fixture<int> f;
	const auto data = generate_numbers(4, 50, 50);
	const vector<int> a(data);
	const vector<int> b(data);
	VERIFY(a == b);
}

QUARK_UNIT_TEST("vector", "operator==()", "1000 vs 1000", "false"){
	test_fixture<int> f;
	const auto data = generate_numbers(4, 50, 50);
	auto data2 = data;
	data2[47] = 0;

	const vector<int> a(data);
	const vector<int> b(data2);
	VERIFY(!(a == b));
}


////////////////////////////////////////////		vector::size()


QUARK_UNIT_TEST("vector", "size()", "empty vector", "0"){
	test_fixture<int> f;
	vector<int> v;
	VERIFY(v.size() == 0);
}

QUARK_UNIT_TEST("vector", "size()", "BranchFactorSquarePlus1", "BranchFactorSquarePlus1"){
	test_fixture<int> f;
	const auto a = make_manual_vector_branchfactor_square_plus_1();
	VERIFY(a.size() == BRANCHING_FACTOR * BRANCHING_FACTOR + 1);
}


////////////////////////////////////////////		vector::vector(const std::vector<T>& vec)


QUARK_UNIT_TEST("vector", "vector(const std::vector<T>& vec)", "0 values", "empty"){
	test_fixture<int> f;
	const std::vector<int> a = {};
	vector<int> v(a);
	VERIFY(v.size() == 0);
}

QUARK_UNIT_TEST("vector", "vector(const std::vector<T>& vec)", "7 values", "read back all"){
	test_fixture<int> f;
	const std::vector<int> a = {	3, 4, 5, 6, 7, 8, 9	};
	vector<int> v(a);
	VERIFY(v.size() == 7);
	VERIFY(v[0] == 3);
	VERIFY(v[1] == 4);
	VERIFY(v[2] == 5);
	VERIFY(v[3] == 6);
	VERIFY(v[4] == 7);
	VERIFY(v[5] == 8);
	VERIFY(v[6] == 9);
}


////////////////////////////////////////////		vector::vector(const T values[], size_t count)

#ifndef _MSC_VER /* SB: Visual studio does not allow 0-length arrays */
QUARK_UNIT_TEST("vector", "vector(const T values[], size_t count)", "0 values", "empty"){
	test_fixture<int> f;
	const int a[] = {};
	vector<int> v(&a[0], 0);
	VERIFY(v.size() == 0);
}
#endif

QUARK_UNIT_TEST("vector", "vector(const T values[], size_t count)", "7 values", "read back all"){
	test_fixture<int> f;
	const int a[] = {	3, 4, 5, 6, 7, 8, 9	};
	vector<int> v(&a[0], 7);
	VERIFY(v.size() == 7);
	VERIFY(v[0] == 3);
	VERIFY(v[1] == 4);
	VERIFY(v[2] == 5);
	VERIFY(v[3] == 6);
	VERIFY(v[4] == 7);
	VERIFY(v[5] == 8);
	VERIFY(v[6] == 9);
}


////////////////////////////////////////////		vector::vector(std::initializer_list<T> args)


QUARK_UNIT_TEST("vector", "vector(std::initializer_list<T> args)", "0 values", "empty"){
	test_fixture<int> f;
	vector<int> v = {};
	VERIFY(v.size() == 0);
}

QUARK_UNIT_TEST("vector", "vector(std::initializer_list<T> args)", "7 values", "read back all"){
	test_fixture<int> f;
	vector<int> v = {	3, 4, 5, 6, 7, 8, 9	};
	VERIFY(v.size() == 7);
	VERIFY(v[0] == 3);
	VERIFY(v[1] == 4);
	VERIFY(v[2] == 5);
	VERIFY(v[3] == 6);
	VERIFY(v[4] == 7);
	VERIFY(v[5] == 8);
	VERIFY(v[6] == 9);
}


////////////////////////////////////////////		vector::to_vec()


QUARK_UNIT_TEST("vector", "to_vec()", "0", "empty"){
	test_fixture<int> f;
	const auto a = vector<int>();
	VERIFY(a.to_vec() == std::vector<int>());
}

QUARK_UNIT_TEST("vector", "to_vec()", "50", "correct data"){
	test_fixture<int> f;
	const auto data = generate_numbers(4, 50, 50);
	const auto a = vector<int>(data);
	VERIFY(a.to_vec() == data);
}


////////////////////////////////////////////		vector::vector(const vector& rhs)


QUARK_UNIT_TEST("vector", "vector(const vector& rhs)", "empty", "empty"){
	test_fixture<int> f;
	const auto a = vector<int>();
	const auto b(a);
	VERIFY(a.empty());
	VERIFY(b.empty());
}

template <class T>
bool same_root(const vector<T>& a, const vector<T>& b){
	if(a.get_root().get_type() == node_type::inode){
		return a.get_root().get_inode() == b.get_root().get_inode();
	}
	else{
		return a.get_root().get_leaf_node() == b.get_root().get_leaf_node();
	}
}

QUARK_UNIT_TEST("vector", "vector(const vector& rhs)", "7 values", "identical, sharing root"){
	test_fixture<int> f;
	const auto data = std::vector<int>{	3, 4, 5, 6, 7, 8, 9	};
	const vector<int> a = data;
	const auto b(a);

	VERIFY(a.to_vec() == data);
	VERIFY(b.to_vec() == data);
	VERIFY(same_root(a, b));
}



////////////////////////////////////////////		vector::operator=()


QUARK_UNIT_TEST("vector", "operator=()", "empty", "empty"){
	test_fixture<int> f;
	const auto a = vector<int>();
	auto b = vector<int>();

	b = a;

	VERIFY(a.empty());
	VERIFY(b.empty());
}

QUARK_UNIT_TEST("vector", "operator=()", "7 values", "identical, sharing root"){
	test_fixture<int> f;
	const auto data = std::vector<int>{	3, 4, 5, 6, 7, 8, 9	};
	const vector<int> a = data;
	auto b = vector<int>();

	b = a;

	VERIFY(a.to_vec() == data);
	VERIFY(b.to_vec() == data);
	VERIFY(same_root(a, b));
}


////////////////////////////////////////////		operator+()


QUARK_UNIT_TEST("vector", "operator+()", "3 + 4 values", "7 values"){
	test_fixture<int> f;
	const vector<int> a{ 2, 3, 4 };
	const vector<int> b{ 5, 6, 7, 8 };

	const auto c = a + b;

	VERIFY(c.to_vec() == (std::vector<int>{ 2, 3, 4, 5, 6, 7, 8 }));
}



////////////////////////////////////////////		T = std::string



QUARK_UNIT_TEST("vector", "operator+()", "3 + 4 values", "7 values"){
	using std::string;
	test_fixture<string> f;

	const steady::vector<string> a{ "one", "two", "three" };

	const steady::vector<string> b{ "four", "five" };
	const auto c = a + b;

	assert(a == (steady::vector<string>{ "one", "two", "three" }));
	assert(b == (steady::vector<string>{ "four", "five" }));
	assert(c == (steady::vector<string>{ "one", "two", "three", "four", "five" }));
}


}	//	steady
