//#include <stdio.h>
#define _WINCON_ // to prevent including wincon.h
#include <windows.h>
#undef _WINCON_
#pragma pack(8)
#include <wincon.h> //this file wants 8-byte alignment
#include "HexEdit.h"

// X-позиции начала Hex/Char блоков
#define X_HEX  11
#define X_CHAR 63

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

//////////////////////////////////////////////////////////////

#define SET(var,data) if ((data)!=-1) var=(data)

SMALL_RECT _SMALL_RECT(int x1,int y1,int x2,int y2)
{
  SMALL_RECT r={x1,y1,x2,y2};
  return r;
}

COORD _COORD(SHORT x,SHORT y)
{
  COORD c={x,y};
  return c;
}

char toupper(char ch)
{
  if (ch>='a'&&ch<='z')
    ch-=0x20;
  return ch;
}

//////////////////////////////////////////////////////////////

char HexChars[] = "0123456789ABCDEF";

char *Byte2Hex(byte b, char *s)
{
  *s++=HexChars[b >> 4];
  *s++=HexChars[b & 0xF];
  *s=0;
  return (s-2);
}

byte HexChar2Byte(char ch)
{
  byte b=0;
  if (ch>='0'&&ch<='9')
    b=ch-'0';
  else
    if (ch>='A'&&ch<='F')
      b=ch-'A'+0xA;
    else
      if (ch>='a'&&ch<='f')
        b=ch-'a'+0xA;
  return b;
}

byte Hex2Byte(char *s)
{
  return (HexChar2Byte(*s++)<<4)+HexChar2Byte(*s);
}

char *DWord2Hex(dword x, char *s)
{
  strcpy(s,"00000000");
  s+=7;
  for(int i=1;i<=8;i++)
  {
    *s--=HexChars[x & 0xF];
    x>>=4;
  }
  return s+1;
}

dword Hex2DWord(char *s)
{
  dword x=0;
  int l=strlen(s);
  if (l==0) return 0;
  for (int i=0;i<l;i++)

  while (*s)
  {
    x = (x<<4) | HexChar2Byte(*s++);
  }
  return x;
}

byte *HexStr2Bytes(char *hexStr, byte *data, int *dataSize)
{
  char *s=hexStr;
  byte nibble=0, b=0, *x=data;

  while (*s) {
    if (*s==' ') {
      if (nibble) // ожидалась вторая цифра
        *x++=b;
      b=0;
      nibble=0;
      s++;
      continue;
    }
    if (nibble) // вторая цифра
      *x++= b<<4 | HexChar2Byte(*s++);
    else
      b=HexChar2Byte(*s++);
    nibble=!nibble;
  }
  if (nibble) // ожидалась вторая цифра, но кончилась строка
    *x++=b;
  *dataSize=x-data;
  return data;
}

//////////////////////////////////////////////////////////////

BaseInput::BaseInput()
{
  Quit=FALSE;
  InH=GetStdHandle(STD_INPUT_HANDLE);
  SetConsoleMode(InH,ENABLE_MOUSE_INPUT);
}

BaseInput::~BaseInput()
{
}

void BaseInput::Run()
{
  INPUT_RECORD in;
  //DWORD n;

  while (!Quit)
  {
    //ReadConsoleInput(InH,&in,1,&n);
    ReadInput(&in);
    switch (in.EventType)
    {
      case KEY_EVENT:
        if (!PreProcessKey(&in.Event.KeyEvent)) {
          ProcessKey(&in.Event.KeyEvent);
          PostProcessKey(&in.Event.KeyEvent);
        }
        break;
      case MOUSE_EVENT:
        if (!PreProcessMouse(&in.Event.MouseEvent)) {
          ProcessMouse(&in.Event.MouseEvent);
          PostProcessMouse(&in.Event.MouseEvent);
        }
        break;
    }
  }
}

int BaseInput::KeyPressed()
{
  INPUT_RECORD in;
  do {
      DWORD n=0;
      PeekConsoleInput(InH,&in,1,&n);
      if (n==0) return 0;
      if (!(in.EventType==KEY_EVENT && in.Event.KeyEvent.bKeyDown))
        ReadConsoleInput(InH,&in,1,&n);
  } while (!(in.EventType==KEY_EVENT && in.Event.KeyEvent.bKeyDown));
  return in.Event.KeyEvent.wVirtualKeyCode;
}

int BaseInput::ReadKey()
{
  INPUT_RECORD in;
  while (!KeyPressed());
  ReadInput(&in);
  return in.Event.KeyEvent.wVirtualKeyCode;
}

void BaseInput::ProcessKey(KEY_EVENT_RECORD *k)
{
  if (k->bKeyDown && k->uChar.AsciiChar==27)
    Quit=TRUE;
}

void BaseInput::ProcessMouse(MOUSE_EVENT_RECORD *m)
{
}

BOOL BaseInput::PreProcessKey(KEY_EVENT_RECORD *k)
{
  return FALSE;
}

BOOL BaseInput::PreProcessMouse(MOUSE_EVENT_RECORD *m)
{
  return FALSE;
}

void BaseInput::PostProcessKey(KEY_EVENT_RECORD *k)
{
}

void BaseInput::PostProcessMouse(MOUSE_EVENT_RECORD *m)
{
}

//////////////////////////////////////////////////////////////
ScreenObject::ScreenObject()
{
  Init();
}

ScreenObject::ScreenObject(int x1,int y1,int x2,int y2)
{
  Init();
  SET(X1,x1);
  SET(Y1,y1);
  SET(X2,x2);
  SET(Y2,y2);
}

void ScreenObject::Init()
{
  CONSOLE_SCREEN_BUFFER_INFO sbi;
  OutH=GetStdHandle(STD_OUTPUT_HANDLE);
  GetConsoleScreenBufferInfo(OutH, &sbi);
  ScrSize = sbi.dwSize;
  X1=0; Y1=0;
  X2=ScrSize.X-1; Y2=ScrSize.Y-1;
  Attr=0x07;
  Buf=(CHAR_INFO*)MALLOC(sizeof(CHAR_INFO)*ScrSize.X*ScrSize.Y);
  OldBuf=NULL;
  Visible=FALSE;
}

ScreenObject::~ScreenObject()
{
  if (OldBuf) FREE(OldBuf);
  if (Buf) FREE(Buf);
}

void ScreenObject::Draw()
{
  SMALL_RECT r={X1,Y1,X2,Y2};
  COORD xy={X1,Y1};
  CHAR_INFO ch={' ',Attr};
  CHAR_INFO *p;
  for (int y=Y1;y<=Y2;y++)
  {
    p=Buf+(ScrSize.X*y+X1); //*sizeof(ch)
    for (int x=X1;x<=X2;x++)
      *p++=ch;
  }
  WriteConsoleOutput(OutH,Buf,ScrSize,xy,&r);
}

void ScreenObject::WriteStr(char *s,int x,int y,int Color)
{
  if (*s==0) return;

  SMALL_RECT r={x,y,x+strlen(s)-1,y};
  COORD xy={x,y};
  CHAR_INFO ch={' ',Attr};
  CHAR_INFO *p=Buf+(ScrSize.X*y+x);

  if (Color) ch.Attributes=Color;
  for (int i=0;s[i];i++)
  {
    ch.Char.AsciiChar=s[i];
    *p++=ch;
  }
  WriteConsoleOutput(OutH,Buf,ScrSize,xy,&r);
}

void ScreenObject::InitBuf()
{
  COORD dbSize={Width(),Height()},
        dbCoord={0,0};
  SMALL_RECT r={X1,Y1,X2,Y2};

  ReadConsoleOutput(OutH,Buf,dbSize,dbCoord,&r);
}

void ScreenObject::WriteStrBuf(char *s,int x,int y,int Color)
{
  if (*s==0) return;

  CHAR_INFO ch={' ',Attr};
  CHAR_INFO *p=Buf+(ScrSize.X*y+x);

  if (Color) ch.Attributes=Color;
  for (int i=0;s[i];i++)
  {
    ch.Char.AsciiChar=s[i];
    *p++=ch;
  }
}

void ScreenObject::FlushBuf()
{
  COORD dbSize={Width(),Height()},
        dbCoord={0,0};
  SMALL_RECT r={X1,Y1,X2,Y2};

  WriteConsoleOutput(OutH,OldBuf,dbSize,dbCoord,&r);
}

void ScreenObject::Show()
{
  if (Visible) return;

  COORD dbSize={Width(),Height()},
        dbCoord={0,0};
  SMALL_RECT r={X1,Y1,X2,Y2};

  if (OldBuf) FREE(OldBuf);
  OldBuf=(CHAR_INFO*)MALLOC(sizeof(CHAR_INFO)*dbSize.X*dbSize.Y);
  ReadConsoleOutput(OutH,OldBuf,dbSize,dbCoord,&r);
  Draw();

  Visible=TRUE;
}

void ScreenObject::Hide()
{
  if (!Visible) return;

  COORD dbSize={Width(),Height()},
        dbCoord={0,0};
  SMALL_RECT r={X1,Y1,X2,Y2};

  WriteConsoleOutput(OutH,OldBuf,dbSize,dbCoord,&r);
  FREE(OldBuf);
  OldBuf=NULL;

  Visible=FALSE;
}

//////////////////////////////////////////////////////////////

HexEditor::HexEditor()
{
  Data=NULL;
  DataSize=0;
  DataTop=NULL;
  DataPos=NULL;
  X1=Y1=X2=Y2=-1;
  Init();
}

HexEditor::HexEditor(byte *data, dword dataSize, byte *dataTop, byte *dataPos,
                     int x1,int y1,int x2,int y2)
{
  SET(X1,x1);
  SET(Y1,y1);
  SET(X2,x2);
  SET(Y2,y2);
  Data=data;
  DataSize=dataSize;
  DataTop=dataTop;
  DataPos=dataPos;
  Init();
}

void HexEditor::Init()
{
  if (DataTop==NULL || DataTop<Data || DataTop>=Data+DataSize)
    DataTop=Data;
  DataTop=Data+((DataTop-Data)&0xFFFFFFF0); // выравниваем до 10h байт

  CalcDataScrSize();

  if (DataPos==NULL || DataPos<DataTop)
    DataPos=DataTop;
  else
    if (DataPos>DataTop+DataScrSize-1)
      DataPos=DataTop+DataScrSize-1;

  DataNibble=0;
  Attr=0x1B;
  EditMode=HEX_MODE;
  Changed=FALSE;
  DecodeTable=NULL;
  EncodeTable=NULL;
}

void HexEditor::OutLine(int from,int y)
{
/*///////
  char ss[80];
  sprintf(ss,"From=%08X  y=%d; DataPos=%08X",
            from,y,DataPos-DataTop);
  WriteStr(ss,0,24,0x4f);

*////////

  byte *p=DataTop+from, *pp; // указатель на текущий байт
  char s[256],  // строка для вывода
       *ch,     // текущий символ
       hex[10]; // строка для перевода чисел в hex
  int i;

  strcpy(s,DWord2Hex((dword)(p-Data),hex)); // адрес
  strcat(s,":  ");
  pp=p;
  for (i=0;i<=0xF;i++) // формируем Hex-представление очередных 16 байт
  {
    if ((dword)(p-Data)<DataSize) {
      strcat(s,Byte2Hex(*p++,hex));
      strcat(s," ");
    }
    else strcat(s,"   ");
    if (i==7) strcat(s,"\263 "); // в ДОС-кодировке это вертикальная линия
  }
  strcat(s,"  ");
  p=pp; // возврат на начало этих 16 байт
  ch=s+strlen(s); // встали на конец s
  for (i=0;i<=0xF;i++) // формируем Char-представление этих 16 байт
  {
    if ((dword)(p-Data)<DataSize)
      *ch++=(*p)?(DecodeTable?DecodeTable[*p]:*p):' ';
    else
      *ch++=' ';
    p++;
  }
  for (i=X_CHAR+16;i<X2-X1+1;i++)
    *ch++=' ';
  *ch=0; //EOL
  WriteStr(s,X1,y);
}

void HexEditor::Draw()
{
//  ScreenObject::Draw(); // рисуем "фон"
  byte *p=DataTop; // указатель на текущий байт
  char s[256];  // строка для вывода

  for (int y=Y1;y<=Y2;y++) // по всем строчкам
  {
    if ((dword)(p-Data)<DataSize)
    {
      OutLine(p-DataTop,y);
      p+=16;
    }
    else {
      memset(s,' ',X2-X1+1); s[X2-X1+1]=0;
      WriteStr(s,X1,y);
    }
  }

  MoveCursor();
}

void HexEditor::MoveRight(BOOL Byte)
{
  if (EditMode==HEX_MODE) {
    if (Byte) {
      DataNibble=0;
      if (DataPos<DataTop+DataScrSize-1)
        DataPos++;
    }
    else {
      if (DataNibble==0)
        DataNibble=1;
      else
        if (DataPos<DataTop+DataScrSize-1) {
          DataNibble=0;
          DataPos++;
        }
    }
  }
  else // CHAR_MODE
    if (DataPos<DataTop+DataScrSize-1)
      DataPos++;
  MoveCursor();
}

void HexEditor::doPageUp()
{
  DataPos=(DataPos-DataTop<=0xF) ? DataTop : DataTop+((DataPos-DataTop)&0xF);
}

void HexEditor::doPageDown()
{
  if ((dword)(DataPos-DataTop)>=DataScrSize-16)
    DataPos=DataTop+DataScrSize-1;
  else {
    if (DataScrSize&0xF) // не на весь экран
      DataPos=DataTop+(DataScrSize&0xFFFFFFF0)+((DataPos-DataTop)&0xF);
    else
      DataPos=DataTop+DataScrSize-16+((DataPos-DataTop)&0xF);
    if (DataPos>DataTop+DataScrSize-1)
      DataPos=DataTop+DataScrSize-1;
  }
}

void HexEditor::ProcessKey(KEY_EVENT_RECORD *k)
{
  if (!(k->bKeyDown)) return;
  if (k->uChar.AsciiChar==27)
  {
    Quit=TRUE;
    return;
  }
  switch (k->wVirtualKeyCode) {

    case VK_LEFT:
    case VK_BACK:

      if (EditMode==HEX_MODE) {
        if (k->dwControlKeyState&(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) {
          DataNibble=0;
          if (DataPos>DataTop)
            DataPos--;
        }
        else
          if (DataNibble==1)
            DataNibble=0;
          else
            if (DataPos>DataTop) {
              DataNibble=1;
              DataPos--;
            }
      }
      else // CHAR_MODE
        if (DataPos>DataTop)
          DataPos--;
      MoveCursor();
      break;

    case VK_RIGHT:
      MoveRight(k->dwControlKeyState&(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED));
      break;

    case VK_UP:

      if (DataPos-16>=Data)
        DataPos-=16;
      if (DataPos<DataTop) {
        DataTop-=16;
        CalcDataScrSize();
        Draw();
      }
      MoveCursor();
      break;

    case VK_DOWN:

      if (DataPos+16<Data+DataSize)
        DataPos+=16;
      if (DataPos>=DataTop+DataScrSize) {
        DataTop+=16;
        CalcDataScrSize();
        Draw();
      }
      MoveCursor();
      break;

    case VK_HOME:

      if (k->dwControlKeyState&(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) {
        DataPos=Data;
        DataTop=Data;
        CalcDataScrSize();
        Draw();
      }
      else
        DataPos=DataTop+((DataPos-DataTop)&0xFFFFFFF0);
      DataNibble=0;
      MoveCursor();
      break;

    case VK_END:

      if (k->dwControlKeyState&(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) {
        DataPos=Data+DataSize-1;
        if (DataPos>=DataTop+DataScrSize) {
          DataTop=DataPos-(Height()-1)*16;
          DataTop=Data+((DataTop-Data)&0xFFFFFFF0);
          CalcDataScrSize();
          Draw();
        }
      }
      else {
        DataPos=DataTop+((DataPos-DataTop)|0xF);
        if (DataPos>DataTop+DataScrSize-1) DataPos=DataTop+DataScrSize-1;
      }
      DataNibble=0;
      MoveCursor();
      break;

    case VK_PRIOR:

      if (k->dwControlKeyState&(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) {
				doPageUp();
      }
      else {
        if (DataTop>Data) {
          byte *oldDataTop=DataTop;

          DataTop-=(Height()-1)*16;
          if (DataTop<Data)
            DataTop=Data;
          DataPos-=oldDataTop-DataTop;

          CalcDataScrSize();
          Draw();
        }
        else
          doPageUp();
      }
      DataNibble=0;
      MoveCursor();
      break;

    case VK_NEXT:

      if (k->dwControlKeyState&(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)) {
        doPageDown();
      }
      else {
        if (DataTop+DataScrSize<Data+DataSize) {
          byte *oldDataTop=DataTop;

          DataTop+=(Height()-1)*16;
          if (DataTop>=Data+DataSize)
            DataTop=Data+((DataTop-Data)&0xFFFFFFF0)-16;
          DataPos+=DataTop-oldDataTop;
          if (DataPos>=Data+DataSize)
            DataPos=Data+DataSize-1;

          CalcDataScrSize();
          Draw();
        }
        else
          doPageDown();
      }
      DataNibble=0;
      MoveCursor();
      break;

    case VK_TAB:

      if (EditMode==HEX_MODE) {
        EditMode=CHAR_MODE;
        DataNibble=0;
      }
      else 
        EditMode=HEX_MODE;
      MoveCursor();
      break;

    default:
      char ch=k->uChar.AsciiChar;
      if (!ch) return;
      if (EditMode==HEX_MODE) {
        ch=toupper(ch);
        if (ch>='A'&&ch<='F'||ch>='0'&&ch<='9') {
          byte n=HexChar2Byte(ch);
          if (DataNibble)
            *DataPos=(*DataPos & 0xF0) | n;
          else
            *DataPos=(*DataPos & 0x0F) | (n<<4);
          COORD curPos=GetCurPos();
          OutLine((dword)(DataPos-DataTop)&0xFFFFFFF0,curPos.Y);
          MoveRight();
          Changed=TRUE;
        }
      }
      else {
        *DataPos=(EncodeTable)?(EncodeTable[(byte)ch]):ch;
        COORD curPos=GetCurPos();
        OutLine((dword)(DataPos-DataTop)&0xFFFFFFF0,curPos.Y);
        MoveRight();
        Changed=TRUE;
      }
  }
}

void HexEditor::ProcessMouse(MOUSE_EVENT_RECORD *m)
{
  if (!m->dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED) return; //кнопка не нажата
  dword y=m->dwMousePosition.Y-Y1;
  if (y*16>(dword)(DataTop+DataScrSize-1)) return; // вне области данных
  dword x=m->dwMousePosition.X-X1;
  int i; // # байта в строке данных
  byte nib=0;
  int edMode=HEX_MODE;

  if (x<X_HEX) i=0;
  else
    if (x<X_HEX+3*8) { i=(x-X_HEX)/3; nib=(x-X_HEX)%3?1:0; }
    else
      if (x<X_CHAR-2) { i=8+(x-(X_HEX+3*8+2))/3; nib=(x-(X_HEX+3*8+2))%3?1:0; }
      else
        if (x<X_CHAR) { i=15; nib=1; }
          else
            if (x<X_CHAR+16) { edMode=CHAR_MODE; i=x-X_CHAR; }
            else { edMode=CHAR_MODE; i=15; }
  byte *p=DataTop+y*16+i;
  if (p>DataTop+DataScrSize-1) return;
  DataPos=p;
  DataNibble=nib;
  EditMode=edMode;
  MoveCursor();
}

void HexEditor::GoTo(byte *Addr)
{
  if (Addr>=Data+DataSize) return;
  DataPos=Addr;
  DataTop=DataPos-Height()/2*16;
  if (DataTop<Data)
    DataTop=Data;
  else
    DataTop=Data+((DataTop-Data)&0xFFFFFFF0); // выравниваем до 10h байт
  CalcDataScrSize();
  Draw();
  MoveCursor();
}
/*
byte *searchbyte(byte *s,byte *x,dword size, BaseInput *in)
{
  return s;
}
*/
byte *search(byte *x, dword xsize, byte *data, dword size)
{
  byte *s=data, *p;
//  BaseInput in;

  if (xsize==0 || size<xsize) return 0;
  size=size-xsize+1;

  for (dword i=0;i<size;i++)
  {
    //p=searchbyte(s,*x,size, &in);// search for the first byte
    p=(byte*)memchr(s,(int)*x,size);// memchr - виснет на больших массивах?!?
    if (p==0) return 0;             // not found
                                    // found =>
    if (memcmp(p,x,xsize)==0)       //  compare other bytes
      return p;                     // all bytes are equal => found!
    size-=p-s; // размер области поиска уменьшился
    s=p+1; // next byte
  }
  return 0;
}

byte *HexEditor::Search(byte *x, dword xsize)
{
  return search(x, xsize, DataPos+1, DataSize-(DataPos-Data)-1);
}

COORD HexEditor::GetCurPos()
{
  dword i=DataPos-DataTop; // номер байта от начала экрана
  int j;                     // номер байта от начала строки
  COORD pt;

  pt.Y=(SHORT)(Y1+i/16);
  j=i%16;
  if (EditMode==HEX_MODE)
    pt.X=X1+X_HEX+j*3+(j>7?2:0)+DataNibble;
  else
    pt.X=X1+X_CHAR+j;

  return pt;
}

void HexEditor::MoveCursor()
{
  SetConsoleCursorPosition(OutH,GetCurPos());
}

byte *HexEditor::GetDataPos()
{
  return DataPos;
}

void HexEditor::CalcDataScrSize()
{
  DataScrSize=MIN(DataSize-(DataTop-Data),(dword)16*(Y2-Y1+1));
              // граница - конец либо данных, либо окна
}

void HexEditor::ChangeWindowSize(int x1, int y1, int x2, int y2)
{
  SET(X1,x1);
  SET(Y1,y1);
  SET(X2,x2);
  SET(Y2,y2);
  CalcDataScrSize();
}

///////////////////////////// END /////////////////////////////////
