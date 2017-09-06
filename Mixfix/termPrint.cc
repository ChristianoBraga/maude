//
//	Term* -> ostream& pretty printer.
//

const char*
MixfixModule::computeColor(SymbolType st)
{
  if (interpreter.getFlag(Interpreter::PRINT_COLOR))
    {
      if (st.hasFlag(SymbolType::ASSOC))
	{
	  if (st.hasFlag(SymbolType::COMM))
	    {
	      return Tty(st.hasFlag(SymbolType::LEFT_ID | SymbolType::RIGHT_ID) ?
			 Tty::MAGENTA : Tty::RED).ctrlSequence();
	    }
	  else
	    {
	      return Tty(st.hasFlag(SymbolType::LEFT_ID | SymbolType::RIGHT_ID) ?
			 Tty::CYAN : Tty::GREEN).ctrlSequence();
	    }
	}
      if (st.hasFlag(SymbolType::COMM))
	return Tty(Tty::BLUE).ctrlSequence();
      if (st.hasFlag(SymbolType::LEFT_ID | SymbolType::RIGHT_ID | SymbolType::IDEM))
	return Tty(Tty::YELLOW).ctrlSequence();
    }
  return 0;
}

/*
void
MixfixModule::prefix(ostream& s, bool needDisambig)
{
  if (needDisambig)
    s << '(';
}
*/

void
MixfixModule::suffix(ostream& s,
		     Term* term,
		     bool needDisambig,
		     const char* color)
{
  if (needDisambig)
    {
      Symbol* symbol = term->symbol();
      int sortIndex = term->getSortIndex();
      if (sortIndex <= Sort::ERROR_SORT)
	sortIndex = chooseDisambiguator(symbol);
      s << ")." << symbol->rangeComponent()->sort(sortIndex);
    }
}

bool
MixfixModule::handleIter(ostream& s,
			 Term* term,
			 SymbolInfo& si,
			 bool rangeKnown,
			 const char* color)
{
  if (!(si.symbolType.hasFlag(SymbolType::ITER)))
    return false;
  if (si.symbolType.getBasicType() == SymbolType::SUCC_SYMBOL &&
      interpreter.getFlag(Interpreter::PRINT_NUMBER))
    {
      SuccSymbol* succSymbol = safeCast(SuccSymbol*, term->symbol());
      if (succSymbol->isNat(term))
	{
	  const mpz_class& nat = succSymbol->getNat(term);
	  bool needDisambig = !rangeKnown &&
	    (kindsWithSucc.size() > 1 || overloadedIntegers.count(nat));
	  prefix(s, needDisambig, color);
	  s << succSymbol->getNat(term);
	  suffix(s, term, needDisambig, color);
	  return true;
	}
    }
  S_Term* st = safeCast(S_Term*, term);
  const mpz_class& number = st->getNumber();
  if (number == 1)
    return false;  // do default thing

  // NEED TO FIX: disambig
  string prefixName;
  makeIterName(prefixName, term->symbol()->id(), number);
  if (color != 0)
    s << color << prefixName << Tty(Tty::RESET);
  else
    printPrefixName(s, prefixName.c_str(), si);
  s << '(';
  prettyPrint(s, st->getArgument(),
	      PREFIX_GATHER, UNBOUNDED, 0, UNBOUNDED, 0, rangeKnown);
  s << ')';
  return true;
}

bool
MixfixModule::handleMinus(ostream& s,
			  Term* term,
			  bool rangeKnown,
			  const char* color)
{
  if (interpreter.getFlag(Interpreter::PRINT_NUMBER))
    {
      const MinusSymbol* minusSymbol = safeCast(MinusSymbol*, term->symbol());
      if (minusSymbol->isNeg(term))
	{
	  mpz_class neg;
	  (void) minusSymbol->getNeg(term, neg);
	  bool needDisambig = !rangeKnown &&
	    (kindsWithMinus.size() > 1 || overloadedIntegers.count(neg));
	  prefix(s, needDisambig, color);
	  s << neg;
	  suffix(s, term, needDisambig, color);
	  return true;
	}
    }
  return false;
}

bool
MixfixModule::handleDivision(ostream& s,
			     Term* term,
			     bool rangeKnown,
			     const char* color)
{
  if (interpreter.getFlag(Interpreter::PRINT_RAT))
    {
      const DivisionSymbol* divisionSymbol = safeCast(DivisionSymbol*, term->symbol());
      if (divisionSymbol->isRat(term))
	{
	  pair<mpz_class, mpz_class> rat;
	  rat.second = divisionSymbol->getRat(term, rat.first);
	  bool needDisambig = !rangeKnown &&
	    (kindsWithDivision.size() > 1 || overloadedRationals.count(rat));
	  prefix(s, needDisambig, color);
	  s << rat.first << '/' << rat.second;
	  suffix(s, term, needDisambig, color);
	  return true;
	}
    }
  return false;
}

void
MixfixModule::handleFloat(ostream& s,
			  Term* term,
			  bool rangeKnown,
			  const char* color)
{
  double mfValue = safeCast(FloatTerm*, term)->getValue();
  bool needDisambig = !rangeKnown &&
    (floatSymbols.size() > 1 || overloadedFloats.count(mfValue));
  prefix(s, needDisambig, color);
  s << doubleToString(mfValue);
  suffix(s, term, needDisambig, color);
}

void
MixfixModule::handleString(ostream& s,
			   Term* term,
			   bool rangeKnown,
			   const char* color)
{
  string strValue;
  Token::ropeToString(safeCast(StringTerm*, term)->getValue(), strValue);
  bool needDisambig = !rangeKnown &&
    (stringSymbols.size() > 1 || overloadedStrings.count(strValue));
  prefix(s, needDisambig, color);
  s << strValue;
  suffix(s, term, needDisambig, color);
}

void
MixfixModule::handleQuotedIdentifier(ostream& s,
				     Term* term,
				     bool rangeKnown,
				     const char* color)
{
  int qidCode = safeCast(QuotedIdentifierTerm*, term)->getIdIndex();
  bool needDisambig = !rangeKnown &&
    (quotedIdentifierSymbols.size() > 1 ||
     overloadedQuotedIdentifiers.count(qidCode));
  prefix(s, needDisambig, color);
  s << '\'' << Token::name(qidCode);
  suffix(s, term, needDisambig, color);
}

void
MixfixModule::handleVariable(ostream& s,
			     Term* term,
			     bool rangeKnown,
			     const char* color)
{
  VariableTerm* v = safeCast(VariableTerm*, term);
  Sort* sort = safeCast(VariableSymbol*, term->symbol())->getSort();
  pair<int, int> p(v->id(), sort->id());
  bool needDisambig = !rangeKnown && overloadedVariables.count(p);  // kinds not handled
  prefix(s, needDisambig, color);
  printVariable(s, p.first, sort);
  suffix(s, term, needDisambig, color);
}

void
MixfixModule::prettyPrint(ostream& s,
			  Term* term,
			  int requiredPrec,
			  int leftCapture,
			  const ConnectedComponent* leftCaptureComponent,
			  int rightCapture,
			  const ConnectedComponent* rightCaptureComponent,
			  bool rangeKnown)
{
  if (UserLevelRewritingContext::interrupted())
    return;

  Symbol* symbol = term->symbol();
  SymbolInfo& si = symbolInfo[symbol->getIndexWithinModule()];
  const char* color = computeColor(si.symbolType);
  //
  //	Check for special i/o representation.
  //
  if (handleIter(s, term, si, rangeKnown, color))
    return;
  int basicType = si.symbolType.getBasicType();
  switch (basicType)
    {
    case SymbolType::MINUS_SYMBOL:
      {
	if (handleMinus(s, term, rangeKnown, color))
	  return;
	break;
      }
    case SymbolType::DIVISION_SYMBOL:
      {
	if (handleDivision(s, term, rangeKnown, color))
	  return;
	break;
      }
    case SymbolType::FLOAT:
      {
	handleFloat(s, term, rangeKnown, color);
	return;
      }
    case SymbolType::STRING:
      {
	handleString(s, term, rangeKnown, color);
	return;
      }
    case SymbolType::QUOTED_IDENTIFIER:
      {
	handleQuotedIdentifier(s, term, rangeKnown, color);
	return;
      }
    case SymbolType::VARIABLE:
      {
	handleVariable(s, term, rangeKnown, color);
	return;
      }
    default:
      break;
    }
  //
  //	Default case where no special i/o representation applies.
  //
  int iflags = si.iflags;
  bool needDisambig = !rangeKnown && ambiguous(iflags);
  bool argRangeKnown = !(iflags & ADHOC_OVERLOADED) ||
    (!(iflags & RANGE_OVERLOADED) && (rangeKnown || needDisambig));
  int nrArgs = symbol->arity();
  if (needDisambig)
    s << '(';
  if ((printMixfix && si.mixfixSyntax.length() != 0) ||
      basicType == SymbolType::SORT_TEST)
    {
      bool needParen = !needDisambig &&
	(printWithParens || requiredPrec < si.prec ||
	 ((iflags & LEFT_BARE) && leftCapture <= si.gather[0] &&
	  leftCaptureComponent == symbol->domainComponent(0)) ||
	 ((iflags & RIGHT_BARE) && rightCapture <= si.gather[nrArgs - 1] &&
	  rightCaptureComponent == symbol->domainComponent(nrArgs - 1)));
      bool needAssocParen = si.symbolType.hasFlag(SymbolType::ASSOC) &&
	(printWithParens || si.gather[1] < si.prec ||
	 ((iflags & LEFT_BARE) && (iflags & RIGHT_BARE) &&
	  si.prec <= si.gather[0]));
      if (needParen)
	s << '(';
      int nrTails = 1;
      int pos = 0;
      ArgumentIterator a(*term);
      int moreArgs = a.valid();
      for (int arg = 0; moreArgs; arg++)
	{
	  Term* t = a.argument();
	  a.next();
	  moreArgs = a.valid();
	  pos = printTokens(s, si, pos, color);
	  if (arg == nrArgs - 1 && moreArgs)
	    {
	      ++nrTails;
	      arg = 0;
	      if (needAssocParen)
		s << '(';
	      pos = printTokens(s, si, 0, color);
	    }
	  int lc = UNBOUNDED;
	  const ConnectedComponent* lcc = 0;
	  int rc = UNBOUNDED;
	  const ConnectedComponent* rcc = 0;
	  if (arg == 0 && (iflags & LEFT_BARE))
	    {
	      rc = si.prec;
	      rcc = symbol->domainComponent(0);
	      if (!needParen && !needDisambig)
		{
		  lc = leftCapture;
		  lcc = leftCaptureComponent;
		}
	    }
	  else if (!moreArgs && (iflags & RIGHT_BARE))
	    {
	      lc = si.prec;
	      lcc = symbol->domainComponent(nrArgs - 1);
	      if (!needParen && !needDisambig)
		{
		  rc = rightCapture;
		  rcc = rightCaptureComponent;
		}
	    }
	  prettyPrint(s, t, si.gather[arg], lc, lcc, rc, rcc, argRangeKnown);
	  if (UserLevelRewritingContext::interrupted())
	    return;
	}
      printTails(s, si, pos, nrTails, needAssocParen, true, color);
      if (UserLevelRewritingContext::interrupted())
	return;
      if (needParen)
	s << ')';
    }
  else
    {
      const char* prefixName = Token::name(symbol->id());
      if (color != 0)
	s << color << prefixName << Tty(Tty::RESET);
      else
	printPrefixName(s, prefixName, si);
      ArgumentIterator a(*term);
      if (a.valid())
	{
	  int nrTails = 1;
	  s << '(';
	  for (int arg = 0;; arg++)
	    {
	      Term* t = a.argument();
	      a.next();
	      int moreArgs = a.valid();
	      if (arg >= nrArgs - 1 && !printFlat && moreArgs)
		{
		  ++nrTails;
		  printPrefixName(s, prefixName, si);
		  s << '(';
		}
	      prettyPrint(s, t,
			  PREFIX_GATHER, UNBOUNDED, 0, UNBOUNDED, 0,
			  argRangeKnown);
	      if (UserLevelRewritingContext::interrupted())
		return;
	      if (!moreArgs)
		break;
	      s << ", ";
	    }
	  while (nrTails-- > 0)
	    {
	      if (UserLevelRewritingContext::interrupted())
		return;
	      s << ')';
	    }
	}
    }
  if (needDisambig)
    {
      int sortIndex = term->getSortIndex();
      if (sortIndex <= Sort::ERROR_SORT)
	sortIndex = chooseDisambiguator(symbol);
      s << ")." << symbol->rangeComponent()->sort(sortIndex);
    }
}
