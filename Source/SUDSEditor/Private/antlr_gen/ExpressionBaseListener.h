
// Generated from C:/Users/steve/projects/Games/snuka/Plugins/SUDS/Source/SUDSEditor\Expression.g4 by ANTLR 4.10.1

#pragma once


#include "antlr4-runtime.h"
#include "ExpressionListener.h"


/**
 * This class provides an empty implementation of ExpressionListener,
 * which can be extended to create a listener which only needs to handle a subset
 * of the available methods.
 */
class  ExpressionBaseListener : public ExpressionListener {
public:

  virtual void enterExpr(ExpressionParser::ExprContext * /*ctx*/) override { }
  virtual void exitExpr(ExpressionParser::ExprContext * /*ctx*/) override { }


  virtual void enterEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void exitEveryRule(antlr4::ParserRuleContext * /*ctx*/) override { }
  virtual void visitTerminal(antlr4::tree::TerminalNode * /*node*/) override { }
  virtual void visitErrorNode(antlr4::tree::ErrorNode * /*node*/) override { }

};

