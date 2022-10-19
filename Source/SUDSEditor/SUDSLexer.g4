lexer grammar SUDSLexer;

channels {
    WHITESPACE
}

// Lexer rules
INT     : [0-9]+;
ID      : [a-z]+;
WS      : [ \t\r\n]+ -> channel(WHITESPACE);

OPERATOR : [-+*/];
LPARENS  : '(';
RPARENS  : ')';
