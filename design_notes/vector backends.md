
- Read-cache for complex vector trees.
- Write-cache mapped to one 32-element section of vector. Also appending.


VECTOR BACKENDS:

Vector is a 64 bit word where we stuff type and data when possible.


- Memory usage, cache usage.
- Inline stuff when possible
- Fast mutation
- Compact memory usage
- Fast iteration continous range.
- Fast lookup, random access



Zero elements = store inline


//	64bit struct = 8 bytes
struct vector__bool__inline {
	uint32_t values;	//	32 bits
	uint8_t count;		//	0 - 31
	uint8_t pada;
	uint8_t padb;
	uint8_t 7_type; 	//	#10 = bool_vector_simple_max_32
}

0xffffffff 1f 00 0a		==	[
	true, true, true, true, true, true, true, true,
	true, true, true, true, true, true, true, true, 
	true, true, true, true, true, true, true, true, 
	true, true, true, true, true, true, true, true
]

0x00000003 0b 00 0a		==	[
	false, false, false, false, false, false, false, false,
	true, true, true
]

Note: x86_64 uses 48 bits of pointers. Low bits are zero for aligned to 8 bytes.
Note: x86_64 has 64 byte cache lines. core i7. L1, L2, L3
		64 bytes = 8 x 8 bytes.
https://stackoverflow.com/questions/14707803/line-size-of-l1-and-l2-caches


//	----------------------------------- -----------------------------------
//	66665555 55555544 44444444 33333333 33222222 22221111 11111100 00000000
//	32109876 54321098 76543210 98765432 10987654 32109876 54321098 76543210

//	XXXXXXXX XXXXXXXX PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPppp		48bit Intel x86_64 pointer. ppp = low bits, set to 0, X = bit 47
//	-----------------------------------------------------------------------


# vector__any_value__simple_heap_array__zero_optimization

	aaattttt ssssssss PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPZZZ
	00000001 ssssssss PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPP000

a = #%000 = vector
t = vector backend = #1
s = count 0-254. 255 = count >= 255, check extended block
P = 48 bit pointer / index to extended block
Z = 3 bit unused
If count == 0, allocate no extended block.

extended block:
	64 bit: reference count
	64 bit: element count
	element 0
	element 1
	element 2
	...



# vector__any_value__inline_only
Can store only as many elements as fits inside the 64 bits.

	aaattttt ssssssss PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPZZZ
	00000010 00------ EEEEEEEE EEEEEEEE EEEEEEEE EEEEEEEE EEEEEEEE EEEEEEEE

a = #%000 = vector

t = vector backend = #2

s = count 0-48
E = 48 bits of element-data.

1 x 48 bits
3 x 16 bits
48 x 1 bit
6 x 8 bits -- use for short strings.
8 x 7 bits -- use for short strings.



??? store element-type & element-size!


# vector__any_value__hamt
	aaattttt ssssssss PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPPPP PPPPPZZZ
	00000011 00------ EEEEEEEE EEEEEEEE EEEEEEEE EEEEEEEE EEEEEEEE EEEEEEEE

a = #%000 = vector
t = vector backend = #3
s = count 0-254. 255 = count >= 255, check extended block
P = 48 bit pointer / index to extended block
Z = 3 bit unused
If count == 0, allocate no extended block.

extended block: 16 bytes
	32 bit: reference count
	32 bit: element count
	64 bit pointer to HAMT root.

HAMT block:
	32 pointers = 32 x 48 = 24 int64


	struct hamt_node_260bytes_t {
		void* node_ptrs[32]
		uint32_t rc;
	}
	struct hamt_node_224bytes_t {
		//	32 pointers, 48 bits each
		uint64_t pointers[24];
		uint32_t rc;
	}

Leafx32 node: if element is <=8 bytes, store it directly in node, else allocate it on heap.



??? no need to store all 32 pointers in the last leaf node.






struct vector__any_value__simple_heap_array {
	uint64_t packed
	value_t* elements;
	size_t count
	uint8_t 7_type; 	//	#10 = bool_vector_simple_max_32
}

struct vector__value__simple_inline_array {
	uint8_t inline_data;
	uint8_t inline_data;
	uint8_t inline_data;
	uint8_t inline_data;
	uint8_t inline_data;
	uint8_t inline_data;
	size_t count;
	uint8_t 7_type; 	//	#10 = bool_vector_simple_max_32
}

struct vector__value__simple_inline_array {
	value_t* elements;
	size_t count
}


struct vector__value__two_range_array {
	value_t* elements;
	size_t count
}