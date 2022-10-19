
// Generated from C:/Users/steve/projects/Games/snuka/Plugins/SUDS/Source/SUDSEditor\SUDSParser.g4 by ANTLR 4.10.1

#pragma once


#include "antlr4-runtime.h"
#include "SUDSParserListener.h"


/**
 * This class provides an empty implementation of SUDSParserListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  SUDSParserBaseListener : public SUDSParserListener {
public:

  virtual void enterExpr(SUDSParser::ExprContext * /*ctx*/) override { }
  virtual void exitExpr(SUDSParser::ExprContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

