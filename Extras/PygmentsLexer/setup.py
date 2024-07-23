from setuptools import setup, find_packages
    
setup (
    name='sudslexer',
    packages=find_packages(),
    version="0.2.0",
    entry_points =
    """
    [pygments.lexers]
    sudslexer = sudslexer.lexer:SudsLexer
    """,
)