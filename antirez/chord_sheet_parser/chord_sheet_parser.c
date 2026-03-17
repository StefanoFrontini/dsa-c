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

*/



/* CHORD SHEET GRAMMAR

- Lexical elements (terminal elements or tokens):

    symbol: '[' | ']' | '\n' | '\0'

    stringConstant: A sequence of letters, digits


- Syntax (non terminal elements):

    song: line*

    line: word* (endOfLine | endOfFile)

    word: (chord)? (lyric)?

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

/* DATA STRUCTURES LEXER
*/

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum { SYMBOL = 0, STR} TokenType;

typedef struct Token {
    int refcount;
    TokenType type;
    union {
       char symbol;
       struct {
          char *ptr;
          size_t len;
       } str;
    //    struct Token {
    //       Token **ele;
    //       size_t len;
    //    } list;
    };
} Token;

typedef struct Lexer {
    char *prg; // Program to parse
    char *p; // Next token to parse
    Token *curToken;

} Lexer;

/*  Allocation wrappers */

void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    fprintf(stderr, "Out of memory allocation %zu bytes\n", size);
    exit(1);
  }
  return ptr;
}

int isLyric(Lexer *l){
    return isalnum(l->p[0]) || isblank(l->p[0]);
    // return isascii(l->p[0]);
}


void readLyric(Lexer *l){
    char *start = l->p;

        printf("start: %c\n", l->p[0]);
    while(isLyric(l)){
        l->p++;
    }
    char *end = l->p;
    size_t len = end - start;

        printf("end: %c\n", l->p[0]);
        printf("len: %ld\n", len);

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

void advanceLexer(Lexer *l){
      if(isalnum(l->p[0])){
        readLyric(l);
      }
}

void printToken(Token *t){
    switch(t->type){
        case STR:
           printf("Current token is: %s\n", t->str.ptr);
           break;
        default:
        break;
    }
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
    printToken(l.curToken);
    return 0;
}