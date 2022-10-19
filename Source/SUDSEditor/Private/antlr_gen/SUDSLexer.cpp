
// Generated from C:/Users/steve/projects/Games/snuka/Plugins/SUDS/Source/SUDSEditor\SUDSLexer.g4 by ANTLR 4.10.1


#include "SUDSLexer.h"


using namespace antlr4;



using namespace antlr4;

namespace {

struct SUDSLexerStaticData final {
  SUDSLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  SUDSLexerStaticData(const SUDSLexerStaticData&) = delete;
  SUDSLexerStaticData(SUDSLexerStaticData&&) = delete;
  SUDSLexerStaticData& operator=(const SUDSLexerStaticData&) = delete;
  SUDSLexerStaticData& operator=(SUDSLexerStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> channelNames;
  const std::vector<std::string> modeNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

std::once_flag sudslexerLexerOnceFlag;
SUDSLexerStaticData *sudslexerLexerStaticData = nullptr;

void sudslexerLexerInitialize() {
  assert(sudslexerLexerStaticData == nullptr);
  auto staticData = std::make_unique<SUDSLexerStaticData>(
    std::vector<std::string>{
      "INT", "ID", "BOL", "BOLINDENT", "WS", "OPERATOR", "LPARENS", "RPARENS"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "'('", "')'"
    },
    std::vector<std::string>{
      "", "INT", "ID", "BOL", "BOLINDENT", "WS", "OPERATOR", "LPARENS", 
      "RPARENS"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,0,8,56,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,
  	2,7,7,7,1,0,4,0,19,8,0,11,0,12,0,20,1,1,1,1,4,1,25,8,1,11,1,12,1,26,1,
  	2,3,2,30,8,2,1,2,1,2,4,2,34,8,2,11,2,12,2,35,1,3,1,3,4,3,40,8,3,11,3,
  	12,3,41,1,4,4,4,45,8,4,11,4,12,4,46,1,4,1,4,1,5,1,5,1,6,1,6,1,7,1,7,0,
  	0,8,1,1,3,2,5,3,7,4,9,5,11,6,13,7,15,8,1,0,5,1,0,48,57,2,0,65,90,97,122,
  	3,0,48,57,65,90,97,122,2,0,9,9,32,32,3,0,42,43,45,45,47,47,62,0,1,1,0,
  	0,0,0,3,1,0,0,0,0,5,1,0,0,0,0,7,1,0,0,0,0,9,1,0,0,0,0,11,1,0,0,0,0,13,
  	1,0,0,0,0,15,1,0,0,0,1,18,1,0,0,0,3,22,1,0,0,0,5,33,1,0,0,0,7,37,1,0,
  	0,0,9,44,1,0,0,0,11,50,1,0,0,0,13,52,1,0,0,0,15,54,1,0,0,0,17,19,7,0,
  	0,0,18,17,1,0,0,0,19,20,1,0,0,0,20,18,1,0,0,0,20,21,1,0,0,0,21,2,1,0,
  	0,0,22,24,7,1,0,0,23,25,7,2,0,0,24,23,1,0,0,0,25,26,1,0,0,0,26,24,1,0,
  	0,0,26,27,1,0,0,0,27,4,1,0,0,0,28,30,5,13,0,0,29,28,1,0,0,0,29,30,1,0,
  	0,0,30,31,1,0,0,0,31,34,5,10,0,0,32,34,5,13,0,0,33,29,1,0,0,0,33,32,1,
  	0,0,0,34,35,1,0,0,0,35,33,1,0,0,0,35,36,1,0,0,0,36,6,1,0,0,0,37,39,3,
  	5,2,0,38,40,7,3,0,0,39,38,1,0,0,0,40,41,1,0,0,0,41,39,1,0,0,0,41,42,1,
  	0,0,0,42,8,1,0,0,0,43,45,7,3,0,0,44,43,1,0,0,0,45,46,1,0,0,0,46,44,1,
  	0,0,0,46,47,1,0,0,0,47,48,1,0,0,0,48,49,6,4,0,0,49,10,1,0,0,0,50,51,7,
  	4,0,0,51,12,1,0,0,0,52,53,5,40,0,0,53,14,1,0,0,0,54,55,5,41,0,0,55,16,
  	1,0,0,0,8,0,20,26,29,33,35,41,46,1,0,2,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  sudslexerLexerStaticData = staticData.release();
}

}

SUDSLexer::SUDSLexer(CharStream *input) : Lexer(input) {
  SUDSLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *sudslexerLexerStaticData->atn, sudslexerLexerStaticData->decisionToDFA, sudslexerLexerStaticData->sharedContextCache);
}

SUDSLexer::~SUDSLexer() {
  delete _interpreter;
}

std::string SUDSLexer::getGrammarFileName() const {
  return "SUDSLexer.g4";
}

const std::vector<std::string>& SUDSLexer::getRuleNames() const {
  return sudslexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& SUDSLexer::getChannelNames() const {
  return sudslexerLexerStaticData->channelNames;
}

const std::vector<std::string>& SUDSLexer::getModeNames() const {
  return sudslexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& SUDSLexer::getVocabulary() const {
  return sudslexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView SUDSLexer::getSerializedATN() const {
  return sudslexerLexerStaticData->serializedATN;
}

const atn::ATN& SUDSLexer::getATN() const {
  return *sudslexerLexerStaticData->atn;
}




void SUDSLexer::initialize() {
  std::call_once(sudslexerLexerOnceFlag, sudslexerLexerInitialize);
}
