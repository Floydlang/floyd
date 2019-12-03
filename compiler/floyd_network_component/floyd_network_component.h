//
//  floyd_network_component.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-12-03.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_network_component_hpp
#define floyd_network_component_hpp

#include <string>
#include <map>
#include <vector>

#include "compiler_basics.h"



//??? NOTICE: We don't have a module system yet. The network component is merged into corelib for now.
namespace floyd {



extern const std::string k_network_component_header;




//######################################################################################################################
//	NETWORK COMPONENT
//######################################################################################################################


//??? make_type__dddd_t()
type_t make__network_component_t__type(types_t& types);
type_t make__id_address_and_port_t__type(types_t& types);
type_t make__host_info_t__type(types_t& types);
type_t make__header_t__type(types_t& types);
type_t make__http_request_line_t__type(types_t& types);
type_t make__http_request_t__type(types_t& types);
type_t make__http_response_status_line_t__type(types_t& types);
type_t make__http_response_t__type(types_t& types);



} // floyd

#endif /* floyd_network_component_hpp */
