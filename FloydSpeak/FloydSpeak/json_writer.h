//
//  json.hpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 10/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef json_writer_hpp
#define json_writer_hpp

#include <string>
#include <vector>
#include <map>
#include "quark.h"

struct json_t;

std::string json_to_compact_string(const json_t& v);
std::string json_to_compact_string_minimal_quotes(const json_t& v);


//	Defaults to 120 chars width print out, 4 space-tabs.
std::string json_to_pretty_string(const json_t& v);

struct pretty_t {
	int _max_column_chars;
	int _tab_char_setting;
};
std::string json_to_pretty_string(const json_t& value, int pos, const pretty_t& pretty);



#endif /* json_writer_hpp */
