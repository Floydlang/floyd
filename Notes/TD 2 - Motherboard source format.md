








REFS
- hjson?
- DOT http://www.graphviz.org/doc/info/lang.html
- http://msgpack.org/
- EDN format: https://github.com/edn-format/edn
http://stobml.org/
https://aaronparecki.com/2015/01/22/8/why-not-json
- YAML
- toml: https://github.com/toml-lang/toml
- Lua
- Golang


ALTERNATIVES

A) Custom file format
B) Custom file format, embedd JSON and Markdown
C) Use HJSON
D) Use JSON




# DOT-style graph
digraph G {
	2: main -> parse -> execute;
	3: main -> init;
	4: main -> cleanup;
	5: execute -> make_string;
	6: execute -> printf
	7: init -> make_string;
	8: main -> printf;
	9: execute -> compare
}




on_click(int time, int button, int x, int y, int mods)
obuf on_audio_buffer_switch(ibuf b)
int on_clock(int time)
void on_screen_bufferswitch()

void opengl_begin(glc context)
void opengl_end(glc context)
void opengl_push_vertex(glc context, int x, int y)
void opengl_push_texture(glc context, int x, int y)



# DOT-STYLE

motherboard {
	//	Load parts from library
	vsthost: load("vsthost")

	//	Define some channels
	events: channel<uievent>, mode = block
	control: channel<param>, mode = block
	audiostream: channel<ibuf>, mode = block

	//	Define our custom part that handles the UI
	ui_part: code {
		m = ui_state()
		while(){
			select {
				case events.pop() {
					r = on_uievent(m, _)
					if(_.data.type == draw){
						_.reply.push(r._paint_image)
						m = r.m
					}
					else{
						control.push_vec(r.control_params)
						m = r.m
					}
				}
				case close {
					return nil
				}
			}
		}
	}

	//	Define our custom part that handles realtime audio streaming
	audio_stream_part: code {
		m = audio_stream_ds(2, 44100)
		while(){
			select {
				case audiostream.pop() {
					m = process(m, _)
				}
				case control.pop {
					return nil
				}
				case close {
					return nil
				}
			}
		}
	}

	//	Wire things together
	vsthost.midi -> control
	vsthost.gui -> events
	events -> ui_part.uievent
	ui_part.control -> control
	vsthost.set_param -> control
	vsthost.process -> audiostream
	control -> audio_stream_part.control
	audiostream -> audio_stream_part.audio
}



# CUSTOM
		
		defstruct uievent
			time: timestamp
			variant: type
				click
				int: x
				int: y
				int: mouse_button
				uint32: mods
		
				key
				int keycode
				int: x
				int: y
				uint32: mods
		
				draw
				rect: dirty
				channel<obuf> reply
		
				activate
		
				deactivate
		
		defstruct ibuf
			time timestamp
			vector<float>: left_input
			vector<float>: right_input
			channel<obuf>: reply
		
		defstruct obuf
			vector<float>: left_output
			vector<float>: right_output
		
		defstruct param
			int: param_id
			float: time
			float: value
		
		
		
		[Screen frame switch]
		
		[Audio Streamer]
			[= control =>]
				[Audio Controller]
					[= world state =>]
		
		[Pre-renderer]
			[= world state =>]
		
		[Asset Prefetcher]
			[= world state =>]
		
		[= world state =>]
			[World Simulation]
				[= control =>]
					[Player Input Reactor]
						[= events<uievent> =>]
							[OS GUI]
				[= clock =>]
					[OS Clock]
		
