//
//  ast_helpers.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-07-30.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef ast_helpers_hpp
#define ast_helpers_hpp

#include <vector>
#include <memory>

namespace floyd {

struct general_purpose_ast_t;
struct function_definition_t;
struct expression_t;
struct statement_t;
struct lexical_scope_t;
struct type_t;
struct ast_type_t;
struct types_t;
struct struct_type_desc_t;

/*
	All types can be resolved in types (or defect)
	Interned types are fully defined = all their types and subtypes are OK (undefined or identifer not allowed).
*/

bool check_types_resolved(const types_t& types, const expression_t& e);
bool check_types_resolved(const types_t& types, const std::vector<expression_t>& expressions);

bool check_types_resolved(const types_t& types, const function_definition_t& def);

bool check_types_resolved(const types_t& types, const lexical_scope_t& scope);

bool check_types_resolved(const types_t& types, const statement_t& s);
bool check_types_resolved(const types_t& types, const std::vector<std::shared_ptr<statement_t>>& s);

bool check_types_resolved(const types_t& types, const struct_type_desc_t& s);
bool check_types_resolved(const types_t& types, const type_t& t);

bool check_types_resolved(const types_t& types);
bool check_types_resolved(const general_purpose_ast_t& ast);

bool check_types_resolved__type_vector(const types_t& types, const std::vector<type_t>& elements);


}	//	floyd


#endif /* ast_helpers_hpp */
