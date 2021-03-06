/* vim: set noexpandtab:tabstop=8 */
struct textstyle {
	int		style;
	unsigned	color;
};
struct conf {
	int		tabc;
	int		usetabs;
	int		usebom;
	int		usecrlf;
	int		rows;
	int		cols;
	int		wire[4];
			
	int		bg;
	int		fg;
	int		bg2;
	int		selbg;
	struct textstyle style[8];
	wchar_t		bgimage[128];
	
	int		doublebuffer;
	
	/* Font specifications */
	wchar_t		fontname[128];
	double		fontsz;
	double		fontasp;
	double		leading;
	double		weight;
	double		smooth;
	int		italic;
	
	/* Derived Font metrics */
	int		aheight; /* ascender height */
	int		lheight; /* height w/ leading */
	int		em;
	int		tabw;
	int		widths[65536];
} conf;

struct lang {
	wchar_t		ext[128];
	wchar_t		comment[128];
	wchar_t		brk[128];
	wchar_t		brace[128];
	wchar_t		kwd[64][256];
	wchar_t		kwdcol[64];
	wchar_t		cmdwrapper[128];
	int		nkwd, commentcol;
} lang;

wchar_t		shell[128];

int		file_usecrlf;
int		file_usetabs;
int		file_tabc;
int		file_tabw;
int		file_usebom;

wchar_t		*configfile;
int		nconfs;
int		curconf;
struct conf	confset[32];

int		nlangs;
int		curlang;
struct lang	langset[32];

