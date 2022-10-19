
// Generated from C:/Users/steve/projects/Games/snuka/Plugins/SUDS/Source/SUDSEditor\SUDSParser.g4 by ANTLR 4.10.1

#pragma once


#include "antlr4-runtime.h"
#include "SUDSParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by SUDSParser.
 */
class  SUDSParserVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by SUDSParser.
   */
    virtual std::any visitExpr(SUDSParser::ExprContext *context) = 0;


};

