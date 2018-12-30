#define _XOPEN_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
time_t *get_time_t(const char *time_str)
{
	static time_t time_t = 0;
	struct tm tm;
	if (strptime(time_str, "%Y-%m-%d %H:%M:%S", &tm) == NULL) {
		return NULL;
	}
	time_t = mktime(&tm);
	return &time_t;
}

char *get_time_cstr(time_t t)
{
	static char time_cstr[200];
	const char *format = "%Y-%m-%d %H:%M:%S";
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
int main(int argc, char *argv[])
{
	const char *time_details = "2018-12-29 11:13:25";
	time_t *ptr = get_time_t(time_details);
	if (ptr != NULL) {
		printf("%ld\n", (long) *ptr);
	} else {
		fprintf(stderr, "time conversion error\n");
	}

	time_t t = time(NULL);
	char *time_cstr = get_time_cstr(t);
	fprintf(stderr, "time string: %s\n", time_cstr);

	/*
	const char *format = "%Y-%m-%d %H:%M:%S";
	char outstr[200];
	time_t t;
	struct tm *tmp;

	t = time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL) {
	    perror("localtime");
	    exit(-1);
	}

	if (strftime(outstr, sizeof(outstr), format, tmp) == 0) {
	    fprintf(stderr, "strftime returns 0");
	    exit(-2);
	}

	printf("Result string is \"%s\"\n", outstr);
	exit(0);
	*/
}