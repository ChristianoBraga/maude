//
//      Class for parser tokens.
//
#ifndef _token_hh_
#define _token_hh_
#ifdef __GNUG__
#pragma interface
#endif
#include <gmpxx.h>
#ifdef LIBv3
#include <ext/rope>
using namespace __gnu_cxx;
#else
#include <rope>
#endif
#include "stringTable.hh"

class Token
{
public:
  //
  //	Can't have a constructor or destructor because
  //	we want to be in the bison stack union.
  //
  enum SpecialProperties
  {
    SMALL_NAT,
    SMALL_NEG,
    ZERO,
    QUOTED_IDENTIFIER,
    STRING,
    FLOAT,
    CONTAINS_COLON,
    ENDS_IN_COLON,
    ITER_SYMBOL,
    RATIONAL,
    LAST_PROPERTY
  };

  enum AuxProperties
  {
    AUX_SORT,
    AUX_VARIABLE,
    AUX_CONSTANT,
    AUX_KIND
  };

  void tokenize(const char* tokenString, int lineNumber);
  void tokenize(int code, int lineNumber);
  void fixUp(const char* tokenString, int& lineNumber);

  void makePrefixName(const Vector<Token>& opBubble);
  const char* name() const;
  int code() const;
  int lineNumber() const;
  int specialProperty() const;
  int auxProperty() const;

  static const char* name(int code);
  static int specialProperty(int code);
  static int auxProperty(int code);
  static int encode(const char* tokenString);
  static int dotNameCode(int sortNameCode);
  static int quoteNameCode(int idCode);
  static int extractMixfix(int prefixNameCode, Vector<int>& mixfixSyntax);
  static bool specialChar(char c);

  static int backQuoteSpecials(int code);
  static int unBackQuoteSpecials(int code);
  static void printTokenVector(ostream& s,
			       const Vector<Token>& tokens,
			       int first,
			       int last,
			       bool fancySpacing);

  static bool split(int code, int& prefix, int& suffix);
  static bool split(int code, int& opName, mpz_class& number);
  static bool splitKind(int code, Vector<int>& codes);
  static Int64 codeToInt64(int code);
  static int int64ToCode(Int64 i);
  static double codeToDouble(int code);
  static int doubleToCode(double d);
  static crope codeToRope(int code);
  static void ropeToString(const crope& rope, string& result);
  static int ropeToPrefixNameCode(const crope& r);
  static int bubbleToPrefixNameCode(const Vector<Token>& opBubble);
  void getRational(mpz_class& numerator, mpz_class& denominator);

private:
  static void bufferExpandTo(int size);
  static void reallocateBuffer(int length);
  static void checkForSpecialProperty(const char* tokenString);
  static int computeAuxProperty(const char* tokenString);
  static const char* skipSortName(const char* tokenString);
  static bool looksLikeRational(const char* s);
  static StringTable stringTable;
  static Vector<int> specialProperties;
  static Vector<int> auxProperties;
  static char* buffer;
  static int bufferLength;

  int codeNr;
  int lineNr;
};

inline void
Token::makePrefixName(const Vector<Token>& opBubble)
{
  codeNr = bubbleToPrefixNameCode(opBubble);
  lineNr = opBubble[0].lineNr;
}

inline int
Token::encode(const char* tokenString)
{
  int code = stringTable.encode(tokenString);
  if (code == specialProperties.length())
    checkForSpecialProperty(tokenString);
  return code;
}

inline int
Token::specialProperty() const
{
  return specialProperties[codeNr];
}

inline int
Token::auxProperty() const
{
  return auxProperties[codeNr];
}

inline int
Token::specialProperty(int code)
{
  return specialProperties[code];
}

inline int
Token::auxProperty(int code)
{
  return auxProperties[code];
}

inline bool
Token::specialChar(char c)
{
  return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c  == ',';
}

inline int
Token::backQuoteSpecials(int code)
{
  const char* s = stringTable.name(code);
  char c = s[0];
  if (specialChar(s[0]) && s[1] == '\0')
    {
      char t[3];
      t[0] = '`';
      t[1] = c;
      t[2] = '\0';
      return encode(t);
    }
  return code;
}

inline int
Token::unBackQuoteSpecials(int code)
{
  const char* s = stringTable.name(code);
  if (s[0] == '`' && specialChar(s[1]) && s[2] == '\0')
    return encode(s + 1);
  return code;
}

inline void
Token::bufferExpandTo(int length)
{
  if (length > bufferLength)
    reallocateBuffer(length);
}

inline void
Token::tokenize(const char* tokenString, int lineNumber)
{
  codeNr = encode(tokenString);
  lineNr = lineNumber;
}

inline void
Token::tokenize(int code, int lineNumber)
{
  codeNr = code;
  lineNr = lineNumber;
}

inline const char*
Token::name() const
{
  return stringTable.name(codeNr);
}

inline int
Token::code() const
{
  return codeNr;
}

inline int
Token::lineNumber() const
{
  return lineNr;
}

inline const char*
Token::name(int code)
{
  return stringTable.name(code);
}

ostream& operator<<(ostream& s, const Token& token);
ostream& operator<<(ostream& s, const Vector<Token>& tokens);

#endif
