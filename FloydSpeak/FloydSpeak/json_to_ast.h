//
//  json_to_ast.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 18/09/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef json_to_ast_hpp
#define json_to_ast_hpp

#include <map>

namespace floyd_parser {
	struct ast_t;
	struct expression_t;
	struct type_def_t;
}
struct json_t;

floyd_parser::ast_t json_to_ast(const json_t& program);

/*
	Recursive function
	Pure
	Converts input program in JSON format into an expression_t, using
*/
floyd_parser::expression_t conv_expression(const json_t& e, const std::map<std::string, std::shared_ptr<floyd_parser::type_def_t>>& types);


#endif /* json_to_ast_hpp */
