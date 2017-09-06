//
//	Implementation for class ACU_Term.
//
#ifdef __GNUG__
#pragma implementation
#endif

//	utility stuff
#include "macros.hh"
#include "vector.hh"
#include "graph.hh"
#include "digraph.hh"
#include "indent.hh"

//      forward declarations
#include "interface.hh"
#include "core.hh"
#include "variable.hh"
#include "ACU_Theory.hh"

//      interface class definitions
#include "argVec.hh"
#include "associativeSymbol.hh"
#include "dagNode.hh"
#include "term.hh"
#include "rawArgumentIterator.hh"
#include "lhsAutomaton.hh"
#include "rhsAutomaton.hh"
 
//      core class definitions
#include "rewritingContext.hh"
#include "equation.hh"
#include "symbolMap.hh"
#include "termBag.hh"
#include "rhsBuilder.hh"

//	variable class definitions
#include "variableTerm.hh"

//	ACU theory class definitions
#include "ACU_Symbol.hh"
#include "ACU_DagNode.hh"
#include "ACU_Term.hh"
#include "ACU_ArgumentIterator.hh"
#include "ACU_LhsAutomaton.hh"
#include "ACU_RhsAutomaton.hh"

//	extra source files
#include "ACU_LhsCompiler0.cc"
#include "ACU_LhsCompiler1.cc"
#include "ACU_LhsCompiler2.cc"
#include "ACU_LhsCompiler3.cc"

ACU_Term::ACU_Term(ACU_Symbol* symbol, const Vector<Term*>& arguments)
: Term(symbol), argArray(arguments.length())
{
  int nrArgs = arguments.length();
  Assert(nrArgs >= 2, cerr << "insufficient arguments for operator " << symbol);
  for (int i = 0; i < nrArgs; i++)
    {
      argArray[i].term = arguments[i];
      argArray[i].multiplicity = 1;
    }
}

ACU_Term::ACU_Term(const ACU_Term& original, SymbolMap* map)
: Term(map == 0 ? original.symbol() : map->translate(original.symbol())),
  argArray(original.argArray.length())
{
  int nrArgs = original.argArray.length();
  for (int i = 0; i < nrArgs; i++)
    {
      argArray[i].term = original.argArray[i].term->deepCopy(map);
      argArray[i].multiplicity = original.argArray[i].multiplicity;
    }
}

RawArgumentIterator*
ACU_Term::arguments()
{
  return new ACU_ArgumentIterator(&argArray);
}

void
ACU_Term::deepSelfDestruct()
{
  int nrArgs = argArray.length();
  for (int i = 0; i < nrArgs; i++)
    argArray[i].term->deepSelfDestruct();
  delete this;
}

Term*
ACU_Term::deepCopy2(SymbolMap* map) const
{
  return new ACU_Term(*this, map);
}

bool
ACU_Term::normalizeAliensAndFlatten()
{
  bool changed = false;
  Symbol* s = symbol();
  int nrArgs = argArray.length();
  //
  //	Pass 1: normalize aliens and recusively flatten non-aliens, computing
  //	the number of extra arguments that will result from flattening at the top.
  //
  bool needToFlatten = false;
  int expansion = 0;
  for (int i = 0; i < nrArgs; i++)
    {
      Term* t = argArray[i].term;
      if (t->symbol() == s)
	{
	  changed = true;
	  ACU_Term* ac = static_cast<ACU_Term*>(t);
	  (void) ac->normalizeAliensAndFlatten();
	  expansion += ac->argArray.length() - 1;
	  needToFlatten = true;
	}
      else
	{
	  bool subtermChanged;
	  argArray[i].term = t = t->normalize(true, subtermChanged);
	  if (subtermChanged)
	    {
	      changed = true;
	      if (t->symbol() == s)
		{
		  ACU_Term* ac = static_cast<ACU_Term*>(t);
		  expansion += ac->argArray.length() - 1;
		  needToFlatten = true;
		}
	    }
	}
    }
  //
  //	Pass 2 : flatten at the top
  //
  if (needToFlatten)
    {
      argArray.expandBy(expansion);
      int p = nrArgs + expansion - 1;
      for (int i = nrArgs - 1; i >= 0; i--)
	{
	  Assert(p >= i, cerr << "loop invariant broken");
	  Term* t = argArray[i].term;
	  if (t->symbol() == s)
	    {
	      int m = argArray[i].multiplicity;
	      Vector<Pair>& argArray2 = static_cast<ACU_Term*>(t)->argArray;
	      for (int j = argArray2.length() - 1; j >= 0; j--)
		{
		  argArray[p].term = argArray2[j].term;
		  argArray[p].multiplicity = m * argArray2[j].multiplicity;
		  --p;
		}
	      delete t;
	    }
	  else
	    argArray[p--] = argArray[i];
	}
    }
  return changed;
}

local_inline bool
ACU_Term::pairLt(const Pair& p1, const Pair& p2)
{
  return p1.term->compare(p2.term) < 0;  // sort pairs in ascending order of terms
}

Term*
ACU_Term::normalize(bool full, bool& changed)
{
  if (full)
    changed = normalizeAliensAndFlatten();
  else
    {
      changed = false;
      for (int i = argArray.length() - 1; i >= 0; i--)
	{
	  bool subtermChanged;
	  argArray[i].term = argArray[i].term->normalize(false, subtermChanged);
	  if (subtermChanged)
	    changed = true;
	}
    }
  int nrArgs = argArray.length();
  for (int i = 1; i < nrArgs; i++)
    {
      if (argArray[i - 1].term->compare(argArray[i].term) > 0)
	{
	  changed = true;
	  sort(argArray.begin(), argArray.end(), pairLt);
	}
    }
  int d = 0;
  for (int i = 1; i < nrArgs; i++)
    {
      if (argArray[i].term->equal(argArray[d].term))
	{
	  changed = true;
	  argArray[d].multiplicity += argArray[i].multiplicity;
	  argArray[i].term->deepSelfDestruct();
	}
      else
	argArray[++d] = argArray[i];
    }
  ++d;
  const Term* id = symbol()->getIdentity();
  if (id != 0)
    {
      //
      //	Need to eliminate any identity elements (maybe we should use binary search).
      //
      for (int i = 0; i < d; i++)
	{
	  if (id->equal(argArray[i].term))
	    {
	      changed = true;
	      if (d == 1)
		{
		  //
		  //	Only identity element left so collapse to identity element.
		  //
		  Term* t = argArray[i].term;
		  delete this;
		  return t;
		}
	      else if (d == 2 && argArray[1 - i].multiplicity == 1)
		{
		  //
		  //	Only one non-identity subterm left so collapse to it.
		  //
		  argArray[i].term->deepSelfDestruct();
		  Term* t = argArray[1 - i].term;
		  delete this;
		  return t;
		}
	      else
		{
		  //
		  //	Delete identity and shift other subterms to close gap.
		  //
		  argArray[i].term->deepSelfDestruct();
		  --d;
		  for (; i < d; i++)
		    argArray[i] = argArray[i + 1];
		  break;
		}
	    }
	}
    }
  argArray.contractTo(d);
  unsigned int hashValue = symbol()->getHashValue();
  for (int i = 0; i < d; i++)
    hashValue = hash(hashValue, argArray[i].term->getHashValue(), argArray[i].multiplicity);
  setHashValue(hashValue);
  return this;
}

int
ACU_Term::compareArguments(const Term* other) const
{
  int i = argArray.length();
  const Vector<Pair>& argArray2 = static_cast<const ACU_Term*>(other)->argArray;
  int r = i - argArray2.length();
  if (r != 0)
    return r;
  do 
    {
      --i;
      r = argArray[i].multiplicity - argArray2[i].multiplicity;
      if (r != 0)
	return r;
      r = argArray[i].term->compare(argArray2[i].term);
      if (r != 0)
	return r;
    }
  while (i > 0);
  return 0;
}

int
ACU_Term::compareArguments(const DagNode* other) const
{
  int i = argArray.length();
  const ArgVec<ACU_DagNode::Pair>& argArray2 = static_cast<const ACU_DagNode*>(other)->argArray;
  int r = i - argArray2.length();
  if (r != 0)
    return r;
  do
    {
      --i;
      r = argArray[i].multiplicity - argArray2[i].multiplicity;
      if (r != 0)
	return r;
      r = argArray[i].term->compare(argArray2[i].dagNode);
      if (r != 0)
	return r;
    }
  while (i > 0);
  return 0;
}

void
ACU_Term::findEagerVariables(bool atTop, NatSet& eagerVariables) const
{
  BinarySymbol::PermuteStrategy strat = symbol()->getPermuteStrategy();
  if (strat == BinarySymbol::EAGER ||
      (strat == BinarySymbol::SEMI_EAGER && !atTop))
    {
      int nrArgs = argArray.length();
      for (int i = 0; i < nrArgs; i++)
	argArray[i].term->findEagerVariables(false, eagerVariables);
    }
}

void
ACU_Term::markEagerArguments(int nrVariables,
			    const NatSet& eagerVariables,
			    Vector<int>& problemVariables)
{
  if (symbol()->getPermuteStrategy() == BinarySymbol::EAGER)
    {
      int nrArgs = argArray.length();
      for (int i = 0; i < nrArgs; i++)
	argArray[i].term->markEager(nrVariables, eagerVariables, problemVariables);
    }
}

DagNode*
ACU_Term::dagify2()
{
  int nrArgs = argArray.length();
  ACU_DagNode* d = new ACU_DagNode(symbol(), nrArgs);
  ArgVec<ACU_DagNode::Pair>& p = d->argArray;
  for (int i = 0; i < nrArgs; i++)
    {
      p[i].dagNode = argArray[i].term->dagify();
      p[i].multiplicity = argArray[i].multiplicity;
    }
  return d;
}

void
ACU_Term::analyseCollapses()
{
  int nrArgs = argArray.length();
  for (int i = 0; i < nrArgs; i++)
    argArray[i].term->analyseCollapses();
  uniqueCollapseSubtermIndex = NONE;
  ACU_Symbol* topSymbol = symbol();
  const Term* identity = topSymbol->getIdentity();
  if (identity == 0)
    return;  // if no identity element then we can't collapse

  int firstNonIdArg = NONE;
  for (int i = 0; i < nrArgs; i++)
    {
      Pair& p = argArray[i];
      if (!(topSymbol->mightMatchOurIdentity(p.term)))
	{
	  if (firstNonIdArg != NONE || p.multiplicity > 1)
	    return;  // can't collapse
	  firstNonIdArg = i;
	}
    }
  if (firstNonIdArg != NONE)
    {
      //
      //	Can only collapse to firstNonIdArg.
      //
      uniqueCollapseSubtermIndex = firstNonIdArg;
      addCollapseSymbol(argArray[firstNonIdArg].term->symbol());
      addCollapseSymbols(argArray[firstNonIdArg].term->collapseSymbols());
    }
  else
    {
      //
      //	Can collapse to any of our arguments that has multiplicity 1.
      //	If no argument has multiplicity 1 then we can still collapse
      //	to our identity.
      //
      for (int i = 0; i < nrArgs; i++)
	{
	  Pair& p = argArray[i];
	  if (p.multiplicity == 1)
	    {
	      addCollapseSymbol(p.term->symbol());
	      addCollapseSymbols(p.term->collapseSymbols());
	    }
	}
      if (collapseSymbols().empty())
	addCollapseSymbol(identity->symbol());  // bizarre special case
    }
}

void
ACU_Term::insertAbstractionVariables(VariableInfo& variableInfo)
{
  ACU_Symbol* topSymbol = symbol();
  bool honorsGroundOutMatch = true;
  int nrArgs = argArray.length();
  for (int i = 0; i < nrArgs; i++)
    {
      Pair& p = argArray[i];
      p.term->insertAbstractionVariables(variableInfo);
      if (!(p.term->honorsGroundOutMatch()))
	honorsGroundOutMatch = false;
      p.abstractionVariableIndex = NONE;
      p.collapseToOurSymbol = false;
      p.matchOurIdentity = false;
      if (dynamic_cast<VariableTerm*>(p.term) == 0)
	{
	  p.matchOurIdentity = topSymbol->mightMatchOurIdentity(p.term);
	  p.collapseToOurSymbol = topSymbol->mightCollapseToOurSymbol(p.term);
	  if (p.matchOurIdentity || p.collapseToOurSymbol)
	    {
	      p.abstractionVariableIndex = variableInfo.makeProtectedVariable();
	      honorsGroundOutMatch = false;
	      DebugAdvisoryCheck(false, cerr << "Introduced abstraction variable for " <<
				 p.term << " in " << this << '.');
	    }
	}
    }
  setHonorsGroundOutMatch(honorsGroundOutMatch);
}

#ifdef DUMP
void
ACU_Term::dump(ostream& s, const VariableInfo& variableInfo, int indentLevel)
{
  s << Indent(indentLevel) << "Begin{ACU_Term}\n";
  ++indentLevel;
  dumpCommon(s, variableInfo, indentLevel);
  s << Indent(indentLevel) << "arguments:\n";
  ++indentLevel;
  int nrArgs = argArray.length();
  for (int i = 0; i < nrArgs; i++)
    {
      const Pair& p = argArray[i];
      s << Indent(indentLevel) << "multiplicity = " << p.multiplicity <<
	"\tcollapseToOurSymbol = " << bool(p.collapseToOurSymbol) <<
	"\tmatchOurIdentity = " << bool(p.matchOurIdentity);
      if (p.abstractionVariableIndex != NONE)
	s << "\tabstractionVariableIndex = " << p.abstractionVariableIndex;
      s << '\n';
      p.term->dump(s, variableInfo, indentLevel);
    }
  s << Indent(indentLevel - 2) << "End{ACU_Term}\n";
}
#endif

void
ACU_Term::findAvailableTerms(TermBag& availableTerms, bool eagerContext, bool atTop)
{
  if (ground())
    return;
  if (!atTop)
    availableTerms.insertMatchedTerm(this, eagerContext);
  BinarySymbol::PermuteStrategy strat = symbol()->getPermuteStrategy();
  bool argEager = eagerContext && (strat == BinarySymbol::EAGER ||
				   (strat == BinarySymbol::SEMI_EAGER && !atTop));
  int nrArgs = argArray.length();
  for (int i = 0; i < nrArgs; i++)
    argArray[i].term->findAvailableTerms(availableTerms, argEager);
}

int
ACU_Term::compileRhs2(RhsBuilder& rhsBuilder,
		      VariableInfo& variableInfo,
		      TermBag& availableTerms,
		      bool eagerContext)
{
  int nrArgs = argArray.length();
  ACU_RhsAutomaton* automaton = new ACU_RhsAutomaton(symbol(), nrArgs);
  bool argEager = eagerContext && symbol()->getPermuteStrategy() == BinarySymbol::EAGER;
  Vector<int> sources;
  for (int i = 0; i < nrArgs; i++)
    {
      int index = argArray[i].term->compileRhs(rhsBuilder,
					       variableInfo,
					       availableTerms,
					       argEager);

      automaton->addArgument(index, argArray[i].multiplicity);
      sources.append(index);
    }
  //
  //	Need to flag last use of each source.
  //
  for (int i = 0; i < nrArgs; i++)
    variableInfo.useIndex(sources[i]);

  int index = variableInfo.makeConstructionIndex();
  automaton->close(index);
  rhsBuilder.addRhsAutomaton(automaton);
  return index;
}
