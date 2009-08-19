!include <win32.mak>

PROJ = FarHexEdit
PROJ_DLL = HexEdit

all: $(PROJ).dll

PROJ_OBJS  = $(PROJ).obj HexEdit.obj

HexEdit.obj: "HexEdit.cpp"
  $(cc) $(cdebug) $(cflags) $(cvars) "HexEdit.cpp"
# -Zp2

$(PROJ).obj: $(PROJ).cpp
  $(cc) $(cdebug) $(cflags) $(cvars) $(PROJ).cpp

#.c.obj:
#  $(cc) $(cdebug) $(cflags) $(cvars) -Zp2 -D_export= $*.c

#$(PROJ).dll: $(PROJ_OBJS)
#    $(link) $(linkdebug) $(dlllflags) $(PROJ_OBJS) \
#    -out:$(PROJ).dll $(MAPFILE) -def:$(PROJ).def

$(PROJ).dll: $(PROJ_OBJS)
  $(link) /NODEFAULTLIB /INCREMENTAL:NO /PDB:NONE /RELEASE /NOLOGO \
  /NOENTRY -dll $(PROJ_OBJS) kernel32.lib user32.lib advapi32.lib libc.lib \
  -out:$(PROJ_DLL).dll -def:$(PROJ).def

clean:
    del *.bak *.pdb *.obj *.res *.exp *.map *.sbr *.bsc
