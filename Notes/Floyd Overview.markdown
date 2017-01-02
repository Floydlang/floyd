1. Floyd script. Pure compiled language. Pure / referential transparent. Built-in vec/map/seq, JSON. Compilter + tool chain.
2. Motherboard. JSON format. Concrete layout. Can be visual. Clocks, optocouplers, sockets, REST, files, UI.
3. Runtime: Optimizing/profiling runtime. Optimization injection, dynamic data structure.



# FLOYD PARTS
- Floyd Script langugage. Pure, typesafe, compiled+interpreted, no refs. JS-like syntax, single assignment SSA
- Persistent DS
- Standard intermediate format in JSON
- Persistent values/structs
- Clocks
- Optocouplers
- Profiling/optimizing runtime. Dynamic collection backends, caching-precalc-readcache, write cache, parrallelization.
- Scriptable runtime params for optimization
- Embeddable

 

# CLIENT CODE
Code that Floyd programmer writes.
- Has no concept of time and order. (Except locally inside a function.)
- Cannot store state
- Pure. Cannot mutate outside world. No side effects.
- Cannot observer the world changing.
- Any call can be replaced by its output value
- 

# INSIGHTS & QUESTIONS

??? Three languages: pure logic, com/process, tweaks?
