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
struct json_value_t;

floyd_parser::ast_t json_to_ast(const json_value_t& program);

floyd_parser::expression_t conv_expression(const json_value_t& e, const std::map<std::string, std::shared_ptr<floyd_parser::type_def_t>>& temp_type_defs);


#endif /* json_to_ast_hpp */
