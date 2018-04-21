//
//  json_parser.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 12/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef json_parser_hpp
#define json_parser_hpp

#include <string>

struct json_t;
struct seq_t;

std::pair<json_t, seq_t> parse_json(const seq_t& s);


#endif /* json_parser_hpp */
