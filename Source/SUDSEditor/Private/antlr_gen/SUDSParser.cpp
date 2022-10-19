
// Generated from C:/Users/steve/projects/Games/snuka/Plugins/SUDS/Source/SUDSEditor\SUDSParser.g4 by ANTLR 4.10.1


#include "SUDSParserListener.h"
#include "SUDSParserVisitor.h"

#include "SUDSParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct SUDSParserStaticData final {
  SUDSParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  SUDSParserStaticData(const SUDSParserStaticData&) = delete;
  SUDSParserStaticData(SUDSParserStaticData&&) = delete;
  SUDSParserStaticData& operator=(const SUDSParserStaticData&) = delete;
  SUDSParserStaticData& operator=(SUDSParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

std::once_flag sudsparserParserOnceFlag;
SUDSParserStaticData *sudsparserParserStaticData = nullptr;

void sudsparserParserInitialize() {
  assert(sudsparserParserStaticData == nullptr);
  auto staticData = std::make_unique<SUDSParserStaticData>(
    std::vector<std::string>{
      "expr"
    },
    std::vector<std::string>{
      "", "", "", "", "", "'('", "')'"
    },
    std::vector<std::string>{
      "", "INT", "ID", "WS", "OPERATOR", "LPARENS", "RPARENS"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,6,20,2,0,7,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,3,0,10,8,0,1,0,1,0,1,0,5,
  	0,15,8,0,10,0,12,0,18,9,0,1,0,0,1,0,1,0,0,0,21,0,9,1,0,0,0,2,3,6,0,-1,
  	0,3,4,5,5,0,0,4,5,3,0,0,0,5,6,5,6,0,0,6,10,1,0,0,0,7,10,5,1,0,0,8,10,
  	5,2,0,0,9,2,1,0,0,0,9,7,1,0,0,0,9,8,1,0,0,0,10,16,1,0,0,0,11,12,10,4,
  	0,0,12,13,5,4,0,0,13,15,3,0,0,5,14,11,1,0,0,0,15,18,1,0,0,0,16,14,1,0,
  	0,0,16,17,1,0,0,0,17,1,1,0,0,0,18,16,1,0,0,0,2,9,16
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  sudsparserParserStaticData = staticData.release();
}

}

SUDSParser::SUDSParser(TokenStream *input) : SUDSParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

SUDSParser::SUDSParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  SUDSParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *sudsparserParserStaticData->atn, sudsparserParserStaticData->decisionToDFA, sudsparserParserStaticData->sharedContextCache, options);
}

SUDSParser::~SUDSParser() {
  delete _interpreter;
}

const atn::ATN& SUDSParser::getATN() const {
  return *sudsparserParserStaticData->atn;
}

std::string SUDSParser::getGrammarFileName() const {
  return "SUDSParser.g4";
}

const std::vector<std::string>& SUDSParser::getRuleNames() const {
  return sudsparserParserStaticData->ruleNames;
}

const dfa::Vocabulary& SUDSParser::getVocabulary() const {
  return sudsparserParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView SUDSParser::getSerializedATN() const {
  return sudsparserParserStaticData->serializedATN;
}


//----------------- ExprContext ------------------------------------------------------------------

SUDSParser::ExprContext::ExprContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SUDSParser::ExprContext::LPARENS() {
  return getToken(SUDSParser::LPARENS, 0);
}

std::vector<SUDSParser::ExprContext *> SUDSParser::ExprContext::expr() {
  return getRuleContexts<SUDSParser::ExprContext>();
}

SUDSParser::ExprContext* SUDSParser::ExprContext::expr(size_t i) {
  return getRuleContext<SUDSParser::ExprContext>(i);
}

tree::TerminalNode* SUDSParser::ExprContext::RPARENS() {
  return getToken(SUDSParser::RPARENS, 0);
}

tree::TerminalNode* SUDSParser::ExprContext::INT() {
  return getToken(SUDSParser::INT, 0);
}

tree::TerminalNode* SUDSParser::ExprContext::ID() {
  return getToken(SUDSParser::ID, 0);
}

tree::TerminalNode* SUDSParser::ExprContext::OPERATOR() {
  return getToken(SUDSParser::OPERATOR, 0);
}


size_t SUDSParser::ExprContext::getRuleIndex() const {
  return SUDSParser::RuleExpr;
}

void SUDSParser::ExprContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SUDSParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpr(this);
}

void SUDSParser::ExprContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SUDSParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpr(this);
}


std::any SUDSParser::ExprContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SUDSParserVisitor*>(visitor))
    return parserVisitor->visitExpr(this);
  else
    return visitor->visitChildren(this);
}


SUDSParser::ExprContext* SUDSParser::expr() {
   return expr(0);
}

SUDSParser::ExprContext* SUDSParser::expr(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  SUDSParser::ExprContext *_localctx = _tracker.createInstance<ExprContext>(_ctx, parentState);
  SUDSParser::ExprContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 0;
  enterRecursionRule(_localctx, 0, SUDSParser::RuleExpr, precedence);

    

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(9);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SUDSParser::LPARENS: {
        setState(3);
        match(SUDSParser::LPARENS);
        setState(4);
        expr(0);
        setState(5);
        match(SUDSParser::RPARENS);
        break;
      }

      case SUDSParser::INT: {
        setState(7);
        match(SUDSParser::INT);
        break;
      }

      case SUDSParser::ID: {
        setState(8);
        match(SUDSParser::ID);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    _ctx->stop = _input->LT(-1);
    setState(16);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        _localctx = _tracker.createInstance<ExprContext>(parentContext, parentState);
        pushNewRecursionContext(_localctx, startState, RuleExpr);
        setState(11);

        if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
        setState(12);
        match(SUDSParser::OPERATOR);
        setState(13);
        expr(5); 
      }
      setState(18);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

bool SUDSParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 0: return exprSempred(antlrcpp::downCast<ExprContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool SUDSParser::exprSempred(ExprContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 4);

  default:
    break;
  }
  return true;
}

void SUDSParser::initialize() {
  std::call_once(sudsparserParserOnceFlag, sudsparserParserInitialize);
}
