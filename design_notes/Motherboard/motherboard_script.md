#	IMPORT

GUI
	DEST gui_in
	DEST invalidate


vsthost
	gui
		<= request_param
		=> midi_in
		=> set_param
	realtime
		=> process
		=> processReplacing


UI Part



#	Wiring


```

UI_part.events		<---	func { return uievent($0.time * 1000, $0.type) }	<---	gui.gui_in
vsthost.gui.request_param	<---	transform { mode = pull_sample }		<---	UI_part.request_param
audio_part.control		<---	queue { size= 256 }		<---
	SUM(
		func { return midi_to_control($0) }		<---	vsthost.midi_in
		func { return vst_to_control($0) }		<---	vsthost.set_param
		UI_part.control
	)

audio_part.audio	<---	queue { size = 1024 }	<---	vsthost.process

{
	"wires": [
		[
			"UI_part.events",
			["func", [ return uievent($0.time * 1000, $0.type)]],
			"gui.gui_in"
		],
		[
			"vsthost.gui.request_param",
			["transform", [ mode = pull_sample ]],
			"UI_part.request_param"
		],
		[ "audio_part.control", "queue { size= 256 }", [ "SUM",	[
			["func", [ return midi_to_control($0) ]		<---	vsthost.midi_in]
			["func", return vst_to_control($0) }		<---	vsthost.set_param]
			[UI_part.control]
		]]],
		[ "audio_part.audio", "queue { size = 1024 }", "vsthost.process" ]
	]
}
```


# ALT: Use recursion to make a tree.

```
	{
		"wires": [
			{ "dest": "UI_part.events", "sources": {
				"type": "func", "body": [], "sources": []
			} }
		]
	}
```