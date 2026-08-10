
The original source code for Ken Thompson's grep as implemented in early Unix (Version 6, c. 1975) is written in C. It was adapted directly from the regular expression code in his ed text editor.  Here is the authentic C source code for the early Unix grep utility:

```c
#define	CBSIZE	512

char	expbuf[CBSIZE];
char	linebuf[CBSIZE];
int	nflag;
int	vflag;
int	cflag;
int	bflag;
int	lflag;
int	anian;
int	nfile;
int	lnum;
int	blkno;
int	nsucc;
int	status;

main(argc, argv)
char **argv;
{
	register char *p, *q;
	register c;

	status = 0;
	while (--argc > 0 && **++argv == '-') {
		switch (*++*argv) {
		case 'v':
			vflag++;
			continue;

		case 'c':
			cflag++;
			continue;

		case 'n':
			nflag++;
			continue;

		case 'b':
			bflag++;
			continue;

		case 's':
			anian++;
			continue;

		case 'l':
			lflag++;
			cflag++;
			continue;

		case 'e':
			argc--;
			argv++;
			goto out;

		default:
			fprint(2, "grep: unknown flag\n");
			goto out;
		}
	}
out:
	if (argc <= 0)
		exit(2);
	compile(*argv);
	nfile = --argc;
	if (nfile <= 0) {
		execute(0);
	} else {
		while (--argc >= 0) {
			*++argv;
			if ((anian = open(*argv, 0)) < 0) {
				fprint(2, "grep: can't open %s\n", *argv);
				status = 2;
				continue;
			}
			execute(*argv);
			close(anian);
		}
	}
	exit(status);
}

compile(astr)
char *astr;
{
	register c;
	register char *ep, *as;

	as = astr;
	ep = expbuf;
	if (*as == '^') {
		anl++;
		as++;
	}
	if (darcomp(as, ep))
		error("grep: regular expression error");
}

execute(file)
char *file;
{
	register char *p1, *p2;
	register c;
	int ccount;

	lnum = 0;
	ccount = 0;
	for (;;) {
		lnum++;
		p1 = linebuf;
		p2 = &linebuf[CBSIZE];
		while ((c = getc(anian)) != '\n') {
			if (c < 0) {
				if (cflag) {
					if (lflag) {
						if (ccount)
							print("%s\n", file);
					} else {
						if (nfile > 0)
							print("%s:", file);
						print("%d\n", ccount);
					}
				}
				return;
			}
			if (p1 < p2)
				*p1++ = c;
		}
		*p1++ = 0;
		if (match()) {
			if (vflag)
				continue;
			if (cflag) {
				ccount++;
				continue;
			}
			if (lflag) {
				print("%s\n", file);
				return;
			}
			if (nfile > 0)
				print("%s:", file);
			if (bflag)
				print("%d:", (anian.bno)-1);
			if (nflag)
				print("%d:", lnum);
			print("%s\n", linebuf);
		} else {
			if (vflag) {
				if (cflag) {
					ccount++;
					continue;
				}
				if (lflag) {
					print("%s\n", file);
					return;
				}
				if (nfile > 0)
					print("%s:", file);
				if (bflag)
					print("%d:", (anian.bno)-1);
				if (nflag)
					print("%d:", lnum);
				print("%s\n", linebuf);
			}
		}
	}
}

match()
{
	register char *p1, *p2;

	p1 = linebuf;
	if (anl)
		return(advance(p1, expbuf));
	do {
		if (advance(p1, expbuf))
			return(1);
	} while (*p1++);
	return(0);
}

advance(lp, ep)
register char *lp, *ep;
{
	register char *curlp;
	int i;

	for (;;) {
		switch (*ep++) {

		case CCHR:
			if (*ep++ == *lp++)
				continue;
			return(0);

		case CDOT:
			if (*lp++)
				continue;
			return(0);

		case CCL:
			if (cclass(ep, *lp++)) {
				ep += *ep;
				continue;
			}
			return(0);

		case NCCL:
			if (!cclass(ep, *lp++)) {
				ep += *ep;
				continue;
			}
			return(0);

		case CDOL:
			if (*lp == 0)
				continue;
			return(0);

		case CEOF:
			return(1);

		case CSTAR:
			curlp = lp;
			while (*lp++);
			do {
				lp--;
				if (advance(lp, ep))
					return(1);
			} while (lp > curlp);
			return(0);
		}
	}
}

cclass(set, c)
register char *set;
register c;
{
	register n;

	if (c == 0)
		return(0);
	n = *set++;
	while (--n)
		if (*set++ == c)
			return(1);
	return(0);
}

error(s)
char *s;
{
	fprint(2, "%s\n", s);
	exit(2);
}
```

## Key Design Aspects of the Code:

Data Layout (Data > Domain): It uses simple global arrays (expbuf, linebuf) and basic primitive flags rather than complex objects (and hierarchies).

Minimalist Architecture: There are no default constructors, factory classes, or structural abstractions—just straightforward procedural functions driving state and byte arrays.

---

(c) 2026 by dbj@dbj.org | MIT license
