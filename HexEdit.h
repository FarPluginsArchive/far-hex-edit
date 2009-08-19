#ifndef _HEXEDIT_H_
#define _HEXEDIT_H_

#define byte unsigned char
#define dword unsigned long

#define MALLOC(size) GlobalAlloc(GMEM_FIXED,size)
#define FREE(p) GlobalFree((HGLOBAL)p)

// Hex conversion
byte HexChar2Byte(char ch);
byte Hex2Byte(char *s);
char *Byte2Hex(byte b, char *s);
char *DWord2Hex(dword x, char *s);
dword Hex2DWord(char *s);
byte *HexStr2Bytes(char *hexStr, byte *data, int *dataSize);

///////////////////////////////////////////////////////////////////////////
class BaseInput
{
public:
  BaseInput();
  ~BaseInput();
  virtual void ReadInput(INPUT_RECORD *in)
     { DWORD n; ReadConsoleInput(InH,in,1,&n); }

  virtual void ProcessKey(KEY_EVENT_RECORD *k);
  virtual void ProcessMouse(MOUSE_EVENT_RECORD *m);
  virtual void Run();

  // Функции, вызываемые до ProcessXXX.
  // Могут использоваться для своей обработки
  // Результат - TRUE, если событие обработано,
  //    иначе идет вызов ProcessXXX.
  virtual BOOL PreProcessKey(KEY_EVENT_RECORD *k);
  virtual BOOL PreProcessMouse(MOUSE_EVENT_RECORD *m);

  // Функции, вызываемые после отрабатывания ProcessXXX.
  // Могут использоваться для своей дополнительной обработки
  virtual void PostProcessKey(KEY_EVENT_RECORD *k);
  virtual void PostProcessMouse(MOUSE_EVENT_RECORD *m);

  // Проверка, нажата ли клавиша. Если нажата, возвращает virtual code
  int KeyPressed();

  // Ожидает ввода клавиши, возвращает virtual code
  int ReadKey();

public:
  BOOL Quit;
protected:
  HANDLE InH;
};

///////////////////////////////////////////////////////////////////////////
class ScreenObject : public BaseInput
{
private:
  void Init();
public:
  ScreenObject();
  ScreenObject(int x1,int y1,int x2,int y2);
  ~ScreenObject();
  int Width() { return X2-X1+1; };
  int Height() { return Y2-Y1+1; };
  virtual void Show();
  virtual void Hide();
  virtual void Draw();
  virtual void WriteStr(char *s,int x,int y,int Color=0);
  // Buffered output:
  virtual void InitBuf();
  virtual void WriteStrBuf(char *s,int x,int y,int Color=0);
  virtual void FlushBuf();
  //
protected:
  HANDLE OutH;
  COORD ScrSize; // размеры консоли
  CHAR_INFO *Buf, // промежуточный буфер для вывода на экран
         *OldBuf; // буфер для хранения части экрана под объектом
public:
  int X1,Y1,X2,Y2; // координаты окна объекта
  WORD Attr;
  BOOL Visible;
};

///////////////////////////////////////////////////////////////////////////
class HexEditor : public ScreenObject
{
public:
  HexEditor();      // конструктор по-умолчанию

  HexEditor(        // основной конструктор
    byte *data,     // данные
    dword dataSize, // их размер
    byte *dataTop=0,  // начало данных на экране
    byte *dataPos=0, // текущая позиция
    int x1=-1,int y1=-1,int x2=-1,int y2=-1); // координаты окна

  virtual void ProcessKey(KEY_EVENT_RECORD *k);
  virtual void ProcessMouse(MOUSE_EVENT_RECORD *m);
  virtual void Draw();
  void GoTo(byte *Addr);
  byte *Search(byte *x, dword xsize);
  void ChangeWindowSize(int x1, int y1, int x2, int y2);

  enum {HEX_MODE, CHAR_MODE}; // режим редактирования

  BOOL Changed;    // Флаг об изменении
  byte *GetDataPos();
  char *DecodeTable; // Table to decode to DOS codepage
  char *EncodeTable; // Table to encode from DOS codepage

protected:
  byte *Data;      // редактируемые данные
  dword DataSize;  // размер данных
  byte *DataTop;   // указатель на начало данных на экране
  byte *DataPos;   // указатель на байт, где стоит курсор
  byte DataNibble; // полубайт, где стоит курсор
  dword DataScrSize;  // размер данных, видимых на экране
  int EditMode;    // режим редактирования - Hex/Char

  COORD GetCurPos(); // получение координат курсора для редактируемого байта
  void MoveCursor(); // переместить курсор в позицию, соответствующую DataPos
  void MoveRight(BOOL Byte=0);  // перемещение вправо требуется отдельно - при вводе
  void OutLine(int from,int y);    // вывод строки
  void CalcDataScrSize();
private:
  void doPageDown();
  void doPageUp();
  void Init();
};

#endif // _HEXEDIT_H_
