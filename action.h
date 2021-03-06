/* vim: set noexpandtab:tabstop=8 */
enum {
	ToggleLinebreak,
	ToggleTabs,
	Toggle8Tab,
	ToggleBOM,
	LoadFile,
	ReloadFile,
	ReloadFileUTF8,
	ReloadFileUTF16,
	ReloadFileCP1252,
	SaveFile,
	SetUTF8,
	SetUTF16,
	SetCP1252,

	MoveUp,
	MoveDown,
	MoveLeft,
	MoveWordLeft,
	MoveRight,
	MoveWordRight,
	MoveHome,
	MoveEnd,
	MovePageDown,
	MovePageUp,
	MoveSof,
	MoveEof,
	MoveBrace,

	DeleteChar,
	BackspaceChar,
	
	SpaceAbove,
	SpaceBelow,
	SpaceBoth,
	
	DeleteLine,
	BreakLine,
	JoinLine,
	DupLine,
	AscendLine,
	DescendLine,
	
	SelectAll,
	StartSelection,
	EndSelection,
	SelectWord,
	DeleteSelection,
	IndentSelection,
	UnindentSelection,
	CommentSelection,
	
	UndoChange,
	RedoChange,
	
	CopySelection,
	CutSelection,
	PasteClipboard,
	
	PromptGo,
	PromptFind,
	PromptReplace,
	PromptOpen,
	PromptSaveAs,
	
	NewFile,
	ExitEditor,
	SpawnEditor,
	SpawnShell,
	
	SpawnCmd,
	PromptSpawn,
	
	ReloadConfig,
	PrevConfig,
	NextConfig,
	EditConfig
};
