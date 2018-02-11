
# OPEN
- IDEA: Use json to layout contents of source file. List tests as JSON -- expression, expected, proves-what
- IDEA: Round-trip source code -> JSON -> source code -- non-lossy
	??? Call stack and local variables and heap are also just state, just like all other values.
- IDEA: Support for interning parts of data? Deduplicating data? RAM and stringed.



### FLOYD TYPES VS JSON_VALUE

Every single Floyd type can be converted to/from JSON, as a built-in behaviour. This is great for:

1) Logging & debugging: you can always see the contents of any floyd value.

2) You can copy-paste things directly from a log into you source code or a data file to keep it / use it.

3) Simple to convert JSON to/from protocols like REST protocolls.

4) Simple to load/save state to files.

5) Creating test data.



STATES OF DATA

A) A Floyd value - can be composite, contain lots of custom structs. Static typing.
B) json_value -- value is broken down into the 7 JSON types. Dynamic typing.
C) json_value2 -- each value is also tagged with its Floyd-type.
D) String format of json_value -- a sequence of 


??? complex json value:

- Internal RAM format: value_t
- JSON object for tools, logging etc. "normalized json"
- JSON object for persistance


A) Store typeid_t along with value. Lots of duplicates.

B) Alternative: store all typeid_t separately, use sha1 or names to reference them.

C) Expand every custom type directly into its instance.








[
	{
		"def": [
			[
				{ "name": "x", 	"type": "float" },
				{ "name": "y", 	"type": "float" }
			]
		],
		"member_values": [ 100, 200 ]
	},
	{
		"def": [
			[
				{ "name": "x", "type": "float" 	},
				{ "name": "y", "type": "float" 	}
			]
		],
		"member_values": [ 101, 201 ]
	}
]










### QUERRYING AND MODIFYING JSON_VALUEs

A json_value can contain different type of va

json_value can contain one of 6 different types and you can query like this:


??? TBD
	bool is_string(json_value v)
	bool is_number(json_value v)
	bool is_object(json_value v)
	bool is_array(json_value v)
	bool is_bool(json_value v)
	bool is_null(json_value v)
	
	string get_string(json_value v)
	float get_number(json_value v)
	[string: json_value] get_object(json_value v)
	[json_value] get_array(json_value v)
	string get_bool(json_value v)



### SERIALIZATION / DESERIALIZATION


	T json_to_value(json, typeid)
	json value_to_json(T value)





# EXAMPLE USE OF JSON


##### COPY - PASTE VALUE DIRECTLY INTO YOU TEST / SOURCE CODE

Support pasting JSON directly into source file, including any escapes, UTF-8 etc.

cat = unpack_json(unpack_base64(
	"UmVwdWJsaWthbmVybmFzIGxlZGFyZSBpIHNlbmF0ZW4sIE1pdGNoIE1jQ29ubmVsbCwgdmlsbGUg
	dGlkaWdhcmVs5GdnYSBvbXL2c3RuaW5nZW4gdGlsbCBzZW50IHDlIHP2bmRhZ3NrduRsbGVuLCBt
	ZW4gZGV0IHNhdHRlIGRlbW9rcmF0aXNrZSBtaW5vcml0ZXRzbGVkYXJlbiBDaHVjayBTY2h1bWVy
	IHN0b3BwIGb2ci4gU2NodW1lciBz5GdlciBhdHQgdHJvdHMgZvZyaGFuZGxpbmdhcm5hIJRoYXIg
	dmkg5G5udSBhdHQgbuUgZW4g9nZlcmVuc2tvbW1lbHNlIHDlIGVuIHbkZyBzb20g5HIgYWNjZXB0
	YWJlbCBm9nIgYuVkYSBzaWRvcpQuIJRIYW4ga2FsbGFyIGzkZ2V0IGb2ciBUcnVtcCBzaHV0ZG93
	bpQgb2NoIFZpdGEgaHVzZXQga2FsbGFyIGRldCCUU2NodW1lciBzaHV0ZG93bpQuDQoNCg=="
))


##### Example how to add support for a JSON message:

1) Copy example JSON from API docs into your source code.
2) Make your code use that hardcoded JSON and make sure it works.
3) Replace bits of the fixed JSON with expression, step by step.


??? need sandbox-version of parser than cannot execute any code in the JSON.

//  Copy JSON from docs or live system.



//	Uses default JSON representation of image_t
dog = unpack_json(
	image_t,
	{
		"width": 4, "height": 3, "pixels": [
			{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},
			{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},
			{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},
			{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0},{"red": 0, "green": 0, "blue:" 0}, {"red": 0, "green": 0, "blue:" 0}
		]
	}
)

# GENERATE & MANIPULATE FLOYD CODE

- Floyd uses JSON to store its internal AST. There are functions to convert Floyd code to a json and back. You can also compile the ast and run it as a function. This allows you to generate code.


	- Define format for source-as-JSON roundtrip.
	- Normalized source code format.
	- Goal for JSON is easy machine transformation.

The encoding of a Floyd program into a AST (abstrac tsyntax tree) is fully documented. You can generate, manipulate and run these programs from within your own programs as part of their execution, or use this to make tools.

The tree is stored as standard JSON data. Each expression is a JSON-array where elements have defined meaning. You can nest expressions to any depth by specifying an expression instead of a constant.

### STATEMENTS

	[ "bind", identifier-string, expr ]
	[ "assign", identifier-string, expr ]
	[ "block", identifier-string, expr ]
	[ "defstruct", identifier-string, expr ]
	[ "deffunc", identifier-string, expr ]
	[ "return", expr ]
	[ "if", condition_expr, then_body, else_body ]
	[ "for", "open-range"/"close-range", iterator_name, start_expr, end_expr, body ]
	[ "while", condition_expr, body ]
	[ "expression-statement", expr ]


### EXPRESSIONS

This is how all expressions are encoded into JSON format. "expr" is a placehold for another expression array. 

	[ "k", expr ]
	
	[ "unary_minus", expr ]
	[ "vector-def", element_type, [element_expr1, element_exprt2... ]]
	[ "dict-def", element_type, [element_expr1, element_exprt2... ]]
	
	[ "+", left_expr, right_expr ]
	[ "-", left_expr, right_expr ]
	[ "*", left_expr, right_expr ]
	[ "/", left_expr, right_expr ]
	[ "%", left_expr, right_expr ]
	
	[ "<", left_expr, right_expr ]
	[ "<=", left_expr, right_expr ]
	[ ">", left_expr, right_expr ]
	[ ">=", left_expr, right_expr ]
	
	[ "||", left_expr, right_expr ]
	[ "&&", left_expr, right_expr ]
	[ "==", left_expr, right_expr ]
	[ "!=", left_expr, right_expr ]
	
	[ "?:", conditional_expr, a_expr, b_expr ]
	
	[ "call", function_exp, [ arg_expr ] ]
	
	Resolve free variable using static scope
	[ "@", string_name ]
	
	Resolve member
	[ "->", object_id_expr, string_member_name ]
	
	Lookup member
	[ "[]", object_id_expr, key_expr ]


f = compile_function(json_value return_type, [json_value] args, json_value statements_array)

json_value decompile_function(f)



# NORMALIZED JSON

??? "Normalized json" is already used for another concept. Find new name,

The exact same JSON can be encoded as different text strings. This makes it impossible to compare them or make hashes of them etc.

Floyd has a concept of normalized JSON, which means that a JSON containing a set of data has only a single, normalized character layout.

- Remove all whitespace
- Sort all object keys in ascii-order


# REFERENCES

hjson - http://hjson.org/
toml - https://github.com/toml-lang/toml
JSONY - https://metacpan.org/pod/JSONY
HOCON - https://github.com/lightbend/config/blob/master/HOCON.md
EDN - https://github.com/edn-format/edn (clojure)
CSON - https://github.com/bevry/cson

Example JSON: http://json.org/example.html

See "JSON pointers" spec.


# BASE64
base64(string string_as_base64)

cat = unpack_json(unpack_base64(
	"UmVwdWJsaWthbmVybmFzIGxlZGFyZSBpIHNlbmF0ZW4sIE1pdGNoIE1jQ29ubmVsbCwgdmlsbGUg
	dGlkaWdhcmVs5GdnYSBvbXL2c3RuaW5nZW4gdGlsbCBzZW50IHDlIHP2bmRhZ3NrduRsbGVuLCBt
	ZW4gZGV0IHNhdHRlIGRlbW9rcmF0aXNrZSBtaW5vcml0ZXRzbGVkYXJlbiBDaHVjayBTY2h1bWVy
	IHN0b3BwIGb2ci4gU2NodW1lciBz5GdlciBhdHQgdHJvdHMgZvZyaGFuZGxpbmdhcm5hIJRoYXIg
	dmkg5G5udSBhdHQgbuUgZW4g9nZlcmVuc2tvbW1lbHNlIHDlIGVuIHbkZyBzb20g5HIgYWNjZXB0
	YWJlbCBm9nIgYuVkYSBzaWRvcpQuIJRIYW4ga2FsbGFyIGzkZ2V0IGb2ciBUcnVtcCBzaHV0ZG93
	bpQgb2NoIFZpdGEgaHVzZXQga2FsbGFyIGRldCCUU2NodW1lciBzaHV0ZG93bpQuDQoNCg=="
))

