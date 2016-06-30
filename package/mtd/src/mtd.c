/*
 * mtd - simple memory technology device manipulation tool
 *
 * Copyright (C) 2005      Waldemar Brodkorb <wbx@dass-it.de>,
 * Copyright (C) 2005-2009 Felix Fietkau <nbd@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License v2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The code is based on the linux-mtd examples.
 */

#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include "mtd-api.h"
#include "fis.h"
#include "mtd.h"
#include "crc32.h"

#define MAX_ARGS 8
#define JFFS2_DEFAULT_DIR	"" /* directory name without /, empty means root dir */

#if __BYTE_ORDER == __BIG_ENDIAN
#define STORE32_LE(X)           ((((X) & 0x000000FF) << 24) | (((X) & 0x0000FF00) << 8) | (((X) & 0x00FF0000) >> 8) | (((X) & 0xFF000000) >> 24))
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define STORE32_LE(X)           (X)
#else
#error unkown endianness!
#endif

ssize_t pread(int fd, void *buf, size_t count, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);

#define TRX_MAGIC       0x30524448      /* "HDR0" */
struct trx_header {
	uint32_t magic;		/* "HDR0" */
	uint32_t len;		/* Length of file including header */
	uint32_t crc32;		/* 32-bit CRC from flag_version to end of file */
	uint32_t flag_version;	/* 0:15 flags, 16:31 version */
	uint32_t offsets[3];    /* Offsets of partitions from start of header */
};

static char *buf = NULL;
static char *imagefile = NULL;
static char *jffs2file = NULL, *jffs2dir = JFFS2_DEFAULT_DIR;
static int buflen = 0;
int quiet;
int mtdsize = 0;
int erasesize = 0;

int mtd_open(const char *mtd, bool block)
{
	FILE *fp;
	char dev[PATH_MAX];
	int i;
	int ret;
	int flags = O_RDWR | O_SYNC;

	if ((fp = fopen("/proc/mtd", "r"))) {
		while (fgets(dev, sizeof(dev), fp)) {
			if (sscanf(dev, "mtd%d:", &i) && strstr(dev, mtd)) {
				snprintf(dev, sizeof(dev), "/dev/mtd%s/%d", (block ? "block" : ""), i);
				if ((ret=open(dev, flags))<0) {
					snprintf(dev, sizeof(dev), "/dev/mtd%s%d", (block ? "block" : ""), i);
					ret=open(dev, flags);
				}
				fclose(fp);
				return ret;
			}
		}
		fclose(fp);
	}

	return open(mtd, flags);
}

int mtd_check_open(const char *mtd)
{
	struct mtd_info_user mtdInfo;
	int fd;

	fd = mtd_open(mtd, false);
	if(fd < 0) {
		fprintf(stderr, "Could not open mtd device: %s\n", mtd);
		return -1;
	}

	if(ioctl(fd, MEMGETINFO, &mtdInfo)) {
		fprintf(stderr, "Could not get MTD device info from %s\n", mtd);
		close(fd);
		return -1;
	}
	mtdsize = mtdInfo.size;
	erasesize = mtdInfo.erasesize;

	return fd;
}

int mtd_erase_block(int fd, int offset)
{
	struct erase_info_user mtdEraseInfo;

	mtdEraseInfo.start = offset;
	mtdEraseInfo.length = erasesize;
	ioctl(fd, MEMUNLOCK, &mtdEraseInfo);
	if (ioctl (fd, MEMERASE, &mtdEraseInfo) < 0)
		return -1;

	return 0;
}

int mtd_write_buffer(int fd, const char *buf, int offset, int length)
{
	lseek(fd, offset, SEEK_SET);
	write(fd, buf, length);
	return 0;
}


static int
image_check(int imagefd, const char *mtd)
{
	int ret = 1;
#ifdef target_brcm
	ret = trx_check(imagefd, mtd, buf, &buflen);
#endif
	return ret;
}

static int mtd_check(const char *mtd)
{
	char *next = NULL;
	char *str = NULL;
	int fd;

	if (strchr(mtd, ':')) {
		str = strdup(mtd);
		mtd = str;
	}

	do {
		next = strchr(mtd, ':');
		if (next) {
			*next = 0;
			next++;
		}

		fd = mtd_check_open(mtd);
		if (fd < 0)
			return 0;

		if (!buf)
			buf = malloc(erasesize);

		close(fd);
		mtd = next;
	} while (next);

	if (str)
		free(str);

	return 1;
}

static int
mtd_unlock(const char *mtd)
{
	struct erase_info_user mtdLockInfo;
	char *next = NULL;
	char *str = NULL;
	int fd;

	if (strchr(mtd, ':')) {
		str = strdup(mtd);
		mtd = str;
	}

	do {
		next = strchr(mtd, ':');
		if (next) {
			*next = 0;
			next++;
		}

		fd = mtd_check_open(mtd);
		if(fd < 0) {
			fprintf(stderr, "Could not open mtd device: %s\n", mtd);
			exit(1);
		}

		if (quiet < 2)
			fprintf(stderr, "Unlocking %s ...\n", mtd);

		mtdLockInfo.start = 0;
		mtdLockInfo.length = mtdsize;
		ioctl(fd, MEMUNLOCK, &mtdLockInfo);
		close(fd);
		mtd = next;
	} while (next);

	if (str)
		free(str);

	return 0;
}

static int
mtd_erase(const char *mtd)
{
	int fd;
	struct erase_info_user mtdEraseInfo;

	if (quiet < 2)
		fprintf(stderr, "Erasing %s ...\n", mtd);

	fd = mtd_check_open(mtd);
	if(fd < 0) {
		fprintf(stderr, "Could not open mtd device: %s\n", mtd);
		exit(1);
	}

	mtdEraseInfo.length = erasesize;

	for (mtdEraseInfo.start = 0;
		 mtdEraseInfo.start < mtdsize;
		 mtdEraseInfo.start += erasesize) {

		ioctl(fd, MEMUNLOCK, &mtdEraseInfo);
		if(ioctl(fd, MEMERASE, &mtdEraseInfo))
			fprintf(stderr, "Failed to erase block on %s at 0x%x\n", mtd, mtdEraseInfo.start);
	}

	close(fd);
	return 0;

}

static int
mtd_fixtrx(const char *mtd, size_t offset)
{
	int fd;
	struct trx_header *trx;
	char *buf;
	ssize_t res;
	size_t block_offset;

	if (quiet < 2)
		fprintf(stderr, "Trying to fix trx header in %s at 0x%x...\n", mtd, offset);

	block_offset = offset & ~(erasesize - 1);
	offset -= block_offset;

	fd = mtd_check_open(mtd);
	if(fd < 0) {
		fprintf(stderr, "Could not open mtd device: %s\n", mtd);
		exit(1);
	}

	if (block_offset + erasesize > mtdsize) {
		fprintf(stderr, "Offset too large, device size 0x%x\n", mtdsize);
		exit(1);
	}

	buf = malloc(erasesize);
	if (!buf) {
		perror("malloc");
		exit(1);
	}

	res = pread(fd, buf, erasesize, block_offset);
	if (res != erasesize) {
		perror("pread");
		exit(1);
	}

	trx = (struct trx_header *) (buf + offset);
	if (trx->magic != STORE32_LE(0x30524448)) {
		fprintf(stderr, "No trx magic found\n");
		exit(1);
	}

	if (trx->len == STORE32_LE(erasesize - offset)) {
		if (quiet < 2)
			fprintf(stderr, "Header already fixed, exiting\n");
		close(fd);
		return 0;
	}

	trx->len = STORE32_LE(erasesize - offset);

	trx->crc32 = STORE32_LE(crc32buf((char*) &trx->flag_version, erasesize - offset - 3*4));
	if (mtd_erase_block(fd, block_offset)) {
		fprintf(stderr, "Can't erease block at 0x%x (%s)\n", block_offset, strerror(errno));
		exit(1);
	}

	if (quiet < 2)
		fprintf(stderr, "New crc32: 0x%x, rewriting block\n", trx->crc32);

	if (pwrite(fd, buf, erasesize, block_offset) != erasesize) {
		fprintf(stderr, "Error writing block (%s)\n", strerror(errno));
		exit(1);
	}

	if (quiet < 2)
		fprintf(stderr, "Done.\n");

	close (fd);
	sync();
	return 0;

}

static int
mtd_refresh(const char *mtd)
{
	int fd;

	if (quiet < 2)
		fprintf(stderr, "Refreshing mtd partition %s ... ", mtd);

	fd = mtd_check_open(mtd);
	if(fd < 0) {
		fprintf(stderr, "Could not open mtd device: %s\n", mtd);
		exit(1);
	}

	if (ioctl(fd, MTDREFRESH, NULL)) {
		fprintf(stderr, "Failed to refresh the MTD device\n");
		close(fd);
		exit(1);
	}
	close(fd);

	if (quiet < 2)
		fprintf(stderr, "\n");

	return 0;
}

static int
mtd_write(int imagefd, const char *mtd, char *fis_layout)
{
	char *next = NULL;
	char *str = NULL;
	int fd, result;
	ssize_t r, w, e;
	uint32_t offset = 0;

#ifdef FIS_SUPPORT
	static struct fis_part new_parts[MAX_ARGS];
	static struct fis_part old_parts[MAX_ARGS];
	int n_new = 0, n_old = 0;

	if (fis_layout) {
		const char *tmp = mtd;
		char *word, *brkt;
		int ret;

		memset(&old_parts, 0, sizeof(old_parts));
		memset(&new_parts, 0, sizeof(new_parts));

		do {
			next = strchr(tmp, ':');
			if (!next)
				next = (char *) tmp + strlen(tmp);

			memcpy(old_parts[n_old].name, tmp, next - tmp);

			n_old++;
			tmp = next + 1;
		} while(*next);

		for (word = strtok_r(fis_layout, ",", &brkt);
		     word;
			 word = strtok_r(NULL, ",", &brkt)) {

			tmp = strtok(word, ":");
			strncpy((char *) new_parts[n_new].name, tmp, sizeof(new_parts[n_new].name) - 1);

			tmp = strtok(NULL, ":");
			if (!tmp)
				goto next;

			new_parts[n_new].size = strtoul(tmp, NULL, 0);

			tmp = strtok(NULL, ":");
			if (!tmp)
				goto next;

			new_parts[n_new].loadaddr = strtoul(tmp, NULL, 16);
next:
			n_new++;
		}
		ret = fis_validate(old_parts, n_old, new_parts, n_new);
		if (ret < 0) {
			fprintf(stderr, "Failed to validate the new FIS partition table\n");
			exit(1);
		}
		if (ret == 0)
			fis_layout = NULL;
	}
#endif

	if (strchr(mtd, ':')) {
		str = strdup(mtd);
		mtd = str;
	}

	r = 0;

resume:
	next = strchr(mtd, ':');
	if (next) {
		*next = 0;
		next++;
	}

	fd = mtd_check_open(mtd);
	if(fd < 0) {
		fprintf(stderr, "Could not open mtd device: %s\n", mtd);
		exit(1);
	}

	if (quiet < 2)
		fprintf(stderr, "Writing from %s to %s ... ", imagefile, mtd);

	w = e = 0;
	if (!quiet)
		fprintf(stderr, " [ ]");

	for (;;) {
		/* buffer may contain data already (from trx check or last mtd partition write attempt) */
		while (buflen < erasesize) {
			r = read(imagefd, buf + buflen, erasesize - buflen);
			if (r < 0) {
				if ((errno == EINTR) || (errno == EAGAIN))
					continue;
				else {
					perror("read");
					break;
				}
			}

			if (r == 0)
				break;

			buflen += r;
		}

		if (buflen == 0)
			break;

		if (jffs2file) {
			if (memcmp(buf, JFFS2_EOF, sizeof(JFFS2_EOF) - 1) == 0) {
				if (!quiet)
					fprintf(stderr, "\b\b\b   ");
				if (quiet < 2)
					fprintf(stderr, "\nAppending jffs2 data from %s to %s...", jffs2file, mtd);
				/* got an EOF marker - this is the place to add some jffs2 data */
				mtd_replace_jffs2(mtd, fd, e, jffs2file);
				goto done;
			}
			/* no EOF marker, make sure we figure out the last inode number
			 * before appending some data */
			mtd_parse_jffs2data(buf, jffs2dir);
		}

		/* need to erase the next block before writing data to it */
		while (w + buflen > e) {
			if (!quiet)
				fprintf(stderr, "\b\b\b[e]");


			if (mtd_erase_block(fd, e) < 0) {
				if (next) {
					if (w < e) {
						write(fd, buf + offset, e - w);
						offset = e - w;
					}
					w = 0;
					e = 0;
					close(fd);
					mtd = next;
					fprintf(stderr, "\b\b\b   \n");
					goto resume;
				} else {
					fprintf(stderr, "Failed to erase block\n");
					exit(1);
				}
			}

			/* erase the chunk */
			e += erasesize;
		}

		if (!quiet)
			fprintf(stderr, "\b\b\b[w]");

		if ((result = write(fd, buf + offset, buflen)) < buflen) {
			if (result < 0) {
				fprintf(stderr, "Error writing image.\n");
				exit(1);
			} else {
				fprintf(stderr, "Insufficient space.\n");
				exit(1);
			}
		}
		w += buflen;

		buflen = 0;
		offset = 0;
	}

	if (!quiet)
		fprintf(stderr, "\b\b\b\b    ");

done:
	if (quiet < 2)
		fprintf(stderr, "\n");

#ifdef FIS_SUPPORT
	if (fis_layout) {
		if (fis_remap(old_parts, n_old, new_parts, n_new) < 0)
			fprintf(stderr, "Failed to update the FIS partition table\n");
	}
#endif

	close(fd);
	return 0;
}

static void usage(void)
{
	fprintf(stderr, "Usage: mtd [<options> ...] <command> [<arguments> ...] <device>[:<device>...]\n\n"
	"The device is in the format of mtdX (eg: mtd4) or its label.\n"
	"mtd recognizes these commands:\n"
	"        unlock                  unlock the device\n"
	"        refresh                 refresh mtd partition\n"
	"        erase                   erase all data on device\n"
	"        write <imagefile>|-     write <imagefile> (use - for stdin) to device\n"
	"        jffs2write <file>       append <file> to the jffs2 partition on the device\n"
	"        fixtrx                  fix the checksum in a trx header on first boot\n"
	"Following options are available:\n"
	"        -q                      quiet mode (once: no [w] on writing,\n"
	"                                           twice: no status messages)\n"
	"        -r                      reboot after successful command\n"
	"        -f                      force write without trx checks\n"
	"        -e <device>             erase <device> before executing the command\n"
	"        -d <name>               directory for jffs2write, defaults to \"tmp\"\n"
	"        -j <name>               integrate <file> into jffs2 data when writing an image\n"
	"        -o offset               offset of the trx header in the partition (for fixtrx)\n"
#ifdef FIS_SUPPORT
	"        -F <part>[:<size>[:<entrypoint>]][,<part>...]\n"
	"                                alter the fis partition table to create new partitions replacing\n"
	"                                the partitions provided as argument to the write command\n"
	"                                (only valid together with the write command)\n"
#endif
	"\n"
	"Example: To write linux.trx to mtd4 labeled as linux and reboot afterwards\n"
	"         mtd -r write linux.trx linux\n\n");
	exit(1);
}

static void do_reboot(void)
{
	fprintf(stderr, "Rebooting ...\n");
	fflush(stderr);

	/* try regular reboot method first */
	system("/sbin/reboot");
	sleep(2);

	/* if we're still alive at this point, force the kernel to reboot */
	syscall(SYS_reboot,LINUX_REBOOT_MAGIC1,LINUX_REBOOT_MAGIC2,LINUX_REBOOT_CMD_RESTART,NULL);
}

int main (int argc, char **argv)
{
	int ch, i, boot, imagefd = 0, force, unlocked;
	char *erase[MAX_ARGS], *device = NULL;
	char *fis_layout = NULL;
	size_t offset = 0;
	enum {
		CMD_ERASE,
		CMD_WRITE,
		CMD_UNLOCK,
		CMD_REFRESH,
		CMD_JFFS2WRITE,
		CMD_FIXTRX,
	} cmd = -1;

	erase[0] = NULL;
	boot = 0;
	force = 0;
	buflen = 0;
	quiet = 0;

	while ((ch = getopt(argc, argv,
#ifdef FIS_SUPPORT
			"F:"
#endif
			"frqe:d:j:o:")) != -1)
		switch (ch) {
			case 'f':
				force = 1;
				break;
			case 'r':
				boot = 1;
				break;
			case 'j':
				jffs2file = optarg;
				break;
			case 'q':
				quiet++;
				break;
			case 'e':
				i = 0;
				while ((erase[i] != NULL) && ((i + 1) < MAX_ARGS))
					i++;

				erase[i++] = optarg;
				erase[i] = NULL;
				break;
			case 'd':
				jffs2dir = optarg;
				break;
			case 'o':
				errno = 0;
				offset = strtoul(optarg, 0, 0);
				if (errno) {
					fprintf(stderr, "-o: illegal numeric string\n");
					usage();
				}
				break;
#ifdef FIS_SUPPORT
			case 'F':
				fis_layout = optarg;
				break;
#endif
			case '?':
			default:
				usage();
		}
	argc -= optind;
	argv += optind;

	if (argc < 2)
		usage();

	if ((strcmp(argv[0], "unlock") == 0) && (argc == 2)) {
		cmd = CMD_UNLOCK;
		device = argv[1];
	} else if ((strcmp(argv[0], "refresh") == 0) && (argc == 2)) {
		cmd = CMD_REFRESH;
		device = argv[1];
	} else if ((strcmp(argv[0], "erase") == 0) && (argc == 2)) {
		cmd = CMD_ERASE;
		device = argv[1];
	} else if ((strcmp(argv[0], "fixtrx") == 0) && (argc == 2)) {
		cmd = CMD_FIXTRX;
		device = argv[1];
	} else if ((strcmp(argv[0], "write") == 0) && (argc == 3)) {
		cmd = CMD_WRITE;
		device = argv[2];

		if (strcmp(argv[1], "-") == 0) {
			imagefile = "<stdin>";
			imagefd = 0;
		} else {
			imagefile = argv[1];
			if ((imagefd = open(argv[1], O_RDONLY)) < 0) {
				fprintf(stderr, "Couldn't open image file: %s!\n", imagefile);
				exit(1);
			}
		}

		if (!mtd_check(device)) {
			fprintf(stderr, "Can't open device for writing!\n");
			exit(1);
		}
		/* check trx file before erasing or writing anything */
		if (!image_check(imagefd, device) && !force) {
			fprintf(stderr, "Image check failed.\n");
			exit(1);
		}
	} else if ((strcmp(argv[0], "jffs2write") == 0) && (argc == 3)) {
		cmd = CMD_JFFS2WRITE;
		device = argv[2];

		imagefile = argv[1];
		if (!mtd_check(device)) {
			fprintf(stderr, "Can't open device for writing!\n");
			exit(1);
		}
	} else {
		usage();
	}

	sync();

	i = 0;
	unlocked = 0;
	while (erase[i] != NULL) {
		mtd_unlock(erase[i]);
		mtd_erase(erase[i]);
		if (strcmp(erase[i], device) == 0)
			unlocked = 1;
		i++;
	}

	switch (cmd) {
		case CMD_UNLOCK:
			if (!unlocked)
				mtd_unlock(device);
			break;
		case CMD_ERASE:
			if (!unlocked)
				mtd_unlock(device);
			mtd_erase(device);
			break;
		case CMD_WRITE:
			if (!unlocked)
				mtd_unlock(device);
			mtd_write(imagefd, device, fis_layout);
			break;
		case CMD_JFFS2WRITE:
			if (!unlocked)
				mtd_unlock(device);
			mtd_write_jffs2(device, imagefile, jffs2dir);
			break;
		case CMD_REFRESH:
			mtd_refresh(device);
			break;
		case CMD_FIXTRX:
			mtd_fixtrx(device, offset);
			break;
	}

	sync();

	if (boot)
		do_reboot();

	return 0;
}
