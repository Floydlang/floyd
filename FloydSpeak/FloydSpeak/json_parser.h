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

struct json_value_t;

std::pair<json_value_t, std::string> parse_json(const std::string& s);



#endif /* json_parser_hpp */
