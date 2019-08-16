//
//  floyd_corelib.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 2019-06-16.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#include "floyd_corelib.h"


#include "typeid.h"
#include "ast_value.h"
#include "floyd_runtime.h"
#include "sha1_class.h"
#include "file_handling.h"
#include "hardware_caps.h"
#include "format_table.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

namespace floyd {





/*
std::string GetTemporaryFile(const std::string& name){
	//	char *tmpnam(char *str)
	//	tempnam()

	char* temp_buffer = std::tempnam(const char *dir, const char *pfx);
	std::free(temp_buffer);
	temp_buffer = nullptr;


}


## make\_temporary\_path()

Returns a path where you can write a file or a directory and it goes into the file system. The name will be randomized. The operating system will delete this data at some point.

	string make_temporary_path() impure

Example:

	"/var/folders/kw/7178zwsx7p3_g10y7rp2mt5w0000gn/T/g10y/7178zwsxp3_g10y7rp2mt

There is never a file extension. You could add one if you want too.
*/






extern const std::string k_corelib_builtin_types_and_constants = R"(

	struct benchmark_id_t {
		string module
		string test
	}

	struct benchmark_result2_t {
		benchmark_id_t test_id
		benchmark_result_t result
	}




	func benchmark_id_t get_benchmarks_f(benchmark_def_t def, int c){
		return benchmark_id_t( "module x", def.name)
	}

	func [benchmark_id_t] get_benchmarks(){
		return map(benchmark_registry, get_benchmarks_f, 0)
	}




	//[E] filter([E] elements, func bool (E e, C context) f, C context)
	func bool run_benchmarks_f(benchmark_def_t def, string wanted_name){
		return def.name == wanted_name
	}

	func [benchmark_result2_t] run_benchmarks([benchmark_id_t] m){
		mutable [benchmark_result2_t] out = []

		for(i in 0 ..< size(m)){
			let id = m[i]

			let m = filter(benchmark_registry, run_benchmarks_f, id.test)
			assert(size(m) == 0 || size(m) == 1)

			if(size(m) == 1){
				let e = m[0]
				let benchmark_result = e.f()
				for(v in 0 ..< size(benchmark_result)){
					out = push_back(out, benchmark_result2_t(id, benchmark_result[v]))
				}
			}
		}
		return out
	}



	//[R] map([E] elements, func R (E e, C context) f, C context)
	func string trace_benchmarks_f(benchmark_result2_t r, int c){
		return "(" + r.test_id.module + ":" + r.test_id.test + "): dur: " + to_string(r.result.dur) + ", more: " + to_pretty_string(r.result.more)
	}

	func string trace_benchmarks([benchmark_result2_t] r){
		let s = map(r, trace_benchmarks_f, 0)
		mutable acc = ""
		for(i in 0 ..< size(s)){
			acc = acc + s[i] + "\n"
		}
		return acc
	}



	func int max(int a, int b){
		return a > b ? a : b
	}


	func string repeat(int count, int pad_char){
		mutable a = ""
		for(i in 0 ..< count){
			a = push_back(a, pad_char)
		}
		return a
	}

	struct line_t {
		[string] columns
	}

	func string gen_line(line_t line, [int] widths, [int] align, int pad_char){
		mutable acc = "|"
		for(c in 0 ..< size(line.columns)){
			let s = line.columns[c]
			let wanted_width = widths[c] + 1

			let pad_count = wanted_width - size(s)
			let pad_string = repeat(pad_count, pad_char)

			let s2 = align[c] == 0 ? s : (align[c] < 0 ? s + pad_string : pad_string + s)

			acc = acc + s2 + "|"
		}
		return acc
	}


	//[R] map([E] elements, func R (E e, C context) f, C context)
	func line_t table_f(benchmark_result2_t r, int c){
		let columns = [
			r.test_id.module,
			r.test_id.test,
			to_string(r.result.dur) + " ns",
			to_pretty_string(r.result.more)
		]
		return line_t(columns)
	}

	//	R reduce([E] elements, R accumulator_init, func R (R accumulator, E element, C context) f, C context)
	func [int] calc_width_f([int] acc, line_t line, int c){
		return [
			max(acc[0], size(line.columns[0])),
			max(acc[1], size(line.columns[1])),
			max(acc[2], size(line.columns[2])),
			max(acc[3], size(line.columns[3]))
		]
	}

	func [string] make_benchmark_report([benchmark_result2_t] test_results, [int] align){
		let headings = line_t([ "MODULE", "TEST", "DUR", ""])
		let table = map(test_results, table_f, 0)
		let widths = reduce([headings] + table, [0, 0, 0, 0], calc_width_f, 0)

		mutable [string] report = [
			gen_line(headings, widths, [ -1, -1, -1, -1 ], ' '),
			gen_line(line_t([ "", "", "", ""]), widths, [ -1, -1, -1, -1 ], '-')
		]

		for(i in 0 ..< size(table)){
			let line = table[i]
			let line2 = gen_line(line, widths, align, ' ')
			report = push_back(report, line2)
		}
		return report
	}

/*
	if(false){
		mutable module_width = 0
		mutable test_width = 0
		mutable dur_width = 0
		mutable more1_width = 0


		for(i in 0 ..< size(test_results)){
			let r = test_results[i]
			module_width = max(module_width, size(r.test_id.module))
			test_width = max(test_width, size(r.test_id.test))

			dur_width = max(dur_width, size(to_string(r.result.dur)))

			let type = get_json_type(r.result.more)
			if(type == json_object){
			}
			else if(type == json_array){
			}
			else if(type == json_string){
			}
			else if(type == json_number){
			}
			else if(type == json_true){
			}
			else if(type == json_false){
			}
			else if(type == json_null){
			}
			else{
				assert(false)
			}

			let more_string = to_string(r.result.more)
			
			let mode1_width = max(dur_width, size(to_string(r.result.dur)))



			let s = "(" + r.test_id.module + ":" + r.test_id.test + "): dur: " + to_string(r.result.dur) + ", more: " + to_pretty_string(r.result.more)
			print(s)
		}

		for(i in 0 ..< size(test_results)){
			let r = test_results[i]
			let s = "(" + r.test_id.module + ":" + r.test_id.test + "): dur: " + to_string(r.result.dur) + ", more: " + to_pretty_string(r.result.more)
			print(s)
		}
	}
*/








	func [string: json] detect_hardware_caps()






	let double cmath_pi = 3.14159265358979323846


	struct time_ms_t {
		int pos
	}

	struct date_t {
		string utc_date
	}





	struct uuid_t {
		int high64
		int low64
	}


	struct ip_address_t {
		int high64
		int low_64_bits
	}


	struct url_t {
		string absolute_url
	}

	struct url_parts_t {
		string protocol
		string domain
		string path
		[string:string] query_strings
		int port
	}




	struct quick_hash_t {
		int hash
	}

	struct sha1_t {
		string ascii40
	}

	struct key_t {
		quick_hash_t hash
	}



	struct binary_t {
		string bytes
	}

	struct seq_t {
		string str
		int pos
	}

	struct text_t {
		binary_t data
	}

	struct text_resource_id {
		quick_hash_t id
	}

	struct image_id_t {
		int id
	}

	struct color_t {
		double red
		double green
		double blue
		double alpha
	}

	struct vector2_t {
		double x
		double y
	}



	let color__black = color_t(0.0, 0.0, 0.0, 1.0)
	let color__white = color_t(1.0, 1.0, 1.0, 1.0)


	func color_t add_colors(color_t a, color_t b){
		return color_t(
			a.red + b.red,
			a.green + b.green,
			a.blue + b.blue,
			a.alpha + b.alpha
		)
	}


	////////////////////////////		FILE SYSTEM TYPES


	struct fsentry_t {
		string type	//	"dir" or "file"
		string abs_parent_path
		string name
	}

	struct fsentry_info_t {
		string type	//	"file" or "dir"
		string name
		string abs_parent_path

		string creation_date
		string modification_date
		int file_size
	}

	struct fs_environment_t {
		string home_dir
		string documents_dir
		string desktop_dir

		string hidden_persistence_dir
		string preferences_dir
		string cache_dir
		string temp_dir

		string executable_dir
	}


	func sha1_t calc_string_sha1(string s)
	func sha1_t calc_binary_sha1(binary_t d)

	func int get_time_of_day() impure

	func string read_text_file(string abs_path) impure
	func void write_text_file(string abs_path, string data) impure

	func [fsentry_t] get_fsentries_shallow(string abs_path) impure
	func [fsentry_t] get_fsentries_deep(string abs_path) impure
	func fsentry_info_t get_fsentry_info(string abs_path) impure
	func fs_environment_t get_fs_environment() impure
	func bool does_fsentry_exist(string abs_path) impure
	func void create_directory_branch(string abs_path) impure
	func void delete_fsentry_deep(string abs_path) impure
	func void rename_fsentry(string abs_path, string n) impure

)";



static json_t json_from_uint64(uint64_t v){
	const int64_t a = v;
	return json_t(a);
}



std::vector<std::pair<std::string, json_t>> corelib_detect_hardware_caps(){
	const auto caps = read_hardware_caps();
	const auto result = std::vector<std::pair<std::string, json_t>> {
		{ "machdep_cpu_brand_string", json_t(caps._machdep_cpu_brand_string) },

		{ "machine", json_t(caps._hw_machine) },
		{ "model", json_t(caps._hw_model) },
		{ "ncpu", json_from_uint64(caps._hw_ncpu) },

		{ "byteorder", json_from_uint64(caps._hw_byteorder) },
		{ "physmem", json_from_uint64(caps._hw_physmem) },
		{ "usermem", json_from_uint64(caps._hw_usermem) },


		{ "epoch", json_from_uint64(caps._hw_epoch) },
		{ "floatingpoint", json_from_uint64(caps._hw_floatingpoint) },
		{ "machinearch", json_t(caps._hw_machinearch) },

		{ "vectorunit", json_from_uint64(caps._hw_vectorunit) },
		{ "tbfrequency", json_from_uint64(caps._hw_tbfrequency) },
		{ "availcpu", json_from_uint64(caps._hw_availcpu) },


		{ "cpu_type", json_from_uint64(caps._hw_cpu_type) },
		{ "cpu_type_subtype", json_from_uint64(caps._hw_cpu_type_subtype) },

		{ "packaged", json_from_uint64(caps._hw_packaged) },

		{ "physical_processor_count", json_from_uint64(caps._hw_physical_processor_count) },


		{ "logical_processor_count", json_from_uint64(caps._hw_logical_processor_count) },


		{ "cpu_freq_hz", json_from_uint64(caps._hw_cpu_freq_hz) },
		{ "bus_freq_hz", json_from_uint64(caps._hw_bus_freq_hz) },

		{ "mem_size", json_from_uint64(caps._hw_mem_size) },
		{ "page_size", json_from_uint64(caps._hw_page_size) },
		{ "cacheline_size", json_from_uint64(caps._hw_cacheline_size) },


		{ "scalar_align", json_from_uint64(caps._hw_scalar_align) },

		{ "l1_data_cache_size", json_from_uint64(caps._hw_l1_data_cache_size) },
		{ "l1_instruction_cache_size", json_from_uint64(caps._hw_l1_instruction_cache_size) },
		{ "l2_cache_size", json_from_uint64(caps._hw_l2_cache_size) },
		{ "l3_cache_size", json_from_uint64(caps._hw_l3_cache_size) }
	};
	return result;
}


/*
{
	"availcpu": 0,
	"bus_freq_hz": 1e+08,
	"byteorder": 1234,
	"cacheline_size": 64,
	"cpu_freq_hz": 4e+09,
	"cpu_type": 7,
	"cpu_type_subtype": 8,
	"epoch": 0,
	"floatingpoint": 0,
	"l1_data_cache_size": 32768,
	"l1_instruction_cache_size": 32768,
	"l2_cache_size": 262144,
	"l3_cache_size": 8.38861e+06,
	"logical_processor_count": 8,
	"machdep_cpu_brand_string": "Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz",
	"machine": "x86_64",
	"machinearch": "",
	"mem_size": 1.71799e+10,
	"model": "iMac15,1",
	"ncpu": 8,
	"packaged": 1,
	"page_size": 4096,
	"physical_processor_count": 4,
	"physmem": 2.14748e+09,
	"scalar_align": 16,
	"tbfrequency": 1e+09,
	"usermem": 3.95551e+09,
	"vectorunit": 1
}





*/

std::string simplify_mem_size(int64_t value){
	const int64_t k = 1024;

	if((value % (k * k * k)) == 0){
		return std::to_string(value / (k * k * k)) + " GB";
	}
	else if((value % (k * k)) == 0){
		return std::to_string(value / (k * k)) + " MB";
	}
	else if((value % (k)) == 0){
		return std::to_string(value / (k)) + " kB";
	}
	else{
		return std::to_string(value) + " B";
	}
}

QUARK_UNIT_TEST("", "corelib_make_hardware_caps_report()", "", ""){
	const auto r = simplify_mem_size(3943120896);
//	QUARK_UT_VERIFY(r == "4 GB");
}




std::string simplify_freq(int64_t freq){
	if((freq % 1000000000) == 0){
		return std::to_string(freq / 1000000000) + " GHz";
	}
	else if((freq % 1000000) == 0){
		return std::to_string(freq / 1000000) + " MHz";
	}
	else if((freq % 1000) == 0){
		return std::to_string(freq / 1000) + " kHz";
	}
	else{
		return std::to_string(freq) + " Hz";
	}
}



std::vector<std::string> corelib_make_hardware_caps_report(const std::vector<std::pair<std::string, json_t>>& caps){
	const auto m = std::map<std::string, json_t>(caps.begin(), caps.end());
//	const auto machdep_cpu_brand = m.at("machdep_cpu_brand_string").get_string();
	const auto machine = m.at("machine").get_string();
	const auto model = m.at("model").get_string();
	const auto cpu_freq_hz = m.at("cpu_freq_hz").get_number();
	const auto cpu_type = m.at("cpu_type").get_number();
	const auto cpu_type_subtype = m.at("cpu_type_subtype").get_number();
	const auto floatingpoint = m.at("floatingpoint").get_number();
//	const auto logical_processor_count = m.at("logical_processor_count").get_number();
	const auto machinearch = m.at("machinearch").get_string();
	const auto vectorunit = m.at("vectorunit").get_number();

	const auto ncpu = m.at("ncpu").get_number();
	const auto physical_processor_count = m.at("physical_processor_count").get_number();


//	const auto mem_size = m.at("mem_size").get_number();
	const auto bus_freq_hz = m.at("bus_freq_hz").get_number();
	const auto usermem = m.at("usermem").get_number();
	const auto physmem = m.at("physmem").get_number();
	const auto cacheline_size = m.at("cacheline_size").get_number();
	const auto scalar_align = m.at("scalar_align").get_number();
	const auto byteorder = m.at("byteorder").get_number();
	const auto page_size = m.at("page_size").get_number();

	const auto l1_data_cache_size = m.at("l1_data_cache_size").get_number();
	const auto l1_instruction_cache_size = m.at("l1_instruction_cache_size").get_number();
	const auto l2_cache_size = m.at("l2_cache_size").get_number();
	const auto l3_cache_size = m.at("l3_cache_size").get_number();



	std::vector<line_t> table = {
		line_t( { "PROCESSOR", "",												"",		"", "" }, ' ', 0x00),
		line_t( { "", "",														"",		"", "" }, '-', 0x00),
		line_t( { "Model", model,												"",		"ncpu", std::to_string((int)ncpu) }, ' ', 0x00),
		line_t( { "Machine", machine,											"",		"Phys Processor Count", std::to_string((int64_t)physical_processor_count) }, ' ', 0x00),
		line_t( { "CPU Freq", simplify_freq((int64_t)cpu_freq_hz),				"",		"","" }, ' ', 0x00),
		line_t( { "CPU Type", std::to_string((int)cpu_type),					"",		"Vector unit", vectorunit == 1 ? "yes" : "no" }, ' ', 0x00),
		line_t( { "CPU Sub type", std::to_string((int)cpu_type_subtype),		"",		"Floatingpoint", floatingpoint == 1 ? "yes" : "no" }, ' ', 0x00),
		line_t( { "", "",														"",		"","" }, ' ', 0x00),
		line_t( { "MEMORY", "",			"",		"", "" }, ' ', 0x00),
		line_t( { "", "",														"",		"", "" }, '-', 0x00),
		line_t( { "Bus freq", simplify_freq((int64_t)bus_freq_hz),				"",		"Page size", simplify_mem_size((int64_t)page_size) }, ' ', 0x00),
		line_t( { "User mem", simplify_mem_size((int64_t)usermem),				"",		"Scalar align", simplify_mem_size((int64_t)scalar_align) }, ' ', 0x00),
		line_t( { "Physical mem", simplify_mem_size((int64_t)physmem),			"",		"Byte order", std::to_string((int)byteorder) }, ' ', 0x00),
		line_t( { "Cacheline size", simplify_mem_size((int64_t)cacheline_size),		"", "", "" }, ' ', 0x00),
		line_t( { "L1 data", simplify_mem_size((int64_t)l1_data_cache_size),	"",		"L1 instructions", simplify_mem_size((int64_t)l1_instruction_cache_size) }, ' ', 0x00),
		line_t( { "L2", simplify_mem_size((int64_t)l2_cache_size),				"",		"L3", simplify_mem_size((int64_t)l3_cache_size) }, ' ', 0x00)
	};

	const auto default_column = column_t{ 0, -1, 0 };
	const auto columns0 = std::vector<column_t>{ default_column, default_column, column_t{ 10, -1, 0 }, default_column, default_column };
	const auto columns = fit_columns(columns0, table);
	const auto table2 = generate_table(table, columns);
	const auto r = concat(std::vector<std::string>{ corelib_make_hardware_caps_report_brief(caps), "" }, table2);
	return r;
}



QUARK_UNIT_TEST("", "corelib_make_hardware_caps_report()", "", ""){
	const auto caps = corelib_detect_hardware_caps();
	const auto r = corelib_make_hardware_caps_report(caps);
	for(const auto& e: r){
		std::cout << e << std::endl;
	}
//	QUARK_UT_VERIFY(r == "Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz  16 GB DRAM  8 cores");
}



//	"Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz"	mem_size: 1.71799e+10,	logical_processor_count: 8,
//	Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz  16 GB DRAM  8 cores
std::string corelib_make_hardware_caps_report_brief(const std::vector<std::pair<std::string, json_t>>& caps){
	const auto m = std::map<std::string, json_t>(caps.begin(), caps.end());

	std::stringstream r;
	const auto machdep_cpu_brand_string = m.at("machdep_cpu_brand_string").get_string();
	const auto mem_size = m.at("mem_size").get_number();
	const auto logical_processor_count = m.at("logical_processor_count").get_number();

	const auto mem_str = simplify_mem_size(static_cast<int64_t>(mem_size));

	r << machdep_cpu_brand_string << "  " << mem_str << " DRAM  " << logical_processor_count << " cores";
	return r.str();
}

QUARK_UNIT_TEST("", "corelib_make_hardware_caps_report_brief()", "", ""){
	const std::vector<std::pair<std::string, json_t>> caps = {
		{ "machdep_cpu_brand_string", json_t("Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz") },
		{ "logical_processor_count", json_t(8) },
		{ "mem_size", json_t((int64_t)17179869184) }
	};

//	const auto caps = corelib_detect_hardware_caps();
	const auto r = corelib_make_hardware_caps_report_brief(caps);
	QUARK_UT_VERIFY(r == "Intel(R) Core(TM) i7-4790K CPU @ 4.00GHz  16 GB DRAM  8 cores");
}


//??? check path is valid dir
bool is_valid_absolute_dir_path(const std::string& s){
	if(s.empty()){
		return false;
	}
	else{
/*
		if(s.back() != '/'){
			return false;
		}
*/

	}
	return true;
}



std::vector<value_t> directory_entries_to_values(const std::vector<TDirEntry>& v){
	const auto k_fsentry_t__type = make__fsentry_t__type();
	const auto elements = mapf<value_t>(
		v,
		[&k_fsentry_t__type](const auto& e){
//			const auto t = value_t::make_string(e.fName);
			const auto type_string = e.fType == TDirEntry::kFile ? "file": "dir";
			const auto t2 = value_t::make_struct_value(
				k_fsentry_t__type,
				{
					value_t::make_string(type_string),
					value_t::make_string(e.fNameOnly),
					value_t::make_string(e.fParent)
				}
			);
			return t2;
		}
	);
	return elements;
}






typeid_t make__fsentry_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "type" },
		{ typeid_t::make_string(), "abs_parent_path" },
		{ typeid_t::make_string(), "name" }
	});
	return temp;
}

/*
	struct fsentry_info_t {
		string type	//	"file" or "dir"
		string name
		absolute_path_t parent_path

		date_t creation_date
		date_t modification_date
		file_pos_t file_size
	}
*/
typeid_t make__fsentry_info_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "type" },
		{ typeid_t::make_string(), "name" },
		{ typeid_t::make_string(), "abs_parent_path" },

		{ typeid_t::make_string(), "creation_date" },
		{ typeid_t::make_string(), "modification_date" },
		{ typeid_t::make_int(), "file_size" }
	});
	return temp;
}




typeid_t make__fs_environment_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "home_dir" },
		{ typeid_t::make_string(), "documents_dir" },
		{ typeid_t::make_string(), "desktop_dir" },

		{ typeid_t::make_string(), "hidden_persistence_dir" },
		{ typeid_t::make_string(), "preferences_dir" },
		{ typeid_t::make_string(), "cache_dir" },
		{ typeid_t::make_string(), "temp_dir" },

		{ typeid_t::make_string(), "executable_dir" }
	});
	return temp;
}






/*
	struct date_t {
		string utd_date
	}
*/
typeid_t make__date_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "utd_date" }
	});
	return temp;
}

typeid_t make__sha1_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "ascii40" }
	});
	return temp;
}

typeid_t make__binary_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "bytes" }
	});
	return temp;
}

/*
	struct absolute_path_t {
		string absolute_path
	}
*/
typeid_t make__absolute_path_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_string(), "absolute_path" }
	});
	return temp;
}

/*
	struct file_pos_t {
		int pos
	}
*/
typeid_t make__file_pos_t__type(){
	const auto temp = typeid_t::make_struct2({
		{ typeid_t::make_int(), "pos" }
	});
	return temp;
}









std::string corelib_calc_string_sha1(const std::string& s){
	const auto sha1 = CalcSHA1(s);
	const auto ascii40 = SHA1ToStringPlain(sha1);
	return ascii40;
}



std::string corelib_read_text_file(const std::string& abs_path){
	return read_text_file(abs_path);
}

void corelib_write_text_file(const std::string& abs_path, const std::string& file_contents){
	const auto up = UpDir2(abs_path);

	MakeDirectoriesDeep(up.first);

	std::ofstream outputFile;
	outputFile.open(abs_path);
	if (outputFile.fail()) {
		quark::throw_exception();
	}

	outputFile << file_contents;
	outputFile.close();
}



/*
	const auto a = std::chrono::high_resolution_clock::now();
	std::this_thread::sleep_for(std::chrono::milliseconds(7));
	const auto b = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> elapsed_seconds = b - a;
	const int ms = static_cast<int>((static_cast<double>(elapsed_seconds.count()) * 1000.0));
*/
int64_t corelib__get_time_of_day(){
	const std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
//	std::chrono::duration<double> elapsed_seconds = t - 0;
//	const auto ms = t * 1000.0;
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	const auto result = int64_t(ms);
	return result;
}

QUARK_UNIT_TEST("sizeof(int)", "", "", ""){
	QUARK_TRACE(std::to_string(sizeof(int)));
	QUARK_TRACE(std::to_string(sizeof(int64_t)));
}

QUARK_UNIT_TEST("get_time_of_day_ms()", "", "", ""){
	const auto a = std::chrono::high_resolution_clock::now();
	std::this_thread::sleep_for(std::chrono::milliseconds(7));
	const auto b = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> elapsed_seconds = b - a;
	const int ms = static_cast<int>((static_cast<double>(elapsed_seconds.count()) * 1000.0));

	QUARK_UT_VERIFY(ms >= 7)
}




std::vector<TDirEntry> corelib_get_fsentries_shallow(const std::string& abs_path){
	if(is_valid_absolute_dir_path(abs_path) == false){
		quark::throw_runtime_error("get_fsentries_shallow() illegal input path.");
	}

	const auto a = GetDirItems(abs_path);
	return a;
}



std::vector<TDirEntry> corelib_get_fsentries_deep(const std::string& abs_path){
	if(is_valid_absolute_dir_path(abs_path) == false){
		quark::throw_runtime_error("get_fsentries_deep() illegal input path.");
	}

	const auto a = GetDirItemsDeep(abs_path);
	return a;
}





//??? implement
std::string posix_timespec__to__utc(const time_t& t){
	return std::to_string(t);
}

fsentry_info_t corelib_get_fsentry_info(const std::string& abs_path){
	if(is_valid_absolute_dir_path(abs_path) == false){
		quark::throw_runtime_error("get_fsentry_info() illegal input path.");
	}

	TFileInfo info;
	bool ok = GetFileInfo(abs_path, info);
	QUARK_ASSERT(ok);
	if(ok == false){
		quark::throw_exception();
	}

	const auto parts = SplitPath(abs_path);
	const auto parent = UpDir2(abs_path);

	const auto type_string = info.fDirFlag ? "dir" : "string";
	const auto name = info.fDirFlag ? parent.second : parts.fName;
	const auto parent_path = info.fDirFlag ? parent.first : parts.fPath;

	const auto creation_date = posix_timespec__to__utc(info.fCreationDate);
	const auto modification_date = posix_timespec__to__utc(info.fModificationDate);
	const auto file_size = info.fFileSize;

	const auto result = fsentry_info_t {
		type_string,
		name,
		parent_path,

		creation_date,
		modification_date,

		static_cast<int64_t>(file_size)
	};

	return result;
}

value_t pack_fsentry_info(const fsentry_info_t& info){
	const auto result = value_t::make_struct_value(
		make__fsentry_info_t__type(),
		{
			value_t::make_string(info.type),
			value_t::make_string(info.name),
			value_t::make_string(info.abs_parent_path),

			value_t::make_string(info.creation_date),
			value_t::make_string(info.modification_date),

			value_t::make_int(info.file_size)
		}
	);

#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	return result;
}




/*
struct fs_environment_t {
	std::string home_dir;
	std::string documents_dir;
	std::string desktop_dir;

	std::string hidden_persistence_dir;
	std::string preferences_dir;
	std::string cache_dir;
	std::string temp_dir;

	std::string executable_dir;
};
*/

fs_environment_t corelib_get_fs_environment(){
	const auto dirs = GetDirectories();

	const auto result = fs_environment_t {
		dirs.home_dir,
		dirs.documents_dir,
		dirs.desktop_dir,

		dirs.application_support,
		dirs.preferences_dir,
		dirs.cache_dir,
		dirs.temp_dir,

		dirs.process_dir
	};
	return result;
}

value_t pack_fs_environment_t(const fs_environment_t& env){
	const auto result = value_t::make_struct_value(
		make__fs_environment_t__type(),
		{
			value_t::make_string(env.home_dir),
			value_t::make_string(env.documents_dir),
			value_t::make_string(env.desktop_dir),

			value_t::make_string(env.hidden_persistence_dir),
			value_t::make_string(env.preferences_dir),
			value_t::make_string(env.cache_dir),
			value_t::make_string(env.temp_dir),

			value_t::make_string(env.executable_dir)
		}
	);

#if 1
	const auto debug = value_and_type_to_ast_json(result);
	QUARK_TRACE(json_to_pretty_string(debug));
#endif

	return result;
}



bool corelib_does_fsentry_exist(const std::string& abs_path){
	if(is_valid_absolute_dir_path(abs_path) == false){
		quark::throw_runtime_error("does_fsentry_exist() illegal input path.");
	}

	bool exists = DoesEntryExist(abs_path);
	return exists;
}



void corelib_create_directory_branch(const std::string& abs_path){
	if(is_valid_absolute_dir_path(abs_path) == false){
		quark::throw_runtime_error("create_directory_branch() illegal input path.");
	}

	MakeDirectoriesDeep(abs_path);
}



void corelib_delete_fsentry_deep(const std::string& abs_path){
	if(is_valid_absolute_dir_path(abs_path) == false){
		quark::throw_runtime_error("delete_fsentry_deep() illegal input path.");
	}

	DeleteDeep(abs_path);
}

void corelib_rename_fsentry(const std::string& abs_path, const std::string& n){
	if(is_valid_absolute_dir_path(abs_path) == false){
		quark::throw_runtime_error("rename_fsentry() illegal input path.");
	}

	if(n.empty()){
		quark::throw_runtime_error("rename_fsentry() illegal input name.");
	}

	RenameEntry(abs_path, n);
}


}	// floyd

