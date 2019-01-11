#pragma once

#include <vector>
#include <string>
#include <cstdint>

struct FSRef;

struct VRelativePath {
	std::string fRelativePath;
};

struct VAbsolutePath {
	std::string fAbsolutePath;
};

/*
struct VNativeAbsolutePath {
	std::string fPath;
};

struct VVirtualAbsolutePath {
	std::string fPath;
};

//	Input is a virtual absolute path.
VVirtualAbsolutePath MakeVirtualAbsolutePath(const std::string& path);
*/

//	Path is absolute and with native path separators.
std::vector<std::uint8_t> LoadFile(const std::string& completePath);

//	Path is absolute and with native path separators.
std::string LoadTextFile(const std::string& completePath);

//	Path is absolute and with native path separators.
bool DoesFileExist(const std::string& completePath);

//	Same as GetAssetsDir, but referes to a writable directory were we can store files for next run time.
std::string GetCacheDir();

//	Works similar to GetAssetsDir, but to the users preference dir.
std::string GetPreferenceDir();

std::string GetDesktopDir();

std::string GetAppResourceDir();
std::string GetAppReadWriteDir();

std::string GetExecutableDir();

void GetTestDirs(std::string& oReadDir, std::string& oWriteDir);


std::string FromFSRef(const FSRef& fsRef);

//	Path is absolute and with native path separators.
//	Will _create_ any needed directories in the save-path.
void SaveFile(const std::string& completePath, const std::uint8_t data[], std::size_t byteCount);


void MakeDirectoriesDeep(const std::string& nativePath);

std::string UpDir(const std::string& path);


struct TFileInfo {
//	bool fDirFlag;

	//	Dates are undefined unit, but can be compared.
//	std::uint64_t fCreationDate;
	std::uint64_t fModificationDate;
	std::uint64_t fFileSize;
};

bool GetFileInfo(const std::string& completePath, TFileInfo& outInfo);


struct TDirEntry {
	enum EType {
		kFile	=	200,
		kDir	=	201
	};
	std::string fName;
	EType fType;
};


std::vector<TDirEntry> GetDirItems(const std::string& dir);

//	Returns a entiry directory tree deeply.
//	Each TDirEntry name will be prefixed by _prefix_.
//	Contents of sub-directories will be also be prefixed by the sub-directory names.
//	All path names are relative to the input directory - not absolute to file system.
std::vector<TDirEntry> GetDirItemsDeep(const std::string& dir, const std::string& prefix);


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

void RevealItemInFinder(const std::string& path);


/*
	base								relativePath
	"/users/marcus/"					"resources/hello.jpg"			"/users/marcus/resources/hello.jpg"
*/
std::string MakeAbsolutePath(const std::string& base, const std::string& relativePath);

