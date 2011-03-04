#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#define STRICT
#define UNICODE

#include <Windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <stdlib.h>
#include <wchar.h>
#include "conf.h"
#include "wse.h"
#include "action.h"

#define	BOT	(top+vis)

HWND		w;
HMENU		menu;
HWND		dlg;
HBITMAP		dbmp;
HBITMAP		bgbmp;
HBRUSH		bgbrush;
HPEN		bgpen;
HFONT		font,bfont;
UINT		WM_FIND;
Loc		click;
int		width;
int		height;

OPENFILENAME	ofn = {
			sizeof ofn,
			0, 0, 0,
			0, 0, 0, 0, MAX_PATH, 0, 0, 0, 0,
			OFN_OVERWRITEPROMPT,
			0, 0, 0, 0, 0, 0, 0, 0 };
FINDREPLACE	fr = {
			sizeof fr, 0, 0,
			FR_DOWN|FR_DIALOGTERM
			|FR_HIDEMATCHCASE|FR_HIDEWHOLEWORD,
			0, 0, 1024, 1024, 0, 0, 0 };
FINDREPLACE	gofr = {
			sizeof gofr, 0, 0,
			FR_HIDEUPDOWN|FR_HIDEMATCHCASE
			|FR_HIDEWHOLEWORD|FR_DIALOGTERM, 0, 0,
			1024, 1024, 0, 0, 0 };
WNDCLASSEX	wc = {
			sizeof wc,
			CS_VREDRAW|CS_HREDRAW,
			0, 0, 0, 0, 0, 0, 0,
			0, L"Window", 0 };

INT_PTR CALLBACK
SpawnProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	int	len;
	
	switch (msg) {
	case WM_INITDIALOG:
		dlg=hwnd;
		return TRUE;
	case WM_CLOSE:
		dlg=0;
		DestroyWindow(hwnd);
		return TRUE;
	case WM_COMMAND:
		switch (wparam) {
		case IDOK:
		case 105:
			len=GetWindowText(GetDlgItem(hwnd,100),
				lastcmd_cmd,
				sizeof lastcmd_cmd/sizeof *lastcmd_cmd);
			GetWindowText(GetDlgItem(hwnd,101),
				lastcmd_arg,
				sizeof lastcmd_arg/sizeof *lastcmd_arg);
			swprintf(lastcmd,
				sizeof lastcmd/sizeof *lastcmd,
				L"%ls %ls",
				lastcmd_cmd,
				lastcmd_arg);
			dlg=0;
			DestroyWindow(hwnd);
			act(wparam==IDOK? SpawnLastCmd: CaptureSpawn);
			return TRUE;
		case IDCANCEL:
			dlg=0;
			DestroyWindow(hwnd);
			return TRUE;
		}
	}
	return FALSE;
}

static void*
makedlgitem(void *mem, DLGITEMTEMPLATE *it, int class, wchar_t *txt) {
	mem=(void*)((LONG_PTR)mem + 3 & ~3); /* align */
	memcpy(mem,it,sizeof *it);
	mem=(DLGITEMTEMPLATE*)mem + 1;
	*((WORD*)mem)++=0xffff;	/* class */
	*((WORD*)mem)++=class;	/* button */
	mem=wcscpy(mem,txt) + wcslen(txt)+1;
	*((WORD*)mem)++=0;	/* creation data */
	return mem;
}

static
openspawn(HWND hwnd, wchar_t *initcmd, wchar_t *initarg) {
	static char	buf[4096];
	void		*mem;
	wchar_t		*title=L"Spawn";
	struct {
		DLGITEMTEMPLATE	dlg;
		DWORD		cls;
		wchar_t		*txt;
	} *i, items[] = {
		{{WS_BORDER|WS_VISIBLE|WS_TABSTOP,0, 26,8,230,14, 100},
			0x81, initcmd},
		{{WS_BORDER|WS_VISIBLE|WS_TABSTOP,0, 26,8+20,230,14, 101},
			0x81, initarg},
		{{WS_VISIBLE,0, 4,10,22,14, 103},
			0x82, L"cmd: "},
		{{WS_VISIBLE,0, 4,30,22,14, 104},
			0x82, L"arg: "},
		{{WS_VISIBLE|BS_DEFPUSHBUTTON|WS_TABSTOP,0, 300-36,2,28,16, IDOK},
			0x80, L"OK"},
		{{WS_VISIBLE|WS_TABSTOP,0, 300-36,20,28,16, 105},
			0x80, L"Capture"},
		{{WS_VISIBLE|WS_TABSTOP,0, 300-36,40,28,16, IDCANCEL},
			0x80, L"Cancel"}
	};
	DLGTEMPLATE	dlg = { WS_SYSMENU|WS_VISIBLE, 0,
				sizeof items/sizeof *items,
				0,0, 300,80};
	
	memcpy(buf, &dlg, sizeof dlg);
	mem=buf + sizeof dlg;
	*((WORD*)mem)++=0;	/* menu */
	*((WORD*)mem)++=0;	/* class */
	mem=wcscpy(mem,title) + wcslen(title)+1;
	for (i=items; i < items+dlg.cdit; i++)
		mem=makedlgitem(mem, &i->dlg, i->cls, i->txt);
	
	CreateDialogIndirect(GetModuleHandle(0),(void*)buf,
		hwnd,SpawnProc);
}
			
static
settitle(int mod) {
	wchar_t	all[MAX_PATH];
	
	swprintf(all, MAX_PATH, L"%s%ls%ls%ls",
		mod? "*": "",
		filebase,
		*fileext? ".": "",
		fileext);
	SetWindowText(w, all);
}

static
setfilename(wchar_t *fn) {
	wchar_t	*s=fn, *e;

	free(filename);
	filename=wcsdup(fn);
	
	if ((e=wcsrchr(fn, L'\\')) || (e=wcsrchr(fn, L'/'))) {
		wcsncpy(filepath, fn, e-s);
		filepath[e-s]=0;
		e++;
	} else {
		GetCurrentDirectory(512, filepath);
		e=fn;
	}
	
	s=e;
	if ((e=wcsrchr(s, L'.')) && s!=e) {
		wcsncpy(filebase, s, e-s);
		filebase[e-s]=0;
		wcscpy(fileext, e+1);
	} else {
		wcscpy(filebase, s);
		fileext[0]=0;
	}
	
	reinitlang();
}

alertchange(int mod) {
	settitle(mod);
}

alertabort(wchar_t *msg, wchar_t *re) {
	wchar_t	buf[1024];
	swprintf(buf, 1024, L"error: %ls at /...%ls/",
		msg, re);
	MessageBox(w, buf, L"Expression Error", MB_OK);
}

/* This does NOT snap the caret into view because the
 * scrolling routines need to use it
 */
static
movecaret() {
	int		x,y;
	SCROLLINFO	si;
	
	x = ind2px(LN, IND);
	y = line2px(LN);
	if (y > height-conf.lheight)
		y += conf.lheight;
	if (GetFocus()==w)
		SetCaretPos(x, y);
	
	/* Set up the scroll bars */
	si.cbSize = sizeof si;
	si.fMask = SIF_ALL ^ SIF_TRACKPOS;
	si.nMin = 1;
	si.nMax = NLINES;
	si.nPage = vis;
	si.nPos = top;
	SetScrollInfo(w, SB_VERT, &si, 1);
	return 1;
}

static
snap() {
	if (LN < top)
		top=sat(1, LN-1, NLINES);
	else if (BOT <= LN)
		top=sat(1, LN-vis+1, NLINES-vis+1);
	else
		return movecaret();
	invdafter(top);
	return 0;
}

/*
 * Invalidate the given lines and update the display.
 * This comes packaged with snapping to the caret, so if
 * you don't want that, you should call InvalidateRect().
 */
static
invd(int lo, int hi) {
	RECT	rt;
	
	snap();
	rt.top=line2px(lo);
	rt.bottom=line2px(hi+1);
	rt.left=0;
	rt.right=width;
	InvalidateRect(w, &rt, 0);
}

static
invdafter(int lo) {
	invd(lo, BOT);
}

static
commitlatch() {
	wchar_t	*txt;
	if (!latch)
		return 0;

	/* MSDN says you need GMEM_MOVEABLE, but that
	 * doesn't work
	 */
	txt=GlobalAlloc(0, (wcslen(latch)+1)*sizeof (wchar_t));
	wcscpy(txt, latch);
	free(latch);
	latch=0;
	
	OpenClipboard(w);
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, txt);
	CloseClipboard();
	return 1;
}

generalinvd(int onlines, int wassel, Loc *olo, Loc *ohi) {
	int	selnow;
	Loc	lo,hi;
	
	selnow=ordersel(&lo, &hi);
	if (onlines != NLINES)
		invdafter(top);
	else if (wassel != selnow && (wassel||selnow))
		invdafter(top);
	else if (selnow && !samerange(olo, ohi, &lo, &hi))
		invd(min(olo->ln, lo.ln), max(ohi->ln, hi.ln));
}

act(int action) {
	
	STARTUPINFO		si;
	PROCESS_INFORMATION	pi;
	int	ok, wassel, onlines,oldconf;
	int	ln, ind;
	Loc	lo,hi;
	DWORD	sz;
	wchar_t	*txt;
	
	onlines=NLINES;
	wassel=ordersel(&lo, &hi);
	
	switch (action) {
	
	case PasteClipboard:
		if (!OpenClipboard(w))
			return 0;
		latch = GetClipboardData(CF_UNICODETEXT);
		break;
		
	case ReloadConfig:
	case PrevConfig:
	case NextConfig:
		oldconf=curconf;
		break;
	}
	
	ok = _act(action);
	
	switch (action) {
	
	case QuitApp:
		SendMessage(w, WM_CLOSE,0,0);
		break;
	
	case ForkApp:
		ZeroMemory(&si, sizeof si);
		ZeroMemory(&pi, sizeof pi);
		si.cb=sizeof si;
		txt=malloc(MAX_PATH*sizeof(wchar_t));
		GetModuleFileName(0, txt, MAX_PATH);
		if (CreateProcess(txt,0, 0,0,0,0,0,0,&si, &pi)) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		free(txt);
		break;
	
	case SpawnApp:
		ZeroMemory(&si, sizeof si);
		ZeroMemory(&pi, sizeof pi);
		si.cb=sizeof si;
		if (CreateProcess(0,shell, 0,0,0,0,0,0,&si, &pi)) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		break;
		
	case SpawnLastCmd:
	case CaptureSpawn:
		ZeroMemory(&si, sizeof si);
		ZeroMemory(&pi, sizeof pi);
		si.cb=sizeof si;
		
		{
		wchar_t	*cmd, wrap[MAX_PATH*3];
		
		cmd=action==SpawnLastCmd
			? L"cmd /c \"%ls & pause >nul\""
			: L"cmd /c \"%ls 1>wse~stdout"
			  L" & start wse wse~stdout\" ",
		
		swprintf(wrap,
			sizeof wrap/sizeof *wrap,
			cmd,
			lastcmd);
		if (CreateProcess(0,wrap, 0,0,0,0,0,0,&si, &pi)) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		}
		break;
	
	case PromptSpawn:
		ok=openspawn(w, lastcmd_cmd, lastcmd_arg);
		break;
	
	case SpawnSelection:
		txt=copysel();
		ok=openspawn(w, lastcmd_cmd, txt);
		free(txt);
		break;
	
	case NewFile:
		top=1;
		b->changes=0;
		setfilename(L"//Untitled");
		settitle(0);
		break;
	
	case LoadFile:
		if (!ok)
			MessageBox(w, L"Could not load",
				L"Error", MB_OK);
		settitle(0);
		
		/* Can't rely on generalinvd() because the
		 * selection and line counts might not change
		 */
		invdafter(1);
		break;
	
	case ReloadFileUTF8:
	case ReloadFileUTF16:
	case ReloadFile:
		if (!ok)
			MessageBox(w, L"Could not load",
				L"Error", MB_OK);
		settitle(0);
		snap();
		invdafter(top);
		return ok;
	
	case SaveFileUTF8:
	case SaveFileUTF16:
	case SaveFile:
		if (!ok)
			MessageBox(w, L"Could not save",
				L"Error", MB_OK);
		else {
			b->changes=0;
			settitle(0);
		}
		break;
	
	case DeleteLine:
	case JoinLine:
	case DupLine:
	case BreakLine:
		/* Line monitor will handle */
		break;
	
	case SelectAll:
	case StartSelection:
	case IndentSelection:
	case UnindentSelection:
	case EndSelection:
		/* The selection monitor handles updates */
		break;
	case CommentSelection:
		if (!wassel)
			invd(LN, LN);
		/* The selection monitor handles updates */
		break;
	
	case MoveUp:
	case MoveDown:
	case MoveLeft:
	case MoveRight:
	case MoveWordLeft:
	case MoveWordRight:
	case MoveHome:
	case MoveEnd:
	case MovePageUp:
	case MovePageDown:
	case MoveSof:
	case MoveEof:
		snap();
		invd(BOT,BOT);
		break;
	
	case DeleteChar:
	case BackspaceChar:
	case DeleteSelection:
		/* If the update was simple, invalidate the
		 * line; if it was not, the line or selection
		 * monitor will handle it
		 */
		invd(LN, LN);
		break;
	
	case AscendLine:
		if (ok)
			invd(LN, LN+1);
		break;
		
	case DescendLine:
		if (ok)
			invd(LN-1, LN);
		break;
	
	case CutSelection:
		commitlatch();
		break;

	case CopySelection:
		commitlatch();
		break;
		
	case PasteClipboard:
		latch=0;
		CloseClipboard();
		invd(LN, LN);
		break;
	
	case UndoChange:
	case RedoChange:
		if (ok)
			invdafter(top);
		break;
	
	case PromptOpen:
		if (dlg)
			break;
		ofn.lpstrInitialDir=filepath;
		ok=GetOpenFileName(&ofn);
		if (!ok)
			break;
		setfilename(ofn.lpstrFile);
		ok=act(LoadFile);
		break;
	
	case PromptSaveAs:
		if (dlg)
			break;
		ofn.lpstrInitialDir=filepath;
		ok=GetSaveFileName(&ofn);
		if (!ok)
			break;
		setfilename(ofn.lpstrFile);
		ok=act(SaveFile);
		break;
	
	case PromptGo:
		if (dlg)
			break;
		gofr.Flags &= ~FR_DIALOGTERM;
		dlg=FindText(&gofr);
		SetWindowText(dlg, L"Go to Line");
		break;
	
	case PromptFind:
		if (dlg)
			break;
		fr.Flags &= ~FR_DIALOGTERM;
		dlg=FindText(&fr);
		break;
	
	case PromptReplace:
		if (dlg)
			break;
		fr.Flags &= ~FR_DIALOGTERM;
		dlg=ReplaceText(&fr);
		break;
	
	case ReloadConfig:
	case PrevConfig:
	case NextConfig:
		if (ok) {
			if (action==ReloadConfig)
				selectconfig(oldconf);
			reinitconfig();
			invdafter(top);
		}
		break;
	case EditConfig:
		ZeroMemory(&si, sizeof si);
		ZeroMemory(&pi, sizeof pi);
		si.cb=sizeof si;
		txt=malloc((MAX_PATH*2+1)*sizeof(wchar_t));
		sz=GetModuleFileName(0, txt, MAX_PATH);
		swprintf(txt+sz, MAX_PATH*2-sz, L" \"%ls\"",
			configfile);
		if (CreateProcess(0,txt, 0,0,0,0,0,filepath,&si, &pi)) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		free(txt);
		break;
	
	default:
		invdafter(top);
	}
	generalinvd(onlines, wassel, &lo, &hi);
	return ok;
}

actins(int c) {
	int	ok, wassel, onlines;
	Loc	lo,hi;
	wchar_t	*txt;
	
	onlines=NLINES;
	wassel=ordersel(&lo, &hi);
	
	if (!_actins(c))
		return 0;
	invd(LN, LN);
	generalinvd(onlines, wassel, &lo, &hi);
	return 1;
}

actquery(wchar_t *query, int down, int sens) {
	wchar_t	title[2];
	if (!_actquery(query, down, sens))
		return 0;
	GetWindowText(dlg, title, 2);
	if (dlg && title[0]=='F')
		SendMessage(dlg, WM_CLOSE, 0, 0);
	invdafter(top);
	return 1;
}

actreplace(wchar_t *query, wchar_t *repl, int down, int sens) {
	if (!_actreplace(query, repl, down, sens))
		return 0;
	invdafter(top);
	return 1;
}

actreplaceall(wchar_t *query, wchar_t *repl, int down, int sens) {
	int	n;
	if (dlg)
		SendMessage(dlg, WM_CLOSE, 0, 0);
	n=_actreplaceall(query, repl, down, sens);
	if (!n)
		return 0;
	invdafter(top);
	return n;
}

autogo() {
	int	n,ok;
	wchar_t	*after;

	n=wcstol(gofr.lpstrFindWhat, &after, 0);

	if (n<1 || NLINES<n)
		return 0;
	if (*after && !iswspace(*after))
		return 0;
	
	if (dlg)
		SendMessage(dlg, WM_CLOSE, 0, 0);
	gob(b, n, 0);
	act(MoveHome);
	return 1;
}

autoquery() {
	return actquery(fr.lpstrFindWhat,
		fr.Flags & FR_DOWN,
		fr.Flags & FR_MATCHCASE);
}

autoreplace() {
	return actreplace(fr.lpstrFindWhat,
		fr.lpstrReplaceWith,
		fr.Flags & FR_DOWN,
		fr.Flags & FR_MATCHCASE);
}

autoreplaceall() {
	return actreplaceall(fr.lpstrFindWhat,
		fr.lpstrReplaceWith,
		fr.Flags & FR_DOWN,
		fr.Flags & FR_MATCHCASE);
}

wmchar(int c) {
	
	int	ctl=GetAsyncKeyState(VK_CONTROL) & 0x8000;
	int	shift=GetAsyncKeyState(VK_SHIFT) & 0x8000;
	
	switch (c) {
	
	case 1: /* ^A */
		return act(SelectAll);
	
	case 2: /* ^B */
		setsel(shift);
		if (ctl)
			return act(MoveBrace);
		return 0;
	
	case 3: /* ^C */
		return act(CopySelection);
	
	case 4: /* ^D */
		return act(DupLine);
		
	case 5: /* ^E */
		return 0;
	
	case 6: /* ^F */
		if (shift)
			return autoquery();
		return act(PromptFind);
	
	case 7: /* ^G */
		return act(PromptGo);
	
	case 8: /* ^H Bksp */
		return act(BackspaceChar);
	
	case 9: /* ^I Tab */
		if (!SLN)
			goto normal;
		if (shift)
			return act(UnindentSelection);
		return act(IndentSelection);
	
	case 10: /* ^J */
		return act(JoinLine);

	case 11: /* ^K */
		return 0;
	
	case 12: /* ^L */
		return act(DeleteLine);
		
	case 13: /* ^M Enter */
		if (shift)
			return act(SpaceBelow);
		return act(BreakLine);
	
	case 14: /* ^N */
		return act(NewFile);
	
	case 15: /* ^O */
		return act(PromptOpen);
	
	case 16: /* ^P */
		if (shift)
			return act(SpaceBoth);
		return act(SpaceAbove);
	
	case 17: /* ^Q */
		return act(CommentSelection);
	
	case 18: /* ^R */
		if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
			return autoreplace();
		return act(PromptReplace);
		
	case 19: /* ^S */
		return act(SaveFile);
	
	case 20: /* ^T */
		return 0;
	
	case 21: /* ^U */
		return 0;
	
	case 22: /* ^V */
		return act(PasteClipboard);
	
	case 23: /* ^W */
		return 0;
	
	case 24: /* ^X */
		return act(CutSelection);
	
	case 25: /* ^Y */
		return act(RedoChange);
		
	case 26: /* ^Z */
		return act(UndoChange);
	
	case 27: /* ^[ Esc */
		return 0;
	
	case 127: /* ^Bksp */
		act(StartSelection);
		act(MoveWordLeft);
		act(DeleteSelection);
		return 0;
	
	default:
normal:
		return actins(c);
		
	}
}

setsel(int yes) {
	if (yes)
		return act(StartSelection);
	return act(EndSelection);
}

wmkey(int c) {

	int	ctl=GetAsyncKeyState(VK_CONTROL) & 0x8000;
	int	shift=GetAsyncKeyState(VK_SHIFT) & 0x8000;
	int	ok;

	switch (c) {
	
	case VK_UP:
		setsel(shift);
		return act(ctl? AscendLine: MoveUp);
	
	case VK_DOWN:
		setsel(shift);
		return act(ctl? DescendLine: MoveDown);
	
	case VK_LEFT:
		setsel(shift);
		return act(ctl? MoveWordLeft: MoveLeft);

	case VK_RIGHT:
		setsel(shift);
		return act(ctl? MoveWordRight: MoveRight);
	
	case VK_HOME:
		setsel(shift);
		return act(ctl? MoveSof: MoveHome);
	
	case VK_END:
		setsel(shift);
		return act(ctl? MoveEof: MoveEnd);
	
	case VK_PRIOR:
		setsel(shift);
		return act(MovePageUp);
	
	case VK_NEXT:
		setsel(shift);
		return act(MovePageDown);
	
	case VK_INSERT:
		if (ctl)
			return act(CopySelection);
		else if (shift)
			return act(PasteClipboard);
		return 0;
		
	case VK_DELETE:
		if (ctl) {
			act(StartSelection);
			act(MoveWordRight);
			act(DeleteSelection);
			return 0;
		}
		return act(shift? CutSelection: DeleteChar);
	
	case VK_F2:
		return act(ctl? SpawnApp: ForkApp);
		
	case VK_F3:
		if (shift) {
			fr.Flags^=FR_DOWN;
			ok=autoquery();
			fr.Flags^=FR_DOWN;
			return ok;
		}
		return autoquery();
	
	case VK_F5:
		return act(ReloadFile);
		
	case VK_F7:
		if (shift && !ctl)
			return act(PromptSpawn);
		else if (!shift && ctl)
			return act(SpawnSelection);
		else if (!shift && !ctl)
			return act(SpawnLastCmd);
	
	case VK_F12:
		if (ctl)
			return act(ReloadConfig);
		return act(shift? PrevConfig: NextConfig);
		
	}
	return 0;
}

paintsel(HDC dc) {
	Loc	lo, hi;
	int	diff,x1,y1,x2,y2;
	
	if (!SLN)
		return 0;
	
	diff = abs(SLN-LN);
	ordersel(&lo, &hi);
	
	SetDCBrushColor(dc, conf.selbg);
	SetDCPenColor(dc, conf.selbg);
	x1=ind2px(lo.ln, lo.ind);
	y1=line2px(lo.ln);
	if (diff)
		Rectangle(dc, x1, y1, width, y1 + conf.lheight);
	
	x1=diff? 0: x1;
	x2=ind2px(hi.ln, hi.ind);
	y2=line2px(hi.ln);
	Rectangle(dc, x1, y2, x2, y2 + conf.lheight);
	
	if (diff > 1)
		Rectangle(dc, 0, y1 + conf.lheight, 
			width, y2);
	return 1;
}

paintstatus(HDC dc) {
	wchar_t	buf[1024];
	wchar_t *selmsg=L"%d:%d of %d Sel %d:%d (%d)";
	wchar_t *noselmsg=L"%d:%d of %d";
	int	len;
	
	SetTextColor(dc, conf.bg);
	SetDCPenColor(dc, conf.fg);
	SetDCBrushColor(dc, conf.fg);
	Rectangle(dc, 0, height-conf.lheight, width, height);
	
	len=swprintf(buf, 1024, SLN? selmsg: noselmsg,
		LN, ind2col(LN, IND),
		NLINES,
		SLN, ind2col(SLN, SIND), abs(SLN-LN));
	TextOut(dc, 0,
		height-conf.lheight+(conf.lheight-conf.aheight)/2,
		buf, len);
}

#include "re.h"

paintlines(HDC dc, int first, int last) {
	int	_y=line2px(first);
	int	line,len,k;
	wchar_t	*txt;
	
	SetTextColor(dc, conf.fg);
	for (line=first; line<=last; line++) {
		int	x=0, y=_y + (conf.lheight-conf.aheight)/2;
		int	i,j,k,sect;
		SIZE	size;
		
		txt = getb(b,line,&len);
		i=j=0;
		while (j<len) {
			/*
			 * Match a keyword followed by break.
			 * Start of line or break precedes it.
			 */
			for (k=0,sect=0; k<lang.nkwd; k++) {
				sect=re_run(txt+j, lang.kwd[k]);
				if (sect>0
				  && (!txt[j+sect]|| brktbl[txt[j+sect]&0xffff]))
					break;
			}
		
			if (sect>0) {
				int bold=lang.kwdbold[k];
				/*
				 * Draw the preceding section
				 * Then draw the keyword
				 */
				TabbedTextOut(dc, x,y, txt+i,
					j-i, 1, &conf.tabw,0);
				x=ind2px(line,j);
				SetTextColor(dc, conf.color[lang.kwdcol[k]]);
				bold && SelectObject(dc,bfont);
				TabbedTextOut(dc, x,y, txt+j,
					sect, 1, &conf.tabw,0);
				bold && SelectObject(dc,font);
				SetTextColor(dc, conf.fg);
				i=j+=sect;
				x=ind2px(line,j);
			} else {
				/*
				 * advance past next punct
				 */
				while (txt[j] && !brktbl[txt[j]&0xffff])
					j++;
				if (txt[j])
					j++;
			}
		}
		if (j>i)
			TabbedTextOut(dc, x,y, txt+i,
				j-i, 1, &conf.tabw,0);
		_y += conf.lheight;
	}
}

paint(PAINTSTRUCT *ps) {
	HDC	dc;
	wchar_t	*txt;
	SIZE	size;
	int	i,n,clen,y,x,len,first,last;

	if (conf.doublebuffer) {
		dc=CreateCompatibleDC(ps->hdc);
		SelectObject(dc, dbmp);
	} else
		dc=ps->hdc;
	SetBkMode(dc, TRANSPARENT);
	SelectObject(dc, font);
	
	first = px2line(ps->rcPaint.top);
	last = px2line(ps->rcPaint.bottom);
	
	/* Clear the background */
	SelectObject(dc, bgbrush);
	SelectObject(dc, bgpen);
	Rectangle(dc, ps->rcPaint.left-1, ps->rcPaint.top-1,
		ps->rcPaint.right+1, ps->rcPaint.bottom+1);

	SelectObject(dc, GetStockObject(DC_BRUSH));
	SelectObject(dc, GetStockObject(DC_PEN));
	
	SetDCPenColor(dc, conf.color[lang.commentcol]);
	SetDCBrushColor(dc, conf.color[lang.commentcol]);
	clen=wcslen(lang.comment);
	y=line2px(first);
	for (i=first; i<=last; i++) {
		txt = getb(b, i, &len);
		if (clen && !wcsncmp(txt, lang.comment, clen))
			Rectangle(dc, 0, y, width, y+conf.lheight);
		y += conf.lheight;
	}
	
	paintsel(dc);
	
	/* Draw the wire */
	if (conf.wire) {
		SetDCPenColor(dc, conf.fg);
		x=conf.wire*conf.em;
		MoveToEx(dc, x, ps->rcPaint.top, 0);
		LineTo(dc, x, ps->rcPaint.bottom);
	}
	
	paintlines(dc,first,last);
	
	if (conf.doublebuffer) {
		BitBlt(ps->hdc,
			ps->rcPaint.left,
			ps->rcPaint.top,
			ps->rcPaint.right,
			ps->rcPaint.bottom,
			dc,
			ps->rcPaint.left,
			ps->rcPaint.top,
			SRCCOPY);
		DeleteDC(dc);
	}
	
	{
		dc=GetDC(w);
		SelectObject(dc, GetStockObject(DC_BRUSH));
		SelectObject(dc, GetStockObject(DC_PEN));
		SetBkMode(dc, TRANSPARENT);
		SelectObject(dc, font);
		paintstatus(dc);
		ReleaseDC(w,dc);
	}
}

wmscroll(int action) {
	SCROLLINFO si;

	switch (action) {

	case SB_PAGEUP:
		top = sat(1, top-vis+1, NLINES);
		break;

	case SB_LINEUP:
		top = sat(1, top-1, NLINES);
		break;

	case SB_PAGEDOWN:
		top = sat(1, BOT-1, NLINES);
		break;

	case SB_LINEDOWN:
		top = sat(1, top+1, NLINES);
		break;

	case SB_THUMBTRACK:
		si.cbSize = sizeof si;
		si.fMask = SIF_TRACKPOS;
		GetScrollInfo(w, SB_VERT, &si);
		top = si.nTrackPos;
		break;
	}
	
	movecaret();
	InvalidateRect(w, 0, 0);
}

wmwheel(int clicks) {
	int	d, dy;

	d=3;
	SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &d, 0);
	
	dy=clicks * d / -WHEEL_DELTA;
	if (vis<NLINES)
		top = sat(1, top+dy, NLINES-vis+1);
	
	movecaret();
	InvalidateRect(w, 0, 0);
}

wm_find() {
	if (fr.Flags&FR_DIALOGTERM && gofr.Flags&FR_DIALOGTERM) {
		dlg=0;
		return 0;
	}
	
	if (! (gofr.Flags & FR_DIALOGTERM))
		return autogo();
	
	if (fr.Flags & FR_FINDNEXT)
		return autoquery();
	
	if (fr.Flags & FR_REPLACE)
		return autoreplace();
	
	if (fr.Flags & FR_REPLACEALL)
		return autoreplaceall();
	return 0;
}

wm_click(int x, int y) {
	click.ln=px2line(y);
	click.ind=px2ind(click.ln, x);
	gob(b, click.ln, click.ind);
	act(EndSelection);
	act(StartSelection);
	SetCapture(w);
}

wm_drag(int x, int y) {
	int	ln,ind;
	Loc	olo, ohi, lo, hi;
	
	if (!click.ln)
		return 0;
	
	ordersel(&olo, &ohi);
	
	ln=px2line(y);
	ind=px2ind(ln, x);
	gob(b, ln, ind);
	
	ordersel(&lo, &hi);
	invd(min(lo.ln, olo.ln), max(hi.ln, ohi.ln));
	return 1;
}

LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	PAINTSTRUCT ps;
	HDC	dc;

	switch (msg) {

	case WM_PAINT:
		BeginPaint(hwnd, &ps);
		paint(&ps);
		EndPaint(hwnd, &ps);
		return 0;
	
	case WM_CHAR:
		wmchar(wparam);
		return 0;
	
	case WM_KEYDOWN:
		wmkey(wparam);
		return 0;
	
	case WM_SETFOCUS:
		CreateCaret(hwnd, 0, 0, conf.lheight);
		movecaret();
		ShowCaret(hwnd);
		return 0;
	
	case WM_KILLFOCUS:
		DestroyCaret();
		return 0;
	
	case WM_SIZE:
		width = (short) LOWORD(lparam);
		height = (short) HIWORD(lparam);
		vis = height/conf.lheight - 1;
		
		/* Resize double-buffer */
		if (conf.doublebuffer) {
			DeleteObject(dbmp);
			dc=GetDC(hwnd);
			dbmp=CreateCompatibleBitmap(dc, width, height);
			ReleaseDC(hwnd, dc);
		}
		return 0;
		
	case WM_MOUSEWHEEL:
		wmwheel((short) HIWORD(wparam));
		return 0;
	
	case WM_VSCROLL:
		wmscroll((short) LOWORD(wparam));
		return 0;
	
	case WM_LBUTTONDOWN:
		wm_click((short)LOWORD(lparam),
			(short)HIWORD(lparam));
		return 0;
	
	case WM_MOUSEMOVE:
		wm_drag((short)LOWORD(lparam),
			(short)HIWORD(lparam));
		return 0;
		
	case WM_LBUTTONUP:
		click.ln=0;
		if (sameloc(&CAR, &SEL))
			SLN=0;
		ReleaseCapture();
		return 0;
	
	case WM_COMMAND:
		act(LOWORD(wparam));
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	
	/* Find dialog notification */
	if (msg==WM_FIND)
		return wm_find();

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

static
mark(int *tbl, wchar_t *list, int x) {
	while (*list)
		tbl[*list++ & 0xffff]=x;
}

static
instrlist(wchar_t *list, wchar_t *s) {
	
	return 0;
}

static
autoselectlang() {
	int	i;
	wchar_t	*list;
	
	for (i=0; i<nlangs; i++)
	for (list=langset[i].ext; *list; list+=wcslen(list)+1)
		if (!wcscmp(list, fileext) || !wcscmp(list, L"*")) {
			lang=langset[i];
			return 1;
		}
	return 0;
}

static
reinitlang() {
	wchar_t	*s;
	
	autoselectlang();
	
	/*
	 * Construct tables
	 * 1=punctuation
	 * 2=space
	 */
	ZeroMemory(brktbl, sizeof brktbl);
	s=lang.brk;
	while (*s)
		brktbl[*s++ & 0xffff] = 1;
	
	s=L" \t\n";
	while (*s)
		brktbl[*s++ & 0xffff] = 2;
	
	s=lang.brace;
	ZeroMemory(opentbl, sizeof opentbl);
	ZeroMemory(closetbl, sizeof opentbl);
	for ( ; *s; s+=2) {
		closetbl[s[0] & 0xffff] = s[1];
		opentbl[s[1] & 0xffff] = s[0];
	}
}

static
reinitconfig() {
	RECT	rt;
	wchar_t	*s;
	
	configfont();
	reinitlang();
	
	if (bgbmp)
		DeleteObject(bgbmp);

	bgbmp=LoadImage(GetModuleHandle(0),
		conf.bgimage? conf.bgimage: L"",
		IMAGE_BITMAP, 0, 0,
		LR_LOADFROMFILE);
	if (bgbmp) {
		bgbrush=CreatePatternBrush(bgbmp);
		bgpen=GetStockObject(NULL_PEN);
	} else {
		bgbrush=CreateSolidBrush(conf.bg);
		bgpen=CreatePen(PS_SOLID, 1, conf.bg);
	}
	
	SetRect(&rt, 0,0, conf.cols * conf.em,
		(conf.rows+1) * conf.lheight);
	AdjustWindowRect(&rt, WS_OVERLAPPEDWINDOW, 1);
	if (w)
		SetWindowPos(w, 0, 0, 0,
			rt.right, rt.bottom,
			SWP_NOMOVE|SWP_NOZORDER);
	
	/* Fix the caret size */
	if (GetFocus()==w)
		SendMessage(w, WM_SETFOCUS, 0, 0);
}

static
init() {
	HDC	dc;
	RECT	rt;
	
	initb(b);
	top=1;

	wc.lpfnWndProc = WndProc;
	wc.hIcon = LoadIcon(GetModuleHandle(0), (void*) 100);
	wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
	RegisterClassEx(&wc);
	
	/* Configure Open Dialog */
	ofn.lpstrFile = malloc(MAX_PATH * sizeof (wchar_t));
	ofn.lpstrFile[0] = 0;
	ofn.hwndOwner = w;
	
	WM_FIND = RegisterWindowMessage(FINDMSGSTRING);
	fr.lpstrFindWhat = malloc(MAX_PATH * sizeof (wchar_t));
	fr.lpstrFindWhat[0] = 0;
	fr.lpstrReplaceWith = malloc(MAX_PATH * sizeof (wchar_t));
	fr.lpstrReplaceWith[0] = 0;
	
	gofr.lpstrFindWhat = malloc(MAX_PATH * sizeof (wchar_t));
	gofr.lpstrFindWhat[0] = 0;

	if (conf.doublebuffer) {
		dc=GetDC(0);
		dbmp=CreateCompatibleBitmap(dc, 1,1);
		ReleaseDC(w, dc);
	}
	
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rt, 0);
	w = CreateWindow(L"Window", L"",
		WS_OVERLAPPEDWINDOW|WS_VISIBLE,
		(rt.right-rt.left)/2 - conf.cols*conf.em/2,
		(rt.bottom-rt.top)/2 - (conf.rows+1)*conf.lheight/2,
		conf.cols * conf.em,
		(conf.rows+1) * conf.lheight,
		NULL, menu, GetModuleHandle(0), NULL);
	reinitconfig();
	gofr.hwndOwner = w;
	fr.hwndOwner = w;
}

static
initmenu() {
	HMENU	m;
	
	menu=CreateMenu();
	
	m=CreatePopupMenu();
	AppendMenu(m, MF_STRING, NewFile, L"&New	^N");
	AppendMenu(m, MF_STRING, PromptOpen, L"&Open...	^O");
	AppendMenu(m, MF_STRING, SaveFile, L"&Save	^S");
	AppendMenu(m, MF_STRING, PromptSaveAs, L"Save &As...");
	AppendMenu(m, MF_STRING, ReloadFile, L"&Reload	F5");
	AppendMenu(m, MF_SEPARATOR, 0, 0);
	AppendMenu(m, MF_STRING, ForkApp, L"New Window	F2");
	AppendMenu(m, MF_STRING, SpawnApp, L"Shell	^F2");
	AppendMenu(m, MF_STRING, QuitApp, L"E&xit");
	AppendMenu(menu, MF_POPUP, (INT_PTR)m, L"&File");
	
	m=CreatePopupMenu();
	AppendMenu(m, MF_STRING, UndoChange, L"&Undo	^Z");
	AppendMenu(m, MF_STRING, RedoChange, L"&Redo	^Y");
	AppendMenu(m, MF_SEPARATOR, 0, 0);
	AppendMenu(m, MF_STRING, CutSelection, L"Cu&t	^X");
	AppendMenu(m, MF_STRING, CopySelection, L"&Copy	^C");
	AppendMenu(m, MF_STRING, PasteClipboard, L"Paste	^V");
	AppendMenu(m, MF_STRING, DeleteSelection, L"De&lete	Del");
	AppendMenu(m, MF_SEPARATOR, 0, 0);
	AppendMenu(m, MF_STRING, PromptFind, L"&Find...	^F");
	AppendMenu(m, MF_STRING, PromptReplace, L"&Replace...	^R");
	AppendMenu(m, MF_STRING, PromptGo, L"&Go To Line...	^G");
	AppendMenu(m, MF_SEPARATOR, 0, 0);
	AppendMenu(m, MF_STRING, SelectAll, L"Select All	^A");
	AppendMenu(menu, MF_POPUP, (INT_PTR)m, L"&Edit");
	
	m=CreatePopupMenu();
	AppendMenu(m, MF_STRING, PromptSpawn, L"Run...	Shift+F7");
	AppendMenu(m, MF_STRING, SpawnSelection, L"Run w/ &Selection...	^F7");
	AppendMenu(m, MF_STRING, SpawnLastCmd, L"&Run Last Command	F7");
	AppendMenu(menu, MF_POPUP, (INT_PTR)m, L"&Run");
	
	m=CreatePopupMenu();
	AppendMenu(m, MF_STRING, SaveFileUTF8, L"Save as UTF-8");
	AppendMenu(m, MF_STRING, SaveFileUTF16, L"Save as UTF-16");
	AppendMenu(m, MF_STRING, ReloadFileUTF8, L"Reload as UTF-8");
	AppendMenu(m, MF_STRING, ReloadFileUTF16, L"Reload as UTF-16");
	AppendMenu(menu, MF_POPUP, (INT_PTR)m, L"&Encoding");
	
	m=CreatePopupMenu();
	AppendMenu(m, MF_STRING, NextConfig, L"&Next	F12");
	AppendMenu(m, MF_STRING, PrevConfig, L"&Prev	_F12");
	AppendMenu(m, MF_STRING, ReloadConfig, L"&Reload	^F12");
	AppendMenu(m, MF_STRING, EditConfig, L"&Edit");
	AppendMenu(menu, MF_POPUP, (INT_PTR)m, L"&Config");
}

int CALLBACK
WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show) {

	wchar_t	**argv;
	MSG	msg;
	int	argc, ok;
	Buf	buf;

	b = &buf;
	config();
	initmenu();
	init();

	/*
	 * Load initial file
	 */
	argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (argc>1) {
		setfilename(argv[1]);
		LocalFree(argv);
		
		ok=act(LoadFile) || !*filename;
		if (!ok)
			MessageBox(w, L"Could not open file",
				L"Error", MB_OK);
	} else
		act(NewFile);
	
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (dlg && IsDialogMessage(dlg, &msg))
			continue;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

/*
 * Creates a console when compiled for conosle.
 * Ignored otherwise.
 */
main() {
	return WinMain(0,0,0,0);
}