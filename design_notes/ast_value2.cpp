//////////////////////////////		DEFINITIONS

	struct vector_def_t {
		/////////////////////////////		STATE
		public: floyd::typeid_t _element_type;
	};

	struct dict_def_t {
		/////////////////////////////		STATE
		public: floyd::typeid_t _value_type;
	};

	struct function_definition_t {
		const std::vector<member_t> _args;
		const std::vector<std::shared_ptr<statement_t>> _statements;
		const int _host_function;
		const typeid_t _return_type;
	};


//////////////////////////////		VALUE TYPE


	struct struct_instance_t {
		public: struct_instance_t(const struct_definition_t& def, const std::vector<value_t>& member_values) :
			_def(def),
			_member_values(member_values)
		{
			QUARK_ASSERT(_def.check_invariant());

			QUARK_ASSERT(check_invariant());
		}
		public: bool check_invariant() const;
		public: bool operator==(const struct_instance_t& other) const;


		public: struct_definition_t _def;
		public: std::vector<value_t> _member_values;
	};

	struct vector_instance_t {
		public: bool check_invariant() const;
		public: bool operator==(const vector_instance_t& other) const;

		public: vector_instance_t(
			typeid_t element_type,
			const std::vector<value_t>& elements
		):
			_element_type(element_type),
			_elements(elements)
		{
		}

		typeid_t _element_type;
		std::vector<value_t> _elements;
	};

	struct dict_instance_t {
		public: bool check_invariant() const;
		public: bool operator==(const dict_instance_t& other) const;

		public: dict_instance_t(
			typeid_t value_type,
			const std::map<std::string, value_t>& elements
		):
			_value_type(value_type),
			_elements(elements)
		{
		}

		typeid_t _value_type;
		std::map<std::string, value_t> _elements;
	};

	struct function_instance_t {
		public: bool check_invariant() const;
		public: bool operator==(const function_instance_t& other) const;

		public: function_definition_t _def;
	};


		////////////////////		STATE

#if DEBUG
		private: std::string DEBUG_STR;
#endif
		private: typeid_t _typeid;

		private: bool _bool = false;
		private: int _int = 0;
		private: float _float = 0.0f;
		private: std::string _string = "";
		private: std::shared_ptr<json_t> _json_value;
		private: typeid_t _typeid_value = typeid_t::make_null();
		private: std::shared_ptr<struct_instance_t> _struct;
		private: std::shared_ptr<vector_instance_t> _vector;
		private: std::shared_ptr<dict_instance_t> _dict;
		private: std::shared_ptr<const function_instance_t> _function;




COMPACTION 1: USE TAGGED UNION._dict




//	----------------------------------- -----------------------------------
//	66665555 55555544 44444444 33333333 33222222 22221111 11111100 00000000
//	32109876 54321098 76543210 98765432 10987654 32109876 54321098 76543210
//	XXXXXXXX XXXXXXXX PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPppp		48bit Intel x86_64 pointer. ppp = low bits, set to 0, X = bit 47
//	-----------------------------------------------------------------------

nill:
//	00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

bool:
//	00000000 00000000 00000000 00000000 00000000 00000000 00000000 0000000B

B == 0: false
B == 1: true


string:
//	CCCCCCCC AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD EEEEEEEE FFFFFFFF GGGGGGGG

C: Control byte
S: encodes 7 bytes of data. data[7]. Encoded in AAAAAAAA = data[6] GGGGGGGG == data[0]

C: 0 = empty string, 
C: 1...7 = short string, stored directly in S
C: 8: long string. P holds pointer to exteranal string object

??? always intern strings?




struct external_string_t {
	mutable std::atomic<unsigned> count;
	size_t size;
    char s[];
}

struct my_struct *s = malloc(sizeof(struct my_struct) + 50);









Type is not stored in the the values.

struct compact_value_t {
	uint64_t data0;
}


void add_ref(compact_value_t& value){
	std::atomic_fetch_add_explicit (&count, 1u, std::memory_order_consume);
}
void release_ref(compact_value_t& value){
    bool destroy = std::atomic_fetch_sub_explicit (&count, 1u, std::memory_order_consume) == 1;
    if (destroy){
        delete this;
    }
}






