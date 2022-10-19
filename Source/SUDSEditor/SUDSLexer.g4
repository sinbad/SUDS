lexer grammar SUDSLexer;

channels {
    WHITESPACE
}

// Lexer rules
INT         : [0-9]+;
ID          : [a-zA-Z][a-zA-Z0-9]+;
BOL         : ('\r'? '\n' | '\r')+ ;
BOLINDENT   : BOL [\t ]+;
WS          : [ \t]+ -> channel(WHITESPACE);

OPERATOR    : [-+*/];
LPARENS     : '(';
RPARENS     : ')';
