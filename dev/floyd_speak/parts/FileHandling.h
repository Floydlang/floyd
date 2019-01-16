#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <map>




struct VRelativePath {
	std::string fRelativePath;
};

struct VAbsolutePath {
	std::string fAbsolutePath;
};




//	Path is absolute and with native path separators.
std::vector<std::uint8_t> LoadFile(const std::string& completePath);

//	Path is absolute and with native path separators.
std::string LoadTextFile(const std::string& completePath);

//	Path is absolute and with native path separators.
bool DoesEntryExist(const std::string& completePath);



///////////////////////////////////////////////////			DIRECTOR ROOTS

//??? Add help creating the directory with the correct name.
struct directories_t {

	//??? add app resource root.

	//	Directory where your executable or bundle lives.
	std::string process_dir;


	//	Current logged-in user's home directory. Ex: "/Users/marcus"
	//	User-visible files.
	std::string home_dir;

	//	Current logged-in user's documents directory. Ex: "/Users/marcus/Documents"
	//	User-visible files.
	std::string documents_dir;

	//	Current logged-in user's desktop directory. Ex: "/Users/marcus/Desktop"
	//	User-visible files.
	std::string desktop_dir;

	//	Current logged-in user's preference directory. Ex: "/Users/marcus/Library/Preferences"
	//	Notice that this points to a directory shared by many applications: store your data in a sub directory!
	//	User don't see these files.
	std::string preferences_dir;

	//	Current logged-in user's cache directory. Ex: "/Users/marcus/Library/Caches"
	//	Notice that this points to a directory shared by many applications: store your data in a sub directory!
	//	User don't see these files.
	std::string cache_dir;


	//	Temporary directory. Will be erased soon.
	//	Notice that this points to a directory shared by many applications: store your data in a sub directory!
	//	User don't see these files.
	std::string temp_dir;


	//	Current logged-in user's Application Support directory. Ex: "/Users/marcus/Library/Application Support"
	//	Notice that this points to a directory shared by many applications: store your data in a sub directory!
	//	App creates and manages on behalf of the user and can include files that contain user data.
	//	User don't see these files.
	std::string application_support;
};

directories_t GetDirectories();

struct process_info_t {
	std::string process_path;
};
process_info_t get_process_info();


///////////////////////////////////////////////////			ADVANCED


//	Path is absolute and with native path separators.
//	Will _create_ any needed directories in the save-path.
void SaveFile(const std::string& completePath, const std::uint8_t data[], std::size_t byteCount);

/*
	dir_a
	dir_b
		file_5
		dir_d
			file_4
			file_2
			dir_e
				file_1
				file_2
				file_3
	dir_c


	DeleteDeep("dir_b") will delete file_5, file_4, file_2, file_1, file_2, file_3, dir_e, dir_d, dir_b
*/
void DeleteDeep(const std::string& path);

void RenameEntry(const std::string& path, const std::string& n);

void MakeDirectoriesDeep(const std::string& nativePath);

std::string UpDir(const std::string& path);
std::pair<std::string, std::string> UpDir2(const std::string& path);


struct TFileInfo {
	bool fDirFlag;

	//	Dates are undefined unit, but can be compared.
	std::uint64_t fCreationDate;
	std::uint64_t fModificationDate;
	std::uint64_t fFileSize;
};

bool GetFileInfo(const std::string& completePath, TFileInfo& outInfo);


struct TDirEntry {
	enum EType {
		kFile	=	200,
		kDir	=	201
	};

	EType fType;

	//	Name of file or dir.
	std::string fNameOnly;

	//	Parent path.
	std::string fParent;
};

//	Never includes "." or "..".
std::vector<TDirEntry> GetDirItems(const std::string& dir);

//	Returns a entiry directory tree deeply.
//	Each TDirEntry name will be prefixed by _prefix_.
//	Contents of sub-directories will be also be prefixed by the sub-directory names.
//	All path names are relative to the input directory - not absolute to file system.
//	Never includes "." or "..".
std::vector<TDirEntry> GetDirItemsDeep(const std::string& dir);


//	Converts all forward slashes "/" to the path separator of the current operating system:
//		Windows is "\"
//		Unix is "/"
//		Mac OS 9 is ":"
std::string ToNativePath(const std::string& path);
std::string FromNativePath(const std::string& path);


std::string RemoveExtension(const std::string& s);

//	Returns "" when there is no extension.
//	Returned extension includes the leading ".".
//	Examples:
//		".wav"
//		""
//		".doc"
//		".AIFF"
std::string GetExtension(const std::string& s);


std::pair<std::string, std::string> SplitExtension(const std::string& s);


struct TPathParts {
	public: TPathParts();
	public: TPathParts(const std::string& path, const std::string& name, const std::string& extension);

	//	"/Volumes/MyHD/SomeDir/"
	std::string fPath;

	//	"MyFileName"
	std::string fName;

	//	".txt"
	std::string fExtension;
};

TPathParts SplitPath(const std::string& path);

std::vector<std::string> SplitPath2(const std::string& path);



/*
	base								relativePath
	"/users/marcus/"					"resources/hello.jpg"			"/users/marcus/resources/hello.jpg"
*/
std::string MakeAbsolutePath(const std::string& base, const std::string& relativePath);



////////////////////////////		COMMAND LINE ARGUMENTS


std::vector<std::string> args_to_vector(int argc, const char * argv[]);

//	Flags: x: means x supports parameter.
struct command_line_args_t {
	std::string command;
	std::string subcommand;

	//	Key: flag character, value: parameter or ""
	std::map<std::string, std::string> flags;


	//	Extra arguments, after the flags have been parsed.
	std::vector<std::string> extra_arguments;
};


command_line_args_t parse_command_line_args(const std::vector<std::string>& args, const std::string& flags);

/*
	git commit -m "Commit message"
		command: "git"
		subcommand: "commit"
		flags
			m: ""
		extra_arguments
			"Commit message"
*/
command_line_args_t parse_command_line_args_subcommands(const std::vector<std::string>& args, const std::string& flags);
