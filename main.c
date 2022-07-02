#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <pwd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/statvfs.h>
#define error(i, ...) { fprintf(stderr, ##__VA_ARGS__); return i; }
#define OS "/etc/os-release"
#define BLOCK 1024
#define KEY "\x1b[38;5;14m"
#define SEPARATOR "\x1b[38;5;7m ~ \x1b[0m"
#define SLASH "\x1b[38;5;7m / \x1b[0m"
#define RESET "\x1b[0m"
char* display_bytes(double bytes) {
	if (bytes == 0) return "0";
	const char *prefixes[] = { "", "K", "M", "G", "T", "P", "E", "Z", "Y" };
	uint8_t t = 0;
	while (bytes >= 1024) {
		bytes /= 1024;
		++t;
	}
	char* z = malloc(24);
	sprintf(z, "%.1f%s", bytes, prefixes[t]);
	return z;
}
char* display_time(long seconds) {
	long weeks   = 0;
	unsigned int  days    = 0;
	unsigned int  hours   = 0;
	unsigned int  minutes = 0;
	while (seconds >= 3600l*24l*7l) { seconds -= 3600l*24l*7l; ++weeks; }
	while (seconds >= 3600l*24l)    { seconds -= 3600l*24l;    ++days; }
	while (seconds >= 3600l)        { seconds -= 3600l;        ++hours; }
	while (seconds >= 60l)          { seconds -= 60l;          ++minutes; }
	char* string = malloc(100);
	if (weeks > 0)        sprintf(string, "%liw %id %ih %im %lis", weeks, days, hours, minutes, seconds);
	else if (days > 0)    sprintf(string,      "%id %ih %im %lis",        days, hours, minutes, seconds);
	else if (hours > 0)   sprintf(string,          "%ih %im %lis",              hours, minutes, seconds);
	else if (minutes > 0) sprintf(string,              "%im %lis",                     minutes, seconds);
	else                  sprintf(string,                  "%lis",                              seconds);
	return string;
}
int main() {
	uid_t uid = getuid();
	struct passwd* pw = getpwuid(uid);
	if (!pw) { fprintf(stderr, "getpwuid: %i: %s\n", uid, strerror(errno)); return errno; }
	struct utsname un;
	uname(&un);
	struct sysinfo si;
	sysinfo(&si);
	char* sh = getenv("SHELL");
	char* lang = getenv("LANG");
	if (!sh) sh = pw->pw_shell;
	char* sh_ = strrchr(sh, '/');
	if (!sh_) sh_ = sh; else ++sh_;
	FILE* fp = fopen(OS, "r");
	if (!fp) { fprintf(stderr, "%s: %s\n", OS, strerror(errno)); return errno; }
	char** os = NULL;
	size_t oslen = 0;
	size_t zlen;
	while (1) {
		char* z = NULL;
		zlen = 1;
		int c;
		while (1) {
			c = fgetc(fp);
			if (c == EOF || c == '\n' || c == '\r' || c == '\0')
				break;
			z = realloc(z, ++zlen);
			z[zlen-2] = c;
			z[zlen-1] = '\0';
		}
		if (c == EOF) break;
		if (zlen == 0) continue;
		++oslen;
		os = realloc(os, sizeof(char*)*oslen);
		os[oslen-1] = z;
	}
	char* NAME;
	uint8_t is_pretty = 0;
	for (size_t i = 0; i < oslen; ++i) {
		char* eq = strrchr(os[i], '=');
		if (eq) {
			if (!is_pretty && strncmp(os[i], "NAME", eq-os[i]) == 0) { NAME = eq+1; continue; }
			if (strncmp(os[i], "PRETTY_NAME", eq-os[i]) == 0) { NAME = eq+1; is_pretty = 1; continue; }
		}
		free(os[i]);
	}
	struct statvfs vfs;
	char* root = "/";
	if (statvfs(root, &vfs) < 0) { fprintf(stderr, "statvfs: %s: %s\n", root, strerror(errno)); return errno; }
	if (NAME != NULL) {
		size_t len = strlen(NAME);
		if ((NAME[len-1] == '"' && NAME[0] == '"') || (NAME[len-1] == '\'' && NAME[0] == '\'')) {
			NAME[len-1] = '\0';
			++NAME;
		}
		printf(KEY"distro"SEPARATOR"%s"RESET"\n", NAME);
	}
	printf(KEY"    os"SEPARATOR"%s %s %s"RESET"\n", un.sysname, un.machine, un.release);
	printf(KEY"  user"SEPARATOR"%s"RESET"\n", pw->pw_name);
	printf(KEY"  host"SEPARATOR"%s"RESET"\n", un.nodename);
	printf(KEY" shell"SEPARATOR"%s"RESET"\n", sh_);
	printf(KEY"locale"SEPARATOR"%s"RESET"\n", lang);
	printf(KEY"uptime"SEPARATOR"%s"RESET"\n", display_time(si.uptime));
	printf(KEY"  proc"SEPARATOR"%i"RESET"\n", si.procs);
	printf(KEY"   ram"SEPARATOR"%s"SLASH"%s"RESET"\n", display_bytes( si.totalram  - si.freeram),                  display_bytes(si.totalram));
	printf(KEY"  swap"SEPARATOR"%s"SLASH"%s"RESET"\n", display_bytes( si.totalswap - si.freeswap),                 display_bytes(si.totalswap));
	printf(KEY" inode"SEPARATOR"%s"SLASH"%s"RESET"\n", display_bytes( vfs.f_files  - vfs.f_ffree),                 display_bytes(vfs.f_files));
	printf(KEY" block"SEPARATOR"%s"SLASH"%s"RESET"\n", display_bytes((vfs.f_blocks - vfs.f_bfree) * vfs.f_frsize), display_bytes(vfs.f_blocks * vfs.f_frsize));
	return 0;
}
