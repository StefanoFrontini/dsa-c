/*

Now I've [C]heard there was a [Am]

....


{
  type: "song";
  value:  [{
            type: "line";
            value: [{
                    type: "word";
                    value: [{
                            type: "Chord";
                            value: ""
                            }, {
                            type: "Lyric";
                            value: "Now I've "
                            }]
                    }, {
                    type: "word";
                    value: [{
                                type: "Chord";
                                value: "C";
                                },
                                {
                                type: "Lyric";
                                value:"heard there was a"
                            }
                          ]
            }]

}

*/

/*

GRAMMAR NOTATION (taken fron Nand To Tetris course):

'xxx' : used to list language tokens that appear verbatim ("terminals");

xxx   : regular typeface represents names of non-terminals;

()    : used for grouping

x | y : indicates that either x or y appear;

x  y  : indicates that x appears and then y appears;

x?    : indicates that x appears 0 or 1 times;

x*    : indicates that x appears 0 or more times;

x+    : indicates that x appears 1 or more times;

*/



/* CHORD SHEET GRAMMAR

- Lexical elements (terminal elements or tokens):

    symbol: '[' | ']' | '\n' | '\r' | '\0'

    stringConstant: A sequence of ascii chars, not including symbols


- Syntax (non terminal elements):

    song: line+

    line: word+ ('\n' | '\r' | '\0')

    word: chord lyric

    chord: '[' stringConstant ']'

    lyric: stringConstant


*/

/* Lexer

Advancing the input, one token at a time
Getting the value and type of the current token

advanceLexer() -> reads the next token from the input and makes it the current token.

*/

/* Parsing functions

The parsing function reads the tokens in order to build the object specified by the grammar rule.

One parsing function for each grammar rule.

*/


#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* DATA STRUCTURES OBJECT */

typedef enum {SONG = 0, LINE, WORD, CHORD, LYRIC} CsObjType;

typedef struct CsObj{
    int refcount;
    CsObjType type;
    union {
        struct {
            struct CsObj **ele;
            size_t len;
        } list;
        struct {
            char *ptr;
            size_t len;

        } str;
    };
} CsObj;

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


/* Object related functions */

CsObj *createObject(CsObjType type){
    CsObj *o = xmalloc(sizeof(CsObj));
    o->refcount = 1;
    o->type = type;
    return o;
}

CsObj *createSongObject(void){
    CsObj *o = createObject(SONG);
    o->list.ele = NULL;
    o->list.len = 0;
    return o;
}

CsObj *createLineObject(void){
    CsObj *o = createObject(LINE);
    o->list.ele = NULL;
    o->list.len = 0;
    return o;
}

CsObj *createWordObject(void){
    CsObj *o = createObject(WORD);
    o->list.ele = NULL;
    o->list.len = 0;
    return o;
}

CsObj *createChordObject(char *str, size_t len){
    CsObj *o = createObject(CHORD);
    if(len > 0){
        o->str.ptr = xmalloc(len + 1);
        memcpy(o->str.ptr, str, len);
        o->str.ptr[len] = 0;
    } else {
        o->str.ptr = str;
    }
        o->str.len = len;
    return o;
}

CsObj *createLyricObject(char *str, size_t len){
    CsObj *o = createObject(LYRIC);
    if(len > 0){
        o->str.ptr = xmalloc(len + 1);
        memcpy(o->str.ptr, str, len);
        o->str.ptr[len] = 0;
    } else {
        o->str.ptr = str;
    }
    o->str.len = len;
    return o;
}
/* DATA STRUCTURES LEXER */

typedef enum {STR = 0, OPENPAREN, CLOSEPAREN, ENDOFLINE, ENDOFFILE } TokenType;

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

/* ref counting */
void retainToken(Token *t){
    t->refcount++;
}
void releaseToken(Token *t){
    assert(t->refcount > 0);
    t->refcount--;
    if(t->refcount == 0){
        switch(t->type){
            case STR:
              free(t->str.ptr);
              break;
            default:
              break;
        }
        free(t);
    }
}

void retainCsObj(CsObj *o){
    o->refcount++;
}


void releaseCsObj(CsObj *o){
    assert(o->refcount > 0);
    o->refcount--;
    if(o->refcount == 0){
        switch(o->type){
            case SONG:
            case LINE:
            case WORD:
               for(size_t i = 0; i < o->list.len; i++){
                  releaseCsObj(o->list.ele[i]);
               }
               break;
            case CHORD:
            case LYRIC:
              free(o->str.ptr);
              break;
            default:
              break;
        }
        free(o);
    }
}

/* Return true if the character 'c' is one of the characters acceptable for the
 * stringConstant token. */
int isStringConstant(Lexer *l){
    char c = l->p[0];
    char symbols_char[] = "[]\n\r";
    return isascii(c) && strchr(symbols_char, c) == NULL;
}


void readStringConstant(Lexer *l){
    char *start = l->p;
    while(isStringConstant(l)){
        l->p++;
    }
    char *end = l->p;
    size_t len = end - start;

    Token *t = xmalloc(sizeof(Token));
    char *str = xmalloc(len + 1);
    t->refcount = 1;
    t->type = STR;
    t->str.ptr = str;
    t->str.len = len;
    memcpy(t->str.ptr, start, len);
    t->str.ptr[len] = 0;
    l->curToken = t;
}

void readOpenParen(Lexer *l){
    Token *t = xmalloc(sizeof(Token));
    t->refcount = 1;
    t->type = OPENPAREN;
    t->symbol = '[';
    l->curToken = t;
    l->p++;

}

void readCloseParen(Lexer *l){
    Token *t = xmalloc(sizeof(Token));
    t->refcount = 1;
    t->type = CLOSEPAREN;
    t->symbol = ']';
    l->curToken = t;
    l->p++;
}

void readEndOfLine(Lexer *l, char s){
    Token *t = xmalloc(sizeof(Token));
    t->refcount = 1;
    t->type = ENDOFLINE;
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
      if(isalnum(l->p[0])){
        readStringConstant(l);
      } else if(l->p[0] == '['){
        readOpenParen(l);
      } else if(l->p[0] == ']'){
        readCloseParen(l);
      } else if(l->p[0] == '\n' || l->p[0] == '\r'){
        readEndOfLine(l, l->p[0]);
      }
      else {
        readEndOfFile(l);
      }
}

void printToken(Token *t){
    switch(t->type){
        case STR:
           printf("Current token is: %s\n", t->str.ptr);
           break;
        case OPENPAREN:
        case CLOSEPAREN:
           printf("Current token is: %c\n", t->symbol);
           break;
        case ENDOFLINE:
           printf("Current token is: 'newLine'\n");
           break;

        default:
        break;
    }
}
void printCsObj(CsObj *o){
    switch(o->type){
        case SONG:
        case LINE:
        case WORD:
          for(size_t i = 0; i < o->list.len; i++){
            if(o->list.len == 0){
                printf("");
            } else {
                printCsObj(o->list.ele[i]);
            }
          }
          if(o->type == LINE){
            printf("\n");
          }
          break;
        case CHORD:
           if(o->str.len == 0){
            printf("");
           } else {
            printf("[%s]", o->str.ptr);
           }
           break;
        case LYRIC:
           if(o->str.len == 0){
            printf("");
           } else {
           printf("%s", o->str.ptr);
           }
           break;
        default:
           break;
    }

}

void eatToken(Lexer *l, TokenType t){
    if(l->curToken->type == ENDOFFILE || l->curToken->type != t){
        fprintf(stderr, "Error cannot eat token: %d\n", t);
        fprintf(stderr, "curToken was %d\n", l->curToken->type);
        exit(1);
    }
    advanceLexer(l);

}
CsObj *parseChord(Lexer *l){
   eatToken(l, OPENPAREN);
   char *str_ptr = l->curToken->str.ptr;
   size_t len = l->curToken->str.len;
   eatToken(l, STR);
   eatToken(l, CLOSEPAREN);
   CsObj *o = createChordObject(str_ptr, len);
   return o;
  }


CsObj *parseLyric(Lexer *l){
    CsObj *o = createLyricObject(l->curToken->str.ptr, l->curToken->str.len);
    eatToken(l, STR);
    return o;
}
void listPush(CsObj *l, CsObj *ele){
   l->list.ele = xrealloc(l->list.ele, sizeof(CsObj *) * (l->list.len + 1));
   l->list.ele[l->list.len] = ele;
   l->list.len++;

}

CsObj *parseWord(Lexer *l){
    CsObj *o = createWordObject();
    if(l->curToken->type == OPENPAREN){
        listPush(o, parseChord(l));

        if(l->curToken->type == ENDOFFILE || l->curToken->type == ENDOFLINE || l->curToken->type == OPENPAREN || l->curToken->type == CLOSEPAREN ){
            listPush(o, createLyricObject(NULL, 0));
        }

        else if(l->curToken->type == STR){
            listPush(o, parseLyric(l));
        }

    } else if(l->curToken->type == STR) {
        listPush(o, createChordObject(NULL, 0));
        listPush(o, parseLyric(l));
    } else {
        listPush(o, createChordObject(NULL, 0));
        listPush(o, createLyricObject(NULL, 0));
    }
    return o;
}
CsObj *parseLine(Lexer *l){
    CsObj *o = createLineObject();
    while(l->curToken->type != ENDOFFILE && l->curToken->type != ENDOFLINE){
        listPush(o, parseWord(l));
    }
    if(l->curToken->type == ENDOFLINE){
        eatToken(l, ENDOFLINE);
    }
    return o;
}
CsObj *parseSong(Lexer *l){
    CsObj *o = createSongObject();
    while(l->curToken->type != ENDOFFILE){
        listPush(o, parseLine(l));
    }
    releaseToken(l->curToken);
    return o;
}


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
    CsObj *parsed = parseSong(&l);
    printCsObj(parsed);
    return 0;
}