



struct cache_line64_t {
	uint32_t _values[64 / 4];
}

/*
	Most generic, and biggest value. 16 bytes. Maybe can squeeze down to 8 if we can steal 2 pointer bits (bit 0-1).


	struct string_body_y {
		int _rc;
	}

	struct string_ptr_t {
		char* 
	}

	//	Points to the next layout, if chained. List cons persistence? Move
	type_next_layout_ptr
		layout1_t* _next;

	type_bool_vec
		uint8_t _type;
		uint8_t _bits[7]; 	//	 holds 56 bool:s.

	type_string_inline
		uint8_t _type;
		uint8_t _count;		//	 0-7. (Wastes 7 bits)
		uint8_t _chars[6];

	type_string_ref
		string_ptr_t _string_ptr;

	type_string_vec_inline
		uint8_t _type;
		uint8_t _counts4;
		uint8_t _str0[2];
		uint8_t _str0[2];
		uint8_t _str0[2];
*/
struct rt_value8_t {
	int _type;
	string* _string;
}


struct layout1_t {
	uint32_t _rc;
	rt_value8_t _values[8];
}


struct layout_member_t {
	type_def_id* _type_def;
	string _name;
	shared_ptr<value_t> _optional_default;
}

struct normalized_layout_t {
	vector<layout_member_t> _members;
}

struct scope_instance_t {
	cache_line64_t _data;
}