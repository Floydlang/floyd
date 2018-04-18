





	//////////////////////////////////////		Packblob


#if 0


function: host-function
	int function_id

	Support DYN values.
	Unpacks args to value_t


function: floyd-function
	Unpacks args to bc_pod_value_t temp[].
	Uses open_stack_frame2_nobump() / execute_instructions() / close_stack_frame()

k_construct_value
	value_t: dest value type
	[value_t]: constructor arguments.




# Packblob.
These come in some static sizes. They can hold one or serveral values. Are immutable.
They handle RC of any contained objects.
They know where every field sits.

struct packblob_8_t {
}

struct packblob_8_t {
}

struct packblob_16_t {
}

struct packblob_element_layout_ {
	typeid_t _field_type;
	bc_typeid_t _field_itype;
	int _start_pos;
	int _shift;
	int _mask;
}

struct packblob_64_t {
	uint32_t _values[16];
}


#endif

