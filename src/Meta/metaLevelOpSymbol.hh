/*

    This file is part of the Maude 2 interpreter.

    Copyright 1997-2003 SRI International, Menlo Park, CA 94025, USA.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.

*/

//
//      Class for symbols for built-in meta-level operations.
//
#ifndef _metaLevelOpSymbol_hh_
#define _metaLevelOpSymbol_hh_
#include "freeSymbol.hh"
#include "sequenceSearch.hh"

class MetaLevelOpSymbol : public FreeSymbol
{
  NO_COPYING(MetaLevelOpSymbol);

public:
  MetaLevelOpSymbol(int id, int nrArgs, const Vector<int>& strategy);
  ~MetaLevelOpSymbol();

  bool attachData(const Vector<Sort*>& opDeclaration,
		  const char* purpose,
		  const Vector<const char*>& data);
  bool attachSymbol(const char* purpose, Symbol* symbol);
  bool attachTerm(const char* purpose, Term* term);
  void copyAttachments(Symbol* original, SymbolMap* map);
  void getDataAttachments(const Vector<Sort*>& opDeclaration,
			  Vector<const char*>& purposes,
			  Vector<Vector<const char*> >& data);
  void getSymbolAttachments(Vector<const char*>& purposes,
			    Vector<Symbol*>& symbols);
  void getTermAttachments(Vector<const char*>& purposes,
			  Vector<Term*>& terms);

  void postInterSymbolPass();
  void reset();
  bool eqRewrite(DagNode* subject, RewritingContext& context);

  MetaLevel* getMetaLevel() const;

private:
  typedef bool (MetaLevelOpSymbol::*DescentFunctionPtr)
    (FreeDagNode* subject, RewritingContext& context);

  static DagNode* term2Dag(Term* t);
  static RewritingContext* term2RewritingContext(Term* term, RewritingContext& context);
  static bool getCachedRewriteSearchState(MetaModule* m,
					  FreeDagNode* subject,
					  RewritingContext& context,
					  Int64 solutionNr,
					  RewriteSearchState*& state,
					  Int64& lastSolutionNr);
  static bool getCachedMatchSearchState(MetaModule* m,
					FreeDagNode* subject,
					RewritingContext& context,
					Int64 solutionNr,
					MatchSearchState*& state,
					Int64& lastSolutionNr);
  static bool getCachedRewriteSequenceSearch(MetaModule* m,
					     FreeDagNode* subject,
					     RewritingContext& context,
					     Int64 solutionNr,
					     RewriteSequenceSearch*& search,
					     Int64& lastSolutionNr);
  static bool getCachedUnificationProblem(MetaModule* m,
					  FreeDagNode* subject,
					  Int64 solutionNr,
					  UnificationProblem*& unification,
					  Int64& lastSolutionNr);
  static bool getCachedSMT_RewriteSequenceSearch(MetaModule* m,
						 FreeDagNode* subject,
						 RewritingContext& context,
						 Int64 solutionNr,
						 SMT_RewriteSequenceSearch*& search,
						 Int64& lastSolutionNr);


  MatchSearchState* makeMatchSearchState(MetaModule* m,
					 FreeDagNode* subject,
					 RewritingContext& context) const;
  MatchSearchState* makeMatchSearchState2(MetaModule* m,
					  FreeDagNode* subject,
					  RewritingContext& context) const;
  RewriteSequenceSearch* makeRewriteSequenceSearch(MetaModule* m,
						   FreeDagNode* subject,
						   RewritingContext& context) const;
  SMT_RewriteSequenceSearch* makeSMT_RewriteSequenceSearch(MetaModule* m,
							   FreeDagNode* subject,
							   RewritingContext& context) const;

  bool metaUnify2(FreeDagNode* subject, RewritingContext& context, bool disjoint);
  bool metaGetVariant2(FreeDagNode* subject, RewritingContext& context, bool irredundant);
  bool metaVariantUnify2(FreeDagNode* subject, RewritingContext& context, bool disjoint);
  bool okToBind();
  bool downSearchType(DagNode* arg, SequenceSearch::SearchType& searchType) const;

  bool getCachedNarrowingSequenceSearch(MetaModule* m,
					FreeDagNode* subject,
					RewritingContext& context,
					Int64 solutionNr,
					NarrowingSequenceSearch*& search,
					Int64& lastSolutionNr);

  static bool getCachedVariantSearch(MetaModule* m,
				     FreeDagNode* subject,
				     RewritingContext& context,
				     Int64 solutionNr,
				     VariantSearch*& search,
				     Int64& lastSolutionNr);

  NarrowingSequenceSearch* makeNarrowingSequenceSearch(MetaModule* m,
						       FreeDagNode* subject,
						       RewritingContext& context) const;
  NarrowingSequenceSearch* makeNarrowingSequenceSearch2(MetaModule* m,
							FreeDagNode* subject,
							RewritingContext& context) const;

  bool complexStrategy(DagNode* subject, RewritingContext& context);

  //
  //	Descent signature (generated by macro expansion).
  //
#define MACRO(SymbolName, NrArgs) \
  bool SymbolName(FreeDagNode* subject, RewritingContext& context);
#include "descentSignature.cc"
#undef MACRO

  bool noDuplicates(const Vector<Term*>& terms);
  bool dagifySubstitution(const Vector<Term*>& variables,
			  Vector<Term*>& values,
			  Vector<DagRoot*>& dags,
			  RewritingContext& context);
  void initializeSubstitution(Vector<Symbol*>& variables,
			      Vector<Term*>& values,
			      VariableInfo& rule,
			      Substitution& substitution);

  DescentFunctionPtr descentFunction;
  MetaLevel* metaLevel;
  MetaLevelOpSymbol* shareWith;
};

inline MetaLevel*
MetaLevelOpSymbol::getMetaLevel() const
{
  Assert(metaLevel != 0, "null metaLevel");
  return metaLevel;
}

#endif
