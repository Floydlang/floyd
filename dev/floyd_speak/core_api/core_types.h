//
//  core_types.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-01-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef core_types_hpp
#define core_types_hpp

#include <string>
#include <vector>

typedef int cpu_address_t;
//typedef cpu_address_t size_t
typedef int file_pos_t;

typedef int64_t time_seconds_t;


//	This is used to tag message put in a green process inbox. It allows easy detection.
struct inbox_tag_t {
	std::string tag_string;
};

struct inbox_message_t {
	std::string type;
	inbox_tag_t tag;
};


//	A universally unique identifier (UUID) is a 128-bit number used to identify information in computer systems. The term globally unique identifier (GUID) is also used.
struct uuid_t {
	int high64;
	int low64;
};

struct uri_t {
	std::string path;
};

struct quick_hash_t {
	int hash;
};

//	Efficent keying using 64-bit hash instead of a string. Hash can often be computed from string-key at compile time.
struct key_t {
	quick_hash_t hash;
};

struct date_t {
	std::string utd_date;
};

struct url_t {
	std::string complete_url_path;
};


struct sha1_t {
	std::string ascii40;
};


struct relative_path_t {
	std::string fRelativePath;
};

struct absolute_path_t {
	std::string fAbsolutePath;
};

struct binary_t {
	std::string bytes;
};


struct source_code_location {
	absolute_path_t source_file;
	int _line_number;
};




struct text_t {
	int id;
};

struct text_resource_id {
	quick_hash_t id;
};


struct image_id_t {
	int id;
};

struct color_t {
	float red, green, blue, alpha;
};

struct vector2_t {
	float x, y;
};


#endif /* core_types_hpp */
