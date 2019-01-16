#include "FileHandling.h"

#include "quark.h"
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <map>
#include <vector>
#include "text_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libproc.h>

#include <CoreFoundation/CoreFoundation.h>

#include <dirent.h>    /* struct DIR, struct dirent, opendir().. */




#define JIU_MACOS 1

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



///////////////////////////////////////////////////			DIRECTOR ROOTS



std::string get_env(const std::string& s){
	const char* value = getenv(s.c_str());
	QUARK_ASSERT(value != nullptr);
	if(value == nullptr){
		throw std::exception();
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


QUARK_UNIT_TEST("", "GetDirectories()", "", ""){
	const auto temp = GetDirectories();

	QUARK_UT_VERIFY(true)
}



std::string get_process_path (int process_id){
	pid_t pid; int ret;
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

	pid = (pid_t) process_id;
	ret = proc_pidpath (pid, pathbuf, sizeof(pathbuf));
	if ( ret <= 0 ) {
		fprintf(stderr, "PID %d: proc_pidpath ();\n", pid);
		fprintf(stderr, "	%s\n", strerror(get_error()));
		throw std::exception();
	} else {
//		printf("proc %d: %s\n", pid, pathbuf);
		return std::string(pathbuf);
	}
}


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

//??? fix leaks.
process_info_t get_process_info(){
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	if(mainBundle == nullptr){
		throw std::exception();
	}

    CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
	if(mainBundleURL == nullptr){
		throw std::exception();
	}

    CFStringRef cfStringRef = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);
	if(cfStringRef == nullptr){
		throw std::exception();
	}

    char path[1024];
    CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);

    CFRelease(mainBundleURL);
    CFRelease(cfStringRef);

	return process_info_t{
		std::string(path) + "/"
	};
}



QUARK_UNIT_TEST("", "get_info()", "", ""){
	const auto temp = get_process_info();

	TFileInfo info;
	bool ok = GetFileInfo(temp.process_path, info);

	QUARK_UT_VERIFY(true)
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

QUARK_UNIT_TEST("", "UpDir2()","", ""){
	QUARK_UT_VERIFY((UpDir2("/Users/marcus/Desktop/") == std::pair<std::string, std::string>{ "/Users/marcus/", "Desktop" }));
}
QUARK_UNIT_TEST("", "UpDir2()","", ""){
	QUARK_UT_VERIFY((UpDir2("/Users/") == std::pair<std::string, std::string>{ "/", "Users" }));
}
QUARK_UNIT_TEST("", "UpDir2()","", ""){
	QUARK_UT_VERIFY((UpDir2("/") == std::pair<std::string, std::string>{ "", "/" }));
}
QUARK_UNIT_TEST("", "UpDir2()","", ""){
	QUARK_UT_VERIFY((UpDir2("/Users/marcus/original.txt") == std::pair<std::string, std::string>{ "/Users/marcus/", "original.txt" }));
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

QUARK_UNIT_TEST("", "SplitExtension()","", ""){
	QUARK_UT_VERIFY((SplitExtension("snare.wav") == std::pair<std::string, std::string>{ "snare", ".wav" }));
}
QUARK_UNIT_TEST("", "SplitExtension()","", ""){
	QUARK_UT_VERIFY((SplitExtension("snare.drum.wav") == std::pair<std::string, std::string>{ "snare.drum", ".wav" }));
}
QUARK_UNIT_TEST("", "SplitExtension()","", ""){
	QUARK_UT_VERIFY((SplitExtension("snare") == std::pair<std::string, std::string>{ "snare", "" }));
}
QUARK_UNIT_TEST("", "SplitExtension()","", ""){
	QUARK_UT_VERIFY((SplitExtension(".wav") == std::pair<std::string, std::string>{ "", ".wav" }));
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

QUARK_UNIT_TEST("", "TestSplitPath","", ""){
	//	Complex.
	TestSplitPath("/Volumes/MyHD/SomeDir/MyFileName.txt", TPathParts("/Volumes/MyHD/SomeDir/","MyFileName",".txt"));
}

QUARK_UNIT_TEST("", "TestSplitPath","", ""){
	//	Name + extension.
	TestSplitPath("MyFileName.txt", TPathParts("","MyFileName",".txt"));
}

QUARK_UNIT_TEST("", "TestSplitPath","", ""){
	//	Name
	TestSplitPath("MyFileName", TPathParts("","MyFileName",""));
}

QUARK_UNIT_TEST("", "TestSplitPath","", ""){
	//	Path + name
	TestSplitPath("Dir/MyFileName", TPathParts("Dir/","MyFileName",""));
}

QUARK_UNIT_TEST("", "TestSplitPath","", ""){
	//	Path + extension.
	TestSplitPath("/Volumes/MyHD/SomeDir/.txt", TPathParts("/Volumes/MyHD/SomeDir/","",".txt"));
}

QUARK_UNIT_TEST("", "TestSplitPath","", ""){
	TestSplitPath("/Volumes/MyHD/SomeDir/", TPathParts("/Volumes/MyHD/SomeDir/","",""));
}




std::vector<std::uint8_t> LoadFile(const std::string& completePath){
	TRACE_INDENT("LoadFile(" + completePath + ")");

	FILE* f = 0;
	try {
		f = std::fopen(completePath.c_str(), "rb");
		if(f == 0){
			throw std::exception();
		}

		int fseekReturn = std::fseek(f, 0, SEEK_END);
		if (fseekReturn != 0) {
			throw std::exception();
		}

		long fileSize = std::ftell(f);
		if (fileSize < 0) {
			throw std::exception();
		}

		fseekReturn = std::fseek(f, 0, SEEK_SET);
		if (fseekReturn != 0) {
			throw std::exception();
		}

		std::vector<std::uint8_t> data(fileSize);
		size_t freadReturn = std::fread(&data[0], 1, fileSize, f);
		if (freadReturn != fileSize) {
			throw std::exception();
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
			throw std::exception();
		}
		else{
			throw std::exception();
		}
	}

	time_t dataChange = theStat.st_mtime;
	time_t statusChange = theStat.st_ctime;
	off_t size = theStat.st_size;
	result.fModificationDate = std::max(dataChange, statusChange);
	result.fCreationDate = theStat.st_birthtimespec.tv_sec;


	const bool dir = S_ISDIR(theStat.st_mode);
	const bool file = S_ISREG(theStat.st_mode);

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
				throw std::exception();
			}
		}
		else{
			throw std::exception();
		}
	}
	else{
		ASSERT(false);
		throw std::exception();
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
		throw std::exception();
	}
	else{
		ASSERT(false);
		throw std::exception();
	}
}




void MakeDirectoriesDeep(const std::string& path){
	TRACE_INDENT("MakeDirectoriesDeep()");

	//??? Hack! I'm confused how you specify a directory in Mac OS X!!! There is sometimes an ending slash too much.
	std::string temp = path;
	{
//		if(temp.back()=='/')
		if(temp[temp.size() - 1] == '/'){
			temp = std::string(&temp[0], &temp[temp.size() - 1]);
		}
	}

	int error = ::mkdir(temp.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if(error != 0){
		const auto err = get_error();
		if(err == EEXIST){
			return;
		}
		else if(err == EFAULT){
			throw std::exception();
		}
		else if(err == EACCES){
			throw std::exception();
		}
		else if(err == ENAMETOOLONG){
			throw std::exception();
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
				throw std::exception();
			}
		}
		else if(err == ENOTDIR){
			throw std::exception();
		}
		else if(err == ENOMEM){
			throw std::exception();
		}
		else if(err == EROFS){
			throw std::exception();
		}
		else if(err == ELOOP){
			throw std::exception();
		}
		else if(err == ENOSPC){
			throw std::exception();
		}
		else if(err == ENOSPC){
			throw std::exception();
		}
		else{
			throw std::exception();
		}
	}
}

void SaveFile(const std::string& inFileName, const std::uint8_t data[], std::size_t byteCount){
	TRACE_INDENT("SaveFile()");
	TRACE("Byte count: " + std::to_string(byteCount));

	TPathParts split = SplitPath(inFileName);

	MakeDirectoriesDeep(split.fPath);
/*
	int err=mkdir(split.fPath.c_str(),S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if(err !=0){
		if(errno==EEXIST){
		}
		else if(errno==EFAULT){
			throw std::exception();
		}
		else if(errno==EACCES){
			throw std::exception();
		}
		else if(errno==ENAMETOOLONG){
			throw std::exception();
		}
		else if(errno==ENOENT){
			throw std::exception();
		}
		else if(errno==ENOTDIR){
			throw std::exception();
		}
		else if(errno==ENOMEM){
			throw std::exception();
		}
		else if(errno==EROFS){
			throw std::exception();
		}
		else if(errno==ELOOP){
			throw std::exception();
		}
		else if(errno==ENOSPC){
			throw std::exception();
		}
		else if(errno==ENOSPC){
			throw std::exception();
		}
		else{
			throw std::exception();
		}
	}
*/

	FILE* file = 0;
	file = std::fopen(inFileName.c_str(), "wb");
	if(file == 0){
		throw std::exception();
	}

	try{
		size_t actuallyWriteCount = std::fwrite(&data[0], byteCount, 1, file);
		if(actuallyWriteCount != 1){
			throw std::exception();
		}

		long result = std::fclose(file);
		file = 0;
	}
	catch(...){
		if(file != 0){
			long result = std::fclose(file);
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
		throw std::exception();
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
		const std::string name(&e.d_name[0], &e.d_name[e.d_namlen]);

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











////////////////////////////		COMMAND LINE ARGUMENTS




std::vector<std::string> args_to_vector(int argc, const char * argv[]){
	std::vector<std::string> r;
	for(int i = 0 ; i < argc ; i++){
		const std::string s(argv[i]);
		r.push_back(s);
	}
	return r;
}

std::pair<std::string, std::vector<std::string> > extract_key(const std::vector<std::string>& args, const std::string& key){
	const auto it = std::find(args.begin(), args.end(), key);
	if(it != args.end()){
		auto copy = args;
		copy.erase(it);

		return { key, copy };
	}
	else{
		return { "", args };
	}
}

/*
% testopt
aflag = 0, bflag = 0, cvalue = (null)

% testopt -a -b
aflag = 1, bflag = 1, cvalue = (null)

% testopt -ab
aflag = 1, bflag = 1, cvalue = (null)

% testopt -c foo
aflag = 0, bflag = 0, cvalue = foo

% testopt -cfoo
aflag = 0, bflag = 0, cvalue = foo

% testopt arg1
aflag = 0, bflag = 0, cvalue = (null)
Non-option argument arg1

% testopt -a arg1
aflag = 1, bflag = 0, cvalue = (null)
Non-option argument arg1

% testopt -c foo arg1
aflag = 0, bflag = 0, cvalue = foo
Non-option argument arg1

% testopt -a -- -b
aflag = 1, bflag = 0, cvalue = (null)
Non-option argument -b

% testopt -a -
aflag = 1, bflag = 0, cvalue = (null)
Non-option argument -
*/



command_line_args_t parse_command_line_args(const std::vector<std::string>& args, const std::string& flags){
	if(args.size() == 0){
		throw std::exception();
	}

	//	Includes a trailing null-terminator for each arg.
	int byte_count = 0;
	for(const auto& e: args){
		byte_count = byte_count + static_cast<int>(e.size()) + 1;
	}

	//	Make local writeable strings to argv to hand to getopt().
	std::vector<char*> argv;
	std::string concat_args;

	//	Make sure it's not reallocating since we're keeping pointers into the string.
	concat_args.reserve(byte_count);

	for(const auto& e: args){
		char* ptr = &concat_args[concat_args.size()];
		concat_args.insert(concat_args.end(), e.begin(), e.end());
		concat_args.push_back('\0');
		argv.push_back(ptr);
	}

	int argc = static_cast<int>(args.size());

	std::map<std::string, std::string> flags2;

	//	The variable optind is the index of the next element to be processed in argv. The system
	//	initializes this value to 1. The caller can reset it to 1 to restart scanning of the same argv,
	//	or when scanning a new argument vector.
	optind = 1;

	//	Disable getopt() from printing errors to stderr.
	opterr = 0;

    // put ':' in the starting of the
    // string so that program can
    //distinguish between '?' and ':'
    int opt = 0;
    while((opt = getopt(argc, &argv[0], flags.c_str())) != -1){
		const auto opt_string = std::string(1, opt);
		const auto optarg_string = optarg != nullptr ? std::string(optarg) : std::string();
		const auto optopt_string = std::string(1, optopt);

		//	Unknown flag.
		if(opt == '?'){
			flags2.insert({ optopt_string, "?" });
		}

		//	Flag with parameter.
		else if(optarg_string.empty() == false){
			flags2.insert({ opt_string, optarg_string });
		}

		//	Flag needs a value(???)
		else if(opt == ':'){
			flags2.insert({ ":", optarg_string });
		}

		//	Flag without parameter.
		else{
			flags2.insert({ opt_string, "" });
		}
    }

	std::vector<std::string> extras;

    // optind is for the extra arguments
    // which are not parsed
    for(; optind < argc; optind++){
		const auto s = std::string(argv[optind]);
		extras.push_back(s);
    }

	return { args[0], "", flags2, extras };
}

command_line_args_t parse_command_line_args_subcommands(const std::vector<std::string>& args, const std::string& flags){
	const auto a = parse_command_line_args({ args.begin() + 1, args.end()}, flags);
	return { args[0], a.command, a.flags, a.extra_arguments };
}


void trace_command_line_args(const command_line_args_t& v){
	QUARK_SCOPED_TRACE("trace_command_line_args");

	{
		QUARK_SCOPED_TRACE("flags");
		for(const auto& e: v.flags){
			const auto s = e.first + ": " + e.second;
			QUARK_TRACE(s);
		}
	}
}

const auto k_git_command_examples = std::vector<std::string>({
	R"(git init)",

	R"(git clone /path/to/repository)",
	R"(git clone username@host:/path/to/repository)",
	R"(git add <filename>)",
	R"(git add *)",
	R"(git commit -m "Commit message")",

	R"(git push origin master)",
	R"(git remote add origin <server>)",
	R"(git checkout -b feature_x)",
	R"(git tag 1.0.0 1b2e1d63ff)",
	R"(git log --help)"
});




std::vector<std::string> split_command_line(const std::string& s){
	std::vector<std::string> result;
	std::string acc;

	auto pos = 0;
	while(pos < s.size()){
		if(s[pos] == ' '){
			if(acc.empty()){
			}
			else{
				result.push_back(acc);
				acc = "";
			}
			pos++;
		}
		else if (s[pos] == '\"'){
			acc.push_back(s[pos]);

			pos++;
			while(s[pos] != '\"'){
				acc.push_back(s[pos]);
				pos++;
			}

			acc.push_back(s[pos]);
			pos++;
		}
		else{
			acc.push_back(s[pos]);
			pos++;
		}
	}

	if(acc.empty() == false){
		result.push_back(acc);
		acc = "";
	}

	return result;
}

QUARK_UNIT_TEST("", "split_command_line()", "", ""){
	const auto result = split_command_line("");
	QUARK_UT_VERIFY(result == std::vector<std::string>{});
}
QUARK_UNIT_TEST("", "split_command_line()", "", ""){
	const auto result = split_command_line("  ");
	QUARK_UT_VERIFY(result == std::vector<std::string>{});
}
QUARK_UNIT_TEST("", "split_command_line()", "", ""){
	const auto result = split_command_line(" one ");
	QUARK_UT_VERIFY(result == std::vector<std::string>{ "one" });
}
QUARK_UNIT_TEST("", "split_command_line()", "", ""){
	const auto result = split_command_line("one two");
	QUARK_UT_VERIFY((result == std::vector<std::string>{ "one", "two" }));
}
QUARK_UNIT_TEST("", "split_command_line()", "", ""){
	const auto result = split_command_line("one two     three");
	QUARK_UT_VERIFY((result == std::vector<std::string>{ "one", "two", "three" }));
}

QUARK_UNIT_TEST("", "split_command_line()", "", ""){
	const auto result = split_command_line(R"(one "hello world"    three)");
	QUARK_UT_VERIFY((result == std::vector<std::string>{ "one", R"("hello world")", "three" }));
}




QUARK_UNIT_TEST("", "parse_command_line_args()", "", ""){
	const auto result = parse_command_line_args({ "myapp" }, ":if:lrx");
	QUARK_UT_VERIFY(result.command == "myapp");
	QUARK_UT_VERIFY(result.subcommand == "");
	QUARK_UT_VERIFY(result.flags.empty() && result.extra_arguments.empty());
}

QUARK_UNIT_TEST("", "parse_command_line_args()", "", ""){
	const auto result = parse_command_line_args({ "myapp", "file1.txt" }, ":if:lrx");
	QUARK_UT_VERIFY(result.command == "myapp");
	QUARK_UT_VERIFY(result.subcommand == "");
	QUARK_UT_VERIFY(result.flags.empty());
	QUARK_UT_VERIFY(result.extra_arguments == std::vector<std::string>({"file1.txt"}));
}

QUARK_UNIT_TEST("", "parse_command_line_args()", "", ""){
	const auto result = parse_command_line_args({ "myapp", "-ilr", "-f", "doc.txt" }, ":if:lrx");
	QUARK_UT_VERIFY(result.command == "myapp");
	QUARK_UT_VERIFY(result.subcommand == "");
	QUARK_UT_VERIFY(result.flags.find("i")->second == "");
	QUARK_UT_VERIFY(result.flags.find("l")->second == "");
	QUARK_UT_VERIFY(result.flags.find("r")->second == "");
	QUARK_UT_VERIFY(result.flags.find("f")->second == "doc.txt");
	QUARK_UT_VERIFY(result.extra_arguments.size() == 0);
}


//	getopt() uses global state, make sure we can call it several times OK (we reset it).
QUARK_UNIT_TEST("", "parse_command_line_args()", "", ""){
	const auto result = parse_command_line_args({ "myapp", "-ilr", "-f", "doc.txt" }, ":if:lrx");
	QUARK_UT_VERIFY(result.command == "myapp");
	QUARK_UT_VERIFY(result.subcommand == "");
	QUARK_UT_VERIFY(result.flags.find("i")->second == "");
	QUARK_UT_VERIFY(result.flags.find("l")->second == "");
	QUARK_UT_VERIFY(result.flags.find("r")->second == "");
	QUARK_UT_VERIFY(result.flags.find("f")->second == "doc.txt");
	QUARK_UT_VERIFY(result.extra_arguments.size() == 0);

	const auto result1 = parse_command_line_args({ "myapp", "-ilr", "-f", "doc.txt" }, ":if:lrx");
	QUARK_UT_VERIFY(result.flags == result1.flags && result.extra_arguments == result.extra_arguments);
}


QUARK_UNIT_TEST("", "parse_command_line_args()", "", ""){
	const auto result = parse_command_line_args({ "myapp", "-i", "-f", "doc.txt", "extra_one", "extra_two" }, ":if:lrx");
	QUARK_UT_VERIFY(result.command == "myapp");
	QUARK_UT_VERIFY(result.subcommand == "");
	QUARK_UT_VERIFY(result.flags.find("i")->second == "");
	QUARK_UT_VERIFY(result.flags.find("f")->second == "doc.txt");
	QUARK_UT_VERIFY((result.extra_arguments == std::vector<std::string>{ "extra_one", "extra_two" }));
}



QUARK_UNIT_TEST("", "parse_command_line_args_subcommands()", "", ""){
	const auto result = parse_command_line_args_subcommands(split_command_line(k_git_command_examples[0]), "");
	QUARK_UT_VERIFY(result.command == "git");
	QUARK_UT_VERIFY(result.subcommand == "init");
	QUARK_UT_VERIFY((result.flags == std::map<std::string, std::string>{}));
	QUARK_UT_VERIFY((result.extra_arguments == std::vector<std::string>{}));
}
QUARK_UNIT_TEST("", "parse_command_line_args_subcommands()", "", ""){
	const auto result = parse_command_line_args_subcommands(split_command_line(k_git_command_examples[1]), "");
	QUARK_UT_VERIFY(result.command == "git");
	QUARK_UT_VERIFY(result.subcommand == "clone");
	QUARK_UT_VERIFY((result.flags == std::map<std::string, std::string>{}));
	QUARK_UT_VERIFY((result.extra_arguments == std::vector<std::string>{ "/path/to/repository" }));
}

QUARK_UNIT_TEST("", "parse_command_line_args_subcommands()", "", ""){
	const auto result = parse_command_line_args_subcommands(split_command_line(k_git_command_examples[4]), "");
	QUARK_UT_VERIFY(result.command == "git");
	QUARK_UT_VERIFY(result.subcommand == "add");
	QUARK_UT_VERIFY((result.flags == std::map<std::string, std::string>{}));
	QUARK_UT_VERIFY((result.extra_arguments == std::vector<std::string>{ "*" }));
}
QUARK_UNIT_TEST_VIP("", "parse_command_line_args_subcommands()", "", ""){
	const auto result = parse_command_line_args_subcommands(split_command_line(k_git_command_examples[5]), "");
	QUARK_UT_VERIFY(result.command == "git");
	QUARK_UT_VERIFY(result.subcommand == "commit");
	QUARK_UT_VERIFY((result.flags == std::map<std::string, std::string>{ {"m","?"}} ));
	QUARK_UT_VERIFY((result.extra_arguments == std::vector<std::string>{ R"("Commit message")" }));
}
