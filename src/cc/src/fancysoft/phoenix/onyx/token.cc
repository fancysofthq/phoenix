#include "fancysoft/phoenix/onyx/token.hh"

namespace Fancysoft::Phoenix::Onyx {

const char *Token::Keyword::kind_to_str(Kind kind) {
  switch (kind) {
  case Extern:
    return "extern";

  case Import:
    return "import";
  case As:
    return "as";
  case From:
    return "from";

  case Export:
    return "export";
  case Default:
    return "default";

  case Builtin:
    return "builtin";
  case Private:
    return "private";
  case Static:
    return "static";

  case Let:
    return "let";
  case Final:
    return "final";
  case Getter:
    return "getter";

  case Unsafe:
    return "unsafe";
  case Fragile:
    return "fragile";
  case Threadsafe:
    return "threadsafe";

  case UnsafeBang:
    return "unsafe!";
  case FragileBang:
    return "fragile!";
  case ThreadsafeBang:
    return "threadsafe!";

  case Decl:
    return "decl";
  case Redecl:
    return "redecl";
  case Impl:
    return "impl";
  case Def:
    return "def";
  case Reimpl:
    return "reimpl";
  case Extend:
    return "extend";

  case Return:
    return "return";
  case Convey:
    return "convey";

  case Switch:
    return "switch";
  case Case:
    return "case";
  case If:
    return "if";
  case Then:
    return "then";
  case Elif:
    return "elif";
  case Else:
    return "else";

  case While:
    return "while";
  case Do:
    return "do";
  case Break:
    return "break";
  case Continue:
    return "continue";

  case And:
    return "and";
  case Or:
    return "or";
  case Not:
    return "not";

  case Distinct:
    return "distinct";
  case Alias:
    return "alias";
  case To:
    return "to";

  case Trait:
    return "trait";
  case Struct:
    return "struct";
  case Class:
    return "class";
  case Enum:
    return "enum";
  case Unit:
    return "unit";
  case Annotation:
    return "annotation";
  }
};

std::optional<Token::Keyword::Kind> Token::Keyword::parse_kind(std::string string) {
  if (!string.compare("extern"))
    return Extern;

  if (!string.compare("import"))
    return Import;
  if (!string.compare("as"))
    return As;
  if (!string.compare("from"))
    return From;

  if (!string.compare("export"))
    return Export;
  if (!string.compare("default"))
    return Default;

  if (!string.compare("builtin"))
    return Builtin;
  if (!string.compare("private"))
    return Private;
  if (!string.compare("static"))
    return Static;

  else if (!string.compare("let"))
    return Let;
  else if (!string.compare("final"))
    return Final;
  else if (!string.compare("getter"))
    return Getter;

  else if (!string.compare("unsafe"))
    return Unsafe;
  else if (!string.compare("fragile"))
    return Fragile;
  else if (!string.compare("threadsafe"))
    return Threadsafe;

  else if (!string.compare("unsafe!"))
    return UnsafeBang;
  else if (!string.compare("fragile!"))
    return FragileBang;
  else if (!string.compare("threadsafe!"))
    return ThreadsafeBang;

  else if (!string.compare("decl"))
    return Decl;
  else if (!string.compare("redecl"))
    return Redecl;
  else if (!string.compare("impl"))
    return Impl;
  else if (!string.compare("def"))
    return Def;
  else if (!string.compare("reimpl"))
    return Reimpl;
  else if (!string.compare("extend"))
    return Extend;

  else if (!string.compare("return"))
    return Return;
  else if (!string.compare("convey"))
    return Convey;

  else if (!string.compare("switch"))
    return Switch;
  else if (!string.compare("case"))
    return Case;
  else if (!string.compare("if"))
    return If;
  else if (!string.compare("then"))
    return Then;
  else if (!string.compare("elif"))
    return Elif;
  else if (!string.compare("else"))
    return Else;

  else if (!string.compare("while"))
    return While;
  else if (!string.compare("do"))
    return Do;
  else if (!string.compare("break"))
    return Break;
  else if (!string.compare("continue"))
    return Continue;

  else if (!string.compare("and"))
    return And;
  else if (!string.compare("or"))
    return Or;
  else if (!string.compare("Not"))
    return Not;

  else if (!string.compare("distinct"))
    return Distinct;
  else if (!string.compare("alias"))
    return Alias;
  else if (!string.compare("to"))
    return To;

  else if (!string.compare("trait"))
    return Trait;
  else if (!string.compare("struct"))
    return Struct;
  else if (!string.compare("class"))
    return Class;
  else if (!string.compare("enum"))
    return Enum;
  else if (!string.compare("unit"))
    return Unit;
  else if (!string.compare("annotation"))
    return Annotation;

  else
    return std::nullopt;
}

const char *Token::Punct::kind_to_str(Kind kind) {
  switch (kind) {
  case Newline:
    return "\n";
  case Space:
    return " ";

  case Comma:
    return ",";
  case Colon:
    return ":";
  case Semi:
    return ";";

  case AnnotationOpen:
    return "@[";

  case MacroOpen:
    return "{%";
  case MacroClose:
    return "%}";
  case EmitMacroOpen:
    return "{{";
  case EmitMacroClose:
    return "}}";
  case DelayedMacroOpen:
    return "\\{%";
  case DelayedEmitMacroOpen:
    return "\\{{";

  case ParenOpen:
    return "(";
  case ParenClose:
    return ")";

  case BracketOpen:
    return "{";
  case BracketClose:
    return "}";

  case AngleOpen:
    return "<";
  case AngleClose:
    return ">";

  case SquareOpen:
    return "[";
  case SquareClose:
    return "]";

  case Pipe:
    return "|";

  case ScopeStatic:
    return "::";
  case ScopeInstance:
    return ".";
  case ScopeUFCS:
    return ":";

  case ArrowGenerator:
    return "=>";
  case ArrowFunction:
    return "->";
  case ArrowLambda:
    return "~>";
  }
}

const char *Token::Punct::kind_to_safe_str(Kind kind) {
  switch (kind) {
  case Newline:
    return "␤";
  case Space:
    return "␠";
  default:
    return kind_to_str(kind);
  }
}

} // namespace Fancysoft::Phoenix::Onyx
