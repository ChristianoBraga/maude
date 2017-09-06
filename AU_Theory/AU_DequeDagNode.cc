//
//      Implementation for class AU_DequeDagNode.
//
 
//	utility stuff
#include "macros.hh"
#include "vector.hh"
 
//      forward declarations
#include "interface.hh"
#include "core.hh"
#include "AU_Persistent.hh"
#include "AU_Theory.hh"
 
//      interface class definitions
#include "term.hh"

//	AU persistent class definitions
#include "AU_DequeIter.hh"

//      AU theory class definitions
#include "AU_Symbol.hh"
#include "AU_DagNode.hh"
#include "AU_DequeDagNode.hh"
#include "AU_DequeDagArgumentIterator.hh"
#include "AU_ExtensionInfo.hh"

RawDagArgumentIterator*
AU_DequeDagNode::arguments()
{
  return new AU_DequeDagArgumentIterator(deque);
}

size_t
AU_DequeDagNode::getHashValue()
{
  size_t hashValue = symbol()->getHashValue();
  for (AU_DequeIter i(deque); i.valid(); i.next())
    hashValue = hash(hashValue, i.getDagNode()->getHashValue());
  return hashValue;
}

int
AU_DequeDagNode::compareArguments(const DagNode* other) const
{
  if (safeCast(const AU_BaseDagNode*, other)->isDeque())
    {
      const AU_DequeDagNode* d2 = safeCast(const AU_DequeDagNode*, other);
      int r = deque.length() - d2->deque.length();
      if (r != 0)
	return r;

      AU_DequeIter i(deque);
      AU_DequeIter j(d2->deque);
      do
	{
	  int r = (i.getDagNode())->compare(j.getDagNode());
	  if (r != 0)
	    return r;
	  i.next();
	  j.next();
	}
      while (i.valid());
      Assert(!j.valid(), "iterator problem");
    }
  else
    {
      const ArgVec<DagNode*>& argArray2 = safeCast(const AU_DagNode*, other)->argArray;
      int r = deque.length() - argArray2.length();
      if (r != 0)
	return r;

      AU_DequeIter i(deque);
      ArgVec<DagNode*>::const_iterator j = argArray2.begin();
      do
	{
	  int r = (i.getDagNode())->compare(*j);
	  if (r != 0)
	    return r;
	  i.next();
	  ++j;
	}
      while (i.valid());
      Assert(j == argArray2.end(), "iterator problem");
    }
  return 0;
}

DagNode*
AU_DequeDagNode::markArguments()
{
  deque.mark();
  return 0;
}

DagNode*
AU_DequeDagNode::copyEagerUptoReduced2()
{
  return dequeToArgVec(this)->copyEagerUptoReduced2();
}

void
AU_DequeDagNode::clearCopyPointers2()
{
  CantHappen("Should not be copying on AU_DequeDagNode");
}

void
AU_DequeDagNode::overwriteWithClone(DagNode* old)
{
  AU_DequeDagNode* d = new(old) AU_DequeDagNode(symbol(), deque);
  d->copySetRewritingFlags(this);
  d->setSortIndex(getSortIndex());
}

DagNode*
AU_DequeDagNode::makeClone()
{
  AU_DequeDagNode* d = new AU_DequeDagNode(symbol(), deque);
  d->copySetRewritingFlags(this);
  d->setSortIndex(getSortIndex());
  return d;
}

DagNode*
AU_DequeDagNode::copyWithReplacement(int argIndex, DagNode* replacement)
{
  return dequeToArgVec(this)->copyWithReplacement(argIndex, replacement);  // HACK
}

DagNode*
AU_DequeDagNode::copyWithReplacement(Vector<RedexPosition>& redexStack,
				     int first,
				     int last)
{
  return dequeToArgVec(this)->copyWithReplacement(redexStack, first, last);
}

void
AU_DequeDagNode::stackArguments(Vector<RedexPosition>& stack,
				int parentIndex,
				bool respectFrozen)
{
  if (respectFrozen && !(symbol()->getFrozen().empty()))
    return;
  int j = 0;
  for (AU_DequeIter i(deque); i.valid(); i.next(), ++j)
    {
       DagNode* d = i.getDagNode();
      if (!(d->isUnstackable()))
	stack.append(RedexPosition(d, parentIndex, j));
    }
}

ExtensionInfo*
AU_DequeDagNode::makeExtensionInfo()
{
  return new AU_ExtensionInfo(dequeToArgVec(this));
}

AU_DagNode*
AU_DequeDagNode::dequeToArgVec(AU_DequeDagNode* original)
{
  AU_Symbol* s = original->symbol();
  AU_Deque c = original->deque;  // deep copy
  int sortIndex = original->getSortIndex();
  bool redFlag = original->isReduced();
  AU_DagNode* d = new(original) AU_DagNode(s, c.length());
  c.copyToArgVec(d->argArray);
  d->setSortIndex(sortIndex);
  if (redFlag)
    d->setReduced();
  return d;
}
