#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/utsname.h>
#ifndef __APPLE__
#include <sys/sysinfo.h>
#endif
#include <sys/time.h>
#include <sys/statvfs.h>
#include <time.h>
#include <pwd.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <dirent.h>
#include <mntent.h>
#define NERD_FONT_SUPPORT // comment out to compile without nerd font support, useful if you have a minimal system without fonts or a GUI
#define COLOR_SUPPORT // same as above but for colors
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define strerr strerror(errno)
#define OS "/etc/os-release"
#define BATTERY "/sys/class/power_supply/"
#define BLOCK 512
#define SQ_BAR "󱓻 "
#define FG_BAR "███"
#define BG_BAR "   "
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
	eprintf("\
Usage: %s\n"
#ifdef COLOR_SUPPORT
"	-c --color       : enables colors\n\
	-b --colorbars   : shows color bars, best used with -c\n\
	-f --foreground  : use foreground colors for color bars\n"
#endif
#ifdef NERD_FONT_SUPPORT
"	-C --square      : use rounded square symbols for color bars\n\
	-n --nerd        : use nerd font icons\n"
#endif
"	-h --12hour      : use 12 hour time instead of 24 hour time\n\
	-d --noday       : don't show day of the week in time (e.g Thu)\n\
	-o --filesystems : show all mounted file systems instead of just root\n",
	argv0);
	return 2;
}
#ifdef COLOR_SUPPORT
#define color(a, b) color_flag ? a : b
#else
#define color(a, b) b
#endif
#ifdef NERD_FONT_SUPPORT
#define nerd(a) nerd_flag ? a : ""
#else
#define nerd(a) ""
#endif
int main(int argc, char *argv[]) {
#define INVALID { return usage(argv[0]); }
#ifdef COLOR_SUPPORT
	bool color_flag = 0;
	bool colorbars_flag = 0;
	bool foreground_flag = 0;
#endif
#ifdef NERD_FONT_SUPPORT
	bool square_flag = 0;
	bool nerd_flag = 0;
#endif
	bool h12_flag = 0;
	bool noday_flag = 0;
	bool filesystems_flag = 0;
	bool flag_done = 0;
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' && argv[i][1] != '\0' && !flag_done) {
			if (argv[i][1] == '-' && argv[i][2] == '\0') flag_done = 1; // -- denotes end of flags
			else {
#ifdef COLOR_SUPPORT
				if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--color") == 0) {
					if (color_flag) INVALID
					color_flag = 1;
				} else
				if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--colorbars") == 0) {
					if (colorbars_flag) INVALID
					colorbars_flag = 1;
				} else
				if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--foreground") == 0) {
					if (foreground_flag) INVALID
					foreground_flag = 1;
				} else
#endif
#ifdef NERD_FONT_SUPPORT
				if (strcmp(argv[i], "-C") == 0 || strcmp(argv[i], "--square") == 0) {
					if (square_flag) INVALID
					square_flag = 1;
				} else
				if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--nerd") == 0) {
					if (nerd_flag) INVALID
					nerd_flag = 1;
				} else
#endif
				if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--12hour") == 0) {
					if (h12_flag) INVALID
					h12_flag = 1;
				} else
				if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--noday") == 0) {
					if (noday_flag) INVALID
					noday_flag = 1;
				} else
				if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--filesystems") == 0) {
					if (filesystems_flag) INVALID
					filesystems_flag = 1;
				} else
				INVALID
			}
		} else INVALID
	}
	uid_t uid = getuid();
	struct passwd *pw = getpwuid(uid); // reads /etc/passwd field of current UID
	if (!pw) { eprintf("getpwuid: %i: %s\n", uid, strerr); return errno; }
	struct utsname un;
	uname(&un);
	struct sysinfo si;
	sysinfo(&si);
	char *sh = getenv("SHELL");
	char *lang = getenv("LANG");
	if (!sh) sh = pw->pw_shell;
	char *sh_ = strrchr(sh, '/');
	if (!sh_) sh_ = sh; else ++sh_;
	char *name;
	FILE *fp = fopen(OS, "r");
	if (!fp) { eprintf("%s: %s\n", OS, strerr); }
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
				if (!is_pretty && strncmp(os[i], "NAME",        eq-os[i]) == 0) { name = eq+1;                continue; }
				if               (strncmp(os[i], "PRETTY_NAME", eq-os[i]) == 0) { name = eq+1; is_pretty = 1; continue; }
			}
			free(os[i]);
		}
	}
	struct timeval timev;
	gettimeofday(&timev, NULL);
	struct tm *time = localtime(&(timev.tv_sec));
	char date_str[64];
	char time_str[64];
	strftime(date_str, sizeof(date_str), noday_flag ? "%d.%m.%Y"    : "%a %d.%m.%Y", time); // optional show day of the week
	strftime(time_str, sizeof(time_str), h12_flag   ? "%I.%M.%S %p" : "%H.%M.%S",    time); // 24 hour or 12 hour
	if (name) {
		size_t len = strlen(name);
		if ((name[len-1] == '"' && name[0] == '"') || (name[len-1] == '\'' && name[0] == '\'')) { // if surrounded with quotes or double quotes, remove them
			name[len-1] = '\0';
			++name;
		}
	}
	DIR *battery_dir = opendir(BATTERY); // lists all batteries
	char *tty = ttyname(STDIN_FILENO);
	char *key_text       = color("\x1b[38;5;14m"         , "");
	char *separator_text = color("\x1b[38;5;7m ~ \x1b[0m", " ~ ");
	char *slash_text     = color("\x1b[38;5;7m / \x1b[0m", " / ");
	char *reset          = color("\x1b[0m"               , "");
	// https://www.nerdfonts.com/
	if (name)
	printf("%s%s distro%s%s%s\n",       key_text, nerd("  "), separator_text, name, reset); // from reading /etc/os-release
	printf("%s%s     os%s%s %s %s%s\n", key_text, nerd("  "), separator_text, un.sysname, un.machine, un.release, reset); // uname
	printf("%s%s   user%s%s%s\n",       key_text, nerd("  "), separator_text, pw->pw_name, reset); // from getpwuid
	printf("%s%s   host%s%s%s\n",       key_text, nerd("  "), separator_text, un.nodename, reset); // uname
	printf("%s%s  shell%s%s%s\n",       key_text, nerd("  "), separator_text, sh_, reset); // SHELL env or from getpwuid
	if (lang)
	printf("%s%s locale%s%s%s\n",       key_text, nerd("  "), separator_text, lang, reset); // LANG env
	if (tty)
	printf("%s%s    tty%s%s%s\n",       key_text, nerd("  "), separator_text, tty, reset); // ttyname
	printf("%s%s   date%s%s%s\n",       key_text, nerd("  "), separator_text, date_str, reset); // time/date format
	printf("%s%s   time%s%s%s\n",       key_text, nerd("  "), separator_text, time_str, reset);
	if (battery_dir) {
		size_t bat_count = 0;
		struct dirent *dir = NULL;
		while ((dir = readdir(battery_dir)) != NULL) {
#ifdef NERD_FONT_SUPPORT
			char *bat = NULL;
#endif
			if (dir->d_name[0] == '.' && (dir->d_name[1] == '\0' || (dir->d_name[1] == '.' && dir->d_name[2] == '\0'))) continue; // ignore . and ..
			char *bat_status_name   = malloc(16 + strlen(BATTERY) + strlen(dir->d_name));
			char *bat_capacity_name = malloc(16 + strlen(BATTERY) + strlen(dir->d_name));
			sprintf(bat_status_name,   "%s/%s/status",   BATTERY, dir->d_name);
			sprintf(bat_capacity_name, "%s/%s/capacity", BATTERY, dir->d_name);
			FILE *bat_status_f   = fopen(bat_status_name,   "r");
			if (!bat_status_f)   continue;
			FILE *bat_capacity_f = fopen(bat_capacity_name, "r");
			if (!bat_capacity_f) continue;
			free(bat_status_name);
			free(bat_capacity_name);
			char* bat_capacity = malloc(BLOCK);
			int   bat_status   = fgetc(bat_status_f);
			size_t bat_capacity_read = fread(bat_capacity, 1, BLOCK, bat_capacity_f);
			if (bat_status == EOF || bat_capacity_read >= BLOCK || bat_capacity_read <= 0) continue;
			if (ferror(bat_status_f) || ferror(bat_capacity_f)) continue;
			if (feof(  bat_status_f) || !feof( bat_capacity_f)) continue;
			for (size_t i = 0; i < 16; ++i) { if (bat_capacity[i] < '0' || bat_capacity[i] > '9') bat_capacity[i] = '\0'; } // ignore everything after numbers
			long bat_capacity_l = atol(bat_capacity); // convert to integer
			free(bat_capacity);
			char *bat_text = malloc(32 * (++bat_count));
			if (bat_status == 'C') { // charging
#ifdef NERD_FONT_SUPPORT
				if (bat_capacity_l < 30)  bat = "\uf585  "; else
				if (bat_capacity_l < 40)  bat = "\uf586  "; else
				if (bat_capacity_l < 50)  bat = "\uf587  "; else
				if (bat_capacity_l < 70)  bat = "\uf588  "; else
				if (bat_capacity_l < 90)  bat = "\uf589  "; else
				if (bat_capacity_l < 100) bat = "\uf58a  "; else
				bat = "\uf584  ";
#endif
				sprintf(bat_text, "%s Charging %li%%", dir->d_name, bat_capacity_l);
			} else
			if (bat_status == 'D') { // discharging
#ifdef NERD_FONT_SUPPORT
				if (bat_capacity_l < 10)  bat = "\uf58d  "; else
				if (bat_capacity_l < 20)  bat = "\uf579  "; else
				if (bat_capacity_l < 30)  bat = "\uf57a  "; else
				if (bat_capacity_l < 40)  bat = "\uf57b  "; else
				if (bat_capacity_l < 50)  bat = "\uf57c  "; else
				if (bat_capacity_l < 60)  bat = "\uf57d  "; else
				if (bat_capacity_l < 70)  bat = "\uf57e  "; else
				if (bat_capacity_l < 80)  bat = "\uf57f  "; else
				if (bat_capacity_l < 90)  bat = "\uf580  "; else
				if (bat_capacity_l < 100) bat = "\uf581  "; else
				bat = "\uf578  ";
#endif
				sprintf(bat_text, "%s Discharging %li%%", dir->d_name, bat_capacity_l);
			} else
			if (bat_status == 'F') { // full
#ifdef NERD_FONT_SUPPORT
				bat = "\uf578  ";
#endif
				sprintf(bat_text, "%s Full %li%%", dir->d_name, bat_capacity_l);
			} else continue;
	printf("%s%sbattery%s%s%s\n",       key_text, nerd(bat),   separator_text, bat_text, reset);
			free(bat_text);
		}
		closedir(battery_dir);
	}
#ifndef __APPLE__
	printf("%s%s uptime%s%s%s\n",       key_text, nerd("  "), separator_text, display_time(si.uptime), reset); // sysinfo
	printf("%s%s   proc%s%i%s\n",       key_text, nerd("  "), separator_text, si.procs, reset);
	printf("%s%s    ram%s%s%s%s%s\n",   key_text, nerd("󰍛  "), separator_text, display_bytes( si.totalram  - si.freeram),                  slash_text, display_bytes(si.totalram), reset);
	if (si.totalswap > 0)
	printf("%s%s   swap%s%s%s%s%s\n",   key_text, nerd("󰾵  "), separator_text, display_bytes( si.totalswap - si.freeswap),                 slash_text, display_bytes(si.totalswap), reset);
#endif
	FILE *f = setmntent("/proc/self/mounts", "r");
	struct mntent *m;
	while ((m = getmntent(f))) {
		if (m->mnt_fsname[0] != '/') continue; // check it's not a psuedo-fs like /dev, /proc, /tmp
		if (m->mnt_dir[0]    != '/') continue;
		if (m->mnt_dir[1]    != '\0' && !filesystems_flag) continue;
		struct statvfs vfs;
		if (statvfs(m->mnt_dir, &vfs) < 0) { eprintf("statvfs: %s: %s\n", m->mnt_dir, strerr); } else if (vfs.f_blocks > 0) {
			if (filesystems_flag) {
			printf("%s%s  mount%s%s%s\n",     key_text, nerd("󰉋  "), separator_text, m->mnt_dir,    reset);
			printf("%s%s device%s%s%s\n",     key_text, nerd("󰋊  "), separator_text, m->mnt_fsname, reset);
			} else {
			printf("%s%s   root%s%s%s\n",     key_text, nerd("󰋊  "), separator_text, m->mnt_fsname, reset);
			}
			if (vfs.f_files > 0)
			printf("%s%s  inode%s%s%s%s%s\n", key_text, nerd("   "), separator_text, display_bytes( vfs.f_files  - vfs.f_ffree),
				slash_text, display_bytes(vfs.f_files), reset);
			printf("%s%s  block%s%s%s%s%s\n", key_text, nerd("   "), separator_text, display_bytes((vfs.f_blocks - vfs.f_bfree) * vfs.f_frsize),
				slash_text, display_bytes(vfs.f_blocks * vfs.f_frsize), reset);
		}
	}
	endmntent(f);
	if (colorbars_flag) {
		for (char j = 0; j < 16; ++j) {
#ifdef NERD_FONT_SUPPORT
			if (square_flag) {
				printf("\x1b[38;5;%im%s", j, SQ_BAR);
			} else
#endif
			if (foreground_flag) {
				printf("\x1b[38;5;%im%s", j, FG_BAR);
			} else {
				printf("\x1b[48;5;%im%s", j, BG_BAR);
			}
			if ((j & 7) == 7) printf("\x1b[0m\n");
		}
	}
	return 0;
}
