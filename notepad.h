#ifndef notepad_h
#define notepad_h

#include<windows.h>


#define EDITOR_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif


typedef struct erow{
  int size;
  char *chars;
} erow;


// global variables

HANDLE hStdin;
HANDLE hStdout;
DWORD fdwSaveOldMode;
DWORD fdwSaveOldModeWrite;
int screenRows = 29;
int screenCols = 120; 
int cx, cy;
int rowoff;
int numrows;
erow *row;
CONSOLE_CURSOR_INFO cursorInfo;

enum editorKey{
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  PAGE_UP,
  PAGE_DOWN,
  HOME_KEY,
  END_KEY,
  DEL_KEY
};



// appaend buffer

#define ABUF_INIT {NULL, 0}

struct abuf {
  char *b; 
  int len;
};

/*** function defs ***/


// terminal funcs
void die(const char *s);
void disableRawMode();
void enableRawMode();
int editorReadKey(PINPUT_RECORD irInBuf, DWORD *cNumRead);
int getWindowSize(int *rows, int *cols);

// file i/o funcs

void editorOpen();

// row opperations

void editorAppendRow(char *s, size_t len);

//append buffer funcs
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

//output funcs
void editorDrawRows(struct abuf *ab);
void printKey(KEY_EVENT_RECORD input);
void editorScroll();
void editorRefreshScreen();

//input funcs
void editorProcessKeypress(PINPUT_RECORD irInBuf, DWORD *cNumRead);
void editorMoveCursor(int key);

// init funcs
void initEditor();



#endif