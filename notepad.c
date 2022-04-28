#define _DEFULT_SOURCE
#define _DSB_SOURCE
#define _GNU_SOURCE

#include<stdio.h>
#include<windows.h>
#include<unistd.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<sys/types.h>
#include"notepad.h"

//TODO set tildas to grow as new lines are added
// set a var for number of lines
// increase that count whenever cr is pressed

/*** defines ***/


/*** terminal **/
void die(const char *s){

  WriteConsoleW(hStdout, L"\x1b[1J", (DWORD)wcslen(L"\x1b[2J"), 0, NULL);
  WriteConsoleW(hStdout, L"\x1b[H", (DWORD)wcslen(L"\x1b[H"), 0, NULL);
  perror(s);
  SetConsoleMode(hStdin, fdwSaveOldMode);
  exit(1);
}

void disableRawMode(){
  if(SetConsoleMode(hStdin, fdwSaveOldMode) == 0) die("set console mode 1");
  if(SetConsoleMode(hStdout, fdwSaveOldModeWrite) == 0) die("set console mode 2");

}

void enableRawMode(){

  // sets input mode
  hStdin = GetStdHandle(STD_INPUT_HANDLE); 
  GetConsoleMode(hStdin, &fdwSaveOldMode);
  DWORD mode = fdwSaveOldMode;
  GetConsoleMode(hStdin, &mode);
  mode |= ENABLE_WINDOW_INPUT;
  if(SetConsoleMode(hStdin, mode & ~(ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_PROCESSED_OUTPUT)) == 0); //die("set console mode 3");

  // sets output mode
  hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
  GetConsoleMode(hStdout, &fdwSaveOldModeWrite);
  DWORD omode = fdwSaveOldModeWrite;
  omode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  //SetConsoleMode(hStdout, omode & (ENABLE_PROCESSED_OUTPUT));
  SetConsoleMode(hStdout, omode);

  //GetConsoleCursorInfo(hStdout, &cursorInfo);
  //cursorInfo.dwSize = 1;
  //cursorInfo.bVisible = ~(CURSOR_SHOWING);
  //cursorInfo.bVisible = FALSE;
  //SetConsoleCursorInfo(hStdout, &cursorInfo);
}

int editorReadKey(PINPUT_RECORD irInBuf, DWORD *cNumRead){
  int nread;
  char c;

  switch (WaitForSingleObject(hStdin, 2000)){
    case(WAIT_TIMEOUT):
      break;
  
    case(WAIT_OBJECT_0):
      if (! ReadConsoleInput(hStdin, irInBuf, 1, cNumRead) ) die("read");
      c = irInBuf->Event.KeyEvent.uChar.AsciiChar;
      //printf("%x", irInBuf->Event.KeyEvent.wVirtualKeyCode);
      if((irInBuf->Event.KeyEvent.wVirtualKeyCode >= 0x21 && irInBuf->Event.KeyEvent.wVirtualKeyCode <= 0x28) || irInBuf->Event.KeyEvent.wVirtualKeyCode == 0x2E){

        switch(irInBuf->Event.KeyEvent.wVirtualKeyCode){
            case 0x21: return PAGE_UP;
            case 0x22: return PAGE_DOWN;
            case 0x23: return END_KEY;
            case 0x24: return HOME_KEY;
            case 0x25: return ARROW_LEFT;
            case 0x26: return ARROW_UP;
            case 0x27: return ARROW_RIGHT;
            case 0x28: return ARROW_DOWN;
            case 0x2E: return DEL_KEY;
            default: break;
          }
        }

    default:
      break;
  }

  return c;
  
}


int getWindowSize(int *rows, int *cols){
  //HANDLE han = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO screenInfo;

  //GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE);, &screenInfo);
  //printf("1");
  if(!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &screenInfo) /*|| screenInfo.dwSize.X == 0*/) {
    printf("%d", GetLastError());
    return -1;
    
  }else{
    *rows = screenInfo.dwSize.Y;
    *cols = screenInfo.dwSize.X;
    return 0;
  }

}

/*** file i/o ***/

void editorOpen(char *filename){
  FILE *fp = fopen(filename, "r");
  if(!fp) die("fopen");

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  linelen = getline(&line, &linecap, fp);
  while((linelen = getline(&line, &linecap, fp)) != -1){ 
    while(linelen > 0 && (line[linelen -1] == '\n' || line[linelen-1] == '\n')){
      linelen--;
    }
    editorAppendRow(line,linelen);
  }
  free(line);
  fclose(fp);
}

/*** row operations ***/

void editorAppendRow(char *s, size_t len){
  row = realloc(row, sizeof(erow) * (numrows+1));

  int at = numrows;
  row[at].size = len;
  row[at].chars = malloc(len+1);
  memcpy(row[at].chars, s, len);
  row[at].chars[len] = '\0';
  numrows++;
}

/*** append buffer ***/

void abAppend(struct abuf *ab, const char *s, int len){
  char *new = realloc(ab->b, ab->len + len);

  if(new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab){
  free(ab->b);
}

/*** output ***/

void editorDrawRows(struct abuf *ab){
  int y;
  //for(y = 0; y<screenRows; y++){
  for(y = 0; y<screenRows; y++){
    int filerow = y+rowoff;
    if(filerow >= numrows){
      if(numrows == 0 && y == screenRows/3){
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome), "Text editor -- version %s", EDITOR_VERSION);
        if(welcomelen > screenCols) welcomelen = screenCols;
        int padding = (screenCols - welcomelen) / 2;
        if (padding){
          
          abAppend(ab, "~", 1);
          padding--;
        }
        while(padding-- > 0) abAppend(ab, " ",1);
        abAppend(ab, welcome, welcomelen);
      }else{
        abAppend(ab, "~", 1);
      //WriteConsole(hStdout, "~", 1, 0, NULL);
      }
    }else{
      int len = row[filerow].size;
      if(len > screenCols) len = screenCols;
      abAppend(ab, row[filerow].chars, len);
    }

    abAppend(ab, "\x1b[0K", 5);
    if(y<36-1){
      abAppend(ab, "\r\n", 2);
      //WriteConsole(hStdout, "\r\n", (DWORD)3, 0, NULL);
    }
  }

}

void printKey(KEY_EVENT_RECORD input){

  if(input.bKeyDown){
    char c = input.uChar.UnicodeChar;
    if(iscntrl(c)){
      printf("%d\n", c);
    }else{
      printf("%d ('%c')\n", c,c );
  }

  }
}

void editorScroll(){
  if(cy < rowoff){
    rowoff = cy;
  }
  if(cy >= rowoff + screenRows){
    rowoff = cy - screenRows +1;
  }
}

void editorRefreshScreen(){
  editorScroll();

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[1;1H", 7);
  //WriteConsoleW(hStdout, L"\x1b[1;1H", (DWORD)wcslen(L"\x1b[1;1H"), 0, NULL);

  editorDrawRows(&ab);

  
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (cy - rowoff) + 1 , cx + 1);
  abAppend(&ab, buf, strlen(buf));

  //SetConsoleCursorPosition(hStdout, cursorCord);
  abAppend(&ab, "\x1b[?25h", 6);




  WriteConsole(hStdout, ab.b, ab.len, 0, NULL);
  //SetConsoleCursorPosition(hStdout, cursorCord);
  //printf("%s", ab.b);
  abFree(&ab);

  //WriteConsoleW(hStdout, L"\x1b[1;1H", (DWORD)wcslen(L"\x1b[1;1H"), 0, NULL);


}

/*** input ***/
void editorProcessKeypress(PINPUT_RECORD irInBuf, DWORD *cNumRead){
  int c = editorReadKey(irInBuf, cNumRead);

  if(irInBuf->Event.KeyEvent.bKeyDown){
    switch(c){
      case CTRL_KEY('q'):
        WriteConsoleW(hStdout, L"\x1b[1J", (DWORD)wcslen(L"\x1b[1J"), 0, NULL);
        WriteConsoleW(hStdout, L"\x1b[H", (DWORD)wcslen(L"\x1b[H"), 0, NULL);
        exit(0);
        break;

      case PAGE_UP:
      case PAGE_DOWN:
        {
          int times = screenRows;
          while(times--) editorMoveCursor(c == PAGE_UP ? ARROW_UP: ARROW_DOWN);
        }

      case HOME_KEY:
        cx = 0;
        break;
      case END_KEY:
        cx = screenCols - 1;
        break;


      case ARROW_UP:
      case ARROW_DOWN:
      case ARROW_LEFT:
      case ARROW_RIGHT:
        editorMoveCursor(c);
        break;
      }
    }
}


void editorMoveCursor(int key){
  switch(key){
    case ARROW_LEFT:
      if(cx != 0) cx--;
      break;
    
    case ARROW_RIGHT:
      if(cx != screenCols) cx++;
      break;

    case ARROW_UP:
      if(cy != 0) cy--;
      break;
    
    case ARROW_DOWN:
      if(cy < numrows)cy++;
      break;
  }
}

/*** init ***/

void initEditor(){
  cx = 0;
  cy = 0;
  rowoff = 0;
  numrows = 0;
  row = malloc(1);

  //if(getWindowSize(&screenRows, &screenCols) == -1) die("window size");
}

int main(int argc, char *argv[]){

  enableRawMode();
  initEditor();
  if(argc >= 2){
    editorOpen(argv[1]);
  }

  DWORD cNumRead, fdwMode, i;
  INPUT_RECORD irInBuf[128];
  int counter=0;
  
  char c;
  while(1){
    editorRefreshScreen();
    editorProcessKeypress(irInBuf, &cNumRead);
  }
  
  return 0;

}