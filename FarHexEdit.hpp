extern "C"
{
  void WINAPI _export SetStartupInfo(struct PluginStartupInfo *Info);
  HANDLE WINAPI _export OpenPlugin(int OpenFrom,int Item);
  void WINAPI _export GetPluginInfo(struct PluginInfo *Info);
//  void WINAPI _export GetOpenPluginInfo(HANDLE hPlugin, struct OpenPluginInfo *Info);
};

static struct PluginStartupInfo Info;

char *GetMsg(int MsgId);

void SetRegKey(HKEY hRoot,char *Key,char *ValueName,DWORD ValueData);
void SetRegKey(HKEY hRoot,char *Key,char *ValueName,char *ValueData);
int GetRegKey(HKEY hRoot,char *Key,char *ValueName,int &ValueData,DWORD Default);
int GetRegKey(HKEY hRoot,char *Key,char *ValueName,DWORD Default);
int GetRegKey(HKEY hRoot,char *Key,char *ValueName,char *ValueData,char *Default,DWORD DataSize);

char FarRootKey[80];
byte FarPalette[100];

#define COLOR_KEYBARNUM 45
#define COLOR_KEYBAR 46
#define COLOR_EDITOR 53
#define COLOR_EDSTATUSBAR 55

struct InitDialogItem
{
  unsigned char Type;
  unsigned char X1,Y1,X2,Y2;
  unsigned char Focus;
  unsigned int Selected;
  unsigned int Flags;
  unsigned char DefaultButton;
  char *Data;
};

void InitDialogItems(struct InitDialogItem *Init,struct FarDialogItem *Item,
                    int ItemsNumber);

///////////////////////////////////////////////////////

class FarHexEditor: public HexEditor
{
public:
  FarHexEditor(      // основной конструктор
    byte *data,      // данные
    dword dataSize,  // их размер
    byte *dataTop=0, // начало данных на экране
    byte *dataPos=0, // текущая позиция
    int x1=-1,int y1=-1,int x2=-1,int y2=-1); // координаты окна

  virtual BOOL PreProcessKey(KEY_EVENT_RECORD *k);
  virtual void PostProcessKey(KEY_EVENT_RECORD *k);
  virtual void WriteStr(char *s,int x,int y,int Color=0);
  virtual void ReadInput(INPUT_RECORD *in);
};

