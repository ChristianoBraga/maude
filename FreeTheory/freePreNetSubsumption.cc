//
//	Code for deciding subsumption of one pattern by another when
//	both patterns have free symbols on top, and all free symbols in
//	either pattern that are not at or below the reduced fringe have
//	been seen in the subject.
//
//	The notion of subsumption used here is that the subsumer subsumes
//	the victim if every subject containing the seen free symbols that
//	matches the victim also matches the subsumer.
//

bool
FreePreNet::subsumesWrtReducedFringe(Term* subsumer,
				     Term* victim,
				     int currentPositionIndex,
				     const NatSet& reducedFringe)
{
  if (!(reducedFringe.contains(currentPositionIndex)))
    {
      if (FreeTerm* sf = dynamic_cast<FreeTerm*>(subsumer))
	{
	  //
	  //	Current position is not at or below reduced fringe and subsumer
	  //	subterm has a free symbol on top.
	  //	Therefore this free symbol will have been matched in
	  //	the subject and we may be able to do better than naive subsumption.
	  //
	  if (FreeTerm* vf = dynamic_cast<FreeTerm*>(victim))
	    {
	      Assert(sf->symbol() == vf->symbol(), cerr << "free symbol clash");
	      //
	      //	victim subterm has the same free symbol on top; check if
	      //	subsumers arguments subsume victims arguments.
	      //
	      Vector<int> argPosition(positions.index2Position(currentPositionIndex));
	      int newComponent = argPosition.length();
	      argPosition.expandBy(1);

	      ArgumentIterator subsumerArgs(*subsumer);
	      ArgumentIterator victimArgs(*victim);
	      for (int i = 0; subsumerArgs.valid(); i++)
		{
		  argPosition[newComponent] = i;
		  if (!(subsumesWrtReducedFringe(subsumerArgs.argument(),
						 victimArgs.argument(),
						 positions.position2Index(argPosition),
						 reducedFringe)))
		    return false;
		  subsumerArgs.next();
		  victimArgs.next();
		}
	      return true;
	    }
	  else
	    {
	      //
	      //	We treat the victims subterm as being a variable
	      //	of the victims subterms sort, and try to subsume it.
	      //
	      return subsumesWrtReducedFringe(sf,
					      victim->getSort()->getLeqSorts(),
					      currentPositionIndex,
					      reducedFringe);
	    }
	}
    }
  return subsumer->subsumes(victim, false);  // can't do any better naive subsumption
}

//
//	Try to subsume a (generalized) variable (represented by a set
//	of sorts) with free subterm. This situation occurs in
//	at least one base case for all subsumptions wrt a reduced fringe
//	that are not naive subsumptions.
//

bool
FreePreNet::subsumesWrtReducedFringe(Term* subsumer,
				     const NatSet& rangeSorts,
				     int currentPositionIndex,
				     const NatSet& reducedFringe)
{
  if (!(reducedFringe.contains(currentPositionIndex)))
    {
      if (FreeTerm* sf = dynamic_cast<FreeTerm*>(subsumer))
	{
	  Symbol* symbol = subsumer->symbol();
	  int nrArgs = symbol->arity();
	  Vector<NatSet> domainSorts(nrArgs);
	  symbol->computePossibleDomainSorts(rangeSorts, domainSorts);
	  
	  Vector<int> argPosition(positions.index2Position(currentPositionIndex));
	  int newComponent = argPosition.length();
	  argPosition.expandBy(1);

	  ArgumentIterator subsumerArgs(*subsumer);
	  for (int i = 0; subsumerArgs.valid(); i++)
	    {
	      argPosition[newComponent] = i;
	      if (!(subsumesWrtReducedFringe(subsumerArgs.argument(),
					     domainSorts[i],
					     positions.position2Index(argPosition),
					     reducedFringe)))
		return false;
	      subsumerArgs.next();
	    }
	  return true;
	}
      else if (VariableTerm* v = dynamic_cast<VariableTerm*>(subsumer))
	{
	  //
	  //	Must be linear and be able to cover all the range sorts.
	  //
	  return (!(v->occursInContext().contains(v->getIndex())) &&
		  v->getSort()->getLeqSorts().contains(rangeSorts));
	}
    }
  return false;
}
