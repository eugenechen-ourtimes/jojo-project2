#ifndef _TIME_UTILS_HPP_
#define _XOPEN_SOURCE
#define _TIME_UTILS_HPP_

#include <stdio.h>
#include <time.h>

class TimeUtils
{
	public:
		static const char *format;

		static time_t *get_time_t(const char *time_cstr)
		{
			if (time_cstr == NULL) return NULL;
			struct tm tm;
			if (strptime(time_cstr, format, &tm) == NULL) return NULL;
			t = mktime(&tm);
			return &t;
		}

		static char *get_time_cstr(time_t t)
		{
			struct tm *tm = localtime(&t);
			if (tm == NULL) {
			    perror("localtime");
			    return NULL;
			}

			if (strftime(time_cstr, sizeof(time_cstr), format, tm) == 0) {
			    fprintf(stderr, "strftime returns 0");
			    return NULL;
			}

			return time_cstr;
		}

		static void showError()
		{
			fprintf(stderr, "time conversion error\n");
		}

	private:
		static time_t t;
		static char time_cstr[32];
};

const char *TimeUtils::format = "%Y/%m/%d--%H:%M:%S";
#endif