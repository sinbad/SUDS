
// Generated from C:/Users/steve/projects/Games/snuka/Plugins/SUDS/Source/SUDSEditor\SUDSLexer.g4 by ANTLR 4.10.1

#pragma once


#include "antlr4-runtime.h"




class  SUDSLexer : public antlr4::Lexer {
public:
  enum {
    INT = 1, ID = 2, WS = 3, OPERATOR = 4, LPARENS = 5, RPARENS = 6
  };

  enum {
    WHITESPACE = 2
  };

  explicit SUDSLexer(antlr4::CharStream *input);

  ~SUDSLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

