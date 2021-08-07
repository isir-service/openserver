#ifndef __TAR_H__
#define __TAR_H__
#define	RECORDSIZE	512
#define	NAMSIZ	100
#define	TUNMLEN	32
#define	TGNMLEN	32

union record {
	unsigned char	charptr[RECORDSIZE];
	struct header {
		char	name[NAMSIZ];
		char	mode[8];
		char	uid[8];
		char	gid[8];
		char	size[12];
		char	mtime[12];
		char	chksum[8];
		char	linkflag;
		char	linkname[NAMSIZ];
		char	magic[8];
		char	uname[TUNMLEN];
		char	gname[TGNMLEN];
		char	devmajor[8];
		char	devminor[8];
	} header;
};

/* The magic field is filled with this if uname and gname are valid. */
#define	TMAGIC		"ustar"		/* 5 chars and a null */
#define	GNUTMAGIC	"ustar  "	/* 7 chars and a null */

#endif
