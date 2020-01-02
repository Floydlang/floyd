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


//??? NOTICE: We don't have a module system yet. The network component is hardcoded, just like corelib.
namespace floyd {

std::string get_network_component_header();
std::map<std::string, void*> get_network_component_binds();

} // floyd

#endif /* floyd_network_component_hpp */
