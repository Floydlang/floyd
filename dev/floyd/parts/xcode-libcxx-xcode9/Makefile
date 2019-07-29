llvm_path=/usr/local/opt/llvm@5/include/c++/v1
xcode_path=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1
missing_headers=any variant optional experimental/filesystem
updated_headers=deque functional istream list memory mutex random sstream string type_traits vector

all: brew $(missing_headers)
	

experimental_dir:
	-mkdir experimental

$(missing_headers): experimental_dir
	cp /usr/local/opt/llvm@5/include/c++/v1/$@ $@

$(updated_headers): experimental_dir
	cp /usr/local/opt/llvm@5/include/c++/v1/$@ $@

clean:
	rm $(missing_headers) $(updated_headers)

brew:
	# For xcode9
	if [ ! -e /usr/local/opt/llvm@5 ]; then brew install llvm@5; fi

install:
	@echo '# Installing this files directly into your Xcode may be dangerous. Chek, copy & paste next scripts on your own risk. (`sudo` must be required)'
	@echo '###############################################################'
	@for header in $(missing_headers); do \
		echo "ln -s '$(llvm_path)/$$header' '$(xcode_path)/$$header'"; \
	done