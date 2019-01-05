//
//  core_types.hpp
//  floyd_speak
//
//  Created by Marcus Zetterquist on 2019-01-05.
//  Copyright © 2019 Marcus Zetterquist. All rights reserved.
//

#ifndef core_types_hpp
#define core_types_hpp

#include <stdio.h>

#if 0
typedef int cpu_address_t
typedef cpu_address_t size_t
typedef int file_pos_t

typedef int64_t time_seconds_t


//	A universally unique identifier (UUID) is a 128-bit number used to identify information in computer systems. The term globally unique identifier (GUID) is also used.
struct uuid_t {
	int high64
	int low64
}

struct uri_t {
	string path
}

//	Efficent keying using 64-bit hash instead of a string. Hash can often be computed from string-key at compile time.
struct key_t {
	quick_hash_t hash
}

struct date_t {
	string utd_date;
}

struct source_code_location {
	absolute_path_t source_file
	int _line_number
}

struct sha1_t {
	string ascii40
}

struct quick_hash_t {
	int hash
}

struct relative_path_t {
	string fRelativePath
}

struct absolute_path_t {
	string fAbsolutePath
}

struct binary_t {
	string bytes
}


struct text_t {
	int id
}

struct text_resource_id {
	quick_hash_t id
}


struct image_id_t {
	int id
}

struct pixel_rgba_t {
	uint8_t red, green, blue, alpha;
}

struct image_id_t {
	float width;
	float height
	vector<pixel_rgba_t> pixels;
}

#endif


#endif /* core_types_hpp */
