//
//  floyd_llvm_types.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-01.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_llvm_types.h"

#include "floyd_llvm_helpers.h"
#include "format_table.h"

#include <llvm/IR/Module.h>

namespace floyd {

struct builder_t;


bool pass_as_ptr(const type_desc_t& desc){
	if(
		desc.is_string()
		|| desc.is_json()
		|| desc.is_struct()
		|| desc.is_vector()
		|| desc.is_dict()
		|| desc.is_function()
	){
		return true;
	}
	else {
		return false;
	}
}


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

		return llvm_function_def_t { def.return_type, arg_results };
	}
}






/*
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
*/

static llvm::StructType* make_generic_vec_type_internal(llvm::LLVMContext& context){
	const auto n = sizeof(heap_alloc_64_t);
	QUARK_ASSERT((n % 8) == 0);

	const std::vector<llvm::Type*> members(n / 8, llvm::Type::getInt64Ty(context));
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
	const auto n = sizeof(heap_alloc_64_t);
	QUARK_ASSERT((n % 8) == 0);

	const std::vector<llvm::Type*> members(n / 8, llvm::Type::getInt64Ty(context));
	llvm::StructType* s = llvm::StructType::create(context, members, "struct");
	return s;
}




struct builder_t {
	llvm::LLVMContext& context;
	state_t acc;
};

static const type_entry_t& touch_type(builder_t& builder, const type_t& type, const type_name_t& optional_name);

static llvm::Type* get_llvm_type_prefer_generic(const type_entry_t& entry){
	if(entry.llvm_type_generic != nullptr){
		return entry.llvm_type_generic;
	}
	else{
		return entry.llvm_type_specific;
	}
}

static llvm_function_def_t map_function_arguments_internal(
	builder_t& builder,
	const floyd::type_t& function_type
){
	const auto& types = builder.acc.types;

	const auto function_peek = peek2(types, function_type);
	QUARK_ASSERT(function_peek.is_function());

	const auto ret = function_peek.get_function_return(types);

	// Notice: we always resolve the return type in semantic analysis -- no need to
	//	use WIDE_RETURN and provide a dynamic type. We use int64 here and cast it when calling.
	llvm::Type* return_type = nullptr;
	if(peek2(types, ret).is_any()){
	 	return_type = llvm::Type::getInt64Ty(builder.context);
	}
	else {
		const auto& a = touch_type(builder, ret, make_empty_type_name());
		return_type = get_llvm_type_prefer_generic(a);
	}

	const auto args = function_peek.get_function_args(types);
	std::vector<llvm_arg_mapping_t> arg_results;

	auto frp_type = builder.acc.runtime_ptr_type;

	//	Pass Floyd runtime as extra, hidden argument #0. It has no representation
	//	in Floyd function type.
	arg_results.push_back({
		frp_type,
		"floyd_runtime_ptr",
		make_undefined(),
		-1,
		llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr
	});

	for(int index = 0 ; index < args.size() ; index++){
		const auto& arg = args[index];
		QUARK_ASSERT(arg.is_undefined() == false);
		QUARK_ASSERT(peek2(types, arg).is_void() == false);

		//	For dynamic values, store its dynamic type as an extra argument.
		if(peek2(types, arg).is_any()){
			arg_results.push_back({
				builder.acc.runtime_value_type,
				std::to_string(index),
				arg,
				index,
				llvm_arg_mapping_t::map_type::k_dyn_value
			});
			arg_results.push_back({
				builder.acc.runtime_type_type,
				std::to_string(index),
				make_undefined(),
				index,
				llvm_arg_mapping_t::map_type::k_dyn_type
			});
		}
		else {
			const auto& a = touch_type(builder, arg, make_empty_type_name());
			auto arg_type = get_llvm_type_prefer_generic(a);
			arg_results.push_back({
				arg_type,
				std::to_string(index),
				arg,
				index,
				llvm_arg_mapping_t::map_type::k_known_value_type
			});
		}
	}

	std::vector<llvm::Type*> llvm_args;
	for(const auto& e: arg_results){
		llvm_args.push_back(e.llvm_type);
	}

	return llvm_function_def_t { return_type, arg_results };
}


//	Function-types are always returned as byvalue.
static llvm::Type* make_function_type(builder_t& builder, const type_t& function_type){
	QUARK_ASSERT(function_type.check_invariant());
	QUARK_ASSERT(peek2(builder.acc.types, function_type).is_function());

	const auto mapping = map_function_arguments_internal(builder, function_type);
	const auto llvm_args = mapf<llvm::Type*>(mapping.args, [&](const auto& e){ return e.llvm_type; });
	llvm::FunctionType* function_type2 = llvm::FunctionType::get(
		mapping.return_type,
		llvm_args,
		false
	);
	return function_type2;
}





//	http://llvm.org/doxygen/classllvm_1_1StructType.html#aa683538f3d55dd3717fbc7a12595654e
//create (LLVMContext &Context, StringRef Name)
//??? Need to skip type nodes that are partially undefined or have symbols in them.
//??? Better to erase nodes with symbols from node list before codegen?

//??? Types can be recursive and type nodes can be in order (named_type, struct).
//	We need recursive creation of LLVM types.

static const type_entry_t& make_anonymous_struct(builder_t& builder, const type_t& type){
	QUARK_ASSERT(type.check_invariant());

	const auto& types = builder.acc.types;
	const auto type_peek = peek2(types, type);
	QUARK_ASSERT(type_peek.is_struct());
	QUARK_ASSERT(is_wellformed(types, type));

	const auto type_index = type.get_lookup_index();

	std::vector<llvm::Type*> members;
	for(const auto& m: type_peek.get_struct(types)._members){
		const auto member_type = m._type;
		const auto& a = touch_type(builder, member_type, make_empty_type_name());
		QUARK_ASSERT(a.use_flag);

		const auto m2 = get_llvm_type_prefer_generic(a);
		members.push_back(m2);
	}
	llvm::StructType* s = llvm::StructType::get(builder.context, members, false);
//	QUARK_TRACE(print_type(s));

	const auto llvm_type = s->getPointerTo();
	llvm::Type* llvm_generic_type = builder.acc.generic_struct_type->getPointerTo();

	const auto entry = type_entry_t{ true, llvm_type, llvm_generic_type, nullptr };
	builder.acc.type_entries[type_index] = entry;
	return builder.acc.type_entries[type_index];
}

static const type_entry_t& make_named_struct(builder_t& builder, const type_t& type, const type_name_t& name){
	QUARK_ASSERT(type.check_invariant());
	QUARK_ASSERT(name.check_invariant());

	const auto& types = builder.acc.types;
	const auto type_peek = peek2(types, type);
	QUARK_ASSERT(type_peek.is_struct());
	QUARK_ASSERT(is_wellformed(types, type));

	const auto type_index = type.get_lookup_index();

	if(builder.acc.type_entries[type_index].llvm_type_specific == nullptr){
		const auto name2 = pack_type_name(name);

		//	1) Create an LLVM struct with the correct (program-unique) name and store it in type_entries[] even though it's incomplete. This let's recursive calls find and reference the new struct, even if it's incomplete.

		auto s = llvm::StructType::create(builder.context, name2);

		const auto llvm_type = s->getPointerTo();
		llvm::Type* llvm_generic_type = builder.acc.generic_struct_type->getPointerTo();

		const auto entry = type_entry_t{ true, llvm_type, llvm_generic_type, nullptr };
		builder.acc.type_entries[type_index] = entry;



		//	2) Add memebers.

		std::vector<llvm::Type*> members;
		for(const auto& m: type_peek.get_struct(types)._members){
			const auto member_type = m._type;
			const auto& a = touch_type(builder, member_type, make_empty_type_name());
			QUARK_ASSERT(a.use_flag);

			const auto memory_type_peek = peek2(types, member_type);
			QUARK_ASSERT(a.llvm_type_specific != nullptr);
			if(memory_type_peek.is_struct()){
				QUARK_ASSERT(a.llvm_type_generic != nullptr);
			}

			const auto m2 = get_llvm_type_prefer_generic(a);
			members.push_back(m2);
		}
		s->setBody(members);
//		llvm::StructType* s = llvm::StructType::get(builder.context, members, false);
	//	QUARK_TRACE(print_type(s));

		return builder.acc.type_entries[type_index];
	}
	else{
		return builder.acc.type_entries[type_index];
	}
}

static const type_entry_t& make_llvm_struct_type(builder_t& builder, const type_t& type, const type_name_t& optional_name){
	const auto& types = builder.acc.types;
	const auto type_peek = peek2(types, type);
	QUARK_ASSERT(type_peek.is_struct());
	QUARK_ASSERT(is_wellformed(types, type));

	if(is_empty_type_name(optional_name)){
		return make_anonymous_struct(builder, type);
	}
	else{
		return make_named_struct(builder, type, optional_name);
	}
}

static const type_entry_t& touch_type(builder_t& builder, const type_t& type, const type_name_t& optional_name){
	QUARK_ASSERT(builder.acc.types.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	struct visitor_t {
		builder_t& builder;
		const type_t& type;
		const type_lookup_index_t type_index;
		const type_name_t optional_name;


		void operator()(const undefined_t& e) const{
			const auto entry = type_entry_t{ true, llvm::Type::getInt16Ty(builder.context), nullptr, nullptr };
			builder.acc.type_entries[type_index] = entry;
		}
		void operator()(const any_t& e) const{
			const auto entry = type_entry_t{ true, builder.acc.runtime_value_type, nullptr, nullptr	};
			builder.acc.type_entries[type_index] = entry;
		}

		void operator()(const void_t& e) const{
			const auto entry = type_entry_t{ true, llvm::Type::getVoidTy(builder.context), nullptr, nullptr	};
			builder.acc.type_entries[type_index] = entry;
		}
		void operator()(const bool_t& e) const{
			const auto entry = type_entry_t{ true, llvm::Type::getInt1Ty(builder.context), nullptr, nullptr	};
			builder.acc.type_entries[type_index] = entry;
		}
		void operator()(const int_t& e) const{
			const auto entry = type_entry_t{ true, llvm::Type::getInt64Ty(builder.context), nullptr, nullptr };
			builder.acc.type_entries[type_index] = entry;
		}
		void operator()(const double_t& e) const{
			const auto entry = type_entry_t{ true, llvm::Type::getDoubleTy(builder.context), nullptr, nullptr };
			builder.acc.type_entries[type_index] = entry;
		}
		void operator()(const string_t& e) const{
			const auto llvm_type = builder.acc.generic_vec_type->getPointerTo();
			const auto entry = type_entry_t{ true, llvm_type, llvm_type, nullptr };
			builder.acc.type_entries[type_index] = entry;
		}

		void operator()(const json_type_t& e) const{
			const auto llvm_type = builder.acc.json_type->getPointerTo();
			const auto entry = type_entry_t{ true, llvm_type, nullptr, nullptr };
			builder.acc.type_entries[type_index] = entry;
		}
		void operator()(const typeid_type_t& e) const{
			const auto t = builder.acc.runtime_type_type;
			const auto entry = type_entry_t{ true, t, nullptr, nullptr };
			builder.acc.type_entries[type_index] = entry;
		}

		void operator()(const struct_t& e) const{
			make_llvm_struct_type(builder, type, optional_name);
		}
		void operator()(const vector_t& e) const{
			const auto llvm_type = builder.acc.generic_vec_type->getPointerTo();
			const auto entry = type_entry_t{ true, llvm_type, llvm_type, nullptr };
			builder.acc.type_entries[type_index] = entry;
		}
		void operator()(const dict_t& e) const{
			const auto llvm_type = builder.acc.generic_dict_type->getPointerTo();
			const auto entry = type_entry_t{ true, llvm_type, llvm_type, nullptr };
			builder.acc.type_entries[type_index] = entry;
		}
		void operator()(const function_t& e) const{
			const auto llvm_type = make_function_type(builder, type)->getPointerTo();

			std::shared_ptr<const llvm_function_def_t> optional_function_def;
			if(peek2(builder.acc.types, type).is_function()){
				const auto function_def = map_function_arguments_internal(builder, type);
				optional_function_def = std::make_shared<llvm_function_def_t>(function_def);
			}

			const auto entry = type_entry_t{
				true,
				llvm_type,
				nullptr,
				optional_function_def
			};
			builder.acc.type_entries[type_index] = entry;
		}
		void operator()(const symbol_ref_t& e) const {
			const auto t = llvm::Type::getInt8Ty(builder.context);
			const auto entry = type_entry_t{ true, t, nullptr, nullptr };
			builder.acc.type_entries[type_index] = entry;
		}
		void operator()(const named_type_t& e) const {
			const auto dest_type = e.destination_type;
			const auto name = type.get_named_type(builder.acc.types);
			const auto entry = touch_type(builder, dest_type, name);
			builder.acc.type_entries[type_index] = entry;
		}
	};
	std::visit(visitor_t{ builder, type, type.get_lookup_index(), optional_name }, get_type_variant(builder.acc.types, type));
	return builder.acc.type_entries[type.get_lookup_index()];
}

//	Notice: the entries in the types may reference eachother = we need to process recursively.
llvm_type_lookup::llvm_type_lookup(llvm::LLVMContext& context, const types_t& types){
	QUARK_ASSERT(types.check_invariant());

	state_t acc;
	acc.types = types;

	//	Make an entry for each entry in types
	acc.type_entries = std::vector<type_entry_t>(types.nodes.size(), type_entry_t());


	acc.generic_vec_type = make_generic_vec_type_internal(context);
	acc.generic_dict_type = make_generic_dict_type_internal(context);
	acc.json_type = make_json_type_internal(context);
	acc.generic_struct_type = make_generic_struct_type_internal(context);
//	acc.wide_return_type = make_wide_return_type_internal(context);
	acc.runtime_ptr_type = make_generic_runtime_type_internal(context)->getPointerTo();

	acc.runtime_type_type = llvm::Type::getInt64Ty(context);
	acc.runtime_value_type = llvm::Type::getInt64Ty(context);


	builder_t builder { context, acc };

	QUARK_ASSERT(builder.acc.types.check_invariant());
	for(type_lookup_index_t i = 0 ; i < acc.types.nodes.size() ; i++){
		const auto& type = lookup_type_from_index(acc.types, i);
		QUARK_ASSERT(type.check_invariant());

		const auto wellformed = is_wellformed(builder.acc.types, type);
		if(wellformed){
			touch_type(builder, type, make_empty_type_name());
		}
	}

	state = builder.acc;

	QUARK_ASSERT(check_invariant());

	if(false) trace_llvm_type_lookup(*this);
}




bool llvm_type_lookup::check_invariant() const {
	QUARK_ASSERT(state.generic_vec_type != nullptr);
	QUARK_ASSERT(state.generic_dict_type != nullptr);
	QUARK_ASSERT(state.json_type != nullptr);
	QUARK_ASSERT(state.generic_struct_type != nullptr);

//	QUARK_ASSERT(state.wide_return_type != nullptr);

	QUARK_ASSERT(state.types.check_invariant());
	QUARK_ASSERT(state.types.nodes.size() == state.type_entries.size());
	return true;
}

const type_entry_t& llvm_type_lookup::find_from_type(const type_t& type) const {
	QUARK_ASSERT(check_invariant());

	const auto index = type.get_lookup_index();
	return state.type_entries[index];
}

void trace_llvm_type_lookup(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	{
		std::vector<std::vector<std::string>> matrix;

		for(int i = 0 ; i < type_lookup.state.type_entries.size() ; i++){
			const auto& e = type_lookup.state.type_entries[i];
			const auto type = lookup_type_from_index(type_lookup.state.types, i);
			const auto l = std::vector<std::string> {
				{
					std::to_string(i),
					e.use_flag ? "USE" : "",
					type_to_compact_string(type_lookup.state.types, type),
					print_type(e.llvm_type_specific),
					print_type(e.llvm_type_generic),
					e.optional_function_def != nullptr ? "YES" : ""
				}
			};
			matrix.push_back(l);
		}

		const auto s = generate_table_type1({ "#", "type_t", "use", "llvm_type_specific", "llvm_type_generic", "optional_function_def" }, matrix);
		QUARK_TRACE(s);
	}

#if 0
	{
		QUARK_SCOPE_TRACE("LLVM IDENTIFIED STRUCT TYPES");

		const std::vector<StructType *> getIdentifiedStructTypes() const;


		std::vector<std::vector<std::string>> matrix;

		for(int i = 0 ; i < type_lookup.state.type_entries.size() ; i++){
			const auto& e = type_lookup.state.type_entries[i];
			const auto type = lookup_type_from_index(type_lookup.state.types, i);
			const auto l = std::vector<std::string> {
				{
					std::to_string(i),
					e.use_flag ? "USE" : "",
					type_to_compact_string(type_lookup.state.types, type),
					print_type(e.llvm_type_specific),
					print_type(e.llvm_type_generic),
					e.optional_function_def != nullptr ? "YES" : ""
				}
			};
			matrix.push_back(l);
		}

		const auto s = generate_table_type1({ "#", "type_t", "use", "llvm_type_specific", "llvm_type_generic", "optional_function_def" }, matrix);
		QUARK_TRACE(s);
	}
#endif

}




llvm::Type* make_runtime_type_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.runtime_type_type;
}

llvm::Type* make_runtime_value_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.runtime_value_type;
}


type_t lookup_type(const llvm_type_lookup& type_lookup, const type_t& type){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type;
}

llvm::StructType* get_exact_struct_type_byvalue(const llvm_type_lookup& i, const type_t& type){
	QUARK_ASSERT(i.check_invariant());
	QUARK_ASSERT(peek2(i.state.types, type).is_struct());

	const auto& entry = i.find_from_type(type);
	auto result = entry.llvm_type_specific;
	auto result2 = deref_ptr(result);
	return llvm::cast<llvm::StructType>(result2);
}

llvm::Type* get_llvm_type_as_arg(const llvm_type_lookup& i, const type_t& type){
	QUARK_ASSERT(i.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	const auto& entry = i.find_from_type(type);
	if(entry.llvm_type_generic != nullptr){
		return entry.llvm_type_generic;
	}
	else{
		return entry.llvm_type_specific;
	}
}


llvm::FunctionType* get_llvm_function_type(const llvm_type_lookup& type_lookup, const type_t& type){
	QUARK_ASSERT(type_lookup.check_invariant());
	QUARK_ASSERT(type.check_invariant());

	auto t = get_llvm_type_as_arg(type_lookup, type);
	return llvm::cast<llvm::FunctionType>(t);
}



llvm::Type* make_generic_vec_type_byvalue(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.generic_vec_type;
}

llvm::Type* make_generic_dict_type_byvalue(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.generic_dict_type;
}

llvm::Type* make_json_type_byvalue(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.json_type;
}

llvm::Type* get_generic_struct_type_byvalue(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.generic_struct_type;
}

llvm::Type* make_frp_type(const llvm_type_lookup& type_lookup){
	QUARK_ASSERT(type_lookup.check_invariant());

	return type_lookup.state.runtime_ptr_type;
}


////////////////////////////////		TESTS


/*
 |54     |/lexical_scope1/object_t           |named-type |55                                                                       |
 |55     |                                   |struct     |struct {/lexical_scope1/object_t left;}                                  |
*/

#if 0
QUARK_TEST_VIP("Types", "update_named_type()", "", ""){
	types_t types;
	const auto name = unpack_type_name("/a/b");
	const auto a = make_named_type(types, name, make_undefined());
	const auto s = make_struct(types, struct_type_desc_t( { member_t(a, "f") } ));
	const auto b = update_named_type(types, a, s);

	if(true) trace_types(types);
	QUARK_ASSERT(is_wellformed(types, b));

	llvm::LLVMContext context;
	const auto lookup = llvm_type_lookup(context, types);

	trace_llvm_type_lookup(lookup);
}
#endif


static llvm_type_lookup make_basic_types(llvm::LLVMContext& context){
	types_t temp;
	return llvm_type_lookup(context, temp);
}


static llvm_type_lookup make_basic_types(llvm::LLVMContext& context);

#if 0
QUARK_TEST("LLVM Codegen", "map_function_arguments()", "func void()", ""){
	llvm::LLVMContext context;
	const auto types = make_basic_types(context);
	auto module = std::make_unique<llvm::Module>("test", context);

	const auto r = map_function_arguments(
		types,
		make_function(type_t::make_void(), {}, epure::pure)
	);

	QUARK_VERIFY(r.return_type != nullptr);
	QUARK_VERIFY(r.return_type->isVoidTy());

	QUARK_VERIFY(r.args.size() == 1);

	QUARK_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);
}

QUARK_TEST("LLVM Codegen", "map_function_arguments()", "func int()", ""){
	llvm::LLVMContext context;
	auto types = make_basic_types(context);
	auto module = std::make_unique<llvm::Module>("test", context);

	const auto r = map_function_arguments(types, make_function(type_t::make_int(), {}, epure::pure));

	QUARK_VERIFY(r.return_type != nullptr);
	QUARK_VERIFY(r.return_type->isIntegerTy(64));

	QUARK_VERIFY(r.args.size() == 1);

	QUARK_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);
}

QUARK_TEST("LLVM Codegen", "map_function_arguments()", "func void(int)", ""){
	llvm::LLVMContext context;
	const auto types = make_basic_types(context);
	auto module = std::make_unique<llvm::Module>("test", context);

	const auto r = map_function_arguments(
		types,
		make_function(type_t::make_void(), { type_t::make_int() }, epure::pure)
	);

	QUARK_VERIFY(r.return_type != nullptr);
	QUARK_VERIFY(r.return_type->isVoidTy());

	QUARK_VERIFY(r.args.size() == 2);

	QUARK_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);

	QUARK_VERIFY(r.args[1].llvm_type->isIntegerTy(64));
	QUARK_VERIFY(r.args[1].floyd_name == "0");
	QUARK_VERIFY(r.args[1].floyd_type.is_int());
	QUARK_VERIFY(r.args[1].floyd_arg_index == 0);
	QUARK_VERIFY(r.args[1].map_type == llvm_arg_mapping_t::map_type::k_known_value_type);
}

QUARK_TEST
("LLVM Codegen", "map_function_arguments()", "func void(int, DYN, bool)", ""){
	llvm::LLVMContext context;
	const auto types = make_basic_types(context);
	auto module = std::make_unique<llvm::Module>("test", context);

	const auto r = map_function_arguments(
		types,
		make_function(
			type_t::make_void(),
			{ type_t::make_int(), type_t::make_any(), type_t::make_bool() },
			epure::pure
		)
	);

	QUARK_VERIFY(r.return_type != nullptr);
	QUARK_VERIFY(r.return_type->isVoidTy());

	QUARK_VERIFY(r.args.size() == 5);

	QUARK_VERIFY(r.args[0].llvm_type->isPointerTy());
	QUARK_VERIFY(r.args[0].floyd_name == "floyd_runtime_ptr");
	QUARK_VERIFY(r.args[0].floyd_type.is_undefined());
	QUARK_VERIFY(r.args[0].floyd_arg_index == -1);
	QUARK_VERIFY(r.args[0].map_type == llvm_arg_mapping_t::map_type::k_floyd_runtime_ptr);

	QUARK_VERIFY(r.args[1].llvm_type->isIntegerTy(64));
	QUARK_VERIFY(r.args[1].floyd_name == "0");
	QUARK_VERIFY(r.args[1].floyd_type.is_int());
	QUARK_VERIFY(r.args[1].floyd_arg_index == 0);
	QUARK_VERIFY(r.args[1].map_type == llvm_arg_mapping_t::map_type::k_known_value_type);

	QUARK_VERIFY(r.args[2].llvm_type->isIntegerTy(64));
	QUARK_VERIFY(r.args[2].floyd_name == "1");
	QUARK_VERIFY(r.args[2].floyd_type.is_any());
	QUARK_VERIFY(r.args[2].floyd_arg_index == 1);
	QUARK_VERIFY(r.args[2].map_type == llvm_arg_mapping_t::map_type::k_dyn_value);

	QUARK_VERIFY(r.args[3].llvm_type->isIntegerTy(64));
	QUARK_VERIFY(r.args[3].floyd_name == "1");
	QUARK_VERIFY(r.args[3].floyd_type.is_undefined());
	QUARK_VERIFY(r.args[3].floyd_arg_index == 1);
	QUARK_VERIFY(r.args[3].map_type == llvm_arg_mapping_t::map_type::k_dyn_type);

	QUARK_VERIFY(r.args[4].llvm_type->isIntegerTy(1));
	QUARK_VERIFY(r.args[4].floyd_name == "2");
	QUARK_VERIFY(r.args[4].floyd_type.is_bool());
	QUARK_VERIFY(r.args[4].floyd_arg_index == 2);
	QUARK_VERIFY(r.args[4].map_type == llvm_arg_mapping_t::map_type::k_known_value_type);
}
#endif


}	// floyd
