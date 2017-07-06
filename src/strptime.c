#include <mruby.h>
#include <stdio.h>
#include <ctype.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include "mruby/range.h"
#include "mruby/array.h"
#include "mruby/class.h"
#include "mruby/string.h"
#include "mruby/variable.h"

static int
num_pattern_p(const char *s)
{
    if (ISDIGIT((unsigned char)*s))
	return 1;
    if (*s == '%') {
	s++;
	if (*s == 'E' || *s == 'O')
	    s++;
	if (*s &&
	    (strchr("CDdeFGgHIjkLlMmNQRrSsTUuVvWwXxYy", *s) ||
	     ISDIGIT((unsigned char)*s)))
	    return 1;
    }
    return 0;
}

#define NUM_PATTERN_P() num_pattern_p(&fmt[fi + 1])

#define fail() \
{ \
    return 0; \
}

static const char *day_names[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday",
    "Sun", "Mon", "Tue", "Wed",
    "Thu", "Fri", "Sat"
};

static const char *month_names[] = {
    "January", "February", "March", "April",
    "May", "June", "July", "August", "September",
    "October", "November", "December",
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char *merid_names[] = {
    "am", "pm",
    "a.m.", "p.m."
};

static const char *extz_pats[] = {
    ":z",
    "::z",
    ":::z"
};
#define sizeof_array(o) (sizeof o / sizeof o[0])

#define issign(c) ((c) == '-' || (c) == '+')

static long
read_digits(mrb_state *mrb, const char *s, mrb_value *n, size_t width)
{
    size_t l;

    l = strspn(s, "0123456789");

    if (l == 0)
	return 0;

    if (width < l)
	l = width;

    if ((4 * l * sizeof(char)) <= (sizeof(long)*CHAR_BIT)) {
	const char *os = s;
	long v;

	v = 0;
	while ((size_t)(s - os) < l) {
	    v *= 10;
	    v += *s - '0';
	    s++;
	}
	if (os == s)
	    return 0;
    *n = mrb_fixnum_value(v);
	return l;
    }
    else {
//  TODO: use alloc
//	mrb_value vbuf = mrb_fixnum_value(0);
//	char *s2 = ALLOCV_N(char, vbuf, l + 1);
	char *s2 = NULL;
	memcpy(s2, s, l);
	s2[l] = '\0';
	*n = mrb_str_to_inum(mrb, mrb_str_new_cstr(mrb, s2), 10, 0);
//	ALLOCV_END(vbuf);
	return l;
    }
}

#define READ_DIGITS(n,w) \
{ \
    size_t l; \
    l = read_digits(mrb, &str[si], &n, w); \
    if (l == 0) \
	fail();	\
    si += l; \
}

static int
valid_range_p(mrb_state *mrb, mrb_value v, int a, int b)
{
    if (mrb_fixnum_p(v)) {
	int vi = mrb_fixnum(v);
	return !(vi < a || vi > b);
    }
    const char c = '<';
    const char d = '>';
    return !(mrb_bool(mrb_funcall(mrb, v, &c, 1, mrb_fixnum_value(a))) || mrb_bool(mrb_funcall(mrb, v, &d, 1, mrb_fixnum_value(b))));
}

#define recur(fmt) \
{ \
    size_t l; \
    l = date__strptime_internal(mrb, &str[si], slen - si, fmt, sizeof fmt - 1, hash); \
    si += l; \
}

mrb_value date_zone_to_diff(mrb_value);

#define READ_DIGITS_MAX(n) READ_DIGITS(n, LONG_MAX)

static size_t
date__strptime_internal(mrb_state *mrb, const char *str, size_t slen, const char *fmt, size_t flen, mrb_value hash)
{

size_t si, fi;
  int c;

  si = fi = 0;

  while (fi < flen) {

  	switch (fmt[fi]) {
  	  case '%':

  	  again:
  	    fi++;
  	    c = fmt[fi];

  	    switch (c) {
  	      case 'E':
  		if (fmt[fi + 1] && strchr("cCxXyY", fmt[fi + 1]))
  		    goto again;
  		fi--;
  		goto ordinal;
  	      case 'O':
  		if (fmt[fi + 1] && strchr("deHImMSuUVwWy", fmt[fi + 1]))
  		    goto again;
  		fi--;
  		goto ordinal;
  	      case ':':
  		{
  		    int i;

  		    for (i = 0; i < (int)sizeof_array(extz_pats); i++)
  			if (strncmp(extz_pats[i], &fmt[fi],
  					strlen(extz_pats[i])) == 0) {
  			    fi += i;
  			    goto again;
  			}
  		    fail();
  		}

  	      case 'A':
  	      case 'a':
  		{
  		    int i;

  		    for (i = 0; i < (int)sizeof_array(day_names); i++) {
  			size_t l = strlen(day_names[i]);
  			if (strncasecmp(day_names[i], &str[si], l) == 0) {
  			    si += l;
  			    mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "wday" ), mrb_fixnum_value(i % 7));
  			    goto matched;
  			}
  		    }
  		    fail();
  		}
  	      case 'B':
  	      case 'b':
  	      case 'h':
  		{
  		    int i;

  		    for (i = 0; i < (int)sizeof_array(month_names); i++) {
  			size_t l = strlen(month_names[i]);
  			if (strncasecmp(month_names[i], &str[si], l) == 0) {
  			    si += l;
  			    mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "mon" ), mrb_fixnum_value((i % 12) + 1));
  			    goto matched;
  			}
  		    }
  		    fail();
  		}

  	      case 'C':
  		{
  		    mrb_value n;

  		    if (NUM_PATTERN_P())
  			READ_DIGITS(n, 2)
  		    else
  			READ_DIGITS_MAX(n)
  		    mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "_cent" ), n);
  		    goto matched;
  		}

  	      case 'c':
  		recur("%a %b %e %H:%M:%S %Y");
  		goto matched;

  	      case 'D':
  		recur("%m/%d/%y");
  		goto matched;

  	      case 'd':
  	      case 'e':
  		{
  		    mrb_value n;

  		    if (str[si] == ' ') {
  			si++;
  			READ_DIGITS(n, 1);
  		    } else {
  			READ_DIGITS(n, 2);
  		    }
  		    if (!valid_range_p(mrb, n, 1, 31))
  			fail();
  		    mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "mday" ), n);
  		    goto matched;
  		}

  	      case 'F':
  		recur("%Y-%m-%d");
  		goto matched;

  	      case 'G':
  		{
  		    mrb_value n;

  		    if (NUM_PATTERN_P())
  			READ_DIGITS(n, 4)
  		    else
  			READ_DIGITS_MAX(n)
  		    mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "cwyear" ), n);
  		    goto matched;
  		}

  	      case 'g':
  		{
  		    mrb_value n;

  		    READ_DIGITS(n, 2);
  		    if (!valid_range_p(mrb, n, 0, 99))
  			fail();
  		    mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "cwyear" ), n);
  		    if (mrb_nil_p(mrb_hash_get(mrb, hash, mrb_str_new_cstr( mrb, "_cent" ))))
  		        mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "_cent" ),  mrb_fixnum_value( mrb_bool(mrb_funcall(mrb, n, ">=", 1, mrb_fixnum_value(69))) ? 19 : 20));
  		    goto matched;
  		}

  	      case 'H':
  	      case 'k':
  		{
  		    mrb_value n;

  		    if (str[si] == ' ') {
  			si++;
  			READ_DIGITS(n, 1);
  		    } else {
  			READ_DIGITS(n, 2);
  		    }
  		    if (!valid_range_p(mrb, n, 0, 24))
  			fail();
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "hour" ), n);
  		    goto matched;
  		}

  	      case 'I':
  	      case 'l':
  		{
  		    mrb_value n;

  		    if (str[si] == ' ') {
  			si++;
  			READ_DIGITS(n, 1);
  		    } else {
  			READ_DIGITS(n, 2);
  		    }
  		    if (!valid_range_p(mrb, n, 1, 12))
  			fail();
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "hour" ), n);
  		    goto matched;
  		}

  	      case 'j':
  		{
  		    mrb_value n;

  		    READ_DIGITS(n, 3);
  		    if (!valid_range_p(mrb, n, 1, 366))
  			fail();
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "yday" ), n);
  		    goto matched;
  		}

  	      case 'L':
  	      case 'N':
  		{
  		    mrb_value n;
  		    int sign = 1;
  		    size_t osi;

  		    if (issign(str[si])) {
  			if (str[si] == '-')
  			    sign = -1;
  			si++;
  		    }
  		    osi = si;
  		    if (NUM_PATTERN_P())
  			READ_DIGITS(n, c == 'L' ? 3 : 9)
  		    else
  			READ_DIGITS_MAX(n)
  		    if (sign == -1)
  		        mrb_funcall(mrb, n, "-@", 0);
//  		    set_hash("sec_fraction",rb_rational_new2(n,f_expt(INT2FIX(10),ULONG2NUM(si - osi))));
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "sec_fraction" ), mrb_str_new_cstr( mrb, "unsupported" ));
  		    goto matched;
  		}

  	      case 'M':
  		{
  		    mrb_value n;

  		    READ_DIGITS(n, 2);
  		    if (!valid_range_p(mrb, n, 0, 59))
  			fail();
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "min" ), n);
  		    goto matched;
  		}

  	      case 'm':
  		{
  		    mrb_value n;

  		    READ_DIGITS(n, 2);
  		    if (!valid_range_p(mrb, n, 1, 12))
  			fail();
  		    mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "mon" ), n);
  		    goto matched;
  		}

  	      case 'n':
  	      case 't':
  		recur(" ");
  		goto matched;

  	      case 'P':
  	      case 'p':
  		{
  		    int i;

  		    for (i = 0; i < 4; i++) {
  			size_t l = strlen(merid_names[i]);
  			if (strncasecmp(merid_names[i], &str[si], l) == 0) {
  			    si += l;
                mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "_merid" ), mrb_fixnum_value((i % 2) == 0 ? 0 : 12));
  			    goto matched;
  			}
  		    }
  		    fail();
  		}

  	      case 'Q':
  		{
  		    mrb_value n;
  		    int sign = 1;

  		    if (str[si] == '-') {
  			sign = -1;
  			si++;
  		    }
  		    READ_DIGITS_MAX(n);
  		    if (sign == -1)
  		        mrb_funcall(mrb, n, "-@", 0);
//  		    set_hash("seconds",rb_rational_new2(n,f_expt(INT2FIX(10),INT2FIX(3))));
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "wday" ), mrb_str_new_cstr( mrb, "unsupported" ));
  		    goto matched;
  		}

  	      case 'R':
  		recur("%H:%M");
  		goto matched;

  	      case 'r':
  		recur("%I:%M:%S %p");
  		goto matched;

  	      case 'S':
  		{
  		    mrb_value n;

  		    READ_DIGITS(n, 2);
  		    if (!valid_range_p(mrb, n, 0, 60))
  			fail();
  		    mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "sec" ), n);
  		    goto matched;
  		}

  	      case 's':
  		{
  		    mrb_value n;
  		    int sign = 1;

  		    if (str[si] == '-') {
  			sign = -1;
  			si++;
  		    }
  		    READ_DIGITS_MAX(n);
  		    if (sign == -1)
  		        mrb_funcall(mrb, n, "-@", 0);
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "seconds" ), n);
  		    goto matched;
  		}

  	      case 'T':
  		recur("%H:%M:%S");
  		goto matched;

  	      case 'U':
  	      case 'W':
  		{
  		    mrb_value n;

  		    READ_DIGITS(n, 2);
  		    if (!valid_range_p(mrb, n, 0, 53))
  			fail();
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, c == 'U' ? "wnum0" : "wnum1" ), n);
  		    goto matched;
  		}

  	      case 'u':
  		{
  		    mrb_value n;

  		    READ_DIGITS(n, 1);
  		    if (!valid_range_p(mrb, n, 1, 7))
  			fail();
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "cwday" ), n);
  		    goto matched;
  		}

  	      case 'V':
  		{
  		    mrb_value n;

  		    READ_DIGITS(n, 2);
  		    if (!valid_range_p(mrb, n, 1, 53))
  			fail();
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "cweek" ), n);
  		    goto matched;
  		}

  	      case 'v':
  		recur("%e-%b-%Y");
  		goto matched;

  	      case 'w':
  		{
  		    mrb_value n;

  		    READ_DIGITS(n, 1);
  		    if (!valid_range_p(mrb, n, 0, 6))
  			fail();
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "wday" ), n);
  		    goto matched;
  		}

  	      case 'X':
  		recur("%H:%M:%S");
  		goto matched;

  	      case 'x':
  		recur("%m/%d/%y");
  		goto matched;

  	      case 'Y':
  		  {
  		      mrb_value n;
  		      int sign = 1;

  		      if (issign(str[si])) {
  			  if (str[si] == '-')
  			      sign = -1;
  			  si++;
  		      }
  		      if (NUM_PATTERN_P())
  			  READ_DIGITS(n, 4)
  		      else
  			  READ_DIGITS_MAX(n)
  		    if (sign == -1)
  		        mrb_funcall(mrb, n, "-@", 0);
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "year" ), n);
  		      goto matched;
  		  }

  	      case 'y':
  		{
  		    mrb_value n;
  		    int sign = 1;

  		    READ_DIGITS(n, 2);
  		    if (!valid_range_p(mrb, n, 0, 99))
  			fail();
  		    if (sign == -1)
  		        mrb_funcall(mrb, n, "-@", 0);
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "year" ), n);
  		    if (mrb_nil_p(mrb_hash_get(mrb, hash, mrb_str_new_cstr( mrb, "_cent" ))))
                mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "_cent" ),  mrb_fixnum_value( mrb_bool(mrb_funcall(mrb, n, ">=", 1, mrb_fixnum_value(69))) ? 19 : 20));
  		    goto matched;
  		}

  	      case 'Z':
  	      case 'z':
  		{
  		        struct RClass *onig;
  		        onig = mrb_class_get(mrb, "OnigRegexp");
                mrb_value onig_value = mrb_obj_value(onig);

                static const char pat_source[] =
                "\\A("
                "(?:gmt|utc?)?[-+]\\d+(?:[,.:]\\d+(?::\\d+)?)?"
                "|(?-i:[[:alpha:].\\s]+)(?:standard|daylight)\\s+time\\b"
                "|(?-i:[[:alpha:]]+)(?:\\s+dst)?\\b"
                ")";

                mrb_value pat = mrb_str_new(mrb, pat_source, sizeof pat_source - 1);

                mrb_value obj = mrb_funcall(mrb, onig_value, "new", 2, pat, mrb_fixnum_value(1));

                mrb_value kawa = mrb_str_new(mrb, &str[si], sizeof &str[si]);

                mrb_value matched = mrb_funcall(mrb, obj, "match", 1, kawa);

  		    if (!mrb_nil_p(matched)) {
  		    mrb_value s, l, o;
  		    mrb_value matched_array = mrb_funcall(mrb, matched, "to_a", 0);
            s = mrb_ary_entry(matched_array, 0);
      		l = mrb_funcall(mrb, matched, "end", 1, mrb_fixnum_value(0));
            o = s;
      		si += mrb_fixnum(l);
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "zone" ), s);
            mrb_hash_set(mrb, hash, mrb_str_new_cstr( mrb, "offset" ), o);
//  		rb_backref_set(b);
  			goto matched;
  		    }
//  		rb_backref_set(b);
  		    fail();
  		}

  	      case '%':
  		if (str[si] != '%')
  		    fail();
  		si++;
  		goto matched;

  	      case '+':
  		recur("%a %b %e %H:%M:%S %Z %Y");
  		goto matched;

  	      default:
  		if (str[si] != '%')
  		    fail();
  		si++;
  		if (fi < flen)
  		    if (str[si] != fmt[fi])
  			fail();
  		si++;
  		goto matched;
  	    }
  	  case ' ':
  	  case '\t':
  	  case '\n':
  	  case '\v':
  	  case '\f':
  	  case '\r':
  	    while (isspace((unsigned char)str[si]))
  		si++;
  	    fi++;
  	    break;
  	  default:
  	  ordinal:
  	    if (str[si] != fmt[fi])
  		fail();
  	    si++;
  	    fi++;
  	    break;
  	  matched:
  	    fi++;
  	    break;
  	}
      }

      return si;
}

static mrb_value
parse_method(mrb_state *mrb, mrb_value self)
{
    char *str;
    static const char *fmt = "%FT%T%z";

    mrb_get_args(mrb, "s", &str);
    mrb_value hash = mrb_hash_new(mrb);

    date__strptime_internal(mrb, str, strlen(str), fmt, strlen(fmt), hash);

    return hash;
}

static mrb_value
strptime_initialize(mrb_state *mrb, mrb_value self)
{
    char *str;
    static const char *fmt = "%FT%T%z";

    mrb_get_args(mrb, "s", &str);
    mrb_value time_hash = mrb_hash_new(mrb);

    date__strptime_internal(mrb, str, strlen(str), fmt, strlen(fmt), time_hash);

    mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@time_hash"), time_hash);
    mrb_value offset = mrb_hash_get(mrb, time_hash, mrb_str_new_cstr( mrb, "offset" ));
    mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@offset"), offset);

    return self;
}

static mrb_value
strptime_to_time_method(mrb_state *mrb, mrb_value self)
{
    mrb_value time_hash = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@time_hash"));

    mrb_value year = mrb_hash_get(mrb, time_hash, mrb_str_new_cstr( mrb, "year" ));
    mrb_value mon = mrb_hash_get(mrb, time_hash, mrb_str_new_cstr( mrb, "mon" ));
    mrb_value mday = mrb_hash_get(mrb, time_hash, mrb_str_new_cstr( mrb, "mday" ));
    mrb_value hour = mrb_hash_get(mrb, time_hash, mrb_str_new_cstr( mrb, "hour" ));
    mrb_value min = mrb_hash_get(mrb, time_hash, mrb_str_new_cstr( mrb, "min" ));
    mrb_value sec = mrb_hash_get(mrb, time_hash, mrb_str_new_cstr( mrb, "sec" ));

    struct RClass *time_class;
    time_class = mrb_class_get(mrb, "Time");
    mrb_value time_obj = mrb_obj_value(time_class);
    mrb_value time;

    if (mrb_nil_p(year)) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Invalid time value");
    } else if (mrb_nil_p(mon)) {
        time = mrb_funcall(mrb, time_obj, "new", 1, year);
    } else if (mrb_nil_p(mday)) {
        time = mrb_funcall(mrb, time_obj, "new", 2, year, mon);
    } else if (mrb_nil_p(hour)) {
        time = mrb_funcall(mrb, time_obj, "new", 3, year, mon, mday);
    } else if (mrb_nil_p(min)) {
        time = mrb_funcall(mrb, time_obj, "new", 4, year, mon, mday, hour);
    } else if (mrb_nil_p(sec)) {
        time = mrb_funcall(mrb, time_obj, "new", 5, year, mon, mday, hour, min);
    } else {
        time = mrb_funcall(mrb, time_obj, "new", 6, year, mon, mday, hour, min, sec);
    }

    return time;
}

static mrb_value
strptime_to_i_method(mrb_state *mrb, mrb_value self)
{
    mrb_value time = strptime_to_time_method(mrb, self);

    mrb_value time_int_without_offset = mrb_funcall(mrb, time, "to_i", 0);

    mrb_value offset = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@offset"));

    if (!mrb_nil_p(offset)) {
        mrb_value ope = mrb_funcall(mrb, offset, "slice!", 2, mrb_fixnum_value(0), mrb_fixnum_value(1));
        if(strcmp(RSTRING_PTR(ope), "+") && strcmp(RSTRING_PTR(ope), "-"))
        {
            return time_int_without_offset;
        }
        mrb_value offset_array = mrb_funcall(mrb, offset, "split", 1, mrb_str_new_cstr(mrb, ":"));
        mrb_value offset_hour = mrb_ary_entry(offset_array, 0);
        mrb_value offset_min = mrb_ary_entry(offset_array, 1);

        mrb_value offset_hour_i = mrb_funcall(mrb, offset_hour, "to_i", 0);
        mrb_value offset_min_i = mrb_funcall(mrb, offset_min, "to_i", 0);
        mrb_value offset_hour_i_sec = mrb_funcall(mrb, offset_hour_i, "*", 1, mrb_fixnum_value(3600));
        mrb_value offset_min_i_sec = mrb_funcall(mrb, offset_min_i, "*", 1, mrb_fixnum_value(60));

        mrb_value total_offset = mrb_funcall(mrb, offset_hour_i_sec, "+", 1, offset_min_i_sec);
        mrb_value time_int = mrb_funcall(mrb, time_int_without_offset, RSTRING_PTR(ope), 1, total_offset);

        return time_int;

    } else {
        return time_int_without_offset;
    }
}

static mrb_value
strptime_to_i_without_offset_method(mrb_state *mrb, mrb_value self)
{
    mrb_value time = strptime_to_time_method(mrb, self);

    mrb_value time_int = mrb_funcall(mrb, time, "to_i", 0);

    return time_int;
}

void
mrb_mruby_strptime_gem_init(mrb_state* mrb) {
    struct RClass *strptime_class = mrb_define_class(mrb, "Strptime", mrb->object_class);
    mrb_define_class_method(mrb, strptime_class, "parse", parse_method, MRB_ARGS_REQ(1));
    mrb_define_method(mrb, strptime_class, "initialize", strptime_initialize, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
    mrb_define_method(mrb, strptime_class, "to_time", strptime_to_time_method, MRB_ARGS_NONE());
    mrb_define_method(mrb, strptime_class, "to_i", strptime_to_i_method, MRB_ARGS_NONE());
    mrb_define_method(mrb, strptime_class, "to_i_without_offset", strptime_to_i_without_offset_method, MRB_ARGS_NONE());
}

void
mrb_mruby_strptime_gem_final(mrb_state* mrb) {
  /* finalizer */
}
