/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

#include "../util.h"

#if defined(__linux__)
	static int
	get_io(uintmax_t *s_in, uintmax_t *s_out)
	{
		FILE *fp;
		struct {
			const char *name;
			const size_t len;
			uintmax_t *var;
		} ent[] = {
			{ "pgpgin",  sizeof("pgpgin") - 1,  s_in  },
			{ "pgpgout", sizeof("pgpgout") - 1, s_out },
		};
		size_t line_len = 0, i, left;
		char *line = NULL;
			
		/* get number of fields we want to extract */
		for (i = 0, left = 0; i < LEN(ent); i++) {
			if (ent[i].var) {
				left++;
			}
		}
                
		if (!(fp = fopen("/proc/vmstat", "r"))) {
			warn("fopen '/proc/vmstat':");
			return 1;
		}

		/* read file line by line and extract field information */
		while (left > 0 && getline(&line, &line_len, fp) >= 0) {
			for (i = 0; i < LEN(ent); i++) {
				if (ent[i].var &&
				!strncmp(line,ent[i].name, ent[i].len)) {
					sscanf(line + ent[i].len + 1,
						"%ju\n", ent[i].var);
					left--;
					break;
				}
			}
		}
		free(line);
		if(ferror(fp)) {
			warn("getline '/proc/vmstat':");
			return 1;
		}
		
		fclose(fp);
		return 0;
	}	
		
	const char *
	io_in(void)
	{
		uintmax_t oldin;
		static uintmax_t newin;

		oldin = newin;

		if (get_io(&newin, NULL)) {
			return NULL;
		}
		if (oldin == 0) {
			return NULL;
		}
		
		return fmt_human((newin - oldin) * 1024, 1000);
	}

	const char *
	io_out(void)
	{
		uintmax_t oldout;
		static uintmax_t newout;

		oldout = newout;

		if (get_io(NULL, &newout)) {
			return NULL;
		}
		if (oldout == 0) {
			return NULL;
		}
		
		return fmt_human((newout - oldout) * 1024, 1000);
	}

	const char *
	io_perc(void)
	{
		struct dirent *dp;
		DIR *bd;
		uintmax_t oldwait;
		static uintmax_t newwait;
		extern const unsigned int interval;

		oldwait = newwait;
		
		if (!(bd = opendir("/sys/block"))) {
			warn("opendir '%s':", "/sys/block");
			return NULL;
		} 

		newwait = 0;
		/* get IO wait stats from the /sys/block directories */
		while ((dp = readdir(bd))) {
			int devlen, chklen, statlen;
			devlen = strlen(dp->d_name);
			char devname[devlen];
			strcpy(devname, dp->d_name);
			if (strstr(devname, "loop") ||
			    strstr(devname, "ram")) {
			   	continue;
			}
			if (!strcmp(devname, ".") ||
			    !strcmp(devname, "..")) {
			    	continue;
			}
		
			statlen = 16 + devlen;
			chklen = 18 + devlen;
			char statpath[statlen], chkpath[chklen];
			strcpy(statpath, "/sys/block/");
			strcat(statpath, devname);
			/* non-virtual devices only */
			strcpy(chkpath, statpath);
			strcat(chkpath, "/device");
			if (access(chkpath, F_OK) != 0) {
				continue;
			}
	
			strcat(statpath, "/stat");
			uintmax_t partwait;
			if (pscanf(statpath,
				"%*d %*d %*d %*d %*d %*d %*d %*d %*d %ju %*d",
	   			&partwait) != 1) {
				continue;
			}
			newwait += partwait;
		}
		closedir(bd);
		if (oldwait == 0 || newwait < oldwait) {
			return NULL;
		}
		
		return bprintf("%0.1f", 100 *
		       (newwait - oldwait) / (float)interval);
	}

#else
	const char *
	io_in(void)
	{
		return NULL;
	}

	const char *
	io_out(void)
	{
		return NULL;
	}

	const char *
	io_perc(void)
	{
		return NULL;
	}
#endif
