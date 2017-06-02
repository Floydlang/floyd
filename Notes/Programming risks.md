


#COMMON AND RISKY CODE

- Store reference to mutable data as optimization
- Caching, precalculating, lazy evaluation. Keep cache correct.
- Many paths to storing the same output property
- Several functions with parasitic behaviour between them (Ex: two functions both updating the same dialog box fields, from different user inputs. Both updates same field).
- Mutation
- Store data to avoid recomputation
- Incremental changes over object with invariant over time -- instead: compute correct object each time.
- Callbacks, observers that have side effects. Instead: no side effects. Make complete change, then diff new value with old value and 
- File systems: many unknown clients
- Keep two parallell systems in sync: data model graph, server database, view hiearchy. Store each in a variable. Use diff and merge.

- Pointers and backlinks. Weak and strong.