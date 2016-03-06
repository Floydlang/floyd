//
//  Compiler.cpp
//  Floyd
//
//  Created by Marcus Zetterquist on 06/03/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include <string>
#include <vector>

#include "cpp_extension.h"
#include "Compiler.hpp"


using std::string;
using std::vector;


/*
	This compiler reads Floyd-script programs and generates C++ code.
*/


const std::string kTestProgram0 =
	"int f1(){"
	"	return 4;"
	"}"
;

const std::string kTestProgram1 =
	"int f1(int b){"
	"	return b + 1;"
	"}"
;


/*
enum EDataType {
	kInt = 4,
	kFloat = 5,
	kString = 6,
	kFunction = 7
};
*/
struct DataTypes {
//	public static const kInt =
};



struct TArg {
	string _type;
	string _name;
};




struct TDeclareFunctionNode {
	string _name;
	vector<TArg> _args;
	string _returnType;
};


struct TTranslationUnit {
	vector<TDeclareFunctionNode> _functions;
};



struct ProgramNode {
	enum EType {
		kDeclareFunction
	};

//	EDeclarationType _declaration;

};

const string kChars = "()<>=_";
const string kAlphas ="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const string kAllChars = kChars + kAlphas;


namespace {

	std::pair<string, string> read_token(string s){
		ASSERT(s.size () > 0);

		size_t pos = 0;
		while(pos < s.size() && s[pos] != ' '){
			pos++;
		}
		if(pos == s.size()){
			return std::pair<string, string>(s, "");
		}
		else{
			return std::pair<string, string>(s.substr(0, pos), s.substr(pos, string::npos));
		}
	}

}


UNIT_TEST("", "read_token", "empty", "blank blank"){
	UT_VERIFY((read_token("") == std::pair<string, string>("", "")));
}

std::string Compile(const std::string program){
	//	Parse text and build a syntax tree.
	std::pair<string, string> pos = read_token(program);
	while(pos.first != ""){
		if(pos.first == "int" || pos.first == "float" || pos.first == "string"){
			var pos2 = read_token(pos.second);
			if(pos2.)
		}
		else{
		}
	}
}



UNIT_TEST("", "CallFunction", "SimpleFunction()", "CorrectValue"){
	UT_VERIFY(kTestProgram1.size () > 0);
}



