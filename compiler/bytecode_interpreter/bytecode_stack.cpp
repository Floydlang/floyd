//
//  bytecode_stack.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2020-01-05.
//  Copyright © 2020 Marcus Zetterquist. All rights reserved.
//

#include "bytecode_stack.h"


namespace floyd {



json_t stack_to_json(const interpreter_stack_t& stack, value_backend_t& backend){
	QUARK_ASSERT(backend.check_invariant());

	const int size = static_cast<int>(stack._stack_size);
	std::vector<json_t> frames;
	std::vector<json_t> elements;

/*
	const auto stack_frames = stack.get_stack_frames_noci();
	for(int64_t i = 0 ; i < stack_frames.size() ; i++){
		auto a = json_t::make_array({
			json_t(i),
			json_t((int)stack_frames[i].start_pos),
			json_t((int)stack_frames[i].end_pos)
		});
		frames.push_back(a);
	}

	for(int64_t i = 0 ; i < size ; i++){
		const auto& frame_it = std::find_if(
			stack_frames.begin(),
			stack_frames.end(),
			[&i](const active_frame_t& e) { return e.start_pos == i; }
		);

		bool frame_start_flag = frame_it != stack_frames.end();

		if(frame_start_flag){
			elements.push_back(json_t("--- frame ---"));
		}

#if DEBUG
		const auto debug_type = stack._entry_types[i];
//		const auto ext = encode_as_external(_types, debug_type);
		const auto bc_pod = stack._entries[i];
		const auto bc = rt_value_t(backend, bc_pod, debug_type, rt_value_t::rc_mode::bump);

		auto a = json_t::make_array({
			json_t(i),
			type_to_json(backend.types, debug_type),
			bcvalue_to_json(backend, rt_value_t{ bc })
		});
		elements.push_back(a);
#endif
	}
*/


	return json_t::make_object({
		{ "size", json_t(size) },
		{ "frames", json_t::make_array(frames) },
		{ "elements", json_t::make_array(elements) }
	});
}
} // floyd
