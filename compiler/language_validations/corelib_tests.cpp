//
//  corelib_tests.cpp
//  floyd
//
//  Created by Marcus Zetterquist on 2020-01-12.
//  Copyright © 2020 Marcus Zetterquist. All rights reserved.
//

#include "floyd_corelib.h"
#include "test_helpers.h"


using namespace floyd;



//######################################################################################################################
//	CORE LIBRARY
//######################################################################################################################


QUARK_TEST("Floyd test suite", "Include library", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let a = 3

		)"
	);
}




QUARK_TEST("Floyd test suite", "detect_hardware_caps()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let caps = detect_hardware_caps()
			print(to_pretty_string(caps))

		)"
	);
}



QUARK_TEST("Floyd test suite", "make_hardware_caps_report()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

		)"
	);
}

QUARK_TEST("Floyd test suite", "make_hardware_caps_report_brief()", "", ""){
	ut_verify_printout_lib(
		QUARK_POS,
		R"(

			let caps = {
				"machdep_cpu_brand_string": json("Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz"),
				"logical_processor_count": json(8),
				"mem_size": json(17179869184)
			};

			let r = make_hardware_caps_report_brief(caps);
			print(r)

		)",
		{ "Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz  16 GB DRAM  8 cores" }
	);
}




QUARK_TEST("Floyd test suite", "make_hardware_caps_report()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let caps = detect_hardware_caps()
			let r = make_hardware_caps_report(caps);
			print(r)

		)"
	);
}

QUARK_TEST("Floyd test suite", "make_hardware_caps_report()", "", ""){
	ut_run_closed_lib(
		QUARK_POS,
		R"(

			let s = get_current_date_and_time_string()
			print(s)

		)"
	);
}





QUARK_TEST("Floyd test suite", "cmath_pi", "", ""){
	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let x = cmath_pi
			let result = x >= 3.14 && x < 3.15

		)",
		make_expected_bool(true)
	);
}



QUARK_TEST("Floyd test suite", "", "", ""){
	types_t temp;
	const auto a = type_t::make_vector(temp, type_t::make_string());
	const auto b = type_t::make_vector(temp, make__fsentry_t__type(temp));
	ut_verify_auto(QUARK_POS, a != b, true);
}





//////////////////////////////////////////		CORE LIBRARY - calc_string_sha1()


QUARK_TEST("Floyd test suite", "calc_string_sha1()", "", ""){
	ut_run_closed_lib(QUARK_POS, R"(

		let a = calc_string_sha1("Violator is the seventh studio album by English electronic music band Depeche Mode.")
//		print(to_string(a))
		assert(a.ascii40 == "4d5a137b3b1faf855872a312a184dd9a24594387")

	)");
}


//////////////////////////////////////////		CORE LIBRARY - calc_binary_sha1()


QUARK_TEST("Floyd test suite", "calc_binary_sha1()", "", ""){
	ut_run_closed_lib(QUARK_POS, R"(

		let bin = binary_t("Violator is the seventh studio album by English electronic music band Depeche Mode.")
		let a = calc_binary_sha1(bin)
//		print(to_string(a))
		assert(a.ascii40 == "4d5a137b3b1faf855872a312a184dd9a24594387")

	)");
}



//////////////////////////////////////////		CORE LIBRARY - get_time_ns()

QUARK_TEST("Floyd test suite", "get_time_ns()", "", ""){
	ut_run_closed_lib(QUARK_POS, R"(

		let start = get_time_ns()
		mutable b = 0
		mutable t = [0]
		for(i in 0...100){
			b = b + 1
			t = push_back(t, b)
		}
		let end = get_time_ns()
//		print("Duration: " + to_string(end - start) + ", number = " + to_string(b))
//		print(t)

	)");
}

QUARK_TEST("Floyd test suite", "get_time_ns()", "", ""){
	ut_run_closed_lib(QUARK_POS, R"(

		let int a = get_time_ns()
		let int b = get_time_ns()
		let int c = b - a
//		print("Delta time:" + to_string(a))

	)");
}




//////////////////////////////////////////		CORE LIBRARY - read_text_file()

/*
QUARK_TEST("Floyd test suite", "read_text_file()", "", ""){
	ut_run_closed_nolib(QUARK_POS, R"(

		path = get_env_path();
		a = read_text_file(path + "/Desktop/test1.json");
		print(a);

	)");
	ut_verify(QUARK_POS, vm->_print_output, { string() + R"({ "magic": 1234 })" + "\n" });
}
*/


//////////////////////////////////////////		CORE LIBRARY - write_text_file()


QUARK_TEST("Floyd test suite", "write_text_file()", "", ""){
	ut_run_closed_lib(QUARK_POS, R"(

		let path = get_fs_environment().desktop_dir
		write_text_file(path + "/test_out.txt", "Floyd wrote this!")

	)");
}



//////////////////////////////////////////		CORE LIBRARY - get_directory_entries()

#if 0
//??? needs clean prep/take down code.
QUARK_TEST("Floyd test suite", "get_fsentries_shallow()", "", ""){
	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let result0 = get_fsentries_shallow("/Users/marcus/Desktop/")
			assert(size(result0) > 3)
//			print(to_pretty_string(result0))

			let result = typeof(result0)

		)",
		value_t::make_typeid_value(
			make_vector(make__fsentry_t__type())
		)
	);
}
#endif


//////////////////////////////////////////		CORE LIBRARY - get_fsentries_deep()

#if 0
//??? needs clean prep/take down code.
QUARK_TEST("Floyd test suite", "get_fsentries_deep()", "", ""){
	ut_run_closed_lib(QUARK_POS,
		R"(

			let r = get_fsentries_deep("/Users/marcus/Desktop/")
			assert(size(r) > 3)
//			print(to_pretty_string(result))

		)"
	);
}
#endif

//////////////////////////////////////////		CORE LIBRARY - get_fsentry_info()

#if 0
//??? needs clean prep/take down code.
QUARK_TEST("Floyd test suite", "get_fsentry_info()", "", ""){
	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let x = get_fsentry_info("/Users/marcus/Desktop/")
//			print(to_pretty_string(x))
			let result = typeof(x)

		)",
		value_t::make_typeid_value(
			make__fsentry_info_t__type()
		)
	);
}
#endif

//////////////////////////////////////////		CORE LIBRARY - get_fs_environment()

#if 0
QUARK_TEST("Floyd test suite", "get_fs_environment()", "", ""){
	types_t temp;

	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let x = get_fs_environment()
//			print(to_pretty_string(x))
			let result = typeof(x)

		)",
		make_expected_typeid(temp, make__fs_environment_t__type(temp))
	);
}
#endif



//////////////////////////////////////////		CORE LIBRARY - does_fsentry_exist()


QUARK_TEST("Floyd test suite", "does_fsentry_exist()", "", ""){
	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let path = get_fs_environment().desktop_dir
			let x = does_fsentry_exist(path)
//			print(to_pretty_string(x))

			assert(x == true)
			let result = typeof(x)

		)",
		make_expected_typeid(types_t(), type_t::make_bool())
	);
}

QUARK_TEST("Floyd test suite", "does_fsentry_exist()", "", ""){
	ut_verify_global_result_lib(
		QUARK_POS,
		R"(

			let path = get_fs_environment().desktop_dir + "xyz"
			let result = does_fsentry_exist(path)
//			print(to_pretty_string(result))

			assert(result == false)

		)",
		make_expected_bool(false)
	);
}

static void remove_test_dir(const std::string& dir_name1, const std::string& dir_name2){
	const auto path1 = GetDirectories().desktop_dir + "/" + dir_name1;
	const auto path2 = path1 + "/" + dir_name2;

	if(DoesEntryExist(path1) == true){
		DeleteDeep(path1);
		QUARK_ASSERT(DoesEntryExist(path1) == false);
		QUARK_ASSERT(DoesEntryExist(path2) == false);
	}
}



//////////////////////////////////////////		CORE LIBRARY - create_directory_branch()


QUARK_TEST("Floyd test suite", "create_directory_branch()", "", ""){
	remove_test_dir("unittest___create_directory_branch", "subdir");

	ut_run_closed_lib(QUARK_POS,
		R"(

			let path1 = get_fs_environment().desktop_dir + "/unittest___create_directory_branch"
			let path2 = path1 + "/subdir"

			assert(does_fsentry_exist(path1) == false)
			assert(does_fsentry_exist(path2) == false)

			//	Make test.
			create_directory_branch(path2)
			assert(does_fsentry_exist(path1) == true)
			assert(does_fsentry_exist(path2) == true)

			delete_fsentry_deep(path1)
			assert(does_fsentry_exist(path1) == false)
			assert(does_fsentry_exist(path2) == false)

		)"
	);
}


//////////////////////////////////////////		CORE LIBRARY - delete_fsentry_deep()


QUARK_TEST("Floyd test suite", "delete_fsentry_deep()", "", ""){
	remove_test_dir("unittest___delete_fsentry_deep", "subdir");

	ut_run_closed_lib(QUARK_POS,
		R"(

			let path1 = get_fs_environment().desktop_dir + "/unittest___delete_fsentry_deep"
			let path2 = path1 + "/subdir"

			assert(does_fsentry_exist(path1) == false)
			assert(does_fsentry_exist(path2) == false)

			create_directory_branch(path2)
			assert(does_fsentry_exist(path1) == true)
			assert(does_fsentry_exist(path2) == true)

			//	Make test.
			delete_fsentry_deep(path1)
			assert(does_fsentry_exist(path1) == false)
			assert(does_fsentry_exist(path2) == false)

		)"
	);
}



//////////////////////////////////////////		CORE LIBRARYCORE LIBRARY - rename_fsentry()


QUARK_TEST("Floyd test suite", "rename_fsentry()", "", ""){
	ut_run_closed_lib(QUARK_POS,
		R"(

			let dir = get_fs_environment().desktop_dir + "/unittest___rename_fsentry"

			delete_fsentry_deep(dir)

			let file_path1 = dir + "/original.txt"
			let file_path2 = dir + "/renamed.txt"

			assert(does_fsentry_exist(file_path1) == false)
			assert(does_fsentry_exist(file_path2) == false)

			//	Will also create parent directory.
			write_text_file(file_path1, "This is the file contents\n")

			assert(does_fsentry_exist(file_path1) == true)
			assert(does_fsentry_exist(file_path2) == false)

			rename_fsentry(file_path1, "renamed.txt")

			assert(does_fsentry_exist(file_path1) == false)
			assert(does_fsentry_exist(file_path2) == true)

			delete_fsentry_deep(dir)

		)"
	);
}



