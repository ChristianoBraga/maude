/*

    This file is part of the Maude 2 interpreter.

    Copyright 1997-2006 SRI International, Menlo Park, CA 94025, USA.

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
//      Class for generators that apply the same strategy multiple times.
//
#ifndef _iterationSetGenerator_hh_
#define _iterationSetGenerator_hh_
#include <list>
#include "dagRoot.hh"
#include "setGenerator.hh"

class IterationSetGenerator : public SetGenerator
{
public:
  IterationSetGenerator(DagNode* start,
			RewritingContext& context,
			StrategyExpression* strategy,
			bool zeroAllowed,
			bool normalForm);
  ~IterationSetGenerator();

  DagNode* findNextSolution();

private:
  struct SearchNode
  {
    SetGenerator* generator;
    DagNode* startDag;
  };

  typedef list<SearchNode> GenQueue;

  DagRoot start;
  RewritingContext& context;
  StrategyExpression* const strategy;
  const bool zeroAllowed;		// allow start dag as a solution
  const bool normalForm;		// insist that solution are not further rewritable
  bool zeroReturnable;
  GenQueue genQueue;
};

#endif
