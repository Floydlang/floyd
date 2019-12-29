//
//  floyd_intrinsics.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-29.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_intrinsics.h"

#include "floyd_llvm_helpers.h"
#include "floyd_llvm_runtime.h"
#include "floyd_llvm_codegen_basics.h"
#include "value_features.h"
#include "floyd_runtime.h"
#include "text_parser.h"
#include "utils.h"


namespace floyd {


/////////////////////////////////////////		print()

void unified_intrinsic__print(floyd_runtime_t* frp, runtime_value_t value, runtime_type_t value_type){
	auto& backend = get_backend(frp);

	const auto s = gen_to_string(backend, value, type_t(value_type));
	on_print(frp, s);
}


} // floyd
