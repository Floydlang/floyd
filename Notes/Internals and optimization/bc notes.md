

		public: std::string _string;
		public: std::shared_ptr<json_t> _json_value;
		public: typeid_t _typeid_value = typeid_t::make_null();
		public: std::shared_ptr<struct_instance_t> _struct;
		public: std::vector<value_t> _vector_elements;
		public: std::map<std::string, value_t> _dict_entries;
		public: int _function_id = -1;


#if DEBUG
		private: std::string DEBUG_STR;
#endif
		enum type_int {
			k_null = 0,
			k_bool = 1,
			k_int = 2,
			k_float = 3,
			k_ext
		};
		private: uint8_t _type_int;

		private: union value_internals_t {
			bool _bool;
			int _int;
			float _float;
		};

		private: value_internals_t _value_internals;
		private: std::shared_ptr<value_ext_t> _ext;
	};





		----------------------------------- -----------------------------------
		66665555 55555544 44444444 33333333 33222222 22221111 11111100 00000000
		32109876 54321098 76543210 98765432 10987654 32109876 54321098 76543210

		XXXXXXXX XXXXXXXX PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPppp
		48bit Intel x86_64 pointer. ppp = low bits, set to 0, X = bit 47

		-----------------------------------
		33222222 22221111 11111100 00000000
		10987654 32109876 54321098 76543210



# Free value BC encoding.
This is how values are stored on stack, in local variables etc in the BC interpreter.


null: 0x00000000
bool: 0x0000000V
int: 0xvvvvvvvv
float: 0xffffffff

string: 0xssssssss
	s = ID of string object. Use RC?


struct bc_value32 {
	uint32_t _data;
}
struct bc_value_obj {
	bc_value(){
	}
	~bc_value(){
	}

	void* _obj;
}



How to store values on BC stack etc and still get them to copy/destruct strings and other objects?
A) Store as objects with destructor etc.
B) Store ptr/ID and DO NOT track lifetime -- generate instructions that handle lifetime of object.

C) Use several value-widths: one for passive 32bits, one 64bit.

D) Use load32, load_obj. load_obj handles RC.

		-----------------------------------
		33222222 22221111 11111100 00000000
		10987654 32109876 54321098 76543210







