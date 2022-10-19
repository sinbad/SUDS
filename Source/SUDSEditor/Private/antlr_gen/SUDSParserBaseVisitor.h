
// Generated from C:/Users/steve/projects/Games/snuka/Plugins/SUDS/Source/SUDSEditor\SUDSParser.g4 by ANTLR 4.10.1

#pragma once


#include "antlr4-runtime.h"
#include "SUDSParserVisitor.h"


/**
 * This class provides an empty implementation of SUDSParserVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  SUDSParserBaseVisitor : public SUDSParserVisitor {
public:

  virtual std::any visitExpr(SUDSParser::ExprContext *ctx) override {
    return visitChildren(ctx);
  }


};

