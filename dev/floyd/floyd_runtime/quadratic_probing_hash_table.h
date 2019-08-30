//
//  quadratic_probing_hash_table.hpp
//  floyd
//
//  Created by Marcus Zetterquist on 2019-08-23.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef quadratic_probing_hash_table_hpp
#define quadratic_probing_hash_table_hpp

#include <string>
#include <vector>


struct kv_t {
	std::string key;
	double value;
};



//	std::size_t h2 = std::hash<std::string>{}(s.last_name);
//	https://www.youtube.com/watch?v=7eLDTtbzX4M

struct quadratic_probing_hash_table_t {
	enum class mode {
		empty,
		tombstone
	};

	quadratic_probing_hash_table_t(int64_t table_size) :
		backstore(table_size, std::pair<mode, kv_t>(mode::empty, kv_t { "-", 3.0 }))
	{
	}

	quadratic_probing_hash_table_t(const kv_t values[], int64_t count, int64_t table_size){
	}

	void insert(const kv_t& kv){
		const auto hash0 = std::hash<std::string>{}(kv.key);
	}



	////////////////////////////////		STATE

	std::vector<std::pair<mode, kv_t>> backstore;
};


#endif /* quadratic_probing_hash_table_hpp */
