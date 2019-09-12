//
//  ast_visitor.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-07-29.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef ast_visitor_hpp
#define ast_visitor_hpp

namespace floyd {

struct expression_t;

struct general_purpose_ast_t;

general_purpose_ast_t visit_ast(const general_purpose_ast_t& ast);



struct visit_ast_i {
	public: virtual ~visit_ast_i() {}

	public: virtual void on_expression(const expression_t& e);
};

struct visit_ast_impl_t : visit_ast_i {
	public: void on_expression(const expression_t& e) override;
};


}	// floyd

#endif /* ast_visitor_hpp */
