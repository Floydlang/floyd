//
//  json_to_ast.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 18/09/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef json_to_ast_hpp
#define json_to_ast_hpp

namespace floyd_parser {
	struct ast_t;
}
struct json_value_t;

floyd_parser::ast_t json_to_ast(const json_value_t& program);


#endif /* json_to_ast_hpp */
