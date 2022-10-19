
// Generated from C:/Users/steve/projects/Games/snuka/Plugins/SUDS/Source/SUDSEditor\SUDSParser.g4 by ANTLR 4.10.1

#pragma once


#include "antlr4-runtime.h"




class  SUDSParser : public antlr4::Parser {
public:
  enum {
    INT = 1, ID = 2, WS = 3, OPERATOR = 4, LPARENS = 5, RPARENS = 6
  };

  enum {
    RuleExpr = 0
  };

  explicit SUDSParser(antlr4::TokenStream *input);

  SUDSParser(antlr4::TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options);

  ~SUDSParser() override;

  std::string getGrammarFileName() const override;

  const antlr4::atn::ATN& getATN() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;


  class ExprContext; 

  class  ExprContext : public antlr4::ParserRuleContext {
  public:
    ExprContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LPARENS();
    std::vector<ExprContext *> expr();
    ExprContext* expr(size_t i);
    antlr4::tree::TerminalNode *RPARENS();
    antlr4::tree::TerminalNode *INT();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *OPERATOR();

    virtual void enterRule(antlr4::tree::ParseTreeListener *listener) override;
    virtual void exitRule(antlr4::tree::ParseTreeListener *listener) override;

    virtual std::any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExprContext* expr();
  ExprContext* expr(int precedence);

  bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

  bool exprSempred(ExprContext *_localctx, size_t predicateIndex);

  // By default the static state used to implement the parser is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:
};

