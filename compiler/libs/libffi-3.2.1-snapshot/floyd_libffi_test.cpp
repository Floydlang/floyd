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
		ffi_call(&cif, FFI_FN(puts), &rc, values);

		/* rc now holds the result of the call to puts */
		/* values holds a pointer to the function’s arg, so to
		call puts() again all we need to do is change the
		value of s */
		s = (char*)"This is cool!";
		ffi_call(&cif, FFI_FN(puts), &rc, values);
	}
	else{
		quark::throw_defective_request();
	}
}




QUARK_TEST("libffi", "libffi closure", "", ""){
	const auto size = sizeof(ffi_closure);
	QUARK_ASSERT(size == 48);
}


/* Acts like puts with the file given at time of enclosure. */
static void puts_binding(ffi_cif *cif, void *ret, void* args[], void *stream){
	*(ffi_arg *)ret = fputs(*(char **)args[0], (FILE *)stream);
}

typedef int (*puts_t)(char *);

static int ffi_closure_test_main(){
	ffi_cif cif;
	ffi_type *args[1];
	ffi_closure *closure = nullptr;
	void *bound_puts;
	int rc;

	/* Allocate closure and bound_puts */
	closure = (ffi_closure*)ffi_closure_alloc(sizeof(ffi_closure), &bound_puts);

	if (closure){

		/* Initialize the argument info vectors */
		args[0] = &ffi_type_pointer;

		/* Initialize the cif */
		if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_sint, args) == FFI_OK){

			/* Initialize the closure, setting stream to stdout */
			if (ffi_prep_closure_loc(closure, &cif, puts_binding, stdout, bound_puts) == FFI_OK){
				rc = ((puts_t)bound_puts)((char*)"Hello World!");

				/* rc now holds the result of the call to fputs */
			}
		}
	}

	/* Deallocate both closure, and bound_puts */
	ffi_closure_free(closure);
	return 0;
}

QUARK_TEST("libffi", "libffi closure example program", "", ""){
	ffi_closure_test_main();
}

