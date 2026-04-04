
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ------------------------ Allocation wrappers ----------------------------*/

void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    fprintf(stderr, "Out of memory allocation %zu bytes\n", size);
    exit(1);
  }
  return ptr;
}

void *xrealloc(void *old_ptr, size_t size) {
  void *ptr = realloc(old_ptr, size);
  if (ptr == NULL) {
    fprintf(stderr, "Out of memory allocation %zu bytes\n", size);
    exit(1);
  }
  return ptr;
}
/* ----------------   Data structures: Lexer ------------------ */

typedef enum {LYRIC = 0, CHORD, ENDOFLINE, ENDOFFILE, ILLEGAL } TokenType;

typedef struct Token {
    int refcount;
    TokenType type;
    union {
       char symbol;
       struct {
          char *ptr;
          size_t len;
       } str;
    };
} Token;

typedef struct Lexer {
    char *prg; // Program to parse
    char *p; // Next token to parse
    Token *curToken;

} Lexer;

/* ------------------  Token related functions ----------------  */

void retainToken(Token *t){
    t->refcount++;
}
void releaseToken(Token *t){
    assert(t->refcount > 0);
    t->refcount--;
    if(t->refcount == 0){
        switch(t->type){
            case LYRIC:
            case CHORD:
              free(t->str.ptr);
              break;
            default:
              break;
        }
        free(t);
    }
}

/* Return true if the character 'c' is one of the characters acceptable for the
 * lyric token. */
int isStringConstant(Lexer *l){
    char c = l->p[0];
    char symbols_char[] = "[]\n\r";
    return isascii(c) && strchr(symbols_char, c) == NULL;
}


void readLyric(Lexer *l){
    char *start = l->p;
    while(isStringConstant(l)){
        l->p++;
    }
    char *end = l->p;
    size_t len = end - start;

    Token *t = xmalloc(sizeof(Token));
    char *str = xmalloc(len + 1);
    t->refcount = 1;
    t->type = LYRIC;
    t->str.ptr = str;
    t->str.len = len;
    memcpy(t->str.ptr, start, len);
    t->str.ptr[len] = 0;
    l->curToken = t;
}

int isBeginningChord(Lexer *l){
    return l->p[0] == '[';
}

void readChord(Lexer *l){
    l->p++;
    if(!isStringConstant(l)){
        fprintf(stderr, "No string after open paren\n");
        exit(1);
    }
    char *start = l->p;
    while(isStringConstant(l)){
        l->p++;
    }
    char *end = l->p;
    size_t len = end - start;
    if(l->p[0] != ']'){
        printf("char: %c\n", l->p[0]);
        fprintf(stderr, "Unmatched close paren\n");
        exit(1);
    }
    l->p++;

    Token *t = xmalloc(sizeof(Token));
    char *str = xmalloc(len + 1);
    t->refcount = 1;
    t->type = CHORD;
    t->str.ptr = str;
    t->str.len = len;
    memcpy(t->str.ptr, start, len);
    t->str.ptr[len] = 0;
    l->curToken = t;

}

void readEndOfLine(Lexer *l, char s){
    Token *t = xmalloc(sizeof(Token));
    t->refcount = 1;
    t->type = ENDOFLINE;
    t->symbol = s;
    l->curToken = t;
    l->p++;
}
void readIllegalChar(Lexer *l, char s){
    Token *t = xmalloc(sizeof(Token));
    t->refcount = 1;
    t->type = ILLEGAL;
    t->symbol = s;
    l->curToken = t;
    l->p++;
}

void readEndOfFile(Lexer *l){
    Token *t = xmalloc(sizeof(Token));
    t->refcount = 1;
    t->type = ENDOFFILE;
    t->symbol = 0;
    l->curToken = t;
}

void advanceLexer(Lexer *l){
      releaseToken(l->curToken);
      char c = l->p[0];
      if(isStringConstant(l)){
        readLyric(l);
      } else if(isBeginningChord(l)){
        readChord(l);
      } else if(c == '\n' || c == '\r'){
        readEndOfLine(l, c);
      }
      else if(c == 0) {
        readEndOfFile(l);
      } else {
        readIllegalChar(l, c);
      }
}

void printToken(Token *t){
    switch(t->type){
        case LYRIC:
        case CHORD:
           printf("Current token is: %s\n", t->str.ptr);
           break;
        case ILLEGAL:
           printf("Current token is: %c\n", t->symbol);
           break;
        case ENDOFLINE:
           printf("Current token is: 'newLine'\n");
           break;

        default:
        break;
    }
}

/* -------------------------  Main ----------------------------- */

int main(int argc, char **argv){
    if(argc !=2){
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");

  if (fp == NULL) {
    fprintf(stderr, "Unable to open the file\n");
    return 1;
  }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = xmalloc(file_size + 1);
    fread(buf, file_size, 1, fp);
    buf[file_size] = 0;
    printf("text is: %s\n", buf);
    fclose(fp);
    Lexer l;
    l.prg = buf;
    l.p = buf;
    readEndOfFile(&l);
    advanceLexer(&l);
    while(l.curToken->type != ENDOFFILE){
        printToken(l.curToken);
        advanceLexer(&l);
    }
    free(buf);
    return 0;
}

