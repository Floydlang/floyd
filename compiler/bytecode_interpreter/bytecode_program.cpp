//
//  bytecode_program.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2020-01-05.
//  Copyright © 2020 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_program.h"

#include "floyd_runtime.h"
#include "text_parser.h"
#include "ast_value.h"
#include "types.h"
#include "value_features.h"

#include "floyd_network_component.h"
#include "floyd_corelib.h"
#include "bytecode_intrinsics.h"

#include <algorithm>


namespace floyd {


//////////////////////////////////////////		bc_static_frame_t


bc_static_frame_t::bc_static_frame_t(
	const types_t& types,
	const std::vector<bc_instruction_t>& instrs2,
	const symbol_table_t& symbols,
	const std::vector<type_t>& args
) :
	_instructions(instrs2),
	_symbols(symbols),
	_arg_count((int)args.size()),
	_locals_count((int)_symbols._symbols.size() - (int)args.size())
{
	QUARK_ASSERT(types.check_invariant());

	for(int symbol_index = 0 ; symbol_index < symbols._symbols.size() ; symbol_index++){
		const auto& symbol_kv = symbols._symbols[symbol_index];
		const auto& symbol = symbol_kv.second;
		const auto type = symbol._value_type;

		if(symbol._symbol_type == symbol_t::symbol_type::immutable_reserve){
			_symbol_effective_type.push_back(type);
		}
		else if(symbol._symbol_type == symbol_t::symbol_type::immutable_arg){
			_symbol_effective_type.push_back(type);
		}
		else if(symbol._symbol_type == symbol_t::symbol_type::immutable_precalc){
			_symbol_effective_type.push_back(type);
			QUARK_ASSERT(type == symbol._init.get_type());
		}
		else if(symbol._symbol_type == symbol_t::symbol_type::named_type){
			_symbol_effective_type.push_back(type_t::make_typeid());
		}
		else if(symbol._symbol_type == symbol_t::symbol_type::mutable_reserve){
			_symbol_effective_type.push_back(type);
		}
		else {
			quark::throw_defective_request();
		}
	}

	QUARK_ASSERT(check_invariant());
}

bool bc_static_frame_t::check_invariant() const {
	QUARK_ASSERT(_instructions.size() > 0);
	QUARK_ASSERT(_instructions.size() < 65000);

	QUARK_ASSERT(_symbols.check_invariant());
	QUARK_ASSERT(_arg_count >= 0 && _arg_count <= _symbols._symbols.size());
	QUARK_ASSERT(_locals_count >= 0 && _locals_count <= _symbols._symbols.size());
	QUARK_ASSERT(_arg_count + _locals_count == _symbols._symbols.size());
	QUARK_ASSERT(_symbol_effective_type.size() == _symbols._symbols.size());
	return true;
}


//////////////////////////////////////////		bc_function_definition_t


bool bc_function_definition_t::check_invariant() const {
	QUARK_ASSERT(func_link.check_invariant());
	QUARK_ASSERT(_frame_ptr == nullptr || _frame_ptr->check_invariant());
	return true;
}








std::vector<std::pair<type_t, struct_layout_t>> bc_make_struct_layouts(const types_t& types){
	QUARK_ASSERT(types.check_invariant());

	std::vector<std::pair<type_t, struct_layout_t>> result;

	for(int i = 0 ; i < types.nodes.size() ; i++){
		const auto& type = lookup_type_from_index(types, i);
		const auto peek_type = peek2(types, type);
		if(peek_type.is_struct() && is_fully_defined(types, peek_type)){
			const auto& source_struct_def = peek_type.get_struct(types);

			const auto struct_bytes = source_struct_def._members.size() * 8;
			std::vector<member_info_t> member_infos;
			for(int member_index = 0 ; member_index < source_struct_def._members.size() ; member_index++){
				const auto& member = source_struct_def._members[member_index];
				const size_t offset = member_index * 8;
				member_infos.push_back(member_info_t { offset, member._type } );
			}

			result.push_back( { type, struct_layout_t{ member_infos, struct_bytes } } );
		}
	}
	return result;
}


	static std::map<std::string, void*> get_corelib_and_network_binds(){
		const std::map<std::string, void*> corelib_binds = get_unified_corelib_binds();
		const auto network_binds = get_network_component_binds();

		std::map<std::string, void*> merge = corelib_binds;
		merge.insert(network_binds.begin(), network_binds.end());

		return merge;
	}

std::vector<func_link_t> link_functions(const bc_program_t& program){
	QUARK_ASSERT(program.check_invariant());

	auto temp_types = program._types;
	const auto intrinsics2 = bc_get_intrinsics(temp_types);
	const auto corelib_native_funcs = get_corelib_and_network_binds();

	const auto funcs = mapf<func_link_t>(
		program._function_defs,
		[&corelib_native_funcs](const auto& e){
			const auto it = corelib_native_funcs.find(e.func_link.module_symbol.s);

			//	There is a native implementation of this function:
			if(it != corelib_native_funcs.end()){
				return set_f(e.func_link, func_link_t::eexecution_model::k_native__floydcc, (void*)it->second);
			}

			//	There is a BC implementation.
			else if(e._frame_ptr != nullptr){
				return set_f(e.func_link, func_link_t::eexecution_model::k_bytecode__floydcc, (void*)e._frame_ptr.get());
			}

			//	No implementation.
			else{
				return set_f(e.func_link, func_link_t::eexecution_model::k_bytecode__floydcc, nullptr);
			}
		}
	);

	//	Remove functions that don't have an implementation.
	const auto funcs2 = filterf<func_link_t>(funcs, [](const auto& e){ return e.f != nullptr; });

	const auto func_lookup = concat(funcs2, intrinsics2);
	return func_lookup;
}

static json_t frame_to_json(value_backend_t& backend, const bc_static_frame_t& frame){
	QUARK_ASSERT(backend.check_invariant());

	std::vector<json_t> instructions;
	int pc = 0;
	for(const auto& e: frame._instructions){
		const auto i = json_t::make_array({
			pc,
			opcode_to_string(e._opcode),
			json_t(e._a),
			json_t(e._b),
			json_t(e._c)
		});
		instructions.push_back(i);
		pc++;
	}

	return json_t::make_object({
		{ "symbols", json_t::make_array(bc_symbols_to_json(backend, frame._symbols)) },
		{ "instructions", json_t::make_array(instructions) }
	});
}

static json_t functiondef_to_json(value_backend_t& backend, const bc_function_definition_t& def){
	QUARK_ASSERT(backend.check_invariant());

	return json_t::make_array({
		func_link_to_json(backend.types, def.func_link),
		def._frame_ptr ? frame_to_json(backend, *def._frame_ptr) : json_t("no BC frame = native func")
	});
}


json_t bcprogram_to_json(const bc_program_t& program){
	auto backend = value_backend_t(
		{},
		bc_make_struct_layouts(program._types),
		program._types,
		config_t { vector_backend::hamt, dict_backend::hamt, false }
	);

	std::vector<json_t> callstack;
	std::vector<json_t> function_defs;
	for(const auto& e: program._function_defs){
		function_defs.push_back(json_t::make_array({
			functiondef_to_json(backend, e)
		}));
	}

	return json_t::make_object({
		{ "globals", frame_to_json(backend, program._globals) },
		{ "types", types_to_json(program._types) },
		{ "function_defs", json_t::make_array(function_defs) }
//		{ "callstack", json_t::make_array(callstack) }
	});
}





} // floyd
