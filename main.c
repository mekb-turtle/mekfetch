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
#include <stdbool.h>
#define NERD_FONT
#define error(i, ...) { fprintf(stderr, ##__VA_ARGS__); return i; }
#define OS "/etc/os-release"
char *display_bytes(double bytes) {
	if (bytes == 0) return "0";
	const char *prefixes[] = { "", "K", "M", "G", "T", "P", "E", "Z", "Y" };
	uint8_t t = 0;
	while (bytes >= 1024) {
		bytes /= 1024;
		++t;
	}
	char *z = malloc(24);
	sprintf(z, "%.1f%s", bytes, prefixes[t]);
	return z;
}
char *display_time(long seconds) {
	long weeks           = 0;
	unsigned int days    = 0;
	unsigned int hours   = 0;
	unsigned int minutes = 0;
	while (seconds >= 3600l*24l*7l) { seconds -= 3600l*24l*7l; ++weeks; }
	while (seconds >= 3600l*24l)    { seconds -= 3600l*24l;    ++days; }
	while (seconds >= 3600l)        { seconds -= 3600l;        ++hours; }
	while (seconds >= 60l)          { seconds -= 60l;          ++minutes; }
	char *string = malloc(100);
	if (weeks > 0)        sprintf(string, "%liw %id %ih %im %lis", weeks, days, hours, minutes, seconds);
	else if (days > 0)    sprintf(string,      "%id %ih %im %lis",        days, hours, minutes, seconds);
	else if (hours > 0)   sprintf(string,          "%ih %im %lis",              hours, minutes, seconds);
	else if (minutes > 0) sprintf(string,              "%im %lis",                     minutes, seconds);
	else                  sprintf(string,                  "%lis",                              seconds);
	return string;
}
int usage(char *argv0) {
	fprintf(stderr, "\
Usage: %s\n"
#ifdef NERD_FONT
"	-n --nerd     : use nerd font icons\n"
#endif
"	-c --nocolor  : disable colors\n\
", argv0);
	return 2;
}
#ifdef NERD_FONT
char* nerd_ternary(bool t, char* a) {
	if (t) return a; else return "";
}
#else
#define nerd_ternary(t, a) ""
#endif
int main(int argc, char *argv[]) {
#define INVALID { return usage(argv[0]); }
#ifdef NERD_FONT
	bool nerd_flag = 0;
#endif
	bool nocolor_flag = 0;
	bool flag_done = 0;
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' && argv[i][1] != '\0' && !flag_done) {
			if (argv[i][1] == '-' && argv[i][2] == '\0') flag_done = 1; // -- denotes end of flags (and -o)
			else {
#ifdef NERD_FONT
				if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--nerd") == 0) {
					if (nerd_flag) INVALID
					nerd_flag = 1;
				} else
#endif
				if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--nocolor") == 0) {
					if (nocolor_flag) INVALID
					nocolor_flag = 1;
				} else INVALID
			}
		} else INVALID
	}
	uid_t uid = getuid();
	struct passwd *pw = getpwuid(uid);
	if (!pw) { fprintf(stderr, "getpwuid: %i: %s\n", uid, strerror(errno)); return errno; }
	struct utsname un;
	uname(&un);
	struct sysinfo si;
	sysinfo(&si);
	char *sh = getenv("SHELL");
	char *lang = getenv("LANG");
	if (!sh) sh = pw->pw_shell;
	char *sh_ = strrchr(sh, '/');
	if (!sh_) sh_ = sh; else ++sh_;
	FILE *fp = fopen(OS, "r");
	if (!fp) { fprintf(stderr, "%s: %s\n", OS, strerror(errno)); return errno; }
	char **os = NULL;
	size_t oslen = 0;
	size_t zlen;
	while (1) {
		char *z = NULL;
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
	char *NAME;
	uint8_t is_name = 0;
	uint8_t is_pretty = 0;
	for (size_t i = 0; i < oslen; ++i) {
		char *eq = strrchr(os[i], '=');
		if (eq) {
			if (!is_pretty && strncmp(os[i], "NAME",        eq-os[i]) == 0) { NAME = eq+1; is_name = 1;             continue; }
			if               (strncmp(os[i], "PRETTY_NAME", eq-os[i]) == 0) { NAME = eq+1; is_pretty = is_name = 1; continue; }
		}
		free(os[i]);
	}
	struct statvfs vfs;
	char *root = "/";
	if (statvfs(root, &vfs) < 0) { fprintf(stderr, "statvfs: %s: %s\n", root, strerror(errno)); return errno; }
	char *key_text       = nocolor_flag ? ""    : "\x1b[38;5;14m";
	char *separator_text = nocolor_flag ? " ~ " : "\x1b[38;5;7m ~ \x1b[0m";
	char *slash_text     = nocolor_flag ? " / " : "\x1b[38;5;7m / \x1b[0m";
	char *reset          = nocolor_flag ? ""    : "\x1b[0m";
	if (is_name) {
		size_t len = strlen(NAME);
		if ((NAME[len-1] == '"' && NAME[0] == '"') || (NAME[len-1] == '\'' && NAME[0] == '\'')) {
			NAME[len-1] = '\0';
			++NAME;
		}
	// https://www.nerdfonts.com/
	printf("%s%sdistro%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "  "), separator_text, NAME, reset); }
	printf("%s%s    os%s%s %s %s%s\n", key_text, nerd_ternary(nerd_flag, "  "), separator_text, un.sysname, un.machine, un.release, reset);
	printf("%s%s  user%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "  "), separator_text, pw->pw_name, reset);
	printf("%s%s  host%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "  "), separator_text, un.nodename, reset);
	printf("%s%s shell%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "  "), separator_text, sh_, reset);
	printf("%s%slocale%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "  "), separator_text, lang, reset);
	printf("%s%suptime%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "﯁  "), separator_text, display_time(si.uptime), reset);
	printf("%s%s  proc%s%i%s\n",       key_text, nerd_ternary(nerd_flag, "缾 "), separator_text, si.procs, reset);
	printf("%s%s   ram%s%s%s%s%s\n",   key_text, nerd_ternary(nerd_flag, "  "), separator_text, display_bytes( si.totalram  - si.freeram),                  slash_text, display_bytes(si.totalram), reset);
	printf("%s%s  swap%s%s%s%s%s\n",   key_text, nerd_ternary(nerd_flag, "易 "), separator_text, display_bytes( si.totalswap - si.freeswap),                 slash_text, display_bytes(si.totalswap), reset);
	printf("%s%s inode%s%s%s%s%s\n",   key_text, nerd_ternary(nerd_flag, "﫭 "), separator_text, display_bytes( vfs.f_files  - vfs.f_ffree),                 slash_text, display_bytes(vfs.f_files), reset);
	printf("%s%s block%s%s%s%s%s\n",   key_text, nerd_ternary(nerd_flag, "﫭 "), separator_text, display_bytes((vfs.f_blocks - vfs.f_bfree) * vfs.f_frsize), slash_text, display_bytes(vfs.f_blocks * vfs.f_frsize), reset);
	return 0;
}
