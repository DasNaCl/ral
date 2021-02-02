

#ifndef TOK
#define TOK(x,y,z)
#endif

TOK(Identifier, "id", 2)
TOK(LiteralNumber, "num", 3)
TOK(Keyword, "key", 4)
TOK(DoubleColonEqual, ":=", 8)
TOK(Semi, ";", ';')
TOK(DoubleColon, ":", ':')
TOK(Comma, ",", ',')
TOK(Plus, "+", '+')
TOK(Minus, "-", '-')
TOK(Tilde, "~", '~')
TOK(MinusGreater, "->", 15)
TOK(Asterisk, "*", '*')
TOK(Slash, "/", '/')
TOK(PlusEqual, "+=", 11)
TOK(MinusEqual, "-=", 12)
TOK(AsteriskEqual, "*=", 13)
TOK(SlashEqual, "/=", 14)
TOK(Equal, "=", '=')
TOK(ExclamationEqual, "!=", 9)
TOK(LessGreater, "<>", 10)
TOK(Less, "<", '<')
TOK(LessEqual, "<=", 6)
TOK(GreaterEqual, ">=", 7)
TOK(Greater, ">", '>')
TOK(LParen, "(", '(')
TOK(RParen, ")", ')')
TOK(LBrace, "{", '{')
TOK(RBrace, "}", '}')
TOK(Undef, "#undef#", 1)
TOK(EndOfFile, "EOF", EOF)


