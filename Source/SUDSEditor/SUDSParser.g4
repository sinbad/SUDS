parser grammar SUDSParser;

options { tokenVocab=SUDSLexer; }

// Parser rule
expr    : expr OPERATOR expr | LPARENS expr RPARENS | INT | ID;
