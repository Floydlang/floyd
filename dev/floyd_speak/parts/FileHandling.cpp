#include "FileHandling.h"

#include "quark.h"
#include <unistd.h>
//#include "Trace.h"
//#include "JiuUtils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
//#include "MacBasics.h"

//#include "SDL_filesystem.h"


#define JIU_MACOS 1

#define TRACE_INDENT QUARK_SCOPED_TRACE
#define TRACE QUARK_TRACE
#define ASSERT QUARK_ASSERT





/*
	Example:
		"/Users/marcus/Library/Developer/Xcode/DerivedData/Jiu-hfmjsbcsetvlvjgzvlfcucroeybg/Build/Products/Debug/Jiu Runtime.app/Contents/Resources/"
*/

std::string GetAppResourceDir(){
	return "";
/*
	//??? hardcoded.
	char* p = ::SDL_GetBasePath();
	assert(p != nullptr);

	const std::string result(p);
	::SDL_free(p);
	p = nullptr;
	return result;
*/

}
std::string GetAppReadWriteDir(){
	return "";
/*
	//??? hardcoded.
	char* p = ::SDL_GetPrefPath("MarcusGameAB", "MyGame");
	assert(p != nullptr);

	const std::string result(p);
	::SDL_free(p);
	p = nullptr;
	return result;
*/}



void GetTestDirs(std::string& oReadDir, std::string& oWriteDir){
	oReadDir = GetAppResourceDir() + "TestInput/";
	oWriteDir = GetAppReadWriteDir() + "TestOutput/";
	TRACE("oReadDir: " + oReadDir);
	TRACE("oWriteDir: " + oWriteDir);
}



#if JIU_MACOS || JIU_IOS

std::string UpDir(const std::string& path){
	std::string res = path;
	std::string::size_type pos = path.rfind('/', std::string::npos);
	if(pos != std::string::npos){
		res.erase(pos, std::string::npos);
	}
	return res;
}

std::pair<std::string, std::string> UpDir2(const std::string& path){
	ASSERT(path.empty() || path.back() == '/');

	if(path == "/"){
		return { "", "/" };
	}
	else if(path == ""){
		return { "", "" };
	}
	else{
		const auto pos = path.rfind('/', path.size() - 2);
		if(pos != std::string::npos){
			const auto left = std::string(path.begin(), path.begin() + pos + 1);
			const auto right = std::string(path.begin() + pos + 1, path.end() - 1);
			return { left, right };
		}
		else{
			return { "", path };
		}
	}
}

QUARK_UNIT_TEST_VIP("", "UpDir2()","", ""){
	QUARK_UT_VERIFY((UpDir2("/Users/marcus/Desktop/") == std::pair<std::string, std::string>{ "/Users/marcus/", "Desktop" }));
}
QUARK_UNIT_TEST_VIP("", "UpDir2()","", ""){
	QUARK_UT_VERIFY((UpDir2("/Users/") == std::pair<std::string, std::string>{ "/", "Users" }));
}
QUARK_UNIT_TEST_VIP("", "UpDir2()","", ""){
	QUARK_UT_VERIFY((UpDir2("/") == std::pair<std::string, std::string>{ "", "/" }));
}


std::string GetExecutableDir(){
	return GetAppResourceDir();
}



std::string GetCacheDir(){
	return GetAppReadWriteDir() + "/temp";
}

std::string GetPreferenceDir(){
	return GetAppReadWriteDir() + "/prefs";
}

std::string GetDesktopDir(){
	const char* homeDir = getenv("HOME");
    return std::string(homeDir) + "/Desktop";
}

#endif


#if JIU_MACOS && 0
#include <Carbon/Carbon.h>

static OSErr FSMakeFSRef(
	FSVolumeRefNum volRefNum,
	SInt32 dirID,
	ConstStr255Param name,
	FSRef *ref)
{
	OSErr		result;
	FSRefParam	pb;
	
	//	check parameters
	require_action(NULL != ref, BadParameter, result = paramErr);
	
	pb.ioVRefNum = volRefNum;
	pb.ioDirID = dirID;
	pb.ioNamePtr = (StringPtr)name;
	pb.newRef = ref;
	result = PBMakeFSRefSync(&pb);
	require_noerr(result, PBMakeFSRefSync);
PBMakeFSRefSync:
BadParameter:
	return ( result );
}

static OSStatus FSMakePath(
	SInt16 volRefNum,
	SInt32 dirID,
	ConstStr255Param name,
	UInt8 *path,
	UInt32 maxPathSize)
{
	OSStatus	result;
	FSRef		ref;
	
	// check parameters 
	require_action(NULL != path, BadParameter, result = paramErr);
	
	// convert the inputs to an FSRef
	result = FSMakeFSRef(volRefNum, dirID, name, &ref);
	require_noerr(result, FSMakeFSRef);
	
	// and then convert the FSRef to a path
	result = FSRefMakePath(&ref, path, maxPathSize);
	require_noerr(result, FSRefMakePath);
FSRefMakePath:
FSMakeFSRef:
BadParameter:
	return ( result );
}

static ::FSSpec GetApplicationFSSpec(){
	static bool sInitedFlag=false;
	static ::FSSpec sApplicationFSSpec;

	if(!sInitedFlag){
		::ProcessSerialNumber psn;
		OSErr err=::GetCurrentProcess(&psn);
		if(err !=::noErr){
			throw std::exception();
		}
		::ProcessInfoRec info;
		std::memset(&info,0,sizeof(info));
		info.processInfoLength=sizeof(info);
		info.processAppSpec=&sApplicationFSSpec;
		err=::GetProcessInformation(&psn,&info);
		if(err !=::noErr){
			throw std::exception();
		}
		sInitedFlag=true;
	}
	return sApplicationFSSpec;
}

//### Static!
static std::string sCachesDir;
static std::string sPreferencesPath;




std::string GetExecutableDir(){
	TRACE_INDENT("GetExecutableDir()");

	::FSSpec applicationFSSpec=GetApplicationFSSpec();

	std::uint8_t temp[1024 * 2 + 1];
	OSStatus status=FSMakePath(
		applicationFSSpec.vRefNum,
		applicationFSSpec.parID,
		applicationFSSpec.name,
		temp,
		1024 * 2 + 1);
	if(status !=noErr){
		throw std::exception();
	}

	//"Application path:/Volumes/LaCie disk/MZ Para - latest/Para to Backup/para project/Jiu Debug.app/Contents/MacOS/Jiu Debug"

	std::string appPath=std::string(reinterpret_cast<const char*>(temp));
	TRACE("path: " + appPath);
	return appPath;
}

std::string FromFSRef(const FSRef& fsRef){
	char tempBuffer[8192];
	OSStatus status = FSRefMakePath(&fsRef, reinterpret_cast<UInt8*>(tempBuffer), 8191);
	ThrowErr(status);

	std::string s(tempBuffer);
	return s;
}

static std::string FindFolderPath(OSType folderType, bool createDirFlag){
	FSRef foundRef;
	std::memset(&foundRef, 0, sizeof(FSRef));
	OSErr err=FSFindFolder(kUserDomain, folderType, createDirFlag ? TRUE : FALSE, &foundRef);
	if(err !=noErr){
		throw std::exception();
	}

	std::string path = FromFSRef(foundRef);
	return path;
}

//		std::string path=FindFolderPath(kPreferencesFolderType, true);
/*
		{
			std::string path;
			path=FindFolderPath(kSystemFolderType, true);
			path=FindFolderPath(kDesktopFolderType, true);
			path=FindFolderPath(kSystemDesktopFolderType, true);
			path=FindFolderPath(kTrashFolderType, true);
	//		path=FindFolderPath(kSystemTrashFolderType, true);

			path=FindFolderPath(kTemporaryFolderType, true);	//	/private/var/tmp/folders.501/TemporaryItems
			path=FindFolderPath(kApplicationsFolderType, true);
			path=FindFolderPath(kDocumentsFolderType, true);
			path=FindFolderPath(kVolumeRootFolderType, true);
			path=FindFolderPath(kChewableItemsFolderType, true);
			path=FindFolderPath(kApplicationSupportFolderType, true);
			path=FindFolderPath(kHelpFolderType, true);
			path=FindFolderPath(kUtilitiesFolderType, true);
			path=FindFolderPath(kFavoritesFolderType, true);


	//http://developer.apple.com/documentation/Carbon/Reference/Folder_Manager/folder_manager_ref/chapter_1.4_section_8.html

			path=FindFolderPath(kUserSpecificTmpFolderType, true);
		}
*/

std::string GetCacheDir(){
	if(sCachesDir.empty()){
		std::string path=FindFolderPath(kCachedDataFolderType, true);

		sCachesDir=path + "/";
	}
	return sCachesDir;
}

//http://developer.apple.com/documentation/Carbon/Reference/Folder_Manager/folder_manager_ref/chapter_1.2_section_3.html
//		kChewableItemsFolderType
//		kTemporaryFolderType
std::string GetPreferenceDir(){
	if(sPreferencesPath.empty()){
		std::string path=FindFolderPath(kPreferencesFolderType, true);
		sPreferencesPath=path + "/";
	}
	return sPreferencesPath;
}

std::string GetDesktopDir(){
	std::string path=FindFolderPath(kDesktopFolderType, true);
	path=path + "/";
	return path;
}


#endif	//	JIU_MACOS




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

bool DoesFileExist(const std::string& completePath){
	TRACE_INDENT("DoesFileExist(" + completePath + ")");

	FILE* f = 0;
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
	int err = stat(completePath.c_str(), &theStat);
	if(err != 0){
		const int theErrno = errno;

		//	"Error NO ENTry"
		if(theErrno == ENOENT){
			return false;
		}
		else if(theErrno == ENOTDIR){
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


	using namespace std;

void MakeDirectoriesDeep(const std::string& path){
	TRACE_INDENT("MakeDirectoriesDeep()");

	//??? Hack! I'm confused how you specify a directory in Mac OS X!!! There is sometimes an ending slash too much.
	std::string temp=path;
	{
//		if(temp.back()=='/')
		if(temp[temp.size() - 1]=='/'){
			temp=std::string(&temp[0], &temp[temp.size() - 1]);
		}
	}

	int err=mkdir(temp.c_str(),S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
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
			if(temp.size() > 2){
				//	Make the path to the parent dir first.
				TPathParts split=SplitPath(temp);
				MakeDirectoriesDeep(split.fPath);

				//	Now try again making the sub directory.
				MakeDirectoriesDeep(temp);
			}
			else{
				throw std::exception();
			}
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
}

void SaveFile(const std::string& inFileName, const std::uint8_t data[], std::size_t byteCount){
	TRACE_INDENT("SaveFile()");
	TRACE("Byte count: " + std::to_string(byteCount));

	TPathParts split=SplitPath(inFileName);

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

	FILE* file=0;
	file=std::fopen(inFileName.c_str(),"wb");
	if(file==0){
		throw std::exception();
	}

	try{
		size_t actuallyWriteCount=std::fwrite(&data[0],byteCount,1,file);
		if(actuallyWriteCount !=1){
			throw std::exception();
		}

		long result=std::fclose(file);
		file=0;
	}
	catch(...){
		if(file !=0){
			long result=std::fclose(file);
			file=0;
		}
		throw;
	}
}

#if JIU_MACOS || JIU_IOS

std::string ToNativePath(const std::string& path){
	return path;
}

#elif _WINDOWS

??? works?

std::string ToNativePath(const std::string& path){
	std::string temp=path;
	for(std::string::size_type c=0 ; c < temp.size() ; c++){
		if(temp[c]=='/'){
			temp[c]='\\';
		}
	}
	return temp;
}

#else

??? implement!

#endif






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

#include <dirent.h>    /* struct DIR, struct dirent, opendir().. */


std::vector<TDirEntry> GetDirItems(const std::string& inDir){
	std::vector<TDirEntry> r;

	/* open the directory "/home/users" for reading. */
//	::DIR* dir = ::opendir("/home/users");
	::DIR* dir = ::opendir(inDir.c_str());
	if (!dir) {
		throw std::exception();
	}

	/* this structure is used for storing the name of each entry in turn. */
	const struct ::dirent* entry = NULL;

	while ( (entry = ::readdir(dir)) != NULL) {
		if(entry->d_type == DT_REG){
			TDirEntry tempFile;
			tempFile.fNameOnly = entry->d_name;
			tempFile.fParent = inDir;
			tempFile.fType = TDirEntry::kFile;
			r.push_back(tempFile);
		}
		else if(entry->d_type == DT_DIR){
			TDirEntry tempDir;
			tempDir.fNameOnly = entry->d_name;
			tempDir.fParent = inDir;
			tempDir.fType = TDirEntry::kDir;
			r.push_back(tempDir);
		}
		else{
		}
	}

	if (::closedir(dir) == -1) {
		ASSERT(false);
	}
	return r;
}

std::vector<TDirEntry> GetDirItemsDeep(const std::string& inDir){
	ASSERT(inDir.size() > 0 && inDir.back() == '/');

	std::vector<TDirEntry> res;
	const auto items = GetDirItems(inDir);
	for(const auto& e: items){
//xx		TRACE(e.fName + "\t" + std::to_string(e.);
		if(e.fNameOnly[0] != '.'){

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





#if JIU_MACOS && 0

static ProcessSerialNumber MakeNoProcess(){
	ProcessSerialNumber temp;
	temp.lowLongOfPSN = kNoProcess;
	temp.highLongOfPSN = 0;
	return temp;
}

static bool IsNoProcess(const ProcessSerialNumber& psn){
	if(psn.lowLongOfPSN == kNoProcess && psn.highLongOfPSN == 0){
		return true;
	}
	else{
		return false;
	}
}

static bool GetPSNFromSignature(OSType signature, ProcessSerialNumber* finderPSN){
	OSStatus status = noErr;

	ProcessSerialNumber temp = MakeNoProcess();

	status = GetNextProcess(&temp);
	ThrowErr(status);
	if(IsNoProcess(temp)){
		return false;
	}

	while(!IsNoProcess(temp)){
		ProcessInfoRec info;
		std::memset(&info, 0x00, sizeof(ProcessInfoRec));
		status = GetProcessInformation(&temp, &info);
		ThrowErr(status);

		if(info.processSignature == signature){
			*finderPSN = temp;
			return true;
		}
	}
	return false;
}

static OSStatus RevealItemInFinder(const FSRef* pItemRef){
	OSErr	err;
	AEAddressDesc targetAddrDesc = { typeNull, nil };
	AppleEvent theAppleEvent = { typeNull, nil };
	AppleEvent replyAppleEvent = { typeNull, nil };
	AliasHandle alias = nil;
	OSType finderSig = 'MACS';


	err = FSNewAlias(nil, pItemRef, &alias);
	require_noerr(err, Bail);


	HLock((Handle) alias); // HLock is unneeded on Mac OS X

	// address target by signature
	err = AECreateDesc(typeApplSignature, &finderSig, sizeof(OSType), &targetAddrDesc);
	require_noerr(err, Bail);

	// make the event
	err = AECreateAppleEvent(kAEMiscStandards, kAEMakeObjectsVisible, &targetAddrDesc, kAutoGenerateReturnID, kAnyTransactionID, &theAppleEvent);
	require_noerr(err, Bail);

	err = AEPutParamPtr(&theAppleEvent, keyDirectObject, typeAlias, *alias, GetHandleSize((Handle) alias));
	require_noerr(err, Bail);

		// send it
		err = AESend(&theAppleEvent, &replyAppleEvent, kAENoReply,
					kAENormalPriority, kAEDefaultTimeout, NULL, NULL);


	// bring the finder forward
	ProcessSerialNumber finderPSN;
	GetPSNFromSignature(finderSig, &finderPSN); // this calls GetNextProcess until GetProcessInformation finds the serial number of the app with the given signature
	SetFrontProcess(&finderPSN);

Bail:
	if (alias)  DisposeHandle((Handle) alias);
	if (targetAddrDesc.dataHandle) AEDisposeDesc(&targetAddrDesc);
	if (theAppleEvent.dataHandle)  AEDisposeDesc(&theAppleEvent);


	return err;
}

void RevealItemInFinder(const std::string& path){
	OSStatus status = noErr;
	FSRef ref;
	Boolean isDirectory = false;
	status = FSPathMakeRef(reinterpret_cast<const UInt8 *>(path.c_str()), &ref, &isDirectory);
	ThrowErr(status);

	status = RevealItemInFinder(&ref);
	ThrowErr(status);
}

#endif
