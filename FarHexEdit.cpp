#include <string.h>

//#define _WINCON_ // to prevent including wincon.h
#include <windows.h>
//#undef _WINCON_
#include "plugin.hpp" //this file wants 1/2-byte alignment
//#pragma pack(8)
//#include <wincon.h> //this file wants 8-byte alignment

#include "HexEdit.h"
#include "FarHexEdit.hpp"
#include "FarHexLng.hpp"

#include "FarHexReg.cpp"


static int KeyBarWidth=6; // Ширина текста кнопки
static int KeyBarY=24;    // y-координата кнопок

static char searchStr[512]="";
static byte searchData[512]="";
static dword searchLength;
static int searchHex=0;

static int g_nOpenFrom = 0; // Where the plugin was called from

/////////////////////////////////////////////////////////////////////

void About()
{
	char *Msg[]={GetMsg(MAbout),
				"",GetMsg(MPlugin),GetMsg(MVer),
				GetMsg(MCopyright),"\1",
				GetMsg(MSendReports),
				"2:5004/16.99@FidoNet, alex@apsofts.com",
				"\1",
				GetMsg(MHelp),"",
				GetMsg(MOk)};
	Info.Message(Info.ModuleNumber,0,"Contents",Msg,12,1);
}

void WaitWindow()
{
	char *Msg[]={GetMsg(MHexEditor),GetMsg(MWait)};
	Info.Message(Info.ModuleNumber,0,0,Msg,2,0);
}

int ConfirmQuit()
{
	char *Msg[]={GetMsg(MHexEditor),GetMsg(MDataChanged),
				GetMsg(MYes),GetMsg(MNo),GetMsg(MCancel)};
	return Info.Message(Info.ModuleNumber,FMSG_WARNING,0,Msg,5,3);
}

int GetAddr(int Addr)
{
	struct InitDialogItem InitItems[]={
		DI_DOUBLEBOX,3,1,26,6,0,0,0,0,(char *)MGotoAddr,
		DI_EDIT,8,3,20,4,1,(int)"HexEdit_Goto",DIF_HISTORY,0,"",
		DI_BUTTON,0,5,0,0,0,0,DIF_CENTERGROUP,1,(char *)MOk,
		DI_BUTTON,0,5,0,0,0,0,DIF_CENTERGROUP,0,(char *)MCancel
	};

	const int N=sizeof(InitItems)/sizeof(InitItems[0]);

	struct FarDialogItem DialogItems[N];
	InitDialogItems(InitItems,DialogItems,N);
	DWord2Hex((dword)Addr,DialogItems[1].Data);

	int ExitCode=Info.Dialog(Info.ModuleNumber,-1,-1,30,8,NULL,
				DialogItems,N);
	if (ExitCode!=2)
		return(-1);

	return (int)Hex2DWord(DialogItems[1].Data);
}

void SearchingWindow()
{
	char str[65];
	unsigned int l=sizeof(str);
	char *Msg[]={GetMsg(MSearch),GetMsg(MSearching),str};
	strcpy(str,"\"");
	strncat(str,searchStr,l-3);
	if (strlen(searchStr)>l-3)
		strcpy(str+l-5,"...");
	strcat(str,"\"");
	Info.Message(Info.ModuleNumber,0,0,Msg,3,0);
}

int GetSearch(char *codeTable)
{
	struct InitDialogItem InitItems[]={
		DI_DOUBLEBOX,3,1,72,9,0,0,0,0,(char *)MSearch,
		DI_TEXT,5,2,0,0,0,0,0,0,(char *)MSearchFor,
		DI_EDIT,5,3,70,4,1,(int)"HexEdit_Search",DIF_HISTORY,0,"",
		DI_TEXT,0,4,0,0,0,0,DIF_SEPARATOR,0,"",
		DI_RADIOBUTTON,5,5,0,0,0,0,DIF_GROUP,0,(char *)MSearchForText,
		DI_RADIOBUTTON,5,6,0,0,0,0,0,0,(char *)MSearchForHex,
		DI_TEXT,0,7,0,0,0,0,DIF_SEPARATOR,0,"",
		DI_BUTTON,0,8,0,0,0,0,DIF_CENTERGROUP,1,(char *)MSearch,
		DI_BUTTON,0,8,0,0,0,0,DIF_CENTERGROUP,0,(char *)MCancel
	};

	const int N=sizeof(InitItems)/sizeof(InitItems[0]);

	struct FarDialogItem DialogItems[N];
	InitDialogItems(InitItems,DialogItems,N);
	strncpy(DialogItems[2].Data,searchStr,sizeof(searchStr));
	if (searchHex) {
		DialogItems[5].Selected=1;
		DialogItems[4].Selected=0;
	}
	else {
		DialogItems[4].Selected=1;
		DialogItems[5].Selected=0;
	}

	int ExitCode=Info.Dialog(Info.ModuleNumber,-1,-1,76,11,NULL,
				DialogItems,N);
	if (ExitCode!=N-2)
		return 0;

	strncpy(searchStr,DialogItems[2].Data,sizeof(searchStr));
	searchHex=DialogItems[5].Selected;
	if (searchHex) {
		HexStr2Bytes(searchStr,searchData,(int*)&searchLength);
	}
	else {
		strncpy((char*)searchData,searchStr,sizeof(searchData));
		searchLength=strlen(searchStr);
		if (codeTable) {
			byte *p=searchData;
			while (*p)
				*p++=codeTable[*p]; // Encode
		}
	}

	return 1;
}

int ShowNotFound()
{
	char str[65];
	unsigned int l=sizeof(str);
	char *Msg[]={GetMsg(MSearch),GetMsg(MSearchNotFound),str,
				GetMsg(MOk)};
	strcpy(str,"\"");
	strncat(str,searchStr,l-3);
	if (strlen(searchStr)>l-3)
		strcpy(str+l-5,"...");
	strcat(str,"\"");
	return Info.Message(Info.ModuleNumber,FMSG_WARNING,0,Msg,4,1);
}

int ShowMessage(char *sMsg)
{
	char *Msg[]={GetMsg(MError), sMsg, GetMsg(MOk)};
	return Info.Message(Info.ModuleNumber,FMSG_WARNING,0,Msg,3,1);
}

int ShowMessageN(int nMsg)
{
	char *Msg[]={GetMsg(MError), GetMsg(nMsg), GetMsg(MOk)};
	return Info.Message(Info.ModuleNumber,FMSG_WARNING,0,Msg,3,1);
}

void WriteKeyBarText(int KeyNum,char *Text)
{
	// KeyNum must be >1 (???)
	int x=(KeyNum-1)*(KeyBarWidth+2)+1;
	if (KeyNum>9) x+=KeyNum-9;
	Info.Text(x,KeyBarY,FarPalette[COLOR_KEYBAR],Text);
}

void WriteKeyBar(int KeyNum,char *Text)
{
	char sNum[3]="0\0";
	int xNum, xTxt;
	xNum=(KeyNum-1)*(KeyBarWidth+2);
	if (KeyNum>10)
		xNum+=KeyNum-10;
	(KeyNum<10) ? xTxt=xNum+1 : xTxt=xNum+2;

	if (KeyNum<10)
	{
		sNum[0]='0'+KeyNum;
	}
	else
	{
		sNum[0]='1';
		sNum[1]='0'+KeyNum-10;
	}

	Info.Text(xNum,KeyBarY,FarPalette[COLOR_KEYBARNUM],sNum);
	Info.Text(xTxt,KeyBarY,FarPalette[COLOR_KEYBAR],Text);
}

void WriteStatusBar(char *Text)
{
	char s[256];
	memset(s, ' ', 255);
	s[255] = 0;
	memcpy(s, Text, strlen(Text));
	Info.Text(0,0,FarPalette[COLOR_EDSTATUSBAR],s);
}

void WritePos(dword Pos,dword Size)
{
	static dword OldPos=0xFFFFFFFF;
	char Text[30]="  ", Hex[10];
	if (OldPos==Pos) return;
	strcat(Text,DWord2Hex(Pos,Hex));
	strcat(Text,"/");
	strcat(Text,DWord2Hex(Size-1,Hex));
	Info.Text(35,0,FarPalette[COLOR_EDSTATUSBAR],Text);
	OldPos=Pos;
}

char *GetAnsi2OemTable(char *Table)
{
	char TableFrom[256];
	for (int i=0;i<256;i++) {
		TableFrom[i]=(char)i;
	}
	CharToOemBuff(TableFrom,Table,256);
	return Table;
}

char *GetOem2AnsiTable(char *Table)
{
	char TableFrom[256];
	for (int i=0;i<256;i++) {
		TableFrom[i]=(char)i;
	}
	OemToCharBuff(TableFrom,Table,256);
	return Table;
}

/////////////////////////////////////////////////////////////////////

FarHexEditor::FarHexEditor(byte *data, dword dataSize, byte *dataTop,
				byte *dataPos, int x1,int y1,int x2,int y2) :
				HexEditor(data,dataSize,dataTop,dataPos,x1,y1,x2,y2)
{
}

void FarHexEditor::ReadInput(INPUT_RECORD *in)
{
	if ( g_nOpenFrom == OPEN_EDITOR	)
		Info.EditorControl(ECTL_READINPUT,in);
	else
		HexEditor::ReadInput(in);
}

BOOL FarHexEditor::PreProcessKey(KEY_EVENT_RECORD *k)
{
	int Addr;

	if (!(k->bKeyDown)) {
		return FALSE;
	}
	if (k->uChar.AsciiChar==27 || k->wVirtualKeyCode==VK_F10)
	{
		if (Changed) {
			switch (ConfirmQuit()) {
				case 0:
					Quit=TRUE;
					break;
				case 1:
					Changed=FALSE;
					Quit=TRUE;
					break;
				default: // Cancel
					MoveCursor();
			}
		}
		else Quit=TRUE;

		return TRUE;
	}

	switch (k->wVirtualKeyCode) {

		case VK_F1:
			About();
			MoveCursor();
			return TRUE;

		case VK_F4:
			Quit=TRUE;
			return TRUE;

		case VK_F5:
			Addr=GetAddr(DataPos-Data);
			if (Addr!=-1 && (dword)Addr<DataSize)
				GoTo(Data+Addr);
			else
				MoveCursor();
			return TRUE;

		case VK_F7:
			byte *pos;
			int Shift=k->dwControlKeyState&SHIFT_PRESSED;

			if (Shift || (!Shift && GetSearch(EncodeTable))) {

				HANDLE HScr=Info.SaveScreen(0,0,-1,-1);
				SearchingWindow();        // покажем окно с сообщением
				pos=Search(searchData,searchLength); // ищем
				Info.RestoreScreen(HScr); // убираем сообщение

				if (pos!=0)
				GoTo(pos);
				else {
				ShowNotFound();
				MoveCursor();
				}
			}
			else
				MoveCursor();
			return TRUE;

	}
	return FALSE;
}

void FarHexEditor::PostProcessKey(KEY_EVENT_RECORD *k)
{
	WritePos(DataPos-Data,DataSize);
}

void FarHexEditor::WriteStr(char *s,int x,int y,int Color)
{
	if (!Color) Color=Attr;
	Info.Text(x,y,Color,s);
	Info.Text(0,0,0,0); // Flush
}

/////////////////////////////////////////////////////////////////////
//                  Основная функция плагина                       //
/////////////////////////////////////////////////////////////////////
void MainFunc_Editor()
{
	byte *Data,*p; // Данные и текущий указатель
	byte *DataTop, *DataPos; // Указатель на начало и текущую позицию HexEdit'a
	dword Size;
	struct EditorInfo ei;
	struct EditorGetString egs;
	int i;
	char CR[]="\n", CRLF[]="\r\n", NoEOL[]="";//, UnknownEOL[]="?";
	char *DefaultEOL=CRLF, *EOL[3]={NoEOL,CR,CRLF};
	int EolLen=0; // Длина DefaultEOL

	////////////////////// Получение данных из редактора //////////////////////
	HANDLE HScr=Info.SaveScreen(0,0,-1,-1);
	WaitWindow();

	Info.EditorControl(ECTL_GETINFO,&ei);
	KeyBarWidth=(ei.WindowSizeX-26)/12;
	KeyBarY=ei.WindowSizeY+1;
	if (KeyBarWidth<6) KeyBarWidth=6;

	// Подсчитаем размер данных
	{
		Size=0;
		// Обработаем первую строчку отдельно - надо определить
		//   EOL, который будем использовать по-умолчанию
		egs.StringNumber=0;
		Info.EditorControl(ECTL_GETSTRING,&egs);
		Size+=egs.StringLength;
		int l=strlen(egs.StringEOL);
		if (l!=0 || ei.TotalLines==1) EolLen=l;
			else EolLen=2; // если непонятно и строк >1, тогда будет CRLF
		DefaultEOL=EOL[EolLen];
		//if (EolLen==1) DefaultEOL=CR; else DefaultEOL=CRLF;
		Size+=EolLen;
		// пройдем по остальным строкам
		for (i=1;i<ei.TotalLines;i++) {
			egs.StringNumber=i;
			Info.EditorControl(ECTL_GETSTRING,&egs);
			Size+=egs.StringLength;
			l=strlen(egs.StringEOL);
			if (l!=0)
				Size+=l;
			else if (i!=ei.TotalLines-1) // если конца строки нет и это не последняя,
				Size+=EolLen;         //   добавим DefaultEOL
		}
	}
	if (Size==0) return; // ничего нет!
	Data=(byte*)MALLOC(Size);

	// Копируем все построчно в Data
	DataTop=Data;
	DataPos=Data;
	p=Data;

	int key=0;
	BaseInput in;

	for (i=0;i<ei.TotalLines;i++) {

		if ((key=in.KeyPressed())!=0 && key==VK_ESCAPE)
		{
			in.ReadKey();
			key=VK_ESCAPE;
			break;
		}

		if (i==ei.TopScreenLine) {
			DataTop=p;
		}
		if (i==ei.CurLine) {
			DataPos=p+ei.CurPos;
		}
		egs.StringNumber=i;
		Info.EditorControl(ECTL_GETSTRING,&egs);
		memcpy(p,egs.StringText,egs.StringLength);
		p+=egs.StringLength;
		int l=strlen(egs.StringEOL);
		if (l!=0) {
			memcpy(p,egs.StringEOL,l);
			p+=l;
		}
		else if (i!=ei.TotalLines-1) {
		 // если конца строки нет и это не последняя строка - добавим DefaultEOL
				memcpy(p,DefaultEOL,EolLen);
				p+=EolLen;
				}
	}

	Info.RestoreScreen(HScr); // убираем сообщение

	if (key==VK_ESCAPE) {
		FREE(Data);
		return;
	}

	////////////////////// Редактирование данных //////////////////////

	HANDLE HScrKey=Info.SaveScreen(0,KeyBarY,-1,KeyBarY);
	WriteKeyBarText(2,"      ");
	WriteKeyBarText(4,GetMsg(MKeyText));
	WriteKeyBarText(5,GetMsg(MKeyGoto));
	WriteKeyBarText(6,"      ");
	WriteKeyBarText(8,"      ");
	WriteKeyBarText(11,"      ");
	WriteKeyBarText(12,"      ");
	WritePos(DataPos-Data,Size);

	DataTop=DataPos-ei.WindowSizeY/2*16; // примерно пол-экрана
	if (DataTop<Data || Size<=(dword)ei.WindowSizeY*16) DataTop=Data;

	FarHexEditor h(Data,Size,DataTop,DataPos,0,1,ei.WindowSizeX-1,ei.WindowSizeY);

	h.Attr=FarPalette[COLOR_EDITOR];
	struct CharTableSet CharTable;
	if (ei.TableNum!=-1) {
		Info.CharTable(ei.TableNum,(char*)&CharTable,sizeof(CharTable));
		h.DecodeTable=(char*)(CharTable.DecodeTable);
		h.EncodeTable=(char*)(CharTable.EncodeTable);
	}
	else 
		if (ei.AnsiMode) {
			h.DecodeTable=GetAnsi2OemTable((char*)CharTable.DecodeTable);
			h.EncodeTable=GetOem2AnsiTable((char*)CharTable.EncodeTable);
		}
//  h.Show();
	h.Draw();
	h.Run();
//  h.Hide();

	if (h.Changed) { 

	////////////////////// Возвращение данных в редактор //////////////////////

	WaitWindow();

	#define ADD_STR(_EOL) {    \
			ess.StringEOL=_EOL;    \
			ess.StringNumber=Line; /*-1*/ \
			ess.StringText=(char*)LineStart; \
			ess.StringLength=p-LineStart;    \
			Info.EditorControl(ECTL_INSERTSTRING,NULL); \
			Info.EditorControl(ECTL_SETSTRING,&ess);    \
			Line++; \
	}
// esp.CurLine=Line; Info.EditorControl(ECTL_SETPOSITION,&esp);

	struct EditorSetPosition esp={
				0, // int CurLine;
				0, // int CurPos;
				0, // int CurTabPos;
				0, // int TopScreenLine;
				0, // int LeftPos;
				-1 // int Overtype;
				};
	Info.EditorControl(ECTL_SETPOSITION,&esp);
	// Удалить все!
	for (i=0;i<ei.TotalLines;i++)
		Info.EditorControl(ECTL_DELETESTRING,NULL);

	// Формирование строк из данных
	struct EditorSetString ess;
	int Line=0, Len=0;
	byte *LineStart;

	p=Data;
	LineStart=Data;
	for (dword j=0;j<Size;j++) {
		if (*p=='\n') { // EOL!
			ADD_STR(CR);
			p++;
			LineStart=p;
		}
		else
			if (*p=='\r') { // EOL?
				if (p<Data+Size-1 && *(p+1)=='\n') { // EOL! "\r\n"
				ADD_STR(CRLF);
				if (p+1==h.GetDataPos()) { // приходится проверять отдельно
				esp.CurLine=Line-1;
				esp.CurPos=p-LineStart;
				}
				p+=2; j++;
				LineStart=p;
				}
				else
				p++;
			}
			else p++;
		if (p==h.GetDataPos()) {
			esp.CurLine=Line;
			esp.CurPos=p-LineStart;
		}
	}
	if (p>LineStart) { // есть недобавленная строка без EOL
//    ADD_STR(NoEOL);

			ess.StringEOL=NoEOL;    
			ess.StringNumber=Line; /*-1*/ 
			ess.StringText=(char*)LineStart; 
			ess.StringLength=p-LineStart;    
//      if (Line) // если всего одна строка, то вставлять не надо - будет лишний CRLF
//        Info.EditorControl(ECTL_INSERTSTRING,NULL); // иначе - вставим
// А может вообще для последней строки ничего не вставлять?
			Info.EditorControl(ECTL_SETSTRING,&ess);    
			Line++; 

	}

	esp.CurTabPos=-1;
	esp.TopScreenLine=esp.CurLine-ei.WindowSizeY/2;
	if (esp.TopScreenLine<0) esp.TopScreenLine=0;
	if (esp.CurPos>ei.WindowSizeX-1) esp.LeftPos=esp.CurPos-ei.WindowSizeX+1;
	Info.EditorControl(ECTL_SETPOSITION,&esp);
	
	} // if (h.Changed)

	//WriteKeyBarText(4,"      "); WriteKeyBarText(5,"      ");
	Info.RestoreScreen(HScrKey);

	FREE(Data);
}

/////////////////////////////////////////////////////////////////////

BOOL GetFileNameSize(char *sFName, dword *pdwSize)
{
	struct PanelInfo		rPanel;
	struct PluginPanelItem	*pPanelItem;
    WIN32_FIND_DATA			*pFindData;

	*sFName = '\0';
	*pdwSize = 0;
	Info.Control( INVALID_HANDLE_VALUE, FCTL_GETPANELINFO, &rPanel );
	if (rPanel.PanelType != PTYPE_FILEPANEL)
	{
		// don't support other panel types but FILEPANEL
		ShowMessageN( MUnsupportedPanel );
		return FALSE;
	}

	if (rPanel.ItemsNumber > 0)
	{
		pPanelItem = &rPanel.PanelItems[ rPanel.CurrentItem ];
		pFindData = &pPanelItem->FindData;
		if (pFindData->nFileSizeHigh > 0)
		{
			ShowMessageN( MErrBigFile );
			return FALSE;
		}
		if ( pFindData->dwFileAttributes &
				(FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_READONLY) )
		{
			ShowMessage( "Read only file" );
			return FALSE;
		}
		strcpy( sFName, pFindData->cFileName );
		*pdwSize = pFindData->nFileSizeLow;
	}
	return TRUE;
}

BOOL ReadFileData(char *sFName, byte *pData, dword dwSize)
{
	BOOL bResult = FALSE;
	DWORD dwBytesRead;
	HANDLE fh = CreateFile( sFName,	// file name
		GENERIC_READ,				// access mode
		FILE_SHARE_READ,			// share mode
		NULL,						// security
		OPEN_EXISTING,				// how to create
		FILE_ATTRIBUTE_NORMAL,		// file attributes
		NULL );						// handle to template file
	if (fh != INVALID_HANDLE_VALUE)
	{
		bResult = ReadFile( fh, pData, dwSize, &dwBytesRead, NULL );
		CloseHandle( fh );
	}
	if ( !bResult )
		ShowMessage( "Cannot read file" );
	return bResult;
}

BOOL WriteFileData(char *sFName, byte *pData, dword dwSize)
{
	return TRUE;


	BOOL bResult = FALSE;
	DWORD dwBytesRead;
	HANDLE fh = CreateFile( sFName,	// file name
		GENERIC_WRITE,				// access mode
		FILE_SHARE_READ,			// share mode
		NULL,						// security
		OPEN_EXISTING,				// how to create
		FILE_ATTRIBUTE_NORMAL,		// file attributes
		NULL );						// handle to template file
	if (fh != INVALID_HANDLE_VALUE)
	{
		bResult = ReadFile( fh, pData, dwSize, &dwBytesRead, NULL );
		CloseHandle( fh );
	}
	if ( !bResult )
		ShowMessage( "Cannot read file" );
	return bResult;
}

void MainFunc_Menu()
{
	byte *Data; // Данные
	byte *DataTop, *DataPos; // Указатель на начало и текущую позицию HexEdit'a
	dword dwSize = 0;
	char sFName[ MAX_PATH ] = "";

	GetFileNameSize(sFName, &dwSize);

	if (!*sFName || !dwSize)
		return; // file is empty or error has happened

	Data=(byte*)MALLOC(dwSize);
	if (!ReadFileData(sFName, Data, dwSize))
	{
		FREE(Data);
		return;
	}

	DataTop=Data;
	DataPos=Data;

	////////////////////// Редактирование данных //////////////////////

	FarHexEditor h(Data, dwSize,DataTop,DataPos); // full screen
	KeyBarWidth=(h.X2-26)/12;
	KeyBarY=h.Y2;
	if (KeyBarWidth<6) KeyBarWidth=6;

	h.ChangeWindowSize(-1, h.Y1+1, -1, h.Y2-1);

	HANDLE HScrKey=Info.SaveScreen(0,0,-1,-1);
	for (int i=1; i<=12; i++)
		switch (i)
		{
			case 1:
				WriteKeyBar(i,"Help  ");
				break;
			case 2:
				WriteKeyBar(i,"Save  ");
				break;
			case 4:
				WriteKeyBar(i,GetMsg(MKeyText));
				break;
			case 5:
				WriteKeyBar(i,GetMsg(MKeyGoto));
				break;
			default:
				WriteKeyBar(i,"      ");
		}
	WriteStatusBar(sFName);
	WritePos(DataPos-Data, dwSize);

	h.Attr=FarPalette[COLOR_EDITOR];
	/*
	struct CharTableSet CharTable;
	if (ei.TableNum!=-1) {
		Info.CharTable(ei.TableNum,(char*)&CharTable,sizeof(CharTable));
		h.DecodeTable=(char*)(CharTable.DecodeTable);
		h.EncodeTable=(char*)(CharTable.EncodeTable);
	}
	else 
		if (ei.AnsiMode) {
			h.DecodeTable=GetAnsi2OemTable((char*)CharTable.DecodeTable);
			h.EncodeTable=GetOem2AnsiTable((char*)CharTable.EncodeTable);
		}
	*/

	h.Draw();
	h.Run();

	if (h.Changed) { 
		WriteFileData(sFName, Data, dwSize);
	} // if (h.Changed)

	Info.RestoreScreen(HScrKey);

	FREE(Data);
}

/////////////////////////////////////////////////////////////////////

void MainFunc( int OpenFrom )
{
	g_nOpenFrom = OpenFrom;
	switch( OpenFrom )
	{
		case OPEN_EDITOR:
			MainFunc_Editor();
			break;
		case OPEN_VIEWER:
//			MainFunc_Viewer();
			break;
		case OPEN_PLUGINSMENU:
			MainFunc_Menu();
			break;
	}
}

/////////////////////////////////////////////////////////////////////

void WINAPI _export SetStartupInfo(struct PluginStartupInfo *Info)
{
	::Info=*Info;
	strcpy(FarRootKey,Info->RootKey);
        int i;
	for(i=strlen(FarRootKey);i>0&&FarRootKey[i]!='\\';i--);
	FarRootKey[i]=0; // Убрали последнюю ветку - получили \Software\Far
	strcat(FarRootKey,"\\Colors");
	GetRegKey(HKEY_CURRENT_USER,"","CurrentPalette",(char*)FarPalette,"",sizeof(FarPalette));
	if (!*FarPalette) { // чего-то не так
		FarPalette[COLOR_KEYBAR]=0x30;
		FarPalette[COLOR_EDITOR]=0x1B;
		FarPalette[COLOR_EDSTATUSBAR]=0x30;
	}
}

HANDLE WINAPI _export OpenPlugin(int OpenFrom,int Item)
{
//  About();
	MainFunc( OpenFrom );
	return(INVALID_HANDLE_VALUE);
}


void WINAPI _export GetPluginInfo(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(*Info);
	Info->Flags = PF_EDITOR | PF_VIEWER;
	Info->DiskMenuStringsNumber = 0;
	static char *PluginMenuStrings[1];
	PluginMenuStrings[0] = GetMsg(MHexEditor);
	Info->PluginMenuStrings = PluginMenuStrings;
	Info->PluginMenuStringsNumber = sizeof(PluginMenuStrings) / sizeof(PluginMenuStrings[0]);
	Info->PluginConfigStringsNumber = 0;
}

/*
void WINAPI _export GetOpenPluginInfo(HANDLE hPlugin,struct OpenPluginInfo *Info)
{
	TmpPanel *Panel=(TmpPanel *)hPlugin;
	Panel->GetOpenPluginInfo(Info);
}
*/
char *GetMsg(int MsgId)
{
	return(Info.GetMsg(Info.ModuleNumber,MsgId));
}

void InitDialogItems(struct InitDialogItem *Init,struct FarDialogItem *Item,
				int ItemsNumber)
{
	for (int I=0;I<ItemsNumber;I++)
	{
		Item[I].Type=Init[I].Type;
		Item[I].X1=Init[I].X1;
		Item[I].Y1=Init[I].Y1;
		Item[I].X2=Init[I].X2;
		Item[I].Y2=Init[I].Y2;
		Item[I].Focus=Init[I].Focus;
		Item[I].Selected=Init[I].Selected;
		Item[I].Flags=Init[I].Flags;
		Item[I].DefaultButton=Init[I].DefaultButton;
		if ((unsigned int)Init[I].Data<2000)
			strcpy(Item[I].Data,GetMsg((unsigned int)Init[I].Data));
		else
			strcpy(Item[I].Data,Init[I].Data);
	}
}
