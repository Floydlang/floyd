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

struct json_value_t;

std::string json_to_compact_string(const json_value_t& v);


#endif /* json_writer_hpp */
