#include "file_handling.h"

#include "quark.h"
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#ifndef _MSC_VER
	#include <unistd.h>
#else
	#include <direct.h>
#endif
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "text_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#if QUARK_MAC
	#include <libproc.h>
	#include <CoreFoundation/CoreFoundation.h>
#endif

#include <dirent.h>    /* struct DIR, struct dirent, opendir().. */


#include <iostream>
#include <fstream>



#define TRACE_INDENT QUARK_SCOPED_TRACE
#define TRACE QUARK_TRACE
#define ASSERT QUARK_ASSERT





#if 0
From "errno.h"

/*
 * Error codes
 */

#define	EPERM		1		/* Operation not permitted */
#define	ENOENT		2		/* No such file or directory */
#define	ESRCH		3		/* No such process */
#define	EINTR		4		/* Interrupted system call */
#define	EIO		5		/* Input/output error */
#define	ENXIO		6		/* Device not configured */
#define	E2BIG		7		/* Argument list too long */
#define	ENOEXEC		8		/* Exec format error */
#define	EBADF		9		/* Bad file descriptor */
#define	ECHILD		10		/* No child processes */
#define	EDEADLK		11		/* Resource deadlock avoided */
					/* 11 was EAGAIN */
#define	ENOMEM		12		/* Cannot allocate memory */
#define	EACCES		13		/* Permission denied */
#define	EFAULT		14		/* Bad address */
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define	ENOTBLK		15		/* Block device required */
#endif
#define	EBUSY		16		/* Device / Resource busy */
#define	EEXIST		17		/* File exists */
#define	EXDEV		18		/* Cross-device link */
#define	ENODEV		19		/* Operation not supported by device */
#define	ENOTDIR		20		/* Not a directory */
#define	EISDIR		21		/* Is a directory */
#define	EINVAL		22		/* Invalid argument */
#define	ENFILE		23		/* Too many open files in system */
#define	EMFILE		24		/* Too many open files */
#define	ENOTTY		25		/* Inappropriate ioctl for device */
#define	ETXTBSY		26		/* Text file busy */
#define	EFBIG		27		/* File too large */
#define	ENOSPC		28		/* No space left on device */
#define	ESPIPE		29		/* Illegal seek */
#define	EROFS		30		/* Read-only file system */
#define	EMLINK		31		/* Too many links */
#define	EPIPE		32		/* Broken pipe */

/* math software */
#define	EDOM		33		/* Numerical argument out of domain */
#define	ERANGE		34		/* Result too large */

/* non-blocking and interrupt i/o */
#define	EAGAIN		35		/* Resource temporarily unavailable */
#define	EWOULDBLOCK	EAGAIN		/* Operation would block */
#define	EINPROGRESS	36		/* Operation now in progress */
#define	EALREADY	37		/* Operation already in progress */

/* ipc/network software -- argument errors */
#define	ENOTSOCK	38		/* Socket operation on non-socket */
#define	EDESTADDRREQ	39		/* Destination address required */
#define	EMSGSIZE	40		/* Message too long */
#define	EPROTOTYPE	41		/* Protocol wrong type for socket */
#define	ENOPROTOOPT	42		/* Protocol not available */
#define	EPROTONOSUPPORT	43		/* Protocol not supported */
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define	ESOCKTNOSUPPORT	44		/* Socket type not supported */
#endif
#define ENOTSUP		45		/* Operation not supported */
#if !__DARWIN_UNIX03 && !defined(KERNEL)
/*
 * This is the same for binary and source copmpatability, unless compiling
 * the kernel itself, or compiling __DARWIN_UNIX03; if compiling for the
 * kernel, the correct value will be returned.  If compiling non-POSIX
 * source, the kernel return value will be converted by a stub in libc, and
 * if compiling source with __DARWIN_UNIX03, the conversion in libc is not
 * done, and the caller gets the expected (discrete) value.
 */
#define	EOPNOTSUPP	 ENOTSUP	/* Operation not supported on socket */
#endif /* !__DARWIN_UNIX03 && !KERNEL */

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define	EPFNOSUPPORT	46		/* Protocol family not supported */
#endif
#define	EAFNOSUPPORT	47		/* Address family not supported by protocol family */
#define	EADDRINUSE	48		/* Address already in use */
#define	EADDRNOTAVAIL	49		/* Can't assign requested address */

/* ipc/network software -- operational errors */
#define	ENETDOWN	50		/* Network is down */
#define	ENETUNREACH	51		/* Network is unreachable */
#define	ENETRESET	52		/* Network dropped connection on reset */
#define	ECONNABORTED	53		/* Software caused connection abort */
#define	ECONNRESET	54		/* Connection reset by peer */
#define	ENOBUFS		55		/* No buffer space available */
#define	EISCONN		56		/* Socket is already connected */
#define	ENOTCONN	57		/* Socket is not connected */
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define	ESHUTDOWN	58		/* Can't send after socket shutdown */
#define	ETOOMANYREFS	59		/* Too many references: can't splice */
#endif
#define	ETIMEDOUT	60		/* Operation timed out */
#define	ECONNREFUSED	61		/* Connection refused */

#define	ELOOP		62		/* Too many levels of symbolic links */
#define	ENAMETOOLONG	63		/* File name too long */

/* should be rearranged */
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define	EHOSTDOWN	64		/* Host is down */
#endif
#define	EHOSTUNREACH	65		/* No route to host */
#define	ENOTEMPTY	66		/* Directory not empty */

/* quotas & mush */
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define	EPROCLIM	67		/* Too many processes */
#define	EUSERS		68		/* Too many users */
#endif
#define	EDQUOT		69		/* Disc quota exceeded */

/* Network File System */
#define	ESTALE		70		/* Stale NFS file handle */
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define	EREMOTE		71		/* Too many levels of remote in path */
#define	EBADRPC		72		/* RPC struct is bad */
#define	ERPCMISMATCH	73		/* RPC version wrong */
#define	EPROGUNAVAIL	74		/* RPC prog. not avail */
#define	EPROGMISMATCH	75		/* Program version wrong */
#define	EPROCUNAVAIL	76		/* Bad procedure for program */
#endif

#define	ENOLCK		77		/* No locks available */
#define	ENOSYS		78		/* Function not implemented */

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define	EFTYPE		79		/* Inappropriate file type or format */
#define	EAUTH		80		/* Authentication error */
#define	ENEEDAUTH	81		/* Need authenticator */

/* Intelligent device errors */
#define	EPWROFF		82	/* Device power is off */
#define	EDEVERR		83	/* Device error, e.g. paper out */
#endif

#define	EOVERFLOW	84		/* Value too large to be stored in data type */

/* Program loading errors */
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define EBADEXEC	85	/* Bad executable */
#define EBADARCH	86	/* Bad CPU type in executable */
#define ESHLIBVERS	87	/* Shared library version mismatch */
#define EBADMACHO	88	/* Malformed Macho file */
#endif

#define	ECANCELED	89		/* Operation canceled */

#define EIDRM		90		/* Identifier removed */
#define ENOMSG		91		/* No message of desired type */
#define EILSEQ		92		/* Illegal byte sequence */
#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define ENOATTR		93		/* Attribute not found */
#endif

#define EBADMSG		94		/* Bad message */
#define EMULTIHOP	95		/* Reserved */
#define	ENODATA		96		/* No message available on STREAM */
#define ENOLINK		97		/* Reserved */
#define ENOSR		98		/* No STREAM resources */
#define ENOSTR		99		/* Not a STREAM */
#define	EPROTO		100		/* Protocol error */
#define ETIME		101		/* STREAM ioctl timeout */

#if __DARWIN_UNIX03 || defined(KERNEL)
/* This value is only discrete when compiling __DARWIN_UNIX03, or KERNEL */
#define	EOPNOTSUPP	102		/* Operation not supported on socket */
#endif /* __DARWIN_UNIX03 || KERNEL */

#define ENOPOLICY	103		/* No such policy registered */

#if __DARWIN_C_LEVEL >= 200809L
#define ENOTRECOVERABLE 104		/* State not recoverable */
#define EOWNERDEAD      105		/* Previous owner died */
#endif

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL
#define	EQFULL		106		/* Interface output queue is full */
#define	ELAST		106		/* Must be equal largest errno */
#endif


#endif



int get_error(){
	return errno;
}



std::string read_text_file(const std::string& abs_path){
	std::string file_contents;

	std::ifstream f (abs_path);
	if (f.is_open() == false){
		quark::throw_runtime_error(std::string() + "Cannot read text file " + abs_path);
	}
	std::string line;
	while ( getline(f, line) ) {
		file_contents.append(line + "\n");
	}
	f.close();

	return file_contents;
}




///////////////////////////////////////////////////			DIRECTOR ROOTS



std::string get_env(const std::string& s){
	const char* value = getenv(s.c_str());
	QUARK_ASSERT(value != nullptr);
	if(value == nullptr){
		quark::throw_exception();
	}
	return std::string(value);
}

//??? Uses getenv() which may not work with Sandboxes.
//??? Hardcoded subpaths to Preferences etc.
directories_t GetDirectories(){
	const std::string home_dir = get_env("HOME");
	const std::string temp_dir = get_env("TMPDIR");

	directories_t result;
	result.process_dir = get_process_info().process_path;
	result.home_dir = home_dir;
	result.documents_dir = home_dir + "/Documents";
	result.desktop_dir = home_dir + "/Desktop";
	result.preferences_dir = home_dir + "/Library/Preferences";
	result.cache_dir = home_dir + "/Library/Caches";
	result.temp_dir = temp_dir;
	result.application_support = home_dir + "/Library/Application Support";
	return result;
}


QUARK_TEST("", "GetDirectories()", "", ""){
	const auto temp = GetDirectories();

	QUARK_VERIFY(true)
}


#if QUARK_MAC

std::string get_process_path (int process_id){
	pid_t pid; int ret;
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

	pid = (pid_t) process_id;
	ret = proc_pidpath (pid, pathbuf, sizeof(pathbuf));
	if ( ret <= 0 ) {
		fprintf(stderr, "PID %d: proc_pidpath ();\n", pid);
		fprintf(stderr, "	%s\n", strerror(get_error()));
		quark::throw_exception();
	} else {
//		printf("proc %d: %s\n", pid, pathbuf);
		return std::string(pathbuf);
	}
}

#elif QUARK_WINDOWS

std::string get_process_path (int process_id){
	return "???";
}

#elid QUARK_LINUX

std::string get_process_path (int process_id){
	pid_t pid; int ret;
	char pathbuf[512]; /* /proc/<pid>/exe */
	snprintf(pathbuf, sizeof(pathbuf), "/proc/%i/exe", pid);
	return std::string(pathbuf);
}

#endif


/*
std::string MacBundlePath()
{
    char path[1024];
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if(!mainBundle)
        return "";

    CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
    if(!mainBundleURL)
        return "";

    CFStringRef cfStringRef = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);
    if(!cfStringRef)
        return "";

    CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);

    CFRelease(mainBundleURL);
    CFRelease(cfStringRef);

    return std::string(path);
}
*/


#if QUARK_MAC
//??? fix leaks.
process_info_t get_process_info(){
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	if(mainBundle == nullptr){
		quark::throw_exception();
	}

    CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
	if(mainBundleURL == nullptr){
		quark::throw_exception();
	}

    CFStringRef cfStringRef = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);
	if(cfStringRef == nullptr){
		quark::throw_exception();
	}

    char path[1024];
    CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);

    CFRelease(mainBundleURL);
    CFRelease(cfStringRef);

	return process_info_t{
		std::string(path) + "/"
	};
}

#else

process_info_t get_process_info(){
	return process_info_t{
		std::string("path") + "/"
	};
}

#endif

QUARK_TEST("", "get_info()", "", ""){
	const auto temp = get_process_info();

	TFileInfo info;
	bool ok = GetFileInfo(temp.process_path, info);
	QUARK_VERIFY(ok);

	QUARK_VERIFY(true);
}
















std::string UpDir(const std::string& path){
	std::string res = path;
	std::string::size_type pos = path.rfind('/', std::string::npos);
	if(pos != std::string::npos){
		res.erase(pos, std::string::npos);
	}
	return res;
}

std::pair<std::string, std::string> UpDir2(const std::string& path){
//	ASSERT(path.empty() || path.back() == '/');

	if(path == "/"){
		return { "", "/" };
	}
	else if(path == ""){
		return { "", "" };
	}
	else if(path.back() == '/'){
		const auto r = UpDir2(path.substr(0, path.size() - 1));
		return r;
	}
	else{
		const auto pos = path.rfind('/');
		if(pos != std::string::npos){
			const auto left = path.substr(0, pos + 1);
			const auto right = path.substr(pos + 1);
			return { left, right };
		}
		else{
			return { "", path };
		}
	}
}

QUARK_TEST("", "UpDir2()","", ""){
	QUARK_VERIFY((UpDir2("/Users/marcus/Desktop/") == std::pair<std::string, std::string>{ "/Users/marcus/", "Desktop" }));
}
QUARK_TEST("", "UpDir2()","", ""){
	QUARK_VERIFY((UpDir2("/Users/") == std::pair<std::string, std::string>{ "/", "Users" }));
}
QUARK_TEST("", "UpDir2()","", ""){
	QUARK_VERIFY((UpDir2("/") == std::pair<std::string, std::string>{ "", "/" }));
}
QUARK_TEST("", "UpDir2()","", ""){
	QUARK_VERIFY((UpDir2("/Users/marcus/original.txt") == std::pair<std::string, std::string>{ "/Users/marcus/", "original.txt" }));
}



std::string RemoveExtension(const std::string& s){
	return SplitExtension(s).first;
}

std::string GetExtension(const std::string& s){
	return SplitExtension(s).second;
}

std::pair<std::string, std::string> SplitExtension(const std::string& s){
	const auto size = s.size();
	auto pos = size;
	while(pos > 0 && s[pos - 1] != '.'){
		pos--;
	}

	//	Found "."?
	if(pos > 0){
		const auto name = std::string(&s[0], &s[pos - 1]);
		const auto extension = std::string(&s[pos - 1], &s[size]);
		return { name, extension };
	}

	//	No ".".
	else{
		return { s, "" };
	}
}

QUARK_TEST("", "SplitExtension()","", ""){
	QUARK_VERIFY((SplitExtension("snare.wav") == std::pair<std::string, std::string>{ "snare", ".wav" }));
}
QUARK_TEST("", "SplitExtension()","", ""){
	QUARK_VERIFY((SplitExtension("snare.drum.wav") == std::pair<std::string, std::string>{ "snare.drum", ".wav" }));
}
QUARK_TEST("", "SplitExtension()","", ""){
	QUARK_VERIFY((SplitExtension("snare") == std::pair<std::string, std::string>{ "snare", "" }));
}
QUARK_TEST("", "SplitExtension()","", ""){
	QUARK_VERIFY((SplitExtension(".wav") == std::pair<std::string, std::string>{ "", ".wav" }));
}


TPathParts::TPathParts(){
}

TPathParts::TPathParts(const std::string& path, const std::string& name, const std::string& extension) :
	fPath(path),
	fName(name),
	fExtension(extension)
{
}

//	/Volumes/MyHD/SomeDir/MyFileName.txt
TPathParts SplitPath(const std::string& inPath){
	TPathParts temp;

	std::string tempStr=inPath;

	//	Extension, if any.
	{
		std::string::size_type pos=tempStr.find_last_of("./",std::string::npos);
		if(pos !=std::string::npos){
			if(tempStr[pos]=='.'){
				temp.fExtension=std::string(&tempStr[pos],&tempStr[tempStr.size()]);
				tempStr=std::string(&tempStr[0],&tempStr[pos]);
				tempStr=std::string(tempStr,0,pos);
			}
		}
	}

	//	Now tempStr is stripped of any extension.

	//	Path, if any.
	{
		std::string::size_type pos2=tempStr.find_last_of("/",std::string::npos);
		if(pos2 !=std::string::npos){
			temp.fPath=std::string(tempStr,0,pos2 + 1);
			tempStr=std::string(tempStr,pos2  + 1,std::string::npos);
		}
	}

	//	Name.
	temp.fName=tempStr;
	return temp;
}

static void TestSplitPath(const std::string& inPath, const TPathParts& correctParts){
	TRACE_INDENT("TestSplitPath");
	TRACE("path='" + inPath + "'");
	TPathParts result = SplitPath(inPath);
	TRACE("fPath='" + result.fPath + "'");
	TRACE("fName='" + result.fName + "'");
	TRACE("fExtension='" + result.fExtension + "'");

	ASSERT(result.fPath == correctParts.fPath);
	ASSERT(result.fName == correctParts.fName);
	ASSERT(result.fExtension == correctParts.fExtension);
}

QUARK_TEST("", "TestSplitPath","", ""){
	//	Complex.
	TestSplitPath("/Volumes/MyHD/SomeDir/MyFileName.txt", TPathParts("/Volumes/MyHD/SomeDir/","MyFileName",".txt"));
}

QUARK_TEST("", "TestSplitPath","", ""){
	//	Name + extension.
	TestSplitPath("MyFileName.txt", TPathParts("","MyFileName",".txt"));
}

QUARK_TEST("", "TestSplitPath","", ""){
	//	Name
	TestSplitPath("MyFileName", TPathParts("","MyFileName",""));
}

QUARK_TEST("", "TestSplitPath","", ""){
	//	Path + name
	TestSplitPath("Dir/MyFileName", TPathParts("Dir/","MyFileName",""));
}

QUARK_TEST("", "TestSplitPath","", ""){
	//	Path + extension.
	TestSplitPath("/Volumes/MyHD/SomeDir/.txt", TPathParts("/Volumes/MyHD/SomeDir/","",".txt"));
}

QUARK_TEST("", "TestSplitPath","", ""){
	TestSplitPath("/Volumes/MyHD/SomeDir/", TPathParts("/Volumes/MyHD/SomeDir/","",""));
}




std::vector<std::uint8_t> LoadFile(const std::string& completePath){
	TRACE_INDENT("LoadFile(" + completePath + ")");

	FILE* f = 0;
	try {
		f = std::fopen(completePath.c_str(), "rb");
		if(f == 0){
			quark::throw_exception();
		}

		int fseekReturn = std::fseek(f, 0, SEEK_END);
		if (fseekReturn != 0) {
			quark::throw_exception();
		}

		long fileSize = std::ftell(f);
		if (fileSize < 0) {
			quark::throw_exception();
		}

		fseekReturn = std::fseek(f, 0, SEEK_SET);
		if (fseekReturn != 0) {
			quark::throw_exception();
		}

		std::vector<std::uint8_t> data(fileSize);
		size_t freadReturn = std::fread(&data[0], 1, fileSize, f);
		if (freadReturn != fileSize) {
			quark::throw_exception();
		}

		TRACE("File size: " + std::to_string(fileSize));

		std::fclose(f);
		f = 0;

		return data;
	}
	catch(...){
		if (f != 0) {
			std::fclose(f);
			f = 0;
		}
		throw;
	}
	return std::vector<std::uint8_t>();
}

bool DoesEntryExist(const std::string& completePath){
	FILE* f = nullptr;
	f = std::fopen(completePath.c_str(), "rb");
	if(f == 0){
		return false;
	}
	else{
		std::fclose(f);
		return true;
	}
}

// http://en.wikipedia.org/wiki/Unix_epoch
//	http://www.scit.wlv.ac.uk/cgi-bin/mansec?2+stat
//	lstat()
//	stat()
bool GetFileInfo(const std::string& completePath, TFileInfo& outInfo){
//	TRACE_INDENT("GetFileInfo(" + completePath + ")");

	TFileInfo result;
	result.fDirFlag = false;
	result.fModificationDate = 0;
	result.fCreationDate = 0;
	result.fFileSize = 0;

	struct stat theStat;

	//??? Works for dir?
	int error = stat(completePath.c_str(), &theStat);
	if(error != 0){
		const int err = get_error();

		//	"Error NO ENTry"
		if(err == ENOENT){
			return false;
		}
		else if(err == ENOTDIR){
			ASSERT(false);
			quark::throw_exception();
		}
		else{
			quark::throw_exception();
		}
	}

	time_t dataChange = theStat.st_mtime;
	time_t statusChange = theStat.st_ctime;
	off_t size = theStat.st_size;
	result.fModificationDate = MAX(dataChange, statusChange);

#if QUARK_MAC
	result.fCreationDate = theStat.st_birthtimespec.tv_sec;
#endif

	const bool dir = S_ISDIR(theStat.st_mode);
	const bool file = S_ISREG(theStat.st_mode);

	//	There can be entries that are neither dir or file.
	QUARK_ASSERT(file == !dir);

	result.fDirFlag = dir ? true : false;

	result.fFileSize = size;
	outInfo = result;
	return true;
}



void DeleteDeep(const std::string& path){
	//	Attempt to delete base director or file.
	//	If operation fails because it's a directory with contents, first delete contents then retry deleting the dir.
	int error = ::remove(path.c_str());
	if(error == 0){
		return;
	}
	else if(error == -1){
		const int err = get_error();

		//	#define	ENOENT		2		/* No such file or directory */
		if(err == ENOENT){
			return;
		}

		//	Directory not empty.
		else if(err == ENOTEMPTY){
			std::vector<TDirEntry> dirEntries = GetDirItems(path);
			for(const auto& e: dirEntries){
				const auto subpath = e.fParent + "/" + e.fNameOnly;
				DeleteDeep(subpath);
			}
			int error2 = ::remove(path.c_str());
			if(error2 == 0){
				return;
			}
			else{
				quark::throw_exception();
			}
		}
		else{
			quark::throw_exception();
		}
	}
	else{
		ASSERT(false);
		quark::throw_exception();
	}
}

//int	 rename (const char *__old, const char *__new);
void RenameEntry(const std::string& path, const std::string& n){
	const auto parts = SplitPath(path);
	const auto path2 = parts.fPath + "/" + n;
	int error = ::rename(path.c_str(), path2.c_str());
	if(error == 0){
		return;
	}
	else if(error == -1){
		const int err = get_error();
		if(err == EACCES){
			quark::throw_exception();
		}
		else{
			quark::throw_exception();
		}
	}
	else{
		ASSERT(false);
		quark::throw_exception();
	}
}




void MakeDirectoriesDeep(const std::string& path){
	TRACE_INDENT("MakeDirectoriesDeep()");

	if(path == ""){
	}
	else {
		//??? Hack! I'm confused how you specify a directory in Mac OS X!!! There is sometimes an ending slash too much.
		std::string temp = path;
		{
	//		if(temp.back()=='/')
			if(temp[temp.size() - 1] == '/'){
				temp = std::string(&temp[0], &temp[temp.size() - 1]);
			}
		}

	#if defined(_WIN32)
		int error = _mkdir(temp.c_str());
	#else
		int error = ::mkdir(temp.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	#endif
		if(error != 0){
			const auto err = get_error();
			if(err == EEXIST){
				return;
			}
			else if(err == EFAULT){
				quark::throw_exception();
			}
			else if(err == EACCES){
				quark::throw_exception();
			}
			else if(err == ENAMETOOLONG){
				quark::throw_exception();
			}
			else if(err == ENOENT){
				if(temp.size() > 2){
					//	Make the path to the parent dir first.
					TPathParts split = SplitPath(temp);
					MakeDirectoriesDeep(split.fPath);

					//	Now try again making the sub directory.
					MakeDirectoriesDeep(temp);
				}
				else{
					quark::throw_exception();
				}
			}
			else if(err == ENOTDIR){
				quark::throw_exception();
			}
			else if(err == ENOMEM){
				quark::throw_exception();
			}
			else if(err == EROFS){
				quark::throw_exception();
			}
			else if(err == ELOOP){
				quark::throw_exception();
			}
			else if(err == ENOSPC){
				quark::throw_exception();
			}
			else if(err == ENOSPC){
				quark::throw_exception();
			}
			else{
				quark::throw_exception();
			}
		}
	}
}

void SaveFile(const std::string& inFileName, const std::uint8_t data[], std::size_t byteCount){
	TRACE_INDENT(std::string() + "SaveFile() " + inFileName + ", bytes: " + std::to_string(byteCount));

	TPathParts split = SplitPath(inFileName);

	MakeDirectoriesDeep(split.fPath);
/*
	int err=mkdir(split.fPath.c_str(),S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if(err !=0){
		if(errno==EEXIST){
		}
		else if(errno==EFAULT){
			quark::throw_exception();
		}
		else if(errno==EACCES){
			quark::throw_exception();
		}
		else if(errno==ENAMETOOLONG){
			quark::throw_exception();
		}
		else if(errno==ENOENT){
			quark::throw_exception();
		}
		else if(errno==ENOTDIR){
			quark::throw_exception();
		}
		else if(errno==ENOMEM){
			quark::throw_exception();
		}
		else if(errno==EROFS){
			quark::throw_exception();
		}
		else if(errno==ELOOP){
			quark::throw_exception();
		}
		else if(errno==ENOSPC){
			quark::throw_exception();
		}
		else if(errno==ENOSPC){
			quark::throw_exception();
		}
		else{
			quark::throw_exception();
		}
	}
*/

	FILE* file = 0;
	file = std::fopen(inFileName.c_str(), "wb");
	if(file == 0){
		quark::throw_exception();
	}

	try{
		size_t actuallyWriteCount = std::fwrite(&data[0], byteCount, 1, file);
		if(actuallyWriteCount != 1){
			quark::throw_exception();
		}

		long result = std::fclose(file);
		if(result != 0){
//			int error = get_error();
		}
		file = 0;
	}
	catch(...){
		if(file != 0){
			long result = std::fclose(file);
			if(result != 0){
//				int error = get_error();
			}
			file = 0;
		}
		throw;
	}
}


std::string ToNativePath(const std::string& path){
	return path;
}





static void TestGetFileInfo_ForFile(const std::string& path, bool existsFlag, std::uint64_t modificationDate, std::uint64_t fileSize){
	TFileInfo info;
	bool result = GetFileInfo(path, info);
	ASSERT(result == existsFlag);
	if(result){
//		ASSERT(info.fModificationDate ==modificationDate);
		ASSERT(info.fFileSize == fileSize);
	}
}

/*
static void TestGetFileInfo(const std::string& testInputDir, const std::string& testOutputDir){
	TestGetFileInfo_ForFile(testInputDir + "FileHandling/GetFileInfo/file1.txt", true, 12345, 9);
	TestGetFileInfo_ForFile(testInputDir + "FileHandling/GetFileInfo/file2.txt", true, 12345, 407);
	TestGetFileInfo_ForFile(testInputDir + "FileHandling/GetFileInfo/noexist.txt", false, 0, 0);
}
*/







//http://users.actcom.co.il/~choo/lupg/tutorials/handling-files/handling-files.html




//	Includes "." and ".."
std::vector<dirent> read_dir_elements(const std::string& inDir){
	std::vector<dirent> result;

	::DIR* dir = ::opendir(inDir.c_str());
	if (!dir) {
		quark::throw_exception();
	}

	const struct ::dirent* entry = NULL;
	while ( (entry = ::readdir(dir)) != NULL) {
		result.push_back(*entry);
	}

	if (::closedir(dir) == -1) {
		ASSERT(false);
	}
	return result;
}

std::vector<TDirEntry> GetDirItems(const std::string& inDir){
	const auto dir_elements = read_dir_elements(inDir);
	std::vector<TDirEntry> result;

	for(const auto& e: dir_elements){
#if QUARK_MAC
		const std::string name(&e.d_name[0], &e.d_name[e.d_namlen]);
#else
		const std::string name(&e.d_name[0], &e.d_name[256]);

#endif

		if(name == "." || name == ".."){
			//	Skip.
		}
		else if(e.d_type == DT_REG){
			TDirEntry tempFile;
			tempFile.fNameOnly = name;
			tempFile.fParent = inDir;
			tempFile.fType = TDirEntry::kFile;
			result.push_back(tempFile);
		}
		else if(e.d_type == DT_DIR){
			TDirEntry tempDir;
			tempDir.fNameOnly = name;
			tempDir.fParent = inDir;
			tempDir.fType = TDirEntry::kDir;
			result.push_back(tempDir);
		}
		else{
		}
	}
	return result;
}

std::vector<TDirEntry> GetDirItemsDeep(const std::string& inDir){
	ASSERT(inDir.size() > 0 && inDir.back() == '/');

	std::vector<TDirEntry> res;
	const auto items = GetDirItems(inDir);
	for(const auto& e: items){
		//	Use the item as-is.
		res.push_back(e);

		//	For directories, also add their contents.
		if(e.fType == TDirEntry::kDir){
			TDirEntry subDir = e;
			const auto path2 = inDir + e.fNameOnly + "/";
			const auto subDirItems = GetDirItemsDeep(path2);
			res.insert(res.end(), subDirItems.begin(), subDirItems.end());
		}
	}
	return res;
}







std::string LoadTextFile(const std::string& completePath){
	std::vector<std::uint8_t> text = LoadFile(completePath);
	std::string s(reinterpret_cast<const char*>(&text[0]), text.size());
	return s;
}



std::string MakeAbsolutePath(const std::string& base, const std::string& relativePath){
	ASSERT(base.empty() == 0 || base.back() == '/');
	ASSERT(relativePath.empty() || relativePath.front() != '/');
	return base + relativePath;
}










/*
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
*/

std::string get_working_dir(){
   char cwd[PATH_MAX];
   if (getcwd(cwd, sizeof(cwd)) != NULL) {
       return std::string(cwd);
   }
   else {
//       perror("getcwd() error");
       quark::throw_exception();
   }
}
