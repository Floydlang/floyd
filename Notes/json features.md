


# FLOYD JSON FEATURES

JSON is very central to Floyd. It is built directly into the language as the default serialized format for Floyd values. If can be used for custom file format and protocol and to interface with other JSON-based systems. JSON format is also used by the compiler and language, logging and debugging features.


- Floyd has built in support for JSON in the language. It has a a json-type called "json_value" and functions to pack & unpack strings / json files into the json-type.

- Floyd has support for json literals: you can put json data directly into a Floyd file. Great for copy-pasting snippets for tests etc.

- Floyd uses JSON to store its internal AST. There are functions to convert Floyd code to a json and back. You can also compile the ast and run it as a function. This allows you to generate code.


json_value:

	json_value -- an immutable value containing a json file. You can query it for its contents and alter it (you get new values).

a = json(["hello": 1, "bye": 3 ])


Notice that json_value can contain an entire huge json file, with a big tree of json objects and arrays etc. A json_value can also also contain just a string or a number or a single json array of strings. The json_value is used for every node in the json_value tree.


Example json:

	test_json1 = json_value({ "pigcount": 3, "pigcolor": "pink" })

What's really stored is:

	json_value	//	type is object, expressed as a floyd dictionary, [string:json_value]
		[
			"pigcount": json_value(3),
			"pigcolor": json_value("pink")
		]


json_value can contain one of 6 different types and you can query like this:

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
	



# EMBED JSON LITERALS

Embedd json inside source code file. Simple / no escaping needed. Use to paste data, test values etc. Round trip.

Since the JSON code is not a string literal but actual Floyd syntax, there are not problems with escaping strings etc. The Floyd parser will create floyd strings, dictionaries and so on for the JSON data. Then it will create a json_value from that data. This will validate that this indeed is correct JSON data or an exception is thrown (TBD).

This all means you can write Floyd code that generate all or parts of the JSON.
Also: you can nest JSONs in eachother.


	test_json1 = json_value({ "pigcount": 3, "pigcolor": "pink" })


	
	test_json2 = json_value(
		{
			"one": 1,
			"two": 2,
			"three": "three",
			"four": [ 1, 2, 3, 4 ],
			"five": { "alpha": 1000, "beta": 2000 },
			"six": true,
			"seven": false,
		}
	)
	


# JSON AND FLOYD TYPES

Every single Floyd type can be converted to/from JSON, as a built-in behaviour. This is great for:

1) Logging & debugging: you can always see the contents of any floyd value.

2) You can copy-paste things directly from a log into you source code or a data file to keep it / use it.

3) Simple to convert JSON to/from protocolls like REST protocolls.

4) Simple to load/save state to files.

5) Creating test data.




# SERIALIZATION / DESERIALIZATION

Serializing a value is a built in mechanism. It is always deep. The result is always a normalized JSON text file in a Floyd string.

There are two steps: converting your Floyd value to a json-value. Then converting the json-value to a text file.

	string json_to_textfile(json v)
	json textfile_to_json(string s)

	T json_to_value(json, typeid)
	json value_to_json(T value)





## json keyword
This keyword lets you put plain JSON scripts directly into your source code without any need to escape the string. The result is a string value, not any special data type. The json format is validated.
??? how to generate json data / inject variables etc? Maybe better to just have raw-string support?

	json { "age": 10, "last_name": "zoom" };
	json [ "one", "two", "three" ];
	json "one";
	json {
		"age": 10,
		"last_name": "zoom"
	};

	assert(json { "age": 10, "last_name": "zoom" } == "{ \"age\": 10, \"last_name\": \"zoom\" }");	
There are built in features to pack and unpack the JSON data:

	my_json_string = json[
		"+",
		["load",["res_member", ["res_var", "p"], "s"]],
		["load", ["res_var", "a"]]
	];

	my_obj = unpack_json(my_json_string);
	assert(my_obj[0] == "+");
	assert(my_obj[2][0] == "load");

The returned value from unpack_json() is of type *dyn<string, number, vec<dyn>, dict<string,dyn>,bool>* and can hold any value returned.

If you are reading data you can do this:

	string expect_string_load = my_obj[0][2];
	// This throws exception if the value cannot be found or is not of type string = very convenient when parsing files.

	//	Here you get a default value if you cannot read the string.
	string except_string_load2 = try{ my_obj[0][2], "default_string" };


JSON AST -> script file convertion
No preprocessor in floyd - use AST / JSON



# JSON / SERIALIZATION

JavaScript Object Notation (JSON) Pointer: https://tools.ietf.org/html/rfc6901

c++ library -- cool: https://github.com/nlohmann/json


print(a.red)
print(a)	— round trip: JSON? ??? deep vs shallow?
	red: 100
	green: 200
	blue: 210
	alpha: 255

json(a) -- output:
	{
		“red”: 100,
		“green”: 200,
		“blue”:210,
		“alpha”: 255
	}

c = json(
	“{
		“red”: 100,
		“green”: 200,
		“blue”:210,
		“alpha”: 255
	}”


	### Define format for source-as-JSON roundtrip. Normalized source code format. Goal for JSON is easy machine transformation.
!!! JSON does not support multi-line strings.
	### Encode using JSON! Easy to copy-paste, user JSON validators etc:

	"hello[\"troll\"].kitty[10].cat" =>
	"(@load (@res_member (@lookup (@res_member (@lookup (@res_var 'hello') (@k <string>'troll')) 'kitty') (@k <int>10)) 'cat'))"


	["@res_member",
		["@lookup",
			["@res_member",
				["@lookup",
					["@res_member", "nullptr", "hello"],
					["@k", "<string>", "troll"]
				],
				"kitty"
			],
			["@k", "<int>", "10"]
		],
		"cat"
	]








# AST - GENERATE & MANIPULATE FLOYD CODE

The encoding of a Floyd program into a AST (abstrac syntax tree) is fully documented. You can generate, manipulate and run these programs from within your own program.s

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
	




# COPY JSON FROM LIVE SYSTEM

??? need sandbox-version of parser than cannot execute any code in the JSON.


//  Copy JSON from docs or live system.

{
  "CustomerId": "string",
  "PartnerOrderId": "string",
  "Items": [
    {
      "Sku": "string",
      "DocumentReferenceUrl": "string",
      "Quantity": 0,
      "PartnerProductName": "string",
      "PartnerItemId": "string"
    }
  ],
  "Metadata": "string",
  "DeliveryOptionId": "string"
}


//  Add expressions that evaluates each field.
func packOrder(input) -> JSON {
let result = JSON()

  {
    "CustomerId": "string", EVAL: input.customer.findID().toString()
    "PartnerOrderId": "string",
    "Items": [
      {
        "Sku": "string",
        "DocumentReferenceUrl": "string",
        "Quantity": 0,
        "PartnerProductName": "string",
        "PartnerItemId": "string"
      }
    ],
    "Metadata": "string",
    "DeliveryOptionId": "string"
  }

}


JSON(
  {
    "CustomerId": "string",
    "PartnerOrderId": "string",
    "Items": [
      {
        "Sku": "string",
        "DocumentReferenceUrl": "string",
        "Quantity": 0,
        "PartnerProductName": "string",
        "PartnerItemId": "string"
      }
    ],
    "Metadata": "string",
    "DeliveryOptionId": "string"
  }
)





# JSON LITERALS

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




# OPEN
- Use json to layout contents of source file. List tests as JSON -- expression, expected, proves-what

- Round-trip source code -> JSON -> source code -- non-lossy
	??? Call stack and local variables and heap are also just state, just like all other values.
- Support for interning parts of data?
- Deduplicating data?
- De-duplication vs references vs equality vs diffing.
- An output stream is an abstract interface similar to a collection, where you can append values. You cannot unappend an output stream.
- Need tagged-union to support json_t??? Or use dumb solution = struct that stored every type.
	struct json_value {
		string s;
		float n;
		[string: json_value] o;
		[json_value] a;
		bool: b
		bool: null
	}
