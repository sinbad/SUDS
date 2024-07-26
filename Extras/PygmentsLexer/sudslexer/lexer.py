from pygments.lexer import RegexLexer, bygroups
from pygments.token import *

class SudsLexer(RegexLexer):
    name = 'SUDS'
    aliases = ['suds']
    filenames = ['*.sud']

    tokens = {
        'root': [
            (r'===\s*\n', Generic.Heading),
            # Choices
            (r'\s*\*\-?\s+[^\@\n]+[\@\n]', Generic.Subheading),
            (r'\s*#.*\n', Comment),
            # Speaker lines
            (r'\s*\S+\:', Name.Class, 'speakerline'),
            # Open brackets for special functions
            (r'\[', Operator, 'squarebrackets'),       

            # Variables
            (r'(\{)([\w\.]+)(\})', bygroups(Operator, Name.Variable, Operator)),
            (r'(\|)(plural|gender)(\()(.*?)(\))', bygroups(Operator, Keyword, Operator, Keyword, Operator)),
            (r'\b([tT]rue|[fF]alse|[mM]asculine|[fF]eminine|[nN]euter)\b', Name.Constant),
            # Line IDs
            (r'\@[0-9a-fA-F]+\@', Comment.Special),
            # Embedded markup
            (r'\<\w+\>', Name.Decorator),
            (r'\<\/\>', Name.Decorator),
            # Goto labels
            (r'\s*:\S+\n', Name.Label),
            # Fallback for all other text
            # Needs an optional \n on the end to finish lines correctly
            (r'\s+[\n]?', Text),
            (r'\S+?[\n]?', Text), # non-greedy so we don't consume all non-whitespace


        ],
        # While in a speaker line, we ignore everything else except variables & markup
        'speakerline' : [
            # Variables
            (r'(\{)([\w\.]+)(\})', bygroups(Operator, Name.Variable, Operator)),
            (r'(\|)(plural|gender)(\()(.*?)(\))', bygroups(Operator, Keyword, Operator, Keyword, Operator)),
            (r'[\@\n]', Text, '#pop'),
            # Embedded markup
            (r'\<\w+\>', Name.Decorator),
            (r'\<\/\>', Name.Decorator),
            (r'[^\@\n\<\{]+', Text),

        ],
        # While in a [] block, highlight operators etc (don't do it elsewhere)
        'squarebrackets' : [
            # Close bracket finishes
            (r'\]\s*[\n]?', Operator, '#pop'),
            # Variables      
            (r'(\{)([\w\.]+)(\})', bygroups(Operator, Name.Variable, Operator)),
            (r'\+\/\-\*\!\%', Operator),
            (r'\"[^\"]*\"', String.Double),
            (r'\`[^\`]*\`', String.Escape),
            (r'\d+(\.\d+)?', Number),
            (r'\b([tT]rue|[fF]alse|[mM]asculine|[fF]eminine|[nN]euter)\b', Name.Constant),
            # Set, event commands so we can highlight variable/event differently
            (r'\s*(set|event)(\s+)([^\]\s]+)', bygroups(Keyword, Text, Name.Variable)),
            (r'\s*(if|else|elseif|endif|event|return|goto|gosub|go to|go sub|random|endrandom)\b', Keyword),
            (r'\b(and|or|&&|\|\||not)\b', Operator),
            (r'[,]', Punctuation),
            (r'\s+', Text), # whitespace OK

        ]
    }