Built in tags:
	
	#todo
	#defect
	
	#desc
	#my-tag
	#doc
	#pragma

Use # with any string to make custom tags.

@marcusz -- 

# TITLE
## SUBTITLE

??? a component file can't hold ALL functions and structs -- must support including other source files!?

datetime

      !""#$%&/\[]|()=?+-?`´^~*'<>;:,.




let cities = csv(split = ",", titles = true, quoted = true) = {
	"LatD", "LatM", "LatS", "NS", "LonD", "LonM", "LonS", "EW", "City", "State"
   41,    5,   59, "N",     80,   39,    0, "W", "Youngstown", OH
   42,   52,   48, "N",     97,   23,   23, "W", "Yankton", SD
   46,   35,   59, "N",    120,   30,   36, "W", "Yakima", WA
   42,   16,   12, "N",     71,   48,    0, "W", "Worcester", MA
   43,   37,   48, "N",     89,   46,   11, "W", "Wisconsin Dells", WI
   36,    5,   59, "N",     80,   15,    0, "W", "Winston-Salem", NC
   49,   52,   48, "N",     97,    9,    0, "W", "Winnipeg", MB
   39,   11,   23, "N",     78,    9,   36, "W", "Winchester", VA
   34,   14,   24, "N",     77,   55,   11, "W", "Wilmington", NC
}
˛



GOALS
1. Desc + invariant won't hide actual code
2. Easy to type, no --[[ ]] or /*--- */
3. Familiar to Javascripters
4. Mininal visual clutter: .. is better than /* */


-- comment
# comment
.. comment
// comment
desc comment
desc(comment)

!! invariant
