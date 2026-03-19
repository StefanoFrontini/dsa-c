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

    line: word+ (endOfLine | endOfFile)

    word: chord lyric

    chord: '[' stringConstant ']'

    lyric: stringConstant

    endOfLine: '\n'

    endOfFile: '\0'


*/

/* Lexer

Advancing the input, one token at a time
Getting the value and type of the current token

hasMoreTokens() -> true or false
advance() -> gets the next token from the input and makes it the current token.
tokenType() -> returns: symbol or stringConstant
symbol() -> returns the value of the token symbol
stringVal() -> return the value of the token stringConstant

*/

/* Parser


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

typedef enum {STR = 0, OPENPAREN, CLOSEPAREN, ENDOFLINE } TokenType;

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
    // assert(t->refcount > 0);
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

/* Return true if the character 'c' is one of the characters acceptable for our
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


void advanceLexer(Lexer *l){
    if(l->curToken != NULL){
      releaseToken(l->curToken);
    }
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
        l->curToken = NULL;
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
CsObj *parseChord(Lexer *l){
  advanceLexer(l);

  if (l->curToken == NULL || l->curToken->type == ENDOFLINE || l->curToken->type == OPENPAREN) {
    fprintf(stderr, "Error parsing chord symbol\n");
    exit(1);
  } else if (l->curToken->type == CLOSEPAREN){
    fprintf(stderr, "Error: chord symbol cannot be empty\n");
    exit(1);
  } else {
    CsObj *o = createChordObject(l->curToken->str.ptr, l->curToken->str.len);
    advanceLexer(l);
    if(l->curToken->type != CLOSEPAREN){
        fprintf(stderr, "Error parsing chord symbol: unmatched closing bracket\n");
        // TO DO RELEASE
        exit(1);
    }
    advanceLexer(l);
    return o;
  }

}

CsObj *parseLyric(Lexer *l){
    CsObj *o = createLyricObject(l->curToken->str.ptr, l->curToken->str.len);
    advanceLexer(l);
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
        CsObj *parsedChord = parseChord(l);
        listPush(o, parsedChord);
        CsObj *parsedLyric = NULL;
        if(l->curToken->type == STR){
            parsedLyric = parseLyric(l);
        } else {
            parsedLyric = createLyricObject(NULL, 0);
        }
        listPush(o, parsedLyric);

    } else if(l->curToken->type == STR) {
        listPush(o, createChordObject(NULL, 0));
        listPush(o, parseLyric(l));
    } else {
        listPush(o, createChordObject(NULL, 0));
        listPush(o, createLyricObject(NULL, 0));
    }
    // advanceLexer(l);
    return o;
}
CsObj *parseLine(Lexer *l){
    CsObj *o = createLineObject();
    while(l->curToken && l->curToken->type != ENDOFLINE){
        listPush(o, parseWord(l));
        // advanceLexer(l);
    }
    return o;
}
CsObj *parseSong(Lexer *l){
    CsObj *o = createSongObject();
    while(l->curToken){
        listPush(o, parseLine(l));
        // advanceLexer(l);
    }
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
    l.curToken = NULL;
    advanceLexer(&l);
    CsObj *parsed = parseSong(&l);
    printCsObj(parsed);
    return 0;
}