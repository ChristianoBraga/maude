//
//      Implementation for class MetaLevel.
//
#ifdef __GNUG__
#pragma implementation
#endif

//      utility stuff
#include "macros.hh"
#include "vector.hh"
#include "intSet.hh"
#include "pointerSet.hh"
#include "pointerMap.hh"
#include "flagSet.hh"

//      forward declarations
#include "interface.hh"
#include "core.hh"
#include "variable.hh"
#include "higher.hh"
#include "freeTheory.hh"
#include "AU_Theory.hh"
#include "ACU_Theory.hh"
#include "S_Theory.hh"
#include "CUI_Theory.hh"
#include "NA_Theory.hh"
#include "builtIn.hh"
#include "mixfix.hh"

//      interface class definitions
#include "symbol.hh"
#include "dagNode.hh"
#include "term.hh"

//      core class definitions
#include "argumentIterator.hh"
#include "dagArgumentIterator.hh"
#include "conditionFragment.hh"
#include "equation.hh"
#include "rule.hh"
#include "sortConstraint.hh"
#include "substitution.hh"
#include "symbolMap.hh"

//      variable class definitions
#include "variableDagNode.hh"

//      free theory class definitions
#include "freeSymbol.hh"
#include "freeDagNode.hh"

//      AU theory class definitions
#include "AU_Symbol.hh"
#include "AU_DagNode.hh"

//      ACU theory class definitions
#include "ACU_Symbol.hh"
#include "ACU_DagNode.hh"

//      S theory class definitions
#include "S_Symbol.hh"
#include "S_DagNode.hh"
#include "S_Term.hh"

//      CUI theory class definitions
#include "CUI_Symbol.hh"

//	builtin class definitions
#include "bindingMacros.hh"
#include "branchSymbol.hh"
#include "equalitySymbol.hh"
#include "sortTestSymbol.hh"
#include "stringSymbol.hh"
#include "stringDagNode.hh"
#include "stringTerm.hh"
#include "floatDagNode.hh"
#include "floatTerm.hh"
#include "succSymbol.hh"

//     higher class definitions
#include "equalityConditionFragment.hh"
#include "sortTestConditionFragment.hh"
#include "assignmentConditionFragment.hh"
#include "rewriteConditionFragment.hh"

//	front end class definitions
#include "fileTable.hh"
#include "quotedIdentifierSymbol.hh"
#include "quotedIdentifierOpSymbol.hh"
#include "quotedIdentifierTerm.hh"
#include "quotedIdentifierDagNode.hh"
#include "preModule.hh"
#include "metaModule.hh"
#include "metaLevel.hh"
#include "interpreter.hh"
#include "visibleModule.hh"
#include "main.hh"  // HACK to access global module database

//	our stuff
#include "metaDown.cc"
#include "metaDownOps.cc"
#include "metaDownFixUps.cc"
#include "metaUp.cc"
#include "metaUpModule.cc"

MetaLevel::MetaLevel()
{
#define MACRO(SymbolName, SymbolClass, RequiredFlags, NrArgs) \
  SymbolName = 0;
#include "metaLevelSignature.cc"
#undef MACRO
}

MetaLevel::~MetaLevel()
{
}

MetaLevel::MetaLevel(const MetaLevel* original, SymbolMap* map)
{
#define MACRO(SymbolName, SymbolClass, RequiredFlags, NrArgs) \
  SymbolName = (map == 0 || original->SymbolName == 0) ? original->SymbolName : \
    static_cast<SymbolClass*>(map->translate(original->SymbolName));
#include "metaLevelSignature.cc"
#undef MACRO
  Term* tt = original->trueTerm.getTerm();
  trueTerm.setTerm((tt == 0) ? 0 : tt->deepCopy(map));
  Term* ft = original->falseTerm.getTerm();
  falseTerm.setTerm((ft == 0) ? 0 : ft->deepCopy(map));
}

bool
MetaLevel::bind(const char* name, Term* term)
{
  Assert(term != 0, cerr << "null term for " << name);
  BIND_TERM(name, term, trueTerm);
  BIND_TERM(name, term, falseTerm);
  IssueWarning("unrecognized term hook name " << QUOTE(name) << '.');
  return false;
}

void
MetaLevel::postInterSymbolPass()
{
  if (trueTerm.getTerm() != 0)
    {
      (void) trueTerm.normalize();
      trueTerm.prepare();
    }
  if (falseTerm.getTerm() != 0)
    {
      (void) falseTerm.normalize();
      falseTerm.prepare();
    }
}

void
MetaLevel::reset()
{
  trueTerm.reset();  // so true dag can be garbage collected
  falseTerm.reset();  // so false dag can be garbage collected
  cache.flush();
}

bool
MetaLevel::bind(const char* name, Symbol* symbol)
{
  Assert(symbol != 0, cerr << "null symbol for " << name);
#define MACRO(SymbolName, SymbolClass, RequiredFlags, NrArgs) \
  if (strcmp(name, #SymbolName) == 0) SymbolName = static_cast<SymbolClass*>(symbol); else
#include "metaLevelSignature.cc"
#undef MACRO
    {
      IssueWarning("unrecognized symbol hook name " << QUOTE(name) << '.');
      return false;
    }
  return true;
}
