# mruby-strptime

## install by mrbgems
```ruby
MRuby::Build.new do |conf|

    # ... (snip) ...

    conf.gem :github => 'naritta/mruby-strptime'
end
```

## Example
```ruby
# parsed hash
Strptime.parse('2017-07-07T23:59:01+0700'') 
# => {"year"=>2017, "mon"=>7, "mday"=>7, "hour"=>23, "min"=>59, "sec"=>1, "zone"=>"+0700", "offset"=>"+0700"}

# unix time included timezone
Strptime.new('2017-07-07T23:59:01').to_i
# => 1499439541

# unix time included timezone
Strptime.new('2017-07-07T23:59:01+0700').to_i
# => 1501959541

# time without timezone
Strptime.new('2017-07-07T23:59:01').to_time
# => Fri Jul 07 23:59:01 2017 
 
```

## License

MIT