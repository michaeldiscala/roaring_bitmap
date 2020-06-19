Gem::Specification.new do |s|
  s.name        = "roaring_bitmap"
  s.version     = "0.0.0"
  s.date        = "2020-06-19"
  s.summary     = "Ruby bindings for RoaringBitmaps"
  s.description = "Ruby bindings for RoaringBitmaps"
  s.authors     = ["Panorama Education"]
  s.email       = "mdiscala@panoramaed.com"
  s.files       = ["lib/roaring_bitmap.rb"]

  s.add_development_dependency "rspec", "~> 3.9"
  s.add_development_dependency "rake", "~> 12.3"
  s.add_development_dependency "rake-compiler", "~> 1.1"
  s.add_development_dependency "benchmark-ips"
end
