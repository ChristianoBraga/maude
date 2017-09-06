//
//      Implementation for class SuccSymbol.
//
#ifdef __GNUG__
#pragma implementation
#endif

//      utility stuff
#include "macros.hh"
#include "vector.hh"

//      forward declarations
#include "interface.hh"
#include "core.hh"
#include "freeTheory.hh"
#include "S_Theory.hh"

//      interface class definitions
#include "term.hh"

//      core class definitions
#include "rewritingContext.hh"
#include "symbolMap.hh"

//      S theory class definitions
#include "S_Symbol.hh"
#include "S_DagNode.hh"
#include "S_Term.hh"

//      built in stuff
#include "bindingMacros.hh"
#include "succSymbol.hh"

SuccSymbol::SuccSymbol(int id)
  : S_Symbol(id)
{
}

bool
SuccSymbol::attachData(const Vector<Sort*>& opDeclaration,
		       const char* purpose,
		       const Vector<const char*>& data)
{
  NULL_DATA(purpose, SuccSymbol, data);
  return S_Symbol::attachData(opDeclaration, purpose, data);
}

bool
SuccSymbol::attachTerm(const char* purpose, Term* term)
{
  BIND_TERM(purpose, term, zeroTerm);
  return S_Symbol::attachTerm(purpose, term);
}

void
SuccSymbol::copyAttachments(Symbol* original, SymbolMap* map)
{
  SuccSymbol* orig = safeCast(SuccSymbol*, original);
  COPY_TERM(orig, zeroTerm, map);
  S_Symbol::copyAttachments(original, map);
}

void
SuccSymbol::postInterSymbolPass()
{
  PREPARE_TERM(zeroTerm);
}

void
SuccSymbol::reset()
{
  zeroTerm.reset();  // so zero dag can be garbage collected
  S_Symbol::reset();  // parents reset() tasks
}

Term*
SuccSymbol::makeNatTerm(const mpz_class& nat)
{
  Assert(zeroTerm.getTerm() != 0,
	 cerr << "zero not defined for " << this);
  Term* zero = zeroTerm.getTerm()->deepCopy();
  return (nat == 0) ? zero : (new S_Term(this, nat, zero));
}

DagNode*
SuccSymbol::makeNatDag(const mpz_class& nat)
{
  Assert(zeroTerm.getTerm() != 0, cerr << "zero not defined");
  DagNode* zero = zeroTerm.getDag();
  return (nat == 0) ? zero : (new S_DagNode(this, nat, zero));
}

bool
SuccSymbol::isNat(const Term* term) const
{
  const Symbol* s = term->symbol();
  return zeroTerm.getTerm()->
    equal((s != this) ? term :
	  safeCast(const S_Term*, term)->getArgument());
}

bool
SuccSymbol::isNat(const DagNode* dagNode) const
{
  const Symbol* s = dagNode->symbol();
  return zeroTerm.getTerm()->
    equal((s != this) ? dagNode :
	  safeCast(const S_DagNode*, dagNode)->getArgument());
}

const mpz_class&
SuccSymbol::getNat(const Term* term) const
{
  static mpz_class zero(0);

  const Symbol* s = term->symbol();
  if (s != this)
    {
      Assert(zeroTerm.getTerm()->equal(term), cerr << "not a nat");
      return zero;
    }
  const S_Term* st = safeCast(const S_Term*, term);
  Assert(zeroTerm.getTerm()->equal(st->getArgument()), cerr << "arg not zero");
  return st->getNumber();
}

const mpz_class&
SuccSymbol::getNat(const DagNode* dagNode) const
{
  static mpz_class zero(0);

  const Symbol* s = dagNode->symbol();
  if (s != this)
    {
      Assert(zeroTerm.getTerm()->equal(dagNode), cerr << "not a nat");
      return zero;
    }
  const S_DagNode* sd = safeCast(const S_DagNode*, dagNode);
  Assert(zeroTerm.getTerm()->equal(sd->getArgument()), cerr << "arg not zero");
  return sd->getNumber();
}

bool
SuccSymbol::getSignedInt(const DagNode* dagNode, int& value) const
{
  if (isNat(dagNode))
    {
      const mpz_class& n = getNat(dagNode);
      if (n.fits_sint_p())
	{
	  value = n.get_si();
	  return true;
	}
    }
  return false;
}

bool
SuccSymbol::getSignedInt64(const DagNode* dagNode, Int64& value) const
{
  if (isNat(dagNode))
    {
      const mpz_class& n = getNat(dagNode);
      mpz_class u = n >> BITS_PER_UINT;
      if (u.fits_sint_p())
	{
	  value = u.get_si();
	  value <<= BITS_PER_UINT;
	  value |= n.get_ui();
	  return true;
	}
    }
  return false;
}
bool
SuccSymbol::rewriteToNat(DagNode* subject, RewritingContext& context, const mpz_class& result)
{
  Assert(result >= 0, cerr << "-ve");
  Assert(zeroTerm.getTerm() != 0, cerr << "zero not defined");

  DagNode* zero = zeroTerm.getDag();
  if (result == 0)
    return context.builtInReplace(subject, zero);

  bool trace = RewritingContext::getTraceStatus();
  if (trace)
    {
      context.tracePreEqRewrite(subject, 0, RewritingContext::BUILTIN);
      if (context.traceAbort())
	return false;
    }
  (void) new(subject) S_DagNode(this, result, zero);
  context.incrementEqCount();
  if (trace)
    context.tracePostEqRewrite(subject);
  return true;
}
