
// Generated from C:/Users/steve/projects/Games/snuka/Plugins/SUDS/Source/SUDSEditor\Expression.g4 by ANTLR 4.10.1

#pragma once


#include "antlr4-runtime.h"
#include "ExpressionVisitor.h"


/**
 * This class provides an empty implementation of ExpressionVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  ExpressionBaseVisitor : public ExpressionVisitor {
public:

  virtual std::any visitExpr(ExpressionParser::ExprContext *ctx) override {
    return visitChildren(ctx);
  }


};

