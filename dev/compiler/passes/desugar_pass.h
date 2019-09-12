//
//  desugar_pass.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-07-29.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef desugar_pass_hpp
#define desugar_pass_hpp

namespace floyd {

struct unchecked_ast_t;


unchecked_ast_t desugar_pass(const unchecked_ast_t& unchecked_ast);

}	// floyd

#endif /* desugar_pass_hpp */
