//
//  floyd_llvm_types.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-01.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_types.h"

#include <llvm/IR/Module.h>

namespace floyd {



llvm::Type* deref_ptr(llvm::Type* type){
	QUARK_ASSERT(type != nullptr);

	if(type->isPointerTy()){
		llvm::PointerType* type2 = llvm::cast<llvm::PointerType>(type);
  		llvm::Type* element_type = type2->getElementType();
  		return element_type;
	}
	else{
		QUARK_ASSERT(false);
		return type;
	}
}


////////////////////////////////	runtime_type_t


static llvm::StructType* make_exact_struct_type(llvm::LLVMContext& context, const llvm_type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.is_struct());

	std::vector<llvm::Type*> members;
	for(const auto& m: type.get_struct_ref()->_members){
		const auto m2 = get_exact_llvm_type(interner, m._type);
		members.push_back(m2);
	}
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
}



llvm::Type* make_runtime_type_type(llvm::LLVMContext& context){
	return llvm::Type::getInt64Ty(context);
/*
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt32Ty(context)
	};
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
*/

}

runtime_type_t make_runtime_type(int32_t itype){
	return runtime_type_t{ itype };
}

llvm::Type* make_runtime_value_type(llvm::LLVMContext& context){
	return llvm::Type::getInt64Ty(context);
}


////////////////////////////////		llvm_function_def_t



llvm_function_def_t name_args(const llvm_function_def_t& def, const std::vector<member_t>& args){
	QUARK_ASSERT(def.check_invariant());

	if(args.empty()){
		QUARK_ASSERT(def.args.size() == 1);
		return def;
	}
	else{
		//	Skip arg #0, which is "floyd_runtime_ptr".
		const auto floyd_arg_count = def.args.back().floyd_arg_index + 1;
		QUARK_ASSERT(floyd_arg_count == args.size());

		std::vector<llvm_arg_mapping_t> arg_results;

		for(int out_index = 0 ; out_index < def.args.size() ; out_index++){
			auto arg_copy = def.args[out_index];
			if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr){
			}
			else if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_known_value_type){
				auto floyd_arg_index = arg_copy.floyd_arg_index;
				const auto& floyd_arg = args[floyd_arg_index];
				QUARK_ASSERT(arg_copy.floyd_type == floyd_arg._type);

				arg_copy.floyd_name = floyd_arg._name;
			}
			else if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_dyn_value){
				auto floyd_arg_index = arg_copy.floyd_arg_index;
				const auto& floyd_arg = args[floyd_arg_index];
				QUARK_ASSERT(arg_copy.floyd_type == floyd_arg._type);

				arg_copy.floyd_name = floyd_arg._name + "-dynval";
			}
			else if(arg_copy.map_type == llvm_arg_mapping_t::map_type::k_dyn_type){
				auto floyd_arg_index = arg_copy.floyd_arg_index;
				const auto& floyd_arg = args[floyd_arg_index];

				arg_copy.floyd_name = floyd_arg._name + "-dyntype";
			}
			else{
				QUARK_ASSERT(false);
			}
			arg_results.push_back(arg_copy);
		}

		return llvm_function_def_t { def.return_type, arg_results, def.llvm_args };
	}
}


////////////////////////////////		llvm_type_interner_t()




//	Function-types are always returned as pointer-to-function types.
static llvm::Type* make_function_type_internal(llvm::LLVMContext& context, const llvm_type_interner_t& interner, const typeid_t& function_type){
	QUARK_ASSERT(function_type.check_invariant());
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(function_type.is_function());

	const auto mapping = map_function_arguments(context, interner, function_type);
	llvm::FunctionType* function_type2 = llvm::FunctionType::get(mapping.return_type, mapping.llvm_args, false);
	auto function_pointer_type = function_type2->getPointerTo();
	return function_pointer_type;
}
static llvm::Type* make_exact_type_internal(llvm::LLVMContext& context, llvm_type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		llvm::LLVMContext& context;
		llvm_type_interner_t& interner;
		const typeid_t& type;

		llvm::Type* operator()(const typeid_t::undefined_t& e) const{
			return llvm::Type::getInt16Ty(context);
		}
		llvm::Type* operator()(const typeid_t::any_t& e) const{
			return make_runtime_value_type(context);
		}

		llvm::Type* operator()(const typeid_t::void_t& e) const{
			return llvm::Type::getVoidTy(context);
		}
		llvm::Type* operator()(const typeid_t::bool_t& e) const{
			return llvm::Type::getInt1Ty(context);
		}
		llvm::Type* operator()(const typeid_t::int_t& e) const{
			return llvm::Type::getInt64Ty(context);
		}
		llvm::Type* operator()(const typeid_t::double_t& e) const{
			return llvm::Type::getDoubleTy(context);
		}
		llvm::Type* operator()(const typeid_t::string_t& e) const{
			return make_generic_vec_type(interner)->getPointerTo();
		}

		llvm::Type* operator()(const typeid_t::json_type_t& e) const{
			return make_json_type(interner)->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::typeid_type_t& e) const{
			return make_runtime_type_type(context);
		}

		llvm::Type* operator()(const typeid_t::struct_t& e) const{
			return make_exact_struct_type(context, interner, type)->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::vector_t& e) const{
			return make_generic_vec_type(interner)->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::dict_t& e) const{
			return make_generic_dict_type(interner)->getPointerTo();
		}
		llvm::Type* operator()(const typeid_t::function_t& e) const{
			return make_function_type_internal(context, interner, type);
		}
		llvm::Type* operator()(const typeid_t::unresolved_t& e) const{
			UNSUPPORTED();
			return llvm::Type::getInt16Ty(context);
		}
	};
	return std::visit(visitor_t{ context, interner, type }, type._contents);
}


static llvm::StructType* make_wide_return_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		//	a
		llvm::Type::getInt64Ty(context),

		//	b
		llvm::Type::getInt64Ty(context)
	};
	llvm::StructType* s = llvm::StructType::get(context, members, false);
	return s;
}

static llvm::StructType* make_generic_vec_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context),
		llvm::Type::getInt64Ty(context)
	};

	llvm::StructType* s = llvm::StructType::create(context, members, "vec");
	return s;
}

static llvm::StructType* make_generic_dict_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt64Ty(context)->getPointerTo()
	};
	llvm::StructType* s = llvm::StructType::create(context, members, "dict");
	return s;
}

static llvm::StructType* make_json_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt64Ty(context)->getPointerTo()
	};
	llvm::StructType* s = llvm::StructType::create(context, members, "json");
	return s;
}

static llvm::StructType* make_generic_runtime_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt16Ty(context)
	};
	llvm::StructType* s = llvm::StructType::create(context, members, "frp");
	return s;
}

static llvm::StructType* make_generic_struct_type_internal(llvm::LLVMContext& context){
	std::vector<llvm::Type*> members = {
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo(),
		llvm::Type::getInt64Ty(context)->getPointerTo()
	};
	llvm::StructType* s = llvm::StructType::create(context, members, "struct");
	return s;
}





//??? Doesn't work if a type references a type later in the interner vector.
llvm_type_interner_t::llvm_type_interner_t(llvm::LLVMContext& context, const type_interner_t& i){
	QUARK_ASSERT(i.check_invariant());

	generic_vec_type = make_generic_vec_type_internal(context);
	generic_dict_type = make_generic_dict_type_internal(context);
	json_type = make_json_type_internal(context);
	generic_struct_type = make_generic_struct_type_internal(context);
	wide_return_type = make_wide_return_type_internal(context);
	runtime_ptr_type = make_generic_runtime_type_internal(context)->getPointerTo();

	for(const auto& e: i.interned){
		const auto llvm_type = make_exact_type_internal(context, *this, e.second);
		intern_type(interner, e.second);
		exact_llvm_types.push_back(llvm_type);
	}

	QUARK_ASSERT(check_invariant());
}

bool llvm_type_interner_t::check_invariant() const {
	QUARK_ASSERT(interner.check_invariant());
//	QUARK_ASSERT(interner.interned.size() == exact_llvm_types.size());

	QUARK_ASSERT(generic_vec_type != nullptr);
	QUARK_ASSERT(generic_dict_type != nullptr);
	QUARK_ASSERT(json_type != nullptr);
	QUARK_ASSERT(generic_struct_type != nullptr);
	QUARK_ASSERT(wide_return_type != nullptr);
	return true;
}


//??? Make get_exact_llvm_type() return vector, struct etc. directly, not getPointerTo().
llvm::StructType* get_exact_struct_type(const llvm_type_interner_t& i, const typeid_t& type){
	QUARK_ASSERT(i.check_invariant());
	QUARK_ASSERT(type.is_struct());

	const auto it = std::find_if(i.interner.interned.begin(), i.interner.interned.end(), [&](const std::pair<itype_t, typeid_t>& e){ return e.second == type; });
	if(it == i.interner.interned.end()){
		throw std::exception();
	}
	const auto index = it - i.interner.interned.begin();
	QUARK_ASSERT(index >= 0 && index < i.exact_llvm_types.size());
	auto result = i.exact_llvm_types[index];

	auto result2 = deref_ptr(result);
	return llvm::cast<llvm::StructType>(result2);
//	return make_exact_struct_type(context, interner, type)->getPointerTo();

/*
	auto result = get_exact_llvm_type(interner, type);
	auto result2 = deref_ptr(result);
	return llvm::cast<llvm::StructType>(result2);
*/
}

llvm::Type* get_exact_llvm_type(const llvm_type_interner_t& i, const typeid_t& type){
	QUARK_ASSERT(i.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	if(type.is_vector()){
		return i.generic_vec_type->getPointerTo();
	}
	else if(type.is_dict()){
		return i.generic_dict_type->getPointerTo();
	}
	else if(type.is_struct()){
		return i.generic_struct_type->getPointerTo();
	}
	else{
		const auto it = std::find_if(i.interner.interned.begin(), i.interner.interned.end(), [&](const std::pair<itype_t, typeid_t>& e){ return e.second == type; });
		if(it == i.interner.interned.end()){
			throw std::exception();
		}
		const auto index = it - i.interner.interned.begin();
		QUARK_ASSERT(index >= 0 && index < i.exact_llvm_types.size());
		return i.exact_llvm_types[index];
	}
}

llvm::Type* make_function_type(const llvm_type_interner_t& interner, const typeid_t& function_type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(function_type.check_invariant());

	return get_exact_llvm_type(interner, function_type);
}

llvm::StructType* make_wide_return_type(const llvm_type_interner_t& interner){
	QUARK_ASSERT(interner.check_invariant());

	return interner.wide_return_type;
}

llvm::Type* make_generic_vec_type(const llvm_type_interner_t& interner){
	QUARK_ASSERT(interner.check_invariant());

	return interner.generic_vec_type;
}

llvm::Type* make_generic_dict_type(const llvm_type_interner_t& interner){
	QUARK_ASSERT(interner.check_invariant());

	return interner.generic_dict_type;
}

llvm::Type* make_json_type(const llvm_type_interner_t& interner){
	QUARK_ASSERT(interner.check_invariant());

	return interner.json_type;
}

llvm::Type* get_generic_struct_type(const llvm_type_interner_t& interner){
	QUARK_ASSERT(interner.check_invariant());

	return interner.generic_struct_type;
}

llvm_function_def_t map_function_arguments(llvm::LLVMContext& context, const llvm_type_interner_t& interner, const floyd::typeid_t& function_type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(function_type.is_function());

	const auto ret = function_type.get_function_return();
	llvm::Type* return_type = ret.is_any() ? make_wide_return_type(interner) : get_exact_llvm_type(interner, ret);

	const auto args = function_type.get_function_args();
	std::vector<llvm_arg_mapping_t> arg_results;

	//	Pass Floyd runtime as extra, hidden argument #0. It has no representation in Floyd function type.
	arg_results.push_back({ make_frp_type(interner), "floyd_runtime_ptr", floyd::typeid_t::make_undefined(), -1, llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr });

	for(int index = 0 ; index < args.size() ; index++){
		const auto& arg = args[index];
		QUARK_ASSERT(arg.is_undefined() == false);
		QUARK_ASSERT(arg.is_void() == false);

		//	For dynamic values, store its dynamic type as an extra argument.
		if(arg.is_any()){
			arg_results.push_back({ make_runtime_value_type(context), std::to_string(index), arg, index, llvm_arg_mapping_t::map_type::k_dyn_value });
			arg_results.push_back({ make_runtime_type_type(context), std::to_string(index), typeid_t::make_undefined(), index, llvm_arg_mapping_t::map_type::k_dyn_type });
		}
		else {
			auto arg_itype = get_exact_llvm_type(interner, arg);
			arg_results.push_back({ arg_itype, std::to_string(index), arg, index, llvm_arg_mapping_t::map_type::k_known_value_type });
		}
	}

	std::vector<llvm::Type*> llvm_args;
	for(const auto& e: arg_results){
		llvm_args.push_back(e.llvm_type);
	}

	return llvm_function_def_t { return_type, arg_results, llvm_args };
}



llvm::Type* make_frp_type(const llvm_type_interner_t& interner){
	QUARK_ASSERT(interner.check_invariant());

	return interner.runtime_ptr_type;
}




////////////////////////////////		TESTS




static llvm_type_interner_t make_basic_interner(llvm::LLVMContext& context){
	type_interner_t temp;
	intern_type(temp, typeid_t::make_void());
	intern_type(temp, typeid_t::make_int());
	intern_type(temp, typeid_t::make_bool());
	intern_type(temp, typeid_t::make_string());
	return llvm_type_interner_t(context, temp);
}


static llvm_type_interner_t make_basic_interner(llvm::LLVMContext& context);


QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func void()", ""){
	llvm::LLVMContext context;
	const auto interner = make_basic_interner(context);
	auto module = std::make_unique<llvm::Module>("test", context);

	const auto r = map_function_arguments(
		context,
		interner,
		typeid_t::make_function(typeid_t::make_void(), {}, epure::pure)
	);

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isVoidTy());

	QUARK_UT_VERIFY(r.args.size() == 1);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);
}

QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func int()", ""){
	llvm::LLVMContext context;
	const auto interner = make_basic_interner(context);
	auto module = std::make_unique<llvm::Module>("test", context);

	const auto r = map_function_arguments(context, interner, typeid_t::make_function(typeid_t::make_int(), {}, epure::pure));

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isIntegerTy(64));

	QUARK_UT_VERIFY(r.args.size() == 1);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);
}

QUARK_UNIT_TEST("LLVM Codegen", "map_function_arguments()", "func void(int)", ""){
	llvm::LLVMContext context;
	const auto interner = make_basic_interner(context);
	auto module = std::make_unique<llvm::Module>("test", context);

	const auto r = map_function_arguments(context, interner, typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_int() }, epure::pure));

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isVoidTy());

	QUARK_UT_VERIFY(r.args.size() == 2);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);

	QUARK_UT_VERIFY(r.args[1].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[1].floyd_name == "0");
	QUARK_UT_VERIFY(r.args[1].floyd_type.is_int());
	QUARK_UT_VERIFY(r.args[1].floyd_arg_index == 0);
	QUARK_UT_VERIFY(r.args[1].map_type == llvm_arg_mapping_t::map_type::k_known_value_type);
}

QUARK_UNIT_TEST
("LLVM Codegen", "map_function_arguments()", "func void(int, DYN, bool)", ""){
	llvm::LLVMContext context;
	const auto interner = make_basic_interner(context);
	auto module = std::make_unique<llvm::Module>("test", context);

	const auto r = map_function_arguments(context, interner, typeid_t::make_function(typeid_t::make_void(), { typeid_t::make_int(), typeid_t::make_any(), typeid_t::make_bool() }, epure::pure));

	QUARK_UT_VERIFY(r.return_type != nullptr);
	QUARK_UT_VERIFY(r.return_type->isVoidTy());

	QUARK_UT_VERIFY(r.args.size() == 5);

	QUARK_UT_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_UT_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_UT_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_UT_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);

	QUARK_UT_VERIFY(r.args[1].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[1].floyd_name == "0");
	QUARK_UT_VERIFY(r.args[1].floyd_type.is_int());
	QUARK_UT_VERIFY(r.args[1].floyd_arg_index == 0);
	QUARK_UT_VERIFY(r.args[1].map_type == llvm_arg_mapping_t::map_type::k_known_value_type);

	QUARK_UT_VERIFY(r.args[2].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[2].floyd_name == "1");
	QUARK_UT_VERIFY(r.args[2].floyd_type.is_any());
	QUARK_UT_VERIFY(r.args[2].floyd_arg_index == 1);
	QUARK_UT_VERIFY(r.args[2].map_type == llvm_arg_mapping_t::map_type::k_dyn_value);

	QUARK_UT_VERIFY(r.args[3].llvm_type->isIntegerTy(64));
	QUARK_UT_VERIFY(r.args[3].floyd_name == "1");
	QUARK_UT_VERIFY(r.args[3].floyd_type.is_undefined());
	QUARK_UT_VERIFY(r.args[3].floyd_arg_index == 1);
	QUARK_UT_VERIFY(r.args[3].map_type == llvm_arg_mapping_t::map_type::k_dyn_type);

	QUARK_UT_VERIFY(r.args[4].llvm_type->isIntegerTy(1));
	QUARK_UT_VERIFY(r.args[4].floyd_name == "2");
	QUARK_UT_VERIFY(r.args[4].floyd_type.is_bool());
	QUARK_UT_VERIFY(r.args[4].floyd_arg_index == 2);
	QUARK_UT_VERIFY(r.args[4].map_type == llvm_arg_mapping_t::map_type::k_known_value_type);
}




////////////////////////////////	type_interner_t helpers



/*
base_type get_base_type(const type_interner_t& interner, const runtime_type_t& type){
	const auto a = lookup_type(interner, type);
	const auto a_basetype = a.get_base_type();

	//??? We know ranges where type.itype maps to base_type -- no need to look up in type_interner.
	return a_basetype;
}


typeid_t lookup_type(const type_interner_t& interner, const runtime_type_t& type){
	const auto it = std::find_if(interner.interned.begin(), interner.interned.end(), [&](const std::pair<itype_t, typeid_t>& e){ return e.first.itype == type; });
	if(it != interner.interned.end()){
		return it->second;
	}
	throw std::exception();
}
*/
runtime_type_t lookup_runtime_type(const llvm_type_interner_t& interner, const typeid_t& type){
	QUARK_ASSERT(interner.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto a = lookup_itype(interner.interner, type);
	return make_runtime_type(a.itype);
}


}	// floyd
