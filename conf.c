/* vim: set noexpandtab:tabstop=8 */
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <Windows.h>
#include <math.h>
#include <stdlib.h>
#include <wchar.h>
#include "conf.h"
#include "wse.h"
#include "re.h"

enum {
	Boolean,
	Int,
	Color,
	Style,
	Float,
	String,
	Keyword
};

struct field {
	wchar_t	*name;
	int	type;
	void	*ptr;
};

static struct	field fields[] = {
		{L"bg_color", Color, &conf.bg},
		{L"bg_color2", Color, &conf.bg2},
		{L"fg_color", Color, &conf.fg},
		{L"select_color", Color, &conf.selbg},
		{L"bg_image", String, &conf.bgimage},
		{L"doublebuffer", Boolean, &conf.doublebuffer},
		
		{L"style0", Style, &conf.style[0]},
		{L"style1", Style, &conf.style[1]},
		{L"style2", Style, &conf.style[2]},
		{L"style3", Style, &conf.style[3]},
		{L"style4", Style, &conf.style[4]},
		{L"style5", Style, &conf.style[5]},
		{L"style6", Style, &conf.style[6]},
		{L"style7", Style, &conf.style[7]},
		
		{L"font_name", String, &conf.fontname},
		{L"font_size", Float, &conf.fontsz},
		{L"font_aspect", Float, &conf.fontasp},
		{L"font_weight", Float, &conf.weight},
		{L"font_smoothing", Float, &conf.smooth},
		{L"font_italic", Boolean, &conf.italic},
		{L"line_height", Float, &conf.leading},
		
		{L"ext", String, &lang.ext},
		{L"comment", String, &lang.comment},
		{L"comment_color", Int, &lang.commentcol},
		{L"break", String, &lang.brk},
		{L"brace", String, &lang.brace},
		{L"kwd", Keyword, 0},
		{L"cmd_wrapper", String, &lang.cmdwrapper},
		
		{L"wire", Int, conf.wire},
		{L"wire2", Int, conf.wire+1},
		{L"wire3", Int, conf.wire+2},
		{L"wire4", Int, conf.wire+3},
		{L"tab_width", Int, &conf.tabc},
		{L"use_tabs", Boolean, &conf.usetabs},
		{L"use_bom", Boolean, &conf.usebom},
		{L"use_crlf", Boolean, &conf.usecrlf},
		{L"cols", Int, &conf.cols},
		{L"rows", Int, &conf.rows},
		
		{L"shell", String, &shell},
		{0}
		};

static LOGFONT	lf = {
		0, 0, 0,0, FW_NORMAL, 0,0,0,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		FF_DONTCARE|DEFAULT_PITCH, 0
		};
extern HFONT	font[4];

wchar_t		shell[128]=L"cmd";

configfont() {
	HDC		hdc;
	int		dpi,i;
	TEXTMETRIC	tm;
	
	if (font[0]) {
		DeleteObject(font[0]);
		DeleteObject(font[1]);
		DeleteObject(font[2]);
		DeleteObject(font[3]);
	}
	
	/* Get device resolution */
	hdc = GetDC(0);
	dpi = GetDeviceCaps(hdc, LOGPIXELSY);
	
	/* Create font */
	lf.lfHeight = -conf.fontsz * dpi/72.0;
	lf.lfWidth = fabs(lf.lfHeight) * conf.fontasp;
	lf.lfItalic = conf.italic;
	lf.lfWeight = conf.weight * 1000;
	lf.lfQuality = conf.smooth
		? (conf.smooth<=0.5? ANTIALIASED_QUALITY: CLEARTYPE_QUALITY)
		: NONANTIALIASED_QUALITY;
	wcscpy(lf.lfFaceName, conf.fontname);
	font[0] = CreateFontIndirect(&lf); /* Regular */
	
	lf.lfItalic ^= 1;
	font[2] = CreateFontIndirect(&lf); /* Italic */
	
	lf.lfItalic ^= 1;
	lf.lfWeight = lf.lfWeight==900? 400: 900;
	font[1] = CreateFontIndirect(&lf); /* Bold */
	
	lf.lfItalic ^= 1;
	font[3] = CreateFontIndirect(&lf); /* Bold & Italic */
	
	/* Get metrics */
	SelectObject(hdc, font[0]);
	GetTextMetrics(hdc, & tm);
	GetCharWidth32(hdc, 0, 65535, conf.widths);
	ReleaseDC(0, hdc);
	
	conf.aheight = tm.tmHeight;
	conf.lheight = conf.aheight * conf.leading
		+ tm.tmExternalLeading;
	conf.em = tm.tmAveCharWidth;
	conf.tabw = conf.em * conf.tabc;
	return 1;
}

deflang() {
	*lang.ext=0;
	wcscpy(lang.comment, L"");
	wcscpy(lang.brk, L"~!@#$%^&*()-+={}[]\\|;:'\",.<>/?");
	wcscpy(lang.brace, L"()[]{}''\"\"<>``");
	memset(lang.kwd,0,sizeof lang.kwd);
	wcscpy(lang.cmdwrapper, L"cmd /c %ls & pause >nul");
	lang.nkwd=0;
	lang.commentcol=0;
}

defconfig() {
	memset(conf.style, 0, sizeof conf.style);
	conf.bg = RGB(255,255,255);
	conf.bg2 = RGB(245,245,245);
	conf.fg = RGB(64,64,64);
	conf.selbg = RGB(160,160,192);
	conf.style[0].color = RGB(160,160,192);
	wcscpy(conf.bgimage, L"");
	
	conf.doublebuffer=1;
	
	conf.tabc = 4;
	conf.usetabs = 0;
	conf.usebom = 0;
	conf.usecrlf = 0;
	conf.wire[0] = 64;
	conf.wire[1] = 72;
	conf.wire[2] = 80;
	conf.wire[3] = 128;
	conf.cols = 80;
	conf.rows = 24;
	
	wcscpy(conf.fontname, L"Courier New");
	conf.fontsz = 12.0;
	conf.fontasp = 0.0;
	conf.leading = 1.5;
	conf.smooth = 1.0;
	conf.italic = 0;
	conf.weight = .4;
	
	configfont();
	return 1;
}

static
directive(wchar_t *s) {
	int	x;
	wchar_t	*e;

	while (iswspace(*s))
		s++;
	
	if (!*s) {
		confset[nconfs++]=conf;
		defconfig();
	}
	else if (isdigit(*s))
		selectconfig(wcstol(s, 0, 10));
	else if (*s==L'.') {
		x=0;
		s=lang.ext;
		while (iswspace(*s))
			s++;
		
		while (e=wcschr(s, L',')) {
			*e=0;
			s=e+1;
		}
		
		langset[nlangs++]=lang;
		deflang();
	}
	return 1;
}

getcolor(wchar_t *arg) {
	double		h, s, v; /* hue,chroma,luma */
	int		r,g,b;
	
	if (3 == swscanf(arg, L"%lf\x00b0 %lf %lf", &h,&s,&v)
	|| 3 == swscanf(arg, L"%lf* %lf %lf", &h,&s,&v)) {
		double r,g,b, f, p,q,t;
		if (s == 0.0) {
			r=g=b=v;
			return ((unsigned)(b*255)<<16)+
				((unsigned)(g*255)<<8)+
				((unsigned)(r*255));
		}
		h /= 60.0;
		f = h - floor(h);
		p = v * (1.0 - s);
		q = v * (1.0 - s * f);
		t = v * (1.0 - s * (1.0 - f));
		if (h < 1.0) r=v, g=t, b=p;
		else if (h < 2.0) r=q, g=v, b=p;
		else if (h < 3.0) r=p, g=v, b=t;
		else if (h < 4.0) r=p, g=q, b=v;
		else if (h < 5.0) r=t, g=p, b=v;
		else r=v, g=p, b=q;
		
		return ((unsigned)(b*255)<<16)+
			((unsigned)(g*255)<<8)+
			((unsigned)(r*255));
	} else {
		swscanf(arg, L"%d %d %d", &r,&g,&b);
		return (b<<16)+(g<<8)+r;
	}
	return 0;
}

static
configline(int ln, wchar_t *s) {
	wchar_t		*arg;
	struct textstyle *style;
	struct field	*cf;
	
	while (iswspace(*s))
		s++;
	if (*s=='#' || !*s)
		return 0;
	if (*s=='.')
		return directive(s+1);
	
	/* Cut field name */
	for (arg=s; *arg && !iswspace(*arg); arg++);
	*arg++=0;
	while (iswspace(*arg)) arg++;
	
	/* Find field */
	for (cf=fields; cf->name && wcscmp(cf->name, s); cf++);
	if (!cf->name)
		return 0;
	
	/* Remove comment if not string or keyword */
	if (cf->type != String && cf->type != Keyword)
		arg[wcscspn(arg, L"#")] = 0;
	
	switch (cf->type) {
	
	case Boolean:
		if (!wcscmp(arg,L"yes")) *(int*)cf->ptr = 1;
		else if (!wcscmp(arg,L"true")) *(int*)cf->ptr = 1;
		else *(int*)cf->ptr = 0;
		while (*++arg);
		return 1;
	
	case Int:
		*(int*)cf->ptr = wcstol(arg, &arg, 0);
		return 1;

	case Color:
		*(int*)cf->ptr = getcolor(arg);
		return 1;
	
	case Style:
		style = (struct textstyle*)cf->ptr;
		style->style = 0;
		for (;;) {
			while (iswspace(*arg))
				arg++;
			if (!wcsncmp(arg, L"bold", 4))
				arg+=4, style->style |= 1;
			else if (!wcsncmp(arg, L"italics", 7))
				arg+=7, style->style |= 2;
			else if (!wcsncmp(arg, L"italic", 6))
				arg+=6, style->style |= 2;
			else
				break;
		}
		style->color = getcolor(arg);
		return 1;
	
	case Float:
		*(double*)cf->ptr = wcstod(arg, &arg);
		return 1;
	
	case String:
		wcscpy(cf->ptr, arg);
		return 1;
	
	case Keyword:
		lang.kwdcol[lang.nkwd] = wcstoul(arg,&arg,0);
		while (iswspace(*arg))
			arg++;
		re_comp(lang.kwd[lang.nkwd], arg);
		lang.nkwd++;
		return 1;
	}
	return 0;
}

loadconfig(wchar_t *fn) {
	HANDLE		file;
	DWORD		sz, ign;
	int		c,ln,eol;
	char		*utf;
	wchar_t		*buf, *s;
	
	file=CreateFile(fn, GENERIC_READ, FILE_SHARE_READ, 0,
		OPEN_EXISTING, 0, 0);
	if (file==INVALID_HANDLE_VALUE)
		return 0;
	
	sz=GetFileSize(file, 0);
	utf=malloc(sz+1);
	ReadFile(file, utf, sz, &ign, 0);
	CloseHandle(file);
	utf[sz]=0;
	
	sz=MultiByteToWideChar(CP_UTF8, 0, utf, sz+1, 0, 0);
	buf=malloc((sz+1) * sizeof (wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, utf, sz+1, buf, sz+1);
	free(utf);
	
	defconfig();
	deflang();
	confset[nconfs]=conf;
	
	s=buf;
	ln=0;
	while (*s) {
		ln++;
		eol=wcscspn(s, L"\r\n");
		c=s[eol];
		if (c=='\r')
			s[eol++]=0;
		else if (c=='\n')
			s[eol]=0;
		
		configline(ln, s);
		
		s += eol + (c!=0);
	}
	free(buf);
	
	configfont();

	return 1;
}

selectconfig(int x) {
	if (nconfs<=x || x<0)
		return 0;
	curconf=x;
	conf=confset[x];
	return 1;
}

config() {
	wchar_t	path[MAX_PATH];
	
	nconfs=0;
	nlangs=0;
	
	GetModuleFileName(0, path, MAX_PATH);
	wcscpy(wcsrchr(path, L'\\')+1, L"wse.conf");
	if (loadconfig(path)) {
		configfile=wcsdup(path);
		return 1;
	}
	configfile=L"";
	defconfig();
	deflang();
	confset[nconfs++]=conf;
	return 0;
}
