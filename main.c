#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/statvfs.h>
#include <time.h>
#include <pwd.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <dirent.h>
#define NERD_FONT // comment out to compile without nerd font support, useful if you have a minimal system without fonts or a GUI
#define ERROR(format, str, ...) fprintf(stderr, format, str, strerror(errno), ##__VA_ARGS__)
#define OS "/etc/os-release"
#define BATTERY "/sys/class/power_supply/"
#define ROOT "/"
#define BLOCK 512
#define BYTES_PADDING 100 // 10 for 1 extra decimal digit
#define BYTES_FORMAT "%.2f" // replace the number with how many zeros are in BYTES_PADDING
char *display_bytes(unsigned long bytes) {
	if (bytes == 0) return "0";
	bytes *= BYTES_PADDING; // allow for extra decimal digit
	const char *suffixes[] = { "", "K", "M", "G", "T", "P", "E", "Z", "Y" };
	uint8_t t = 0;
	while (bytes >= 1024 * BYTES_PADDING) {
		bytes /= 1024; // divide by 1024 and use the next suffix
		++t;
	}
	char *z = malloc(24);
	double bytes_d = (double)(bytes % BYTES_PADDING) / BYTES_PADDING; // the extra decimal digit
	bytes /= BYTES_PADDING;
	char *y = malloc(8);
	sprintf(y, BYTES_FORMAT, bytes_d); // use temp string so we can remove the 0 before the decimal point, 6.5G would look like 60.5G otherwise
	char *x = strchr(y, '.'); // find dot in the string
	if (!x) {
		sprintf(z, "%li%s",   bytes,    suffixes[t]); // don't put the temp string if there's no dot
	} else {
		sprintf(z, "%li%s%s", bytes, x, suffixes[t]);
	}
	free(y);
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
	-h --12hour   : use 12 hour time instead of 24 hour time\n\
	-d --noday    : don't show day of the week in time (e.g Thu)\n\
", argv0);
	return 2;
}
#ifdef NERD_FONT
char* nerd_ternary(bool t, char* a) {
	if (t) return a; else return ""; // so compiling without nerd fonts still works
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
	bool h12_flag = 0;
	bool noday_flag = 0;
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
				} else
				if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--12hour") == 0) {
					if (h12_flag) INVALID
					h12_flag = 1;
				} else
				if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--noday") == 0) {
					if (noday_flag) INVALID
					noday_flag = 1;
				} else INVALID
			}
		} else INVALID
	}
	uid_t uid = getuid();
	struct passwd *pw = getpwuid(uid); // reads /etc/passwd field of current UID
	if (!pw) { ERROR("getpwuid: %i: %s\n", uid); return errno; }
	struct utsname un;
	uname(&un);
	struct sysinfo si;
	sysinfo(&si);
	char *sh = getenv("SHELL");
	char *lang = getenv("LANG");
	if (!sh) sh = pw->pw_shell;
	char *sh_ = strrchr(sh, '/');
	if (!sh_) sh_ = sh; else ++sh_;
	uint8_t is_name = 0;
	char *name;
	FILE *fp = fopen(OS, "r");
	if (!fp) { ERROR("%s: %s\n", OS); }
	else {
		char **os = NULL;
		size_t oslen = 0;
		size_t zlen;
		while (1) {
			char *z = NULL;
			zlen = 1;
			int c;
			while (1) {
				c = fgetc(fp); // read each line of /etc/os-release
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
		uint8_t is_pretty = 0;
		for (size_t i = 0; i < oslen; ++i) {
			char *eq = strrchr(os[i], '=');
			if (eq) {
				if (!is_pretty && strncmp(os[i], "NAME",        eq-os[i]) == 0) { name = eq+1; is_name = 1;             continue; }
				if               (strncmp(os[i], "PRETTY_NAME", eq-os[i]) == 0) { name = eq+1; is_name = is_pretty = 1; continue; }
			}
			free(os[i]);
		}
	}
	struct statvfs vfs;
	if (statvfs(ROOT, &vfs) < 0) { ERROR("statvfs: %s: %s\n", ROOT); return errno; }
	char *key_text       = nocolor_flag ? ""    : "\x1b[38;5;14m";
	char *separator_text = nocolor_flag ? " ~ " : "\x1b[38;5;7m ~ \x1b[0m";
	char *slash_text     = nocolor_flag ? " / " : "\x1b[38;5;7m / \x1b[0m";
	char *reset          = nocolor_flag ? ""    : "\x1b[0m";
	struct timeval timev;
	gettimeofday(&timev, NULL);
	struct tm *time = localtime(&(timev.tv_sec));
	char date_str[64];
	char time_str[64];
	strftime(date_str, sizeof(date_str), noday_flag ? "%d.%m.%Y"    : "%a %d.%m.%Y", time); // optional show day of the week
	strftime(time_str, sizeof(time_str), h12_flag   ? "%I.%M.%S %p" : "%H.%M.%S",    time); // 24 hour or 12 hour
	if (is_name) {
		size_t len = strlen(name);
		if ((name[len-1] == '"' && name[0] == '"') || (name[len-1] == '\'' && name[0] == '\'')) { // if surrounded with quotes or double quotes, remove them
			name[len-1] = '\0';
			++name;
		}
	}
	DIR *bat = opendir(BATTERY); // lists all batteries
	bool is_battery = 0;
	char *bat_icon = NULL;
	char *bat_text = NULL;
	if (bat) {
		size_t bat_count = 0;
		struct dirent *dir = NULL;
		while ((dir = readdir(bat)) != NULL) {
			if (dir->d_name[0] == '.' && (dir->d_name[1] == '\0' || (dir->d_name[1] == '.' && dir->d_name[2] == '\0'))) continue; // ignore . and ..
			char *bat_status_name   = malloc(32 + strlen(dir->d_name));
			char *bat_capacity_name = malloc(32 + strlen(dir->d_name));
			sprintf(bat_status_name,   "%s/%s/status",   BATTERY, dir->d_name);
			sprintf(bat_capacity_name, "%s/%s/capacity", BATTERY, dir->d_name);
			FILE *bat_status_f   = fopen(bat_status_name,    "r");
			if (!bat_status_f)   continue;
			FILE *bat_capacity_f = fopen(bat_capacity_name, "r");
			if (!bat_capacity_f) continue;
			char* bat_capacity = malloc(BLOCK);
			int   bat_status   = fgetc(bat_status_f);
			size_t bat_capacity_read = fread(bat_capacity, 1, BLOCK, bat_capacity_f);
			if (bat_status == EOF || bat_capacity_read >= BLOCK || bat_capacity_read <= 0) continue;
			if (ferror(bat_status_f) || ferror(bat_capacity_f)) continue;
			if (feof(  bat_status_f) || !feof( bat_capacity_f)) continue;
			for (size_t i = 0; i < 16; ++i) { if (bat_capacity[i] < '0' || bat_capacity[i] > '9') bat_capacity[i] = '\0'; } // ignore everything after numbers
			long bat_capacity_l = atol(bat_capacity); // convert to integer
			bat_text = realloc(bat_text, 32 * (++bat_count));
			if (bat_count == 1) bat_text[0] = '\0';
			char *tmp_text = malloc(32);
			if (bat_status == 'C') { // charging
				is_battery = 1;
				if (!bat_icon) {
					if (bat_capacity_l < 30)  bat_icon = "\uf585  "; else
					if (bat_capacity_l < 40)  bat_icon = "\uf586  "; else
					if (bat_capacity_l < 50)  bat_icon = "\uf587  "; else
					if (bat_capacity_l < 70)  bat_icon = "\uf588  "; else
					if (bat_capacity_l < 90)  bat_icon = "\uf589  "; else
					if (bat_capacity_l < 100) bat_icon = "\uf58a  "; else
					bat_icon = "\uf584  ";
				}
				sprintf(tmp_text, "%s Charging %li%%", dir->d_name, bat_capacity_l);
			} else
			if (bat_status == 'D') { // discharging
				is_battery = 1;
				if (!bat_icon) {
					if (bat_capacity_l < 10)  bat_icon = "\uf58d  "; else
					if (bat_capacity_l < 20)  bat_icon = "\uf579  "; else
					if (bat_capacity_l < 30)  bat_icon = "\uf57a  "; else
					if (bat_capacity_l < 40)  bat_icon = "\uf57b  "; else
					if (bat_capacity_l < 50)  bat_icon = "\uf57c  "; else
					if (bat_capacity_l < 60)  bat_icon = "\uf57d  "; else
					if (bat_capacity_l < 70)  bat_icon = "\uf57e  "; else
					if (bat_capacity_l < 80)  bat_icon = "\uf57f  "; else
					if (bat_capacity_l < 90)  bat_icon = "\uf580  "; else
					if (bat_capacity_l < 100) bat_icon = "\uf581  "; else
					bat_icon = "\uf578  ";
				}
				sprintf(tmp_text, "%s Discharging %li%%", dir->d_name, bat_capacity_l);
			} else
			if (bat_status == 'F') { // full
				is_battery = 1;
				if (!bat_icon)
					bat_icon = "\uf578  ";
				sprintf(tmp_text, "%s Full %li%%", dir->d_name, bat_capacity_l);
			} else continue;
			char *tmp_2_text = malloc(32 * bat_count);
			sprintf(tmp_2_text, "%s%s%s", bat_text, bat_count > 1 ? ", " : "", tmp_text);
			free(bat_text);
			bat_text = tmp_2_text;
		}
		closedir(bat);
	}
	// https://www.nerdfonts.com/
	if (is_name)
	printf("%s%s distro%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "  "), separator_text, name, reset); // from reading /etc/os-release
	printf("%s%s     os%s%s %s %s%s\n", key_text, nerd_ternary(nerd_flag, "  "), separator_text, un.sysname, un.machine, un.release, reset); // uname
	printf("%s%s   user%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "  "), separator_text, pw->pw_name, reset); // from getpwuid
	printf("%s%s   host%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "  "), separator_text, un.nodename, reset); // uname
	printf("%s%s  shell%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "  "), separator_text, sh_, reset); // SHELL env or from getpwuid
	if (lang)
	printf("%s%s locale%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "  "), separator_text, lang, reset); // LANG env
	printf("%s%s   date%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "  "), separator_text, date_str, reset); // time/date format
	printf("%s%s   time%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "  "), separator_text, time_str, reset);
	if (is_battery)
	printf("%s%sbattery%s%s%s\n",       key_text, nerd_ternary(nerd_flag, bat_icon), separator_text, bat_text, reset);
	printf("%s%s uptime%s%s%s\n",       key_text, nerd_ternary(nerd_flag, "﯁  "), separator_text, display_time(si.uptime), reset); // sysinfo
	printf("%s%s   proc%s%i%s\n",       key_text, nerd_ternary(nerd_flag, "缾 "), separator_text, si.procs, reset);
	printf("%s%s    ram%s%s%s%s%s\n",   key_text, nerd_ternary(nerd_flag, "  "), separator_text, display_bytes( si.totalram  - si.freeram),                  slash_text, display_bytes(si.totalram), reset);
	if (si.totalswap > 0)
	printf("%s%s   swap%s%s%s%s%s\n",   key_text, nerd_ternary(nerd_flag, "易 "), separator_text, display_bytes( si.totalswap - si.freeswap),                 slash_text, display_bytes(si.totalswap), reset);
	printf("%s%s  inode%s%s%s%s%s\n",   key_text, nerd_ternary(nerd_flag, "﫭 "), separator_text, display_bytes( vfs.f_files  - vfs.f_ffree),                 slash_text, display_bytes(vfs.f_files), reset); // vfs
	printf("%s%s  block%s%s%s%s%s\n",   key_text, nerd_ternary(nerd_flag, "﫭 "), separator_text, display_bytes((vfs.f_blocks - vfs.f_bfree) * vfs.f_frsize), slash_text, display_bytes(vfs.f_blocks * vfs.f_frsize), reset);
	return 0;
}
