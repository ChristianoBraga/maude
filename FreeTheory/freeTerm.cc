//
//	Implementation for class FreeTerm.
//
#ifdef __GNUG__
#pragma implementation
#endif

//	utility stuff
#include "macros.hh"
#include "vector.hh"

//	forward declarations
#include "interface.hh"
#include "core.hh"
#include "variable.hh"
#include "freeTheory.hh"

//	interface class definitions
#include "symbol.hh"
#include "dagNode.hh"
#include "term.hh"
#include "rawArgumentIterator.hh"
#include "lhsAutomaton.hh"
#include "rhsAutomaton.hh"

//	core class definitions
#include "variableTerm.hh"
#include "rewritingContext.hh"
#include "equation.hh"
#include "symbolMap.hh"
#include "termBag.hh"
#include "rhsBuilder.hh"

//	free theory class definitions
#include "freeNet.hh"
#include "freeSymbol.hh"
#include "freeOccurrence.hh"
#include "freeDagNode.hh"
#include "freeArgumentIterator.hh"
#include "freeLhsAutomaton.hh"
#include "freeRhsAutomaton.hh"
#include "freeRemainder.hh"
#include "freeTerm.hh"

#include "freeLhsCompiler.cc"

#ifdef COMPILER
#include "freeTermFullCompiler.cc"
#endif

FreeTerm::FreeTerm(FreeSymbol* symbol, const Vector<Term*>& arguments)
  : Term(symbol), argArray(arguments.length())
{
  int nrArgs = arguments.length();
  Assert(symbol->arity() == nrArgs,
	 cerr << "number of arguments does not match symbol definition for " << symbol);
  for (int i = 0; i < nrArgs; i++)
    argArray[i] = arguments[i];
  slotIndex = -1;
  visitedFlag = false;
}

FreeTerm::FreeTerm(const FreeTerm& original, SymbolMap* map)
  : Term(map == 0 ? original.symbol() : map->translate(original.symbol())),
    argArray(original.argArray.length())
{
  int nrArgs = original.argArray.length();
  for (int i = 0; i < nrArgs; i++)
    argArray[i] = original.argArray[i]->deepCopy(map);
  slotIndex = -1;
  visitedFlag = false;
}
  
FreeTerm::~FreeTerm()
{
}

RawArgumentIterator*
FreeTerm::arguments()
{
  if (argArray.length() == 0)
    return 0;
  else
    return new FreeArgumentIterator(&argArray);
}

void
FreeTerm::deepSelfDestruct()
{
  int nrArgs = argArray.length();
  for (int i = 0; i < nrArgs; i++)
    argArray[i]->deepSelfDestruct();
  delete this;
}

Term*
FreeTerm::deepCopy2(SymbolMap* map) const
{
  return new FreeTerm(*this, map);
}

Term*
FreeTerm::normalize(bool full, bool& changed)
{
  changed = false;
  unsigned int hashValue = symbol()->getHashValue();
  int nrArgs = argArray.length();
  for (int i = 0; i < nrArgs; i++)
    {
      bool subtermChanged;
      Term* t = argArray[i]->normalize(full, subtermChanged);
      argArray[i] = t;
      hashValue = hash(hashValue, t->getHashValue());
      if (subtermChanged)
	changed = true;
    }
  setHashValue(hashValue);
  return this;
}

int
FreeTerm::compareArguments(const Term* other) const
{
  Assert(symbol() == other->symbol(), cerr << "symbols differ");
  int nrArgs = argArray.length();
  Vector<Term*>& ta = (const_cast<FreeTerm*>(static_cast<const FreeTerm*>(other)))->argArray;
  for (int i = 0; i < nrArgs; i++)
    {
      int r = argArray[i]->compare(ta[i]);
      if (r != 0)
	    return r;
    }
  return 0;
}

int
FreeTerm::compareArguments(const DagNode* other) const
{
  Assert(symbol() == other->symbol(), cerr << "symbols differ");
  int nrArgs = argArray.length();
  if (nrArgs != 0)
    {
      DagNode** da = ((FreeDagNode*) other)->argArray();
      for (int i = 0; i < nrArgs; i++, da++)
	{
	  int r = argArray[i]->compare(*da);
	  if (r != 0)
	    return r;
	}
    }
  return 0;
}

void
FreeTerm::findEagerVariables(bool atTop, NatSet& eagerVariables) const
{
  int nrArgs = argArray.length();
  FreeSymbol* sym = static_cast<FreeSymbol*>(symbol());
  for (int i = 0; i < nrArgs; i++)
    {
      if (atTop ? sym->eagerArgument(i) : sym->evaluatedArgument(i))
	argArray[i]->findEagerVariables(false, eagerVariables);
    }
}

void
FreeTerm::markEagerArguments(int nrVariables,
			     const NatSet& eagerVariables,
			     Vector<int>& problemVariables)
{
  int nrArgs = argArray.length();
  FreeSymbol* sym = static_cast<FreeSymbol*>(symbol());
  for (int i = 0; i < nrArgs; i++)
    {
      if (sym->eagerArgument(i))
	argArray[i]->markEager(nrVariables, eagerVariables, problemVariables);
    }
}

Term*
FreeTerm::locateSubterm(const Vector<int>& position, int backup)
{
  Term* t = this;
  int nrSteps = position.length() - backup;
  for (int i = 0; i < nrSteps; i++)
    {
      FreeTerm* f = dynamic_cast<FreeTerm*>(t);
      if (f == 0)
	return 0;
      int p = position[i];
      if (p >= f->symbol()->arity())
	return 0;
      t = (f->argArray)[p];
    }
  return t;
}

Term*
FreeTerm::locateSubterm2(Vector<int>& position)
{
  Term* t = this;
  int nrSteps = position.length();
  for (int i = 0; i < nrSteps; i++)
    {
      FreeTerm* f = dynamic_cast<FreeTerm*>(t);
      if (f == 0)
	{
	  position.contractTo(i);
	  return t;
	}
      int p = position[i];
      if (p >= f->symbol()->arity())
	{
	  position.contractTo(i);
	  return t;
	}
      t = (f->argArray)[p];
    }
  return t;
}

void
FreeTerm::findActiveSlots(NatSet& slots)
{
  //
  //	A free term's slot is active if it has been visited and it has
  //	at least one subterm which has not been visited (or is alien) or
  //	which has a save index for left -> right sharing.
  //
  int nrArgs = argArray.length();
  bool active = false;
  for (int i = 0; i < nrArgs; i++)
    {
      FreeTerm* f = dynamic_cast<FreeTerm*>(argArray[i]);
      if (f != 0 && f->visitedFlag)
	{
	  f->findActiveSlots(slots);
	  if (f->getSaveIndex() != NONE)
	    active = true;
	}
      else
	active = true;
    }
  if (active)
    {
      Assert(slotIndex != NONE, cerr << "no slot index for active FreeTerm " << this);
      slots.insert(slotIndex);
    }
}

DagNode*
FreeTerm::dagify2()
{
  FreeDagNode* d = new FreeDagNode(symbol());
  int nrArgs = symbol()->arity();
  if (nrArgs != 0)
    {
      DagNode** p = d->argArray();
      for (int i = 0; i < nrArgs; i++, p++)
	(*p) = argArray[i]->dagify();
    }
  return d;
}

bool
FreeTerm::earlyMatchFailOnInstanceOf(const Term* other) const
{
  if (symbol() != other->symbol())
    return other->stable();  // terms headed by free symbols are always stable
  int nrArgs = argArray.length();
  Vector<Term*>& argArray2 =
    const_cast<FreeTerm*>(static_cast<const FreeTerm*>(other))->argArray;
  for (int i = 0; i < nrArgs; i++)
    {
      if (argArray[i]->earlyMatchFailOnInstanceOf(argArray2[i]))
	return true;
    }
  return false;
}

bool
FreeTerm::subsumes(const Term* other, bool sameVariableSet) const
{
  if (symbol() != other->symbol())
    return false;
  int nrArgs = argArray.length();
  const Vector<Term*>& argArray2 = safeCast(const FreeTerm*, other)->argArray;
  for (int i = 0; i < nrArgs; i++)
    {
      if (!argArray[i]->subsumes(argArray2[i], sameVariableSet))
	return false;
    }
  return true;
}

void
FreeTerm::findAvailableTerms(TermBag& availableTerms, bool eagerContext, bool atTop)
{
  if (ground())
    return;
  int nrArgs = argArray.length();
  FreeSymbol* s = safeCast(FreeSymbol*, symbol());
  if (atTop)
    {
      for (int i = 0; i < nrArgs; i++)
	argArray[i]->findAvailableTerms(availableTerms, eagerContext && s->eagerArgument(i));
    }
  else
    {
      availableTerms.insertMatchedTerm(this, eagerContext);
      for (int i = 0; i < nrArgs; i++)
	argArray[i]->findAvailableTerms(availableTerms, eagerContext && s->evaluatedArgument(i));
    }
}

/*
//
//	Simplistic hacky version to test for bugs
//

int
FreeTerm::compileRhs2(RhsBuilder& rhsBuilder,
		      VariableInfo& variableInfo,
		      TermBag& availableTerms,
		      bool eagerContext)
{
  FreeSymbol* s = safeCast(FreeSymbol*, symbol());
  Vector<int> sources;
  int nrArgs = argArray.length();
  for (int i = 0; i < nrArgs; i++)
    {
      bool argEager = eagerContext && s->eagerArgument(i);
      Term* t = argArray[i];
      sources.append(t->compileRhs(rhsBuilder, variableInfo, availableTerms, argEager));
    }
  int index = variableInfo.makeConstructionIndex();
  FreeRhsAutomaton* automaton = new FreeRhsAutomaton();
  automaton->addFree(s, index, sources);
  rhsBuilder.addRhsAutomaton(automaton);
  return index;
}
*/


int
FreeTerm::compileRhs2(RhsBuilder& rhsBuilder,
		      VariableInfo& variableInfo,
		      TermBag& availableTerms,
		      bool eagerContext)
{
  compileRhsAliens(rhsBuilder, variableInfo, availableTerms, eagerContext);
  FreeRhsAutomaton* automaton = new FreeRhsAutomaton();
  int index = compileRhs3(automaton, rhsBuilder, variableInfo, availableTerms, eagerContext);
  rhsBuilder.addRhsAutomaton(automaton);
  return index;
}

void
FreeTerm::compileRhsAliens(RhsBuilder& rhsBuilder,
			   VariableInfo& variableInfo,
			   TermBag& availableTerms,
			   bool eagerContext)
{
  int nrArgs = argArray.length();
  FreeSymbol* s = safeCast(FreeSymbol*, symbol());
  for (int i = 0; i < nrArgs; i++)
    {
      bool argEager = eagerContext && s->eagerArgument(i);
      Term* t = argArray[i];
      if (FreeTerm* f = dynamic_cast<FreeTerm*>(t))
	{
	  if (!(availableTerms.findTerm(f, argEager)))
	    f->compileRhsAliens(rhsBuilder, variableInfo, availableTerms, argEager);
	}
      else
	(void) t->compileRhs(rhsBuilder, variableInfo, availableTerms, argEager);
    }
}

int
FreeTerm::compileRhs3(FreeRhsAutomaton* automaton,
		      RhsBuilder& rhsBuilder,
		      VariableInfo& variableInfo,
		      TermBag& availableTerms,
		      bool eagerContext)
{ // SHOULD BE CLEANED UP NOW WE HAVE compileRhsAliens()
  int nrArgs = argArray.length();
  FreeSymbol* s = safeCast(FreeSymbol*, symbol());
  Vector<int> sources;
  for (int i = 0; i < nrArgs; i++)
    {
      bool argEager = eagerContext && s->eagerArgument(i);
      Term* t = argArray[i];
      if (FreeTerm* f = dynamic_cast<FreeTerm*>(t))
	{
	  if (!(availableTerms.findTerm(f, argEager)))
	    {
	      int source = f->compileRhs3(automaton,
					  rhsBuilder,
					  variableInfo,
					  availableTerms,
					  argEager);
	      sources.append(source);
	      f->setSaveIndex(source);
	      availableTerms.insertBuiltTerm(f, argEager);
	      continue;
	    } 
	}
      //
      //	Alien, variable or available term so use standard mechanism.
      //
      sources.append(t->compileRhs(rhsBuilder, variableInfo, availableTerms, argEager));
    }

  //
  //	Need to flag last use of each source.
  //
  for (int i = 0; i < nrArgs; i++)
    variableInfo.useIndex(sources[i]);

  int index = variableInfo.makeConstructionIndex();
  automaton->addFree(s, index, sources);
  return index;
}
