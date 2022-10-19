
// Generated from C:/Users/steve/projects/Games/snuka/Plugins/SUDS/Source/SUDSEditor\SUDSParser.g4 by ANTLR 4.10.1

#pragma once


#include "antlr4-runtime.h"
#include "SUDSParser.h"


/**
 * This interface defines an abstract listener for a parse tree produced by SUDSParser.
 */
class  SUDSParserListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterExpr(SUDSParser::ExprContext *ctx) = 0;
  virtual void exitExpr(SUDSParser::ExprContext *ctx) = 0;


};

