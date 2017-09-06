//
//	Class for abstract discrimination nets for the free theory which can
//	be use to produce a FreeNet for interpretation or compiled to C++.
//
#ifndef _freePreNet_hh_
#define _freePreNet_hh_
#ifdef __GNUG__
#pragma interface
#endif
#include <map>
#include "unionFind.hh"
#include "freePositionTable.hh"
#include "freeSubterm.hh"

class FreePreNet
{
public:
  FreePreNet(bool expandRemainderNodes);

  void buildNet(FreeSymbol* symbol);

#ifdef COMPILER
  void generateCode(CompilationContext& context);
  int generateCode(CompilationContext& context, bool& tailRecursive) const;
#endif

#ifdef DUMP
  void dump(ostream& s, int indentLevel = 0) const;
#endif

  void semiCompile(FreeNet& freeNet);

private:
  typedef map<int, int> SlotMap;

  enum Flags
  {
    //
    //	Pattern is a free skeleton with linear, error free maximal variables,
    //	and is also a subsumer. Such a pattern must match if the free skeleton
    //	matches.
    //
    UNFAILING = 1,
    //
    //	Pattern does not have a condition associated with it and hence can subsume
    //	other patterns.
    //
    SUBSUMER = 2
  };

  struct Pattern
  {
    Term* term;
    int flags;
    Vector<FreeSubterm> subterms;
  };

  struct Arc
  {
    Symbol* label;	// symbol at test position; 0 denotes ? arc
    int target;		// index of target not if this symbol found
  };

  struct Pair
  {
    int positionIndex;
    int slot;
  };

  struct Node
  {
    NatSet liveSet;	// set of indices to live patterns 
    NatSet reducedFringe;	// set of indices to positions in reduced fringe
    NatSet positionsTested;	// set of indices to positions tested on all paths to this node
    Vector<Arc> sons;	// exiting arcs, empty denotes terminal node
    union
    {
      int neqTarget;	// exiting arc for ? case in test node
      int nextPattern;  // sole exiting arc in remainder node
    };
    union
    {
      int testPositionIndex;	// test position in test node
      int patternIndex;		// pattern index in remainder node
    };
    Vector<Pair> slotMap;
    //
    //	Only used for semi-compiler.
    //
    int freeNetIndex;
    //
    //	Only used for full compiler.
    //
    int nrParents;
    int nrVisits;
  };

  int makeNode(const NatSet& liveSet,
	       const NatSet& reducedFringe,
	       const NatSet& positionsTested);
  void expandFringe(int positionIndex, Symbol* symbol, NatSet& fringe);
  void reduceFringe(const NatSet& liveSet, NatSet& fringe) const;
  void findLiveSet(const NatSet& original,
		   int positionIndex,
		   Symbol* symbol,
		   const NatSet& fringe,
		   NatSet& liveSet);
  bool partiallySubsumed(const NatSet& liveSet,
			 int victim,
			 const NatSet& fringe);
  int findBestPosition(Node& n) const;

  bool subsumesWrtReducedFringe(Term* subsumer,
				Term* victim,
				int currentPositionIndex,
				const NatSet& reducedFringe);

  bool subsumesWrtReducedFringe(Term* subsumer,
				const NatSet& rangeSorts,
				int currentPositionIndex,
				const NatSet& reducedFringe);

  //
  //	Semi-compiler stuff.
  //
  int semiCompileNode(FreeNet& freeNet, int nodeNr, const SlotMap& slotMap);
  void setVisitedFlags(const NatSet& liveSet,
		  const Vector<int>& position,
		  bool state);
  int allocateSlot(const NatSet& liveSet,
		   const Vector<int>& position,
		   Symbol* symbol);
  int buildSlotTranslation(Vector<int>& slotTranslation);

#ifdef COMPILER
  void allocateVariables(int nodeNr);
  void slotMapUnion(Vector<Pair>& to, int fromNodeNr);
  void slotMapInsert(Vector<Pair>& to, int positionIndex, int slot);
  int findSlot(const Vector<Pair>& slotMap, int positionIndex);
  // void deleteSlot(Vector<Pair>& slotMap, int positionIndex);

  void generateNode(CompilationContext& context,
		    int indentLevel,
		    int nodeNr,
		    bool nextRemainder);
  void generatePointer(CompilationContext& context,
		       const Vector<Pair>& slotMap,
		       int positionIndex);
  void generatePointer2(CompilationContext& context,
			const Vector<Pair>& slotMap,
			int positionIndex);
#endif

#ifdef DUMP
  static void dumpNatSet(ostream& s, const NatSet& natSet);
  static void dumpPath(ostream& s, const Vector<int>& path);
  void dumpPositionSet(ostream& s, const NatSet& positionSet) const;
  void dumpSlotMap(ostream& s, const Vector<Pair>& slotMap) const;
#endif

  const bool expandRemainderNodes;  // must be true for full compilation
  Vector<Pattern> patterns;
  Vector<Node> net;
  FreePositionTable positions;
  FreeSymbol* topSymbol;
  int topPositionIndex;
  //
  //	Slot allocation for both semi and full compilers.
  //
  UnionFind slots;
  //
  //	Only used for semi-complier.
  //
  Vector<NatSet> conflicts;
  NatSet patternsUsed;
  //
  //	Only used for full compiler.
  //
  int nrFailParents;
  int nrFailVisits;
  Vector<int> slotTranslation;
};

#endif
