MRuby::Gem::Specification.new('mruby-strptime') do |spec|
  spec.license = 'MIT'
  spec.author  = 'Ritta Narita'

  spec.add_dependency('mruby-onig-regexp', :github => 'mattn/mruby-onig-regexp')
end
