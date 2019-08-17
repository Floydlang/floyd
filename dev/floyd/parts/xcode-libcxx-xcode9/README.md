The missing libc++ headers for Xcode9
=====================================

It seems Apple only update clang compiler itself, but not about the standard
library libc++.
Fortunately, missing headers work well by simplly copying from *REAL* LLVM.

This package includes `any`, `optional` and `variant`.


Installing with Cocoapods
-------------------------

Here is an example of Podfile:

    platform :osx, '10.12'

    target 'MyApp' do
        pod 'libc++'
    end


Installing with Makefile
------------------------

Note: This is not recommended. This method will change your Xcode internal
files. Note that this action might break your Xcode or system. Try it by your
own risk.

    # Clone this repository.
    # Run `make brew` to install llvm@5 via homebrew.
    # Run `make install` and follow the instruction.
