any / generics / tuple (unnamed struct) / sum-type/enum

any-type
typeid-type
typeof()
isa(x)
string x = makea(any)


generics: support types $t0 $t1 automatically. Use $0 and $1 for unnamed variables.


value_t: 16 bytes

static type int32: instance 4 bytes
static type double: instance 8 bytes
static type string: instance 16 bytes
static type struct pixel_t:
	instance 8 byte ptr
	or 16 byte-inplace



type: any
typeid_t: typeid_t
instance:
	8byte void ptr -> value with RC
	8byte typeid_t -> global list of all used typeid_t:s.

type: enum mynode_t { one { int x; int y; }; two { string s; }; three mynode_t; }
instances:
	int tag; // 0 = one, 1 = two, 2 = three
	void* data_with_rc;






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
	};
