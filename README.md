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
Strptime.parse('2017-07-07T23:59:01') 
# => {"year"=>2017, "mon"=>7, "mday"=>7, "hour"=>23, "min"=>59, "sec"=>1}

# unix time included timezone
Strptime.new('2017-07-07T23:59:01').to_i
# => 1499439541

# time without timezone
Strptime.new('2017-07-07T23:59:01').to_time
# => Fri Jul 07 23:59:01 2017 
 
```

## License

MIT