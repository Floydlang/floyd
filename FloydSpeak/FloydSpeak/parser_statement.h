//
//  parser_statement.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_statement_hpp
#define parser_statement_hpp


#include "quark.h"
#include <vector>
#include <string>
#include <map>

#include "parser_ast.h"
#include "expressions.h"


namespace floyd_parser {
	struct statement_t;
	struct expression_t;


    //////////////////////////////////////		bind_statement_t
    
    
    struct bind_statement_t {
        bool operator==(const bind_statement_t& other) const {
            return _identifier == other._identifier && _expression == other._expression;
        }
        
        std::string _identifier;
        std::shared_ptr<expression_t> _expression;
    };
    
    
    //////////////////////////////////////		define_struct_statement_t
    
    
    struct define_struct_statement_t {
        bool operator==(const define_struct_statement_t& other) const {
            return _struct_def == other._struct_def;
        }
        
        struct_def_t _struct_def;
    };
    
    
    //////////////////////////////////////		define_function_statement_t
    
    
    struct define_function_statement_t {
        bool operator==(const define_function_statement_t& other) const {
            return _function_def == other._function_def;
        }
        
        function_def_t _function_def;
    };
    
    
	//////////////////////////////////////		return_statement_t


	struct return_statement_t {
		bool operator==(const return_statement_t& other) const {
			return *_expression == *other._expression;
		}

		std::shared_ptr<expression_t> _expression;
	};


	statement_t makie_return_statement(const expression_t& expression);


    
	//////////////////////////////////////		statement_t


	struct statement_t {
		bool check_invariant() const;

		statement_t(const bind_statement_t& value) :
			_bind_statement(std::make_shared<bind_statement_t>(value))
		{
		}

        statement_t(const define_struct_statement_t& value) :
            _define_struct(std::make_shared<define_struct_statement_t>(value))
        {
        }

        statement_t(const define_function_statement_t& value) :
            _define_function(std::make_shared<define_function_statement_t>(value))
        {
        }

        statement_t(const return_statement_t& value) :
			_return_statement(std::make_shared<return_statement_t>(value))
		{
		}

		bool operator==(const statement_t& other) const {
			if(_bind_statement){
				return other._bind_statement && *_bind_statement == *other._bind_statement;
			}
			else if(_return_statement){
				return other._return_statement && *_return_statement == *other._return_statement;
			}
			else{
				QUARK_ASSERT(false);
				return false;
			}
		}

        const std::shared_ptr<bind_statement_t> _bind_statement;
        const std::shared_ptr<define_struct_statement_t> _define_struct;
        const std::shared_ptr<define_function_statement_t> _define_function;
		const std::shared_ptr<return_statement_t> _return_statement;
	};


	//////////////////////////////////////		Makers


	statement_t make__bind_statement(const bind_statement_t& value);
	statement_t make__bind_statement(const std::string& identifier, const expression_t& e);
	statement_t make__return_statement(const return_statement_t& value);

	void trace(const statement_t& s);

}	//	floyd_parser


#endif /* parser_statement_hpp */
