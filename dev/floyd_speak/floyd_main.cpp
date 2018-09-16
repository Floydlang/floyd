//
//  floyd_main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/04/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "quark.h"

#include "floyd_interpreter.h"
#include "floyd_parser/floyd_parser.h"
#include "ast_value.h"
#include "json_support.h"
#include "ast_basics.h"
#include "text_parser.h"
#include "interpretator_benchmark.h"



#include <cmath>
#include <thread>
#include <future>


/////////////////        Multi-threading example



struct pixel {
	pixel(float red, float green, float blue, float alpha) :
	_red(red),
	_green(green),
	_blue(blue),
	_alpha(alpha)
	{
	}
	pixel() = default;
	pixel& operator=(const pixel& rhs) = default;
	pixel(const pixel& rhs) = default;
	
	
	////////////////    State
	
	float _red = 0.0f;
	float _green = 0.0f;
	float _blue = 0.0f;
	float _alpha = 0.0f;
};

struct image {
	int _width;
	int _height;
	immer::vector<pixel> _pixels;
};

image make_image(int width, int height){
	immer::vector<pixel> pixels;
	
	for(auto y = 0 ; y < height ; y++){
		for(auto x = 0 ; x < width ; x++){
			float s = std::sin(static_cast<float>(M_PI) * 2.0f * (float)x);
			pixel p(0.5f + s, 0.5f, 0.0f, 1.0f);
			pixels.push_back(p);
		}
	}
	
	image result;
	result._width = width;
	result._height = height;
	result._pixels = pixels;
	return result;
}


//    There is no way to trip-up caller because image is a copy.
image worker8(image img) {
	const size_t count = std::min<size_t>(300, img._pixels.size());
	for(size_t i = 0 ; i < count ; i++){
		auto pixel = img._pixels[i];
		pixel._red = 1.0f - pixel._red;
		img._pixels = img._pixels.set(i, pixel);
	}
	return img;
}

//    Demonstates it is safe and easy and efficent to use immer::vector when sharing data between threads.
void example8(){
	QUARK_SCOPED_TRACE(__FUNCTION__);
	
	const auto a = make_image(700, 700);
	
//    const auto inodes1 = steady::get_inode_count<pixel>();
//    const auto leaf_nodes1 = steady::get_leaf_count<pixel>();
//    QUARK_TRACE_SS("When a exists: inodes: " << inodes1 << ", leaf nodes: " << leaf_nodes1);
	
	//    Get a workerthread process image a in the background.
	//    We give the worker a copy of a. This is free.
	//    There is no way for worker thread to have side effects on image a.
	auto future = std::async(worker8, a);
	
	//    ...meanwhile, do some processing of our own on mage a!
	auto b = a;
	for(int i = 0 ; i < 200 ; i++){
		auto pixel = b._pixels[i];
		pixel._red = 1.0f - pixel._red;
		b._pixels = b._pixels.set(i, pixel);
	}
	
	//    Block on worker finishing.
	image c = future.get();
	
	
//    const auto inodes2 = steady::get_inode_count<pixel>();
//    const auto leaf_nodes2 = steady::get_leaf_count<pixel>();
	//QUARK_TRACE_SS("When b + c exists: inodes: " << inodes2 << ", leaf nodes: " << leaf_nodes2);
	
//    const auto extra_inodes = inodes2 - inodes1;
//    const auto extra_leaf_nodes = leaf_nodes2 - leaf_nodes1;
	
//    QUARK_TRACE_SS("Extra storage requried for a + b: inodes: " << extra_inodes << ", leaf nodes: " << extra_leaf_nodes);
}

QUARK_UNIT_TEST("","", "", ""){
	example8();
}


static const int k_hardware_thread_count = 8;




void call_from_thread(int tid) {
	std::cout << tid << std::endl;
}

struct runtime_t {
	std::vector<std::thread> _worker_threads;


	runtime_t(){
		//	Remember that current thread (main) is also a thread.
		for(int i = 0 ; i < k_hardware_thread_count - 1 ; i++){
			_worker_threads.push_back(std::thread(call_from_thread, i));
		}
	}
	~runtime_t(){
		for(auto &t: _worker_threads){
			t.join();
		}
	}
};




QUARK_UNIT_TEST("","", "", ""){
	std::cout << "A" << std::endl;
	{
		runtime_t test_runtime;
		std::cout << "B" << std::endl;
	}
	std::cout << "C" << std::endl;
}









//////////////////////////////////////////////////		main()

#if true

#include <editline/readline.h>

void init_terminal(){
}


std::string get_command(){
	std::string result;
	while(result.empty()){
		char* line_read = readline (">>>");
		if(line_read != nullptr){
			result = std::string(line_read);
			std::free (line_read);
			line_read = nullptr;
		}
	}
	add_history (result.c_str());
	return result;
}


#endif


#if false

void init_terminal(){
}
std::string get_command(){
	std::cout << ">>> " << std::flush;

	std::string line;
	while(line == ""){
		std::cin.clear();
		std::getline (std::cin, line);
	}
	return line;
}
#endif

#if false

std::string get_command(){
	std::cout << ">>> " << std::flush;

	std::string line;
	bool done = false;
	while(done == false){
		int ch = getch();
		std::cout << ch << std::endl;

		if(ch == '\n'){
			done = true;
		}
		else{
			 line = line + std::string(1, ch);
		}
	}
	return line;
}
#endif


#if false

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

void init_terminal(){
	struct termios oldt, newt;

	/* tcgetattr gets the parameters of the current terminal
	* STDIN_FILENO will tell tcgetattr that it should write the settings
	* of stdin to oldt
	*/
	tcgetattr( STDIN_FILENO, &oldt);
	/*now the settings will be copied*/
	memcpy((void *)&newt, (void *)&oldt, sizeof(struct termios));

	newt.c_lflag &= ~(ICANON);  // Reset ICANON so enter after char is not needed
	newt.c_lflag &= ~(ECHO);    // Turn echo off

	/*
	*  Those new settings will be set to STDIN
	*  TCSANOW tells tcsetattr to change attributes immediately.
	*/
	tcsetattr( STDIN_FILENO, TCSANOW, &newt);

//	int option = getchar(); //this is where the getch is used
	//printf("\nYour choice is: %c\n",option);

	/*
	*  Restore original settings
	*/
//	tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}
#endif

#if false

#include<ncurses.h>

void init_terminal(){
			 initscr(); cbreak(); noecho();

//       Most programs would additionally use the sequence:

			 nonl();
			 intrflush(stdscr, FALSE);
			 keypad(stdscr, TRUE);
}

std::string get_command(){
	std::cout << ">>> " << std::flush;

	std::string line;
	bool done = false;
	while(done == false){
		int ch_int = getch();
		char ch = ch_int;
//		std::cout << ch << std::flush;

		if(ch == '\n'){
			done = true;
		}
		else if(ch_int == 239){
			std::cout << "UP" << std::flush;
		}
		else{
			 line = line + std::string(1, ch);
		}
	}
	return line;
}

#endif


std::string floyd_version_string = "0.3";


#if 0
int handle_repl_input(floyd::interpreter_context_t& context, int print_pos, std::shared_ptr<floyd::interpreter_t>& vm_mut, const std::string& line){
	const auto& program1 = vm_mut->_imm->_program;

	const auto blank_program = floyd::compile_to_bytecode(context, "");
	const auto program2 = floyd::compile_to_bytecode(context, line);

	//	Copy the new symbols, compared to a blank program.
	const auto new_symbol_count = program2._globals._symbols.size() - blank_program._globals._symbols.size();
	const auto new_symbols = std::vector<std::pair<std::string, floyd::symbol_t>>(program2._globals._symbols.end() - new_symbol_count, program2._globals._symbols.end());

	const auto new_instruction_count = program2._globals._instrs2.size() - blank_program._globals._instrs2.size();
	const auto new_instructions = std::vector<floyd::bc_instruction_t>(program2._globals._instrs2.end() - new_instruction_count, program2._globals._instrs2.end());

	const auto globals2 = floyd::bc_frame_t(
		program1._globals._instrs2,
		program1._globals._symbols + new_symbols,
		program1._globals._args
	);

	const auto program3 = floyd::bc_program_t{ globals2, program1._function_defs, program1._types };
	auto imm2 = std::make_shared<floyd::interpreter_imm_t>(floyd::interpreter_imm_t{vm_mut->_imm->_start_time, program3, vm_mut->_imm->_host_functions});

	vm_mut->_imm.swap(imm2);

	const auto b = floyd::execute_instructions(*vm_mut, new_instructions);
	while(print_pos < vm_mut->_print_output.size()){
		std::cout << vm_mut->_print_output[print_pos] << std::endl;
		print_pos++;
	}
	const auto return_value_type = vm_mut->_imm->_program._types[b.first];
	if(return_value_type.is_void() == false){
		const auto result_value = floyd::bc_to_value(b.second, return_value_type);
		std::cout << to_compact_string2(result_value) << std::endl;
	}
	return print_pos;
}
#endif

void run_repl(){
	init_terminal();

	floyd::interpreter_context_t context{ quark::make_default_tracer() };

	auto program = floyd::compile_to_bytecode(context, "");
	auto vm = std::make_shared<floyd::interpreter_t>(program);

	std::cout << R"(Floyd " << floyd_version_string << " MIT.)" << std::endl;
	std::cout << R"(Type "help", "copyright" or "license" for more informations!)" << std::endl;

/*
Python 2.7.10 (default, Jul 15 2017, 17:16:57)
[GCC 4.2.1 Compatible Apple LLVM 9.0.0 (clang-900.0.31)] on darwin
Type "help", "copyright", "credits" or "license" for more information.
*/

	int print_pos = 0;
	while(true){
		try {
			const auto line = get_command();

			if(line == "vm"){
				std::cout << json_to_pretty_string(floyd::interpreter_to_json(*vm)) << std::endl;
			}
			else if(line == ""){
			}
			else if(line == "help"){
				std::cout << "vm -- prints complete state of vm." << std::endl;
			}
			else if(line == "copyright"){
				std::cout << "Copyright 2018 Marcus Zetterquist." << std::endl;
			}
			else if(line == "license"){
				std::cout << "MIT license." << std::endl;
			}
			else{
//				const auto print_pos2 = handle_repl_input(context, print_pos, vm, line);
//				print_pos = print_pos2;
			}
		}
		catch(const std::runtime_error& e){
			std::cout << "Runtime error: " << e.what() << std::endl;
		}
		catch(...){
			std::cout << "Runtime error." << std::endl;
		}
	}
}

void run_file(const std::vector<std::string>& args){
	const auto source_path = args[1];
	const std::vector<std::string> args2(args.begin() + 2, args.end());

//	std::cout << "Source file:" << source_path << std::endl;

	std::string source;
	{
		std::ifstream f (source_path);
		if (f.is_open() == false){
			throw std::runtime_error("Cannot read source file.");
		}
		std::string line;
		while ( getline(f, line) ) {
			source.append(line + "\n");
		}
		f.close();
	}

//	std::cout << "Source:" << source << std::endl;


//	std::cout << "Compiling..." << std::endl;
	floyd::interpreter_context_t context{ quark::make_default_tracer() };
	auto program = floyd::compile_to_bytecode(context, source);


//	std::cout << "Preparing arguments..." << std::endl;

	std::vector<floyd::value_t> args3;
	for(const auto e: args2){
		args3.push_back(floyd::value_t::make_string(e));
	}

//	std::cout << "Running..." << source << std::endl;

	const auto result = floyd::run_program(context, program, args3);
	std::cout << result.second.get_int_value() << std::endl;
}

void run_tests(){
	quark::run_tests({
		"quark.cpp",

		"text_parser.cpp",
		"steady_vector.cpp",
		"unused_bits.cpp",
		"sha1_class.cpp",
		"sha1.cpp",
		"json_parser.cpp",
		"json_support.cpp",
		"json_writer.cpp",

		"floyd_basics.cpp",


		"parser2.cpp",
		"parser_value.cpp",
		"parser_primitives.cpp",

		"parser_expression.cpp",
		"parser_function.cpp",
		"parser_statement.cpp",
		"parser_struct.cpp",
		"parse_prefixless_statement.cpp",
		"floyd_parser.cpp",

		"floyd_test_suite.cpp",
		"interpretator_benchmark.cpp",

		"ast_typeid.cpp",

		"pass2.cpp",
		"pass3.cpp",

		"parse_statement.cpp",
		"floyd_interpreter.cpp",

		//	Core libs
/*
		"parser_ast.cpp",
		"ast_utils.cpp",
		"experimental_runtime.cpp",
		"expressions.cpp",
		"llvm_code_gen.cpp",
		"utils.cpp",
		"pass3.cpp",
		"floyd_main.cpp",
*/
	});
}

std::vector<std::string> args_to_vector(int argc, const char * argv[]){
	std::vector<std::string> r;
	for(int i = 0 ; i < argc ; i++){
		const std::string s(argv[i]);
		r.push_back(s);
	}
	return r;
}


bool trace_on = true;


//??? Only exists so we cn control tracing on/off. Delete and use new trace_context_t instead.
struct floyd_tracer : public quark::trace_i {
	public: floyd_tracer();

	public: virtual void trace_i__trace(const char s[]) const;
	public: virtual void trace_i__open_scope(const char s[]) const;
	public: virtual void trace_i__close_scope(const char s[]) const;


	///////////////		State.
	public: quark::default_tracer_t def;
};


void floyd_tracer::trace_i__trace(const char s[]) const {
	if(trace_on){
		def.trace_i__trace(s);
	}
}

void floyd_tracer::trace_i__open_scope(const char s[]) const {
	def.trace_i__open_scope(s);
}

void floyd_tracer::trace_i__close_scope(const char s[]) const{
	def.trace_i__close_scope(s);
}


struct floyd_quark_runtime : public quark::runtime_i {
	floyd_quark_runtime(const std::string& test_data_root);

	public: virtual void runtime_i__on_assert(const quark::source_code_location& location, const char expression[]);
	public: virtual void runtime_i__on_unit_test_failed(const quark::source_code_location& location, const char expression[]);


	///////////////		State.
		const std::string _test_data_root;
		long _indent;
};

floyd_quark_runtime::floyd_quark_runtime(const std::string& test_data_root) :
	_test_data_root(test_data_root),
	_indent(0)
{
}

void floyd_quark_runtime::runtime_i__on_assert(const quark::source_code_location& location, const char expression[]){
	QUARK_TRACE_SS(std::string("Assertion failed ") << location._source_file << ", " << location._line_number << " \"" << expression << "\"");
	perror("perror() says");
	throw std::logic_error("assert");
}

void floyd_quark_runtime::runtime_i__on_unit_test_failed(const quark::source_code_location& location, const char expression[]){
	QUARK_TRACE_SS("Unit test failed " << location._source_file << ", " << location._line_number << " \"" << expression << "\"");
	perror("perror() says");

	throw std::logic_error("Unit test failed");
}


int main(int argc, const char * argv[]) {
	const auto prev_q = quark::get_runtime();
	const auto args = args_to_vector(argc, argv);

	floyd_quark_runtime q("");
	quark::set_runtime(&q);

	bool runtests = std::find(args.begin(), args.end(), "runtests") != args.end();
	bool benchmark = std::find(args.begin(), args.end(), "benchmark") != args.end();
	bool run_as_tool = runtests == false && benchmark == false;

	if(runtests && QUARK_UNIT_TESTS_ON){
		try {
			run_tests();
		}
		catch(...){
			QUARK_TRACE("Error");
			return -1;
		}
	}
	else if(benchmark){
		floyd_benchmark();
	}

	else if(run_as_tool){
		trace_on = false;

		//	Run provided script file.
		if(args.size() == 2){
			const auto floyd_args = std::vector<std::string>(args.begin() + 1, args.end());
			run_file(floyd_args);
		}

		//	Run REPL
		else{
//			run_repl();
		}
	}

	quark::set_runtime(prev_q);

	return 0;
}

