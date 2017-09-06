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
//	Class for task that will make a branch in the strategy depending
//	on what happens with some initial strategy.
//
#ifndef _branchTask_hh_
#define _branchTask_hh_
#include "strategicTask.hh"
#include "branchStrategy.hh"

class BranchTask : public StrategicTask
{
  NO_COPYING(BranchTask);


public:
  BranchTask(StrategicExecution* sibling,
	     DagNode* startDag,
	     StrategyExpression* initialStrategy,
	     BranchStrategy::Action successAction,
	     StrategyExpression* successStrategy,
	     BranchStrategy::Action failureAction,
	     StrategyExpression* failureStrategy,
	     const StrategyStack& pending,
	     StrategicProcess* insertionPoint);
  //
  //	Call-backs for interesting events.
  //
  virtual Survival executionSucceeded(DagNode* result, StrategicProcess* insertionPoint);
  virtual Survival executionsExhausted(StrategicProcess* insertionPoint);

private:
  DagNode* const startDag;
  StrategyExpression* const initialStrategy;
  BranchStrategy::Action successAction;
  StrategyExpression* const successStrategy;
  BranchStrategy::Action failureAction;
  StrategyExpression* const failureStrategy;
  StrategyStack pending;
  bool success;
};

#endif