#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#define IOBufSize 1024
#define MB 1048576
#define Limit 20
#define MaxFile Limit * MB

#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
int mkdirIfNotExist(const char *dirname)
{
	if (dirname == NULL) {
		fprintf(stderr, "can\'t create directory with null argument.\n");
		return -2;
	}

	DIR *dir = opendir(dirname);
	if (dir != NULL) {
		closedir(dir);
		return 0;
	}

	if (mkdir(dirname, 0777) < 0) {
		perror(dirname);
		return -1;
	}

	return 0;
}

#endif