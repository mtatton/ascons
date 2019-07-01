/* 
  You can do whatever You want with it. But there is no warranty
  nor even implied warranty on both binary and code provided.
*/
// 20190701 ANSCONS 0.2.1
// 20190207 Added page down, up, etc.
// 20190207 Added Sauce Record processing

#include <stdio.h>
#include <stdint.h>
#include <locale.h>
#include <curses.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PCOL  8000
#define PROW 64000
#define DEBUG 0
#define TEST 0

#define ESC 27
#define TUI_ROWS LINES

#define EOL_TYPE_DOS  0
#define EOL_TYPE_UNIX 1

int line_length=0;

// Thanks to SAUCE.TXT
// from ACIDDRAW distrib
struct Sauce {
  char           ID[5];
  char           Version[2];
  char           Title[35];
  char           Author[20];
  char           Group[20];
  char           Date[8];
  int32_t        FileSize;
  //signed long    FileSize;
  unsigned char  DataType;
  unsigned char  FileType;
  unsigned short TInfo1;
  unsigned short TInfo2;
  unsigned short TInfo3;
  unsigned short TInfo4;
  unsigned char  Comments;
  unsigned char  Flags;
  char           Filler[22];
}; 

struct Sauce sauce;

// getopt
extern int getopt(int argc, char * const argv[], const char *optstring);
extern int optind, opterr, optopt;
extern char *optarg;
// long
int c;
int digit_optind = 0;

char csi_char[16];

int buffer[PCOL][PROW];

int eol_type=0;

int line_printable_chars=0;
int max_line_printable_chars=0;
char sauce_line[PCOL];
int sauce_width=0;
int sauce_heigth=0;
int sauce_found=0;
int sauce_found_struct=0;
int file_sauce_begin=0;

int last_col_size=0;
int printed_chars=0;

int lines=0;
char *file_name;
char user_name[6]=".user";
char bookmark_file_name[1024];
char sauce_file_name[1024];
int det_lines=0;
int ans_seq_flag=0;

int file_chars=0;
int rows=0;
int cols=0;
int max_cols=0;
int hor_shift=0;

int colorize=1;

int conv_table[2][256]={
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255},
{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,199,252,233,226,228,224,229,231,234,235,232,239,238,236,196,197,201,230,198,244,246,242,251,249,255,214,220,162,163,165,8359,402,225,237,243,250,241,209,170,186,191,8976,172,189,188,161,171,187,9617,9618,9619,9474,9508,9569,9570,9558,9557,9571,9553,9559,9565,9564,9563,9488,9492,9524,9516,9500,9472,9532,9566,9567,9562,9556,9577,9574,9568,9552,9580,9575,9576,9572,9573,9561,9560,9554,9555,9579,9578,9496,9484,9608,9604,9612,9616,9600,945,223,915,960,931,963,181,964,934,920,937,948,8734,966,949,8745,8801,177,8805,8804,8992,8993,247,8776,176,8729,183,8730,8319,178,9632,160}
};

void scr_reset_video_attr() {
  printf("\e[m");
}

void scr_reset_screen() {
  printf("\e[2J\e[0;0H\e[1;37;40m\e[m\r\n"); 
}

void exit_routine(int mode) {

  if (mode==0) {
    scr_reset_video_attr();
  } else if (mode==1) {
    scr_reset_screen();
  } else if (mode==2) {
    scr_reset_video_attr();
    scr_reset_screen();
    printf("File Too Large\n");
  }

  exit(0);

}



int conv2utf8(int charin) {

  int r=0;

  if (r<256) {
    r=conv_table[1][charin];
  } else {
    r=' ';
  }

  return(r);

}

int detect_h(int c) {
  if (c=='h') {
    return(1);
  }
  return(0);
}

int detect_esc(int c) {
  if (c==27) {
    return(1);
  }
  return(0);
}

int detect_m(int c) {
  if (c=='m') {
    return(1);
  }
  return(0);
}

int detect_t(int c) {
  if (c=='t') {
    return(1);
  }
  return(0);
}

int detect_u(int c) {
  if (c=='u') {
    return(1);
  }
  return(0);
}

int detect_s(int c) {
  if (c=='s') {
    return(1);
  }
  return(0);
}

int detect_c(int c) {
  if (c=='C') {
    return(1);
  }
  return(0);
}

int detect_cr(int c) {
  if (c==13) {
    return(1);
  }
  return(0);
}

int is_digit(int c) {
  if (c>='0'&&c<='9') {
    return (1);
  }
  return (0);
}

int detect_eol(int c) {
  if (c==10) {
    return(1);
  }
  return(0);
}

void deinit_console(int mode) {

  endwin();
  nl();
  echo();
  if (mode==1) { printf ("\e[2J\e[0;0H\e[1;37;40m\r\n"); }
  if (mode==2) { scr_reset_video_attr(); }

}

void count_add_characters(int len) {
  line_printable_chars+=len;
}

void count_reset_printable_chars() {
  line_printable_chars=0;
}

void count_add_character() {
  line_printable_chars++;
}


int get_csi_shift() {

  int int_shift=0;
  char char_shift[5];

  if (is_digit(csi_char[1]) && is_digit(csi_char[2])) {
    char_shift[0]=csi_char[1];
    char_shift[1]=csi_char[2];
    char_shift[2]='\0';
    int_shift=atoi(char_shift);
  } else if (is_digit(csi_char[2]) && !is_digit(csi_char[1])) {
    char_shift[0]=csi_char[2];
    char_shift[1]='\0';
    int_shift=atoi(char_shift);
  } else if (is_digit(csi_char[1]) && !is_digit(csi_char[2])) {
    char_shift[0]=csi_char[1];
    char_shift[1]='\0';
    int_shift=atoi(char_shift);
  }

  return (int_shift);

}

int test_file_exists(char *file_name) {

  FILE *file;

  file = fopen(file_name, "rb+");

  if (file) {
    return(1);
  } else {
    return(0);
  }

}

void init_buffer() {

  int i;
  int j;

  for (j=0;j<PROW;j++) {
    for (i=0;i<PCOL;i++) {
      buffer[i][j]=0;
    }
  }
}

void get_sauce_line(int sauce_line_no) {

  int i=0;
  int found_sau_on_line=0;
  int diff_sau_late=0;

  for (i=0;i<PCOL-4;i++) {
    if ((buffer[i][sauce_line_no]=='S'&&
         buffer[i+1][sauce_line_no]=='A'&&
         buffer[i+2][sauce_line_no]=='U')
     && (found_sau_on_line==0)
    ) {
      found_sau_on_line=1;
    } else if (found_sau_on_line==0)  {
      diff_sau_late++;
    }
    if (found_sau_on_line) {
      sauce_line[i-diff_sau_late]=buffer[i][sauce_line_no];
    }
  }

}

int count_printable_chars(int c,int row,int col) {

  int csi_mov=0;

  if (ans_seq_flag==0) {
    if (detect_esc(c)) {
      ans_seq_flag=1;
    } else if (detect_cr(c)) {
      ;
    } else if (detect_eol(c)) {
      count_add_character();
    } else {
      count_add_character();
    }
  } else {
    if (detect_u(c)) {
      ans_seq_flag=0;
    } else if (detect_s(c)) {
      ans_seq_flag=0;
    } else if (detect_h(c)) {
      ans_seq_flag=0;
    } else if (detect_t(c)) {
      ans_seq_flag=0;
      printf("\et");
    } else if (detect_m(c)) {
      ans_seq_flag=0;
    } else if (detect_c(c)) {
      int i=col;
      int j=0;
      for (i=col-3;i<=col;i++) {
        csi_char[j]=buffer[i][row];
        j++;
      }
      csi_char[j]='\0';
      csi_mov=get_csi_shift();
      count_add_characters(csi_mov);
      ans_seq_flag=0;
      mvprintw(row,45,"MOVE %03d %04x",csi_mov,row*col);
    }
  }

  return (0);

}

int get_read_data_size() {

  return(file_chars);

}


int mseek_from_end(int shift) {

  int c=0; int i=0; int last_char_pos=0;

  int end_of_file = get_read_data_size();

  if (end_of_file-shift<=0) { return (-1); }

  for (i=0;i<PCOL-1;i++) {
    c=buffer[i][rows-1];
    if (c!=0 && buffer[i+1][rows]!=0) {
    } else {
      last_char_pos=i;
      break;
    }
  }

  return(last_char_pos);

}

void erase_sauce_from_memory() {

  char sauce_tag_exp[]="SAUCE";
  int sauce_chars_detected=0;
  int i=mseek_from_end(128);
  int j=rows-4;
  int pos_sauce_delete=0;
  int eraser=0; int c;

  for (j=rows-3;j<rows;j++) {
    for (i=0;i<PCOL-1;i++) {
      c=buffer[i][j];
      if (c==sauce_tag_exp[sauce_chars_detected]) {
        sauce_chars_detected++;
        if (sauce_chars_detected==4) { eraser=1; pos_sauce_delete=i; }
        if (eraser) { buffer[i][rows-1]=0; }
      }
    }
  }
  if (eraser) { 
    for (i=0;i<pos_sauce_delete+4;i++) {
       buffer[i][rows-1]=0; 
    }
  }

}

int sauce_get_width(int sauce_line_no) {

  get_sauce_line(sauce_line_no);

  return (sauce_line[96]);

}

void decode_sauce(int line) {

  sauce_width=sauce_get_width(line);

}

int search_for_sauce(char *file_name) {

  FILE *file;
  file = fopen(file_name, "rb+");
  int hcomp=0;

  char sauce_tag[5];
  char sauce_tag_exp[]="SAUCE";

  unsigned int width=0;
  unsigned int height=0;

  // STRUCT
  file = fopen(file_name, "rb");
  if (file) {
    fseek(file, -128L, SEEK_END);
    if (fread(&sauce, sizeof(sauce), 1, file)==0) {
      if (strncmp(sauce.ID,sauce_tag_exp,sizeof(sauce.ID))!=0) {
        sauce_found_struct=0;
      } else {
        sauce_found_struct=1;
      }
    }
  }

  if (DEBUG) { 
     //printf("SAUCE FOUND STRUCT %d\n",sauce_found_struct);
     mvprintw(26,2,"SAUCE FOUND STRUCT %d:\n", sauce_found_struct);
  }

/*
  if (DEBUG) { printf("%d %c\n",sauce.Comments,sauce.Comments); }

  if (sauce_found_struct==1) {
    if (sauce.Comments>0) {
      printf("Comment Not Zero\n");
      process_comments(file_name);
    } else {
      printf("Comments 0\n");
    }
    if (sauce.TInfo1>0) {
      sauce_heigth=sauce.TInfo1;
    }
  
    if (sauce.TInfo2>0) {
      sauce_width=sauce.TInfo2;
    } else {
      sauce_width=80;
    }
  }
*/  

  if (file) {
    fseek(file, -128L, SEEK_END);
    hcomp=fread(&sauce_tag,sizeof(sauce_tag),1,file);
    if (strncmp(sauce_tag,sauce_tag_exp,sizeof(sauce_tag))!=0) {
      if (DEBUG) { printf("Could not find sauce\n"); }
      return (-1);
    } else {
      sauce_heigth=80;
    }
    fseek(file, -32L, SEEK_END);
    file_sauce_begin=ftell(file);
    hcomp=fread(&width,sizeof(char),1,file);
    sauce_width=width;
    fseek(file, -30L, SEEK_END);
    hcomp=fread(&height,sizeof(char),1,file);
    if (height>0) {
      sauce_heigth=height;
    } else {
      sauce_heigth=80;
    }
    fclose(file);
    if (hcomp) { ; }
  }
  
  
  if (DEBUG) { printf("width %d height %d\r\n",width,height); }

  return(0);

}

int is_ctrl_char(char in) {

  int ret=0;

  if (in == ESC) {
    ret=0;
  } 
  return (ret);

}

int is_ctrl_cont(char in) {

  int ret=0;
  
  if (in >= 0x40 && in <= 0x5F) {
    ret=1;
  } 

  return (ret);

}

int get_ansi_seq(char in, char in2) {

  int ret=0;
  
  if (is_ctrl_char(in) && is_ctrl_cont(in2)) {
    ret=1;
  }

  return(ret);

}

void print_ansi_file() {

  int i;
  int j;
  int c;

  count_reset_printable_chars();

  for (j=0;j<rows;j++) {
    for (i=0;i<=max_cols;i++) {
      c=buffer[i][j];
      printf("%lc",c);
      count_printable_chars(c,j,i);
      if (line_printable_chars>=sauce_width
       && ans_seq_flag==0  && c!=10
      ) {
        printed_chars+=line_printable_chars;
        count_reset_printable_chars();
        //scr_reset_video_attr();
        if (c!=10 || c!=13) { printf("\r\n"); }
        break;
      } 
    }
    //printf("%d ",j);
  }
}

void print_page(int shift,int fill_flag) {

  int i=0;
  int j=0;
  int c=0;

  count_reset_printable_chars();
  scr_reset_video_attr();
  scr_reset_screen();
  printed_chars=0;

  for (j=shift;j<TUI_ROWS+shift;j++) {
    if (j>=rows && fill_flag == 1) {
      scr_reset_video_attr();
    } else {
      for (i=0;i<=max_cols;i++) {
      //for (i=0+hor_shift;i<=TUI_COLS+hor_shift;i++) {
        c=buffer[i][j];
        printf("%lc",c);
        count_printable_chars(c,j,i);
        if (c==0x1b) { 
          scr_reset_video_attr(); 
          mvprintw(j,60,"%03d %03d",j,line_printable_chars); 
        }
        if (
         //(line_printable_chars>=sauce_width && ans_seq_flag==0  && c!=10)
         (line_printable_chars>=sauce_width && ans_seq_flag==0 && c != 10)
//      && (sauce_found_struct == 1) 
        ) {
          printed_chars+=line_printable_chars;
          count_reset_printable_chars();
          //scr_reset_video_attr();
          if (c!=10 || c!=13) {  // REMOVE
            //printf(" %d %d %d %d ",shift,j+1, TUI_ROWS, printed_chars);
            //printf("\r\n"); 
            printf("\r\n"); 
          }
          break;
        } 
      }
    }
  }
}

void convert_buffer() {

  int i,j,c;

  for (j=0;j<PROW;j++) {
    for (i=0;i<PCOL;i++) {
      c=conv2utf8(buffer[i][j]); 
      if (c==10) { break; }
    }
    if (j==10) { break; }
  } 
}


void count_lines_in_file(char *fName) {

  FILE *file;
  int c=0;

  file = fopen(fName, "r");
  if (file) {
    while (c != EOF) {
      c = getc(file);
      if (cols>PCOL || c==10) { 
        rows++; 
      }
    }
    fclose(file);
  } else {
    printf("File not found\n"); 
    deinit_console(2);
    exit(1);
  }
}

void load_ansi_file(char *fName) {

  FILE *file;
  file = fopen(fName, "r");
  int c;

  if (file) {
    do {
      c=getc(file);
      if (c==0) { c=' '; }
      file_chars++; 
      if (c!='\n'&&c!='\r' && cols < PCOL) { 
        buffer[cols][rows]=conv2utf8(c); 
      }
      count_printable_chars(c,rows,cols);
      if (line_printable_chars>=max_line_printable_chars) {
        max_line_printable_chars=line_printable_chars;
      }
      if ((line_printable_chars >= sauce_width 
       && ans_seq_flag == 0)
       || c=='\n') {
        count_reset_printable_chars();
        rows++;
        cols=0;
      } else {
        cols++;
      }
      if (rows>=PROW) { 
        exit_routine(2);
      }
      if (cols>max_cols) { max_cols=cols; }
    } while (c!=EOF);
    last_col_size=cols;
    fclose(file);
  } else {
    printf("File not found\n"); 
    deinit_console(2);
    exit(1);
  }

}

void check_term() {

  if (COLS<sauce_width) {
    printf("Terminal size %d is less then %d required by ans file\n",
            COLS,sauce_width);
    exit(1);
  }

}

WINDOW *create_newwin(int height, int width, int starty, int startx) {
  WINDOW *local_win;

  local_win = newwin(height, width, starty, startx);
  box(local_win, 0 , 0);          
  wrefresh(local_win);           
  return local_win;

}

int auto_scroll() {

  int shift = 0;

  while (shift < rows-TUI_ROWS) {
    print_page(shift,0);
    timeout(60);
    if (getch()=='a') { break; }
    sleep(1);
    shift++;
  }
  return (shift);

}

void info_window(int shift) {

  WINDOW *info_win;

  info_win=create_newwin(TUI_ROWS,COLS,00,00);
  box(info_win,0,0);
  wrefresh(info_win);
  info_win=create_newwin(TUI_ROWS,COLS,00,00);
  wrefresh(info_win);
  if (sauce_found_struct==1) { 
    mvprintw( 1,2,"ID       : %.*s",(int) sizeof(sauce.ID),sauce.ID);
    mvprintw( 2,2,"VERSION  : %.*s",(int) sizeof(sauce.Version),sauce.Version);
    mvprintw( 3,2,"AUTHOR   : %.*s",(int) sizeof(sauce.Author),sauce.Author);
    mvprintw( 4,2,"TITLE    : %.*s",(int) sizeof(sauce.Title),sauce.Title);
    mvprintw( 5,2,"GROUP    : %.*s",(int) sizeof(sauce.Group),sauce.Group);
    mvprintw( 6,2,"DATE     : %.*s",(int) sizeof(sauce.Date),sauce.Date);
    //mvprintw( 7,2,"FILESIZE : %.*l",sizeof(sauce.FileSize), sauce.FileSize);
    //mvprintw( 7,2,"FILESIZE : %d",(int) sauce.FileSize);
    mvprintw( 7,2,"FILESIZE : %d",file_chars);
    mvprintw( 8,2,"DataType : %d", (int) sauce.DataType);
    mvprintw( 9,2,"FileType : %d", sauce.FileType);
    mvprintw(10,2,"TInfo1   : %.*d",sizeof(sauce.TInfo1),sauce.TInfo1);
    mvprintw(11,2,"TInfo2   : %.*d",sizeof(sauce.TInfo2),sauce.TInfo2);
    mvprintw(12,2,"TInfo3   : %.*d",sizeof(sauce.TInfo3),sauce.TInfo3);
    mvprintw(13,2,"TInfo4   : %.*d",sizeof(sauce.TInfo4),sauce.TInfo4);
  } else {
    mvprintw( 1,2,"COULD NOT READ SAUCE");
  }

 
}


void help_window(int shift) {

  WINDOW *help_win;

  help_win=create_newwin(TUI_ROWS,COLS,00,00);
  box(help_win,0,0);
  wrefresh(help_win);
  help_win=create_newwin(TUI_ROWS,COLS,00,00);
  wrefresh(help_win);
  mvprintw(1,2,"File: %s", file_name);
  mvprintw(2,2,"Your terminal dimensions are: %d %d", COLS, TUI_ROWS);
  mvprintw(3,2,"File has %d rows",rows);
  mvprintw(4,2,"Your are on %d position in file",shift);
  mvprintw(6,2,"Press <r> to return to ANSI file", TUI_ROWS, COLS);
  mvprintw(7,2,"Press <h> to display this Help screen", TUI_ROWS, COLS);
  mvprintw(8,2,"Press <n> in ANSI file to scroll page down", TUI_ROWS, COLS);
  mvprintw(9,2,"Press <u> in ANSI file to scroll page up", TUI_ROWS, COLS);
  mvprintw(10,2,"Press <j> in ANSI file to scroll line down", TUI_ROWS, COLS);
  mvprintw(11,2,"Press <k> in ANSI file to scroll line up", TUI_ROWS, COLS);
  mvprintw(12,2,"Press <a> in ANSI file to auto-scroll", TUI_ROWS, COLS);
  mvprintw(13,2,"Press <a> in ANSI file auto-scroll to stop", TUI_ROWS, COLS);
  mvprintw(14,2,"Press <b> to bookmark Your current position", TUI_ROWS, COLS);
  mvprintw(15,2,"Press <g> go to Your last position", TUI_ROWS, COLS);
  mvprintw(16,2,"Press <i> get the sauce information", TUI_ROWS, COLS);
  mvprintw(17,2,"Press <s> go to the start of file ", TUI_ROWS, COLS);
  mvprintw(18,2,"Press <e> go to end of file", TUI_ROWS, COLS);
  mvprintw(19,2,"Press <i> for file sauce information", TUI_ROWS, COLS);

}

int read_pos_from_file() {

  int shift=0;

  FILE *f = fopen(bookmark_file_name, "r");

  if (f) {
    if (f == NULL) { ; }
    if (fscanf(f, "%d", &shift) == 1) {
    }
    fclose(f);
  } // else {
    //printw("Bookmark file doesn't exists\n");
  // }
  
  return(shift);

}

void save_sauce_size(int x, int y) {

  FILE *f = fopen(sauce_file_name, "w");

  if (f) { 
    fprintf(f, "%d,%d|",x,y);
    fclose(f);
  }

}

void save_pos_in_file(int shift) {

  FILE *f = fopen(bookmark_file_name, "w");

  if (f) { 
    fprintf(f, "%d", shift);
    fclose(f);
  } else {
    printf("Error opening bookmark file!\n");
  }

}

void keyboard_render_routines(int shift) {

  clear();
  refresh();
  if (shift+TUI_ROWS<rows) {
    print_page(shift,0);
  } else {
    print_page(shift,1);
    printf("\n\r");
  }

}


void keyboard_handler() {

  char key='r';
  int shift=0;
  int render=1;
  int esc_detected=0;
  int help_shown=0;
  int info_shown=0;

  keyboard_render_routines(shift);

  while (key!='q') {
    if (key=='j') { // key down
      if (shift<rows-TUI_ROWS-1) { shift++; render=1; }
    } else if (key=='s') {
      if (shift>0) { shift=0; render=1; }
    } else if (key=='e') {
      if (shift != rows-TUI_ROWS-1) { shift=rows-TUI_ROWS-1; render=1; }
    } else if (key=='k') { // key up
      if (shift>0) { shift--; render=1; }
    } else if (key=='n' && shift != rows-TUI_ROWS-1) {
      if (shift+TUI_ROWS<rows-TUI_ROWS) { shift+=TUI_ROWS; render=1; }
      else if (shift+TUI_ROWS<rows) { shift=rows-TUI_ROWS-1; render=1; }
      else { ; }
    } else if (key=='u' && shift != 0) {
      if (shift-TUI_ROWS>0) { shift=shift-TUI_ROWS; render=1; }
      else { shift=0; render=1;}
    } else if (key=='i') {
      if (info_shown==0) { 
        info_window(shift); 
        info_shown=1; 
      } else if (info_shown==1) { 
        render=1; 
        info_shown=0; 
      }
    } else if (key=='h') {
      if (help_shown==0) { 
        help_window(shift); 
        help_shown=1; 
      } else if (help_shown==1) { 
        render=1; 
        help_shown=0; 
      }
    } else if (key=='r') {
      render=1;
    } else if (key=='a') {
      shift = auto_scroll();
    } else if (key=='b') {
      save_pos_in_file(shift);
    } else if (key==27) { // ESC
      if (esc_detected==0) {
        esc_detected=1; 
      } 
    } else if (key=='g') {
      shift = read_pos_from_file();
      render=1;
    } else {
      render=0;
    }
    if (esc_detected==1 && key=='5' && shift != 0) { // Page Up
      if (shift-TUI_ROWS>0) { shift=shift-TUI_ROWS; render=1; }
      else { shift=0; render=1;}
      if (esc_detected==1) { esc_detected=0; }
    } else if (esc_detected==1 && key=='6' && shift != rows-TUI_ROWS-1 ) { // Page Down
      if (shift+TUI_ROWS<rows-TUI_ROWS) { shift+=TUI_ROWS; render=1; }
      else if (shift+TUI_ROWS<rows) { shift=rows-TUI_ROWS-1; render=1; }
      else { ; }
      if (esc_detected==1) { esc_detected=0; }
    } else if (esc_detected==1 && key=='B') { // key up
      if (shift<rows-TUI_ROWS-1) { shift++; render=1; }
      if (esc_detected==1) { esc_detected=0; }
    } else if (esc_detected==1 && key=='A') { // key down
      if (shift>0) { shift--; render=1; }
      if (esc_detected==1) { esc_detected=0; }
    } else if (esc_detected==1 && key=='C') { // key right
      if (shift+TUI_ROWS<rows-TUI_ROWS) { shift+=TUI_ROWS; render=1; }
      else if (shift+TUI_ROWS<rows) { shift=rows-TUI_ROWS-1; render=1; }
      else { ; }
      if (esc_detected==1) { esc_detected=0; }
    } else if (esc_detected==1 && key=='D') { // key left
      if (shift-TUI_ROWS>0) { shift=shift-TUI_ROWS; render=1; }
      else { shift=0; render=1;}
      if (esc_detected==1) { esc_detected=0; }
    }
    if (render==1) {
      keyboard_render_routines(shift);
      render=0;
    }
    key=getch();   
  }
}

void init_sauce_file_name() {
  sprintf(sauce_file_name,"%s%s.sau",file_name,user_name);
}

void init_bookmark_file_name() {
  sprintf(bookmark_file_name,"%s%s.bmk",file_name,user_name);
}

void init_console() {
  scr_reset_video_attr();
  setlocale(LC_ALL, "");
  initscr();
  nonl(); 
  noecho();
  curs_set(0);
  scrollok(stdscr,TRUE);
  //keypad(stdscr,TRUE);
}

void process_sauce(char *fname) {
  sauce_found=search_for_sauce(fname);
  if (sauce_found==-1) {
    sauce_width=80;
  } 
}

void detect_eol_type() {
  int i=0;

  for (i=1;i<COLS;i++) {
    if (buffer[i][0]=='\n') {
      if (buffer[i-1][0]=='\r') {
        eol_type=EOL_TYPE_DOS;
      } else {
        eol_type=EOL_TYPE_UNIX;
      }
    }
  }
  if (DEBUG) {
    if (eol_type==EOL_TYPE_DOS) {
      printf("ENCODING EOL TYPE DOS\n");
    } else { 
      printf("ENCODING EOL TYPE UNIX\n");
    }
  }
}

void print_usage(void) {

  printf("Usage: ansiu [-i] file.ans\n");

}

int process_arguments(int argc, char **argv) {

  int retval = 0;

/*

  if (argc==3) { // interactive file display
    printf("name argument = %s\n", argv[1]);
    printf("name argument = %s\n", argv[2]);
    file_name = argv[2];
    retval=1;
  } else if (argc==2) {
    printf("file_name= %s\n", argv[1]);
    file_name = argv[1];
    retval=0;
  } else {
    print_usage();
    retval=-1;
  }

  if (!test_file_exists(file_name)) {
    retval=-1; 
  }

*/

  int flags, opt=0;
  int nsecs, tfnd;

  nsecs = 0;
  tfnd = 0;
  flags = 0;
  const char *optstring = "i"; // interactive mode toggler

  opt = getopt(argc, argv, optstring);
  while (opt != -1) {
    switch (opt) {
      case 'i':
        flags = 1;
        retval = 1;
        break;
      default:
        printf("Usage: %s [-i] file.ans\n",argv[0]);
        retval=-1;
    }
    opt = getopt(argc, argv, optstring);
  }

  if (DEBUG) {

    printf("flags=%d; tfnd=%d; nsecs=%d; optind=%d\n",
          flags, tfnd, nsecs, optind);
  }

  if (optind >= argc && argc>=1) {
    printf("Specify file to process\n");
    retval=-1;
  }

  if (DEBUG) {
    printf("name argument = %s\n", argv[optind]);
  }

  file_name = argv[optind];
  
  return (retval);

}

int main(int argc, char **argv) {

  if (argc==1) { 
    print_usage(); 
  }

  int interactive_display=0;

  interactive_display=process_arguments(argc,argv);
  if (interactive_display==-1) { 
    exit_routine(0);
  }

  scr_reset_screen();
  init_console();
  process_sauce(file_name);
  load_ansi_file(file_name);
  detect_eol_type();
  erase_sauce_from_memory();
  init_bookmark_file_name();
  if (interactive_display==1) {
    keyboard_handler();
    deinit_console(1);
  } else if (interactive_display==0) {
    print_ansi_file();
    //deinit_console(2);
  } else {
    deinit_console(1);
  }

  deinit_console(2);

}
