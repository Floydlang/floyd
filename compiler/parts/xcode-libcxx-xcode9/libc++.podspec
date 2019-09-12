Pod::Spec.new do |s|
  s.name         = "libc++"
  s.version      = "5.0.1"
  s.summary      = "The missing libc++ headers in Xcode9 (Sigh)."
  s.description  = <<-DESC
    It seems Apple forgot to update libc++ in Xcode for a while. So I did.
    The headers here are copied from homebrewed llvm@5 - which corresponding
    to Xcode9.
  DESC
  s.homepage     = "http://github.com/youknowone/xcode-libcxx"
  s.license      = { :type => "UIUC", :file => "CLANG_LICENSE.txt" }
  s.author             = { "Jeong Yunwon" => "jeong@youknowone.org" }
  s.social_media_url   = "https://github.com/youknowone"
  s.source       = { :git => "https://github.com/youknowone/xcode-libcxx.git", :tag => "#{s.version}" }
  s.source_files  = "**", "experimental/**"
  s.exclude_files = "Makefile", "*.podspec", "CLANG_LICENSE.*"
  s.public_header_files = "**", "experimental/**"
end
