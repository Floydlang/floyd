//
//  floyd_libffi_test.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-27.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//


#include <stdio.h>
#include <ffi.h>

#include "quark.h"


typedef void(*VOID_VOID_F)(void);

QUARK_TEST("libffi", "libffi example program", "", "check output in console"){
	ffi_cif cif;
	ffi_type* args[1];
	void* values[1];
	char* s = nullptr;
	ffi_arg rc;

	/* Initialize the argument info vectors */
	args[0] = &ffi_type_pointer;
	values[0] = &s;

	/* Initialize the cif */
	if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_sint, args) == FFI_OK) {
		s = (char*)"Hello World!";
		ffi_call(&cif, (VOID_VOID_F)puts, &rc, values);

		/* rc now holds the result of the call to puts */
		/* values holds a pointer to the function’s arg, so to
		call puts() again all we need to do is change the
		value of s */
		s = (char*)"This is cool!";
		ffi_call(&cif, (VOID_VOID_F)puts, &rc, values);
	}
}
