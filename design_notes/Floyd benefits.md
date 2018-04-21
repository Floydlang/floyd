# Killer apps for languages
- C: port operating systems when HW change a lot
- Fortran

# Why do I want Floyd instead of C++, Rust or Swift?

- Build huge systems. Easily compose parts into bigger ones
- Programs are very fast (native or interpreted)
- No fad distractions, just industrial-strength tools
- Best practises defined and enforced (naming and layout)
- Full set of nextgen interactive tools
- Programming is easy and risk free
- Programs are very small - very lite accidental complexity
- Simple built in toolkit for all basics
- Easy to learn, Javascript-like.
- Fun and reliable concurrency
- Embeddable and mixable with other languages

- Supports C ABI
- Compiled nate


# parts

Cache
FileSystem
Server comm
S3 share
user-account w server
HTML DOM
App Data Model
Document File with versions
ABI
Internet stream
Audio
Encode / decode binary and text formats
Images
Emails
Open GLs

# External file system value, like React

? Can the fs be a normal Floyd value and file data stored as [uint8]? Or do we need to have special accessors?

Read and write entire fs and every byte in files and meta data is if it was only values.

	struct Permissions {
		reading: bool
		writing: bool
		execution: bool
	}
	
	struct FSNode {
		fileSize: size_t
		ownerUserID: uint32
		ownerGroupD: uint32
		userPermissions: Permissions
		groupPermissions: Permissions
		othersPermissions: Permissions
		file: [uint8]?
		dir: [FSNode]?
		inodeChangeTime: uint32
		contentChangeTime: uint32
		lastAccessTime: uint32
	}
	
	struct FileSystem {
		root: INode
	}

	struct APath {
		nodeNames: [string]
	}

- APath: a vector or names from root to a node (file or dir). Nodes may or may not exist in fs right now.


How it would look with wrapping

	let fs = SampleFileSystem("c:")
	let path = APath("users/marcus/desktop/hello.txy")
	let fileContents = fs.GetNodeAsFile(path)
	let unicodeText = unpackUTF8(fileContents)
	let fs2 = fs.SetNodeFileContents(path, unicodeText + "Hello, this is the new file contents")

With mirrored data structures

	let fs = SampleFileSystem("c:")
	let path = APath("users/marcus/desktop/hello.txt")
	let fileContents = fs.root.dir!["users"].dir!["marcus"].dir!["desktop"].dir!["hello.txt"].file!
	let fs2 = fs.root.dir!["users"].dir!["marcus"].dir!["desktop"].dir!["hello.txt"].file! = "hello"


# Other notes

- Support setin and getin, it uses introspection? Don't use string paths, include tech in language. Maybe use vector of keyword-type.
??? every object, value i system has 64bit ID. You can always write code that tracks that ID, for example a view can track the data values it represents as subviews. Mirror tree -- feature.
