parser grammar SUDSParser;

options { tokenVocab=SUDSLexer; }

// Parser rule
lines: (line | indentedline)+ EOF;
line : BOL expr;
indentedline : BOLINDENT expr;
expr    : expr OPERATOR expr | LPARENS expr RPARENS | INT | ID;
