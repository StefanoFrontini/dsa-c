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