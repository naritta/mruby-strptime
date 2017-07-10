
class ToTimeTest < MTest::Unit::TestCase
  def test_year
    time = Strptime.new('2017').to_time
    assert_equal(time.year, 2017)
  end

  def test_year_to_mon
    time = Strptime.new('2017-08').to_time
    assert_equal(time.year, 2017)
    assert_equal(time.mon, 8)
  end

  def test_year_to_day
    time = Strptime.new('2017-08-09').to_time
    assert_equal(time.year, 2017)
    assert_equal(time.mon, 8)
    assert_equal(time.day, 9)
  end

  def test_year_to_hour
    time = Strptime.new('2001-02-03T04').to_time
    assert_equal(time.year, 2001)
    assert_equal(time.mon, 2)
    assert_equal(time.day, 3)
    assert_equal(time.hour, 4)
  end

  def test_year_to_min
    time = Strptime.new('2001-02-03T04:05').to_time
    assert_equal(time.year, 2001)
    assert_equal(time.mon, 2)
    assert_equal(time.day, 3)
    assert_equal(time.hour, 4)
    assert_equal(time.min, 5)
  end

  def test_year_to_sec
    time = Strptime.new('2001-02-03T04:05:06').to_time
    assert_equal(time.year, 2001)
    assert_equal(time.mon, 2)
    assert_equal(time.day, 3)
    assert_equal(time.hour, 4)
    assert_equal(time.min, 5)
    assert_equal(time.sec, 6)
  end
end

class ToIntTest < MTest::Unit::TestCase
  def test_to_i
    time_i = Strptime.new('2001-02-03T04:05:06').to_i
    assert_equal(time_i, 981140706)
  end

  def test_to_i_with_timezone
    time_i = Strptime.new('2001-02-03T04:05:06+0700').to_i
    assert_equal(time_i, 983660706)
  end
end

if __FILE__ == $0
  MTest::Unit.new.run
end