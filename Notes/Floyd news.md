A piece of code depends on: runtime, functions, types and modules. Callbacks. Custom runtimes

A journal of all inputs is the purest of all datastructures

Floyd •cache• primitive: immutable on the outside, mutable on the inside

Reference: Ipfs

Peer to peer deduplication between data structures of same types

Built-in statemachine construct (variant of a process / clock?)

Cache / prefetch / lazy / deduplicate / pair-sync / diff are language features

Runtimes can have internal state, side-effects invisible to clients. Pure bulk code (good code) then time code on top

Anonymous functions and concise lambda syntax: x => x * 2 is a valid function expression in JavaScript. Concise lambdas make it easier to work with higher-order functions.

Often when you use a map you want an array, but with quick access via one key

keyword-string is valuable. Text = user, String = 8bit data, keyword = hash/string pair

Diffing is hard to get right. Caching and invalidating caches are hard to get right. Never do it off the cuff inside other code, always externalize it. Use off-the-shelf mechanisms

Simple to make dll or exe with standard args. Or start Floyd app in server runtime on the fly.


Idea for making tests of pure functions: run your app and tool collects all ins/outs of your function, collapses and sorts. You can just check the tests you want to keep

Feature that updates a collection to same topology of another collection. Use to "decorate" or match a lowlevel collection with UI data.

All values are always futures = composable in time!

User can create and debug boards on iPhone. Apps live on github. Tools work directly on GitHub. 

Source code can have compile time binds to external files containing ID:s like resource files.

Floyd though: GetHughStruct().only.access.subset => only evaluate what's needed to calculate .subset

Make chip for runtime stacks

Make chip for git-style storage.

Make chip for prefetch, jit, read cache, write cache.

Make chip for g2 caching, hashing, stackable ram-cache-algo-disk-s3



Use JSON conceptually as the only values. Internally we can keep it static / structs / binary. Use "RESTFul" concept in language = composable.

Built-In RT profiling

Fast diff. Compare or track? Research git

Source code tweaker + profiler. How to encode? Profiler records stats like big O and DS-usage on high level - not by running code on hw.

Diffing-broadcasting (record changes compared to base version?), sockets, ui, FS. Referenser vs by-value.






Remove file, file system, version and load/restore from apps and put in runtime

Special part for automatic validated JSON serialization / deserialisation, debugging

Special part for binary serialisation / deserialisation, debugging

Special part that represents an external resource (open file, socket, window). Or make snapshot and diff the interface between simulation and the world?


#Playgrounds

A playground is an interactive Swift coding environment that evaluates each statement and displays results as updates are made, without the need to compile and run a project.

Use playgrounds to learn and explore Swift, prototype parts of your app, and create learning environments for others. The interactive Swift environment lets you experiment with algorithms, explore system APIs, and even create custom views, all without the need to create a project.

Share your learning with others by adding notes and guidance using rich comments. Create an explorable learning environment by grouping related concepts into pages and adding navigation

# RoundtripPersistentTree 
Create C++ collection class (not templetized) that supports JSON roundtrip (maybe extensions, hints) and is persistent.

Use as data model of apps.

Simple diff, simple server sync / diff.
Merge

# Ternary operators 
operate on three targets. Like C, Swift has only one ternary operator, the ternary conditional operator (a ? b : c). 

Use instead of switches, with several ranges. Like pattern matching.

let a = (signal ? _ == 0 ? calc_zero(), _ >= 0 && < _< 0.1)

