//
//	Code for loop mode.
//

void
Interpreter::loop(const Vector<Token>& subject)
{
  if (DagNode* d = makeDag(subject))
    {
      savedLoopSubject = subject;  // deep copy
      VisibleModule* fm = currentModule->getFlatModule();
      startUsingModule(fm);
      doLoop(d, fm);
    }
}

bool
Interpreter::contLoop2(const Vector<Token>& input)
{
  if (savedContext != 0)
    {
      DagNode* d = savedContext->root();
      if (LoopSymbol* l = dynamic_cast<LoopSymbol*>(d->symbol()))
	{
	  VisibleModule* fm = savedModule;
	  delete savedContext;
	  savedContext = 0;
	  savedModule = 0;
	  continueFunc = 0;
	  doLoop(l->injectInput(d, input), fm);
	  return true;
	}
      else
	IssueWarning("bad loop state.");
    }
  else
    IssueWarning("no loop state.");
  return false;
}

void
Interpreter::contLoop(const Vector<Token>& input)
{
  if (!contLoop2(input) && savedLoopSubject.length() != 0)
    {
      Vector<Token> savedInput(input);  // in case input gets overwritten, say in debugger
      IssueAdvisory("attempting to reinitialize loop.");
      if (DagNode* d = makeDag(savedLoopSubject))
	{
	  VisibleModule* fm = currentModule->getFlatModule();
	  startUsingModule(fm);
	  doLoop(d, fm);
	  if (contLoop2(savedInput))
	    return;
	}
      IssueAdvisory("unable to reinitialize loop.");
    }
}

void
Interpreter::doLoop(DagNode* d, VisibleModule* module)
{
  UserLevelRewritingContext* context = new UserLevelRewritingContext(d);
  module->resetRules();
#ifdef QUANTIFY_REWRITING
  quantify_start_recording_data();
#endif
  Timer timer(getFlag(SHOW_LOOP_TIMING));
  context->ruleRewrite(NONE);
  timer.stop();
#ifdef QUANTIFY_REWRITING
  quantify_stop_recording_data();
#endif
  clearContinueInfo();
  if (UserLevelRewritingContext::aborted())
    {
      delete context;
      (void) module->unprotect();
    }
  else
    {
      if (getFlag(SHOW_LOOP_STATS))
	printStats(timer, *context, getFlag(SHOW_LOOP_TIMING));
      DagNode* r = context->root();
      if (LoopSymbol* l = dynamic_cast<LoopSymbol*>(r->symbol()))
	{
	  Vector<int> bubble;
	  l->extractOutput(r, bubble);
	  printBubble(cout, bubble);
	}
      else
	cout << "non-loop result " << r->getSort() << ": " << r << '\n';
      cout.flush();

      savedContext = context;
      savedModule = module;
      continueFunc = &Interpreter::rewriteCont;
    }
  UserLevelRewritingContext::clearDebug();  // even if we didn't start in debug mode
}

void
Interpreter::printBubble(ostream& s, const Vector<int>& bubble)
{
  bool ansiActive = false;
  bool needSpace = false;
  int nrTokens = bubble.length();
  for (int i = 0; i < nrTokens; i++)
    {
      const char* n = Token::name(bubble[i]);
      //
      //	Single character case.
      //
      if (n[1] == 0)
	{
	  switch (n[0])
	    {
	    case '(':	case ')':	case '[':	case ']':
	    case '{':	case '}':	case ',':
	      needSpace = false;
	      break;
	    default:
	      if (needSpace)
		s << ' ';
	      needSpace = true;
	    }
	  s << n[0];
	  continue;
	}
      //
      //	First character backslash case.
      //
      if (n[0] == '\\')
	{
	  if (n[2] == 0)
	    {
	      switch (n[1])
		{
		case 'n':
		  {
		    s << '\n';
		    needSpace = false;
		    break;
		  }
		case 't':
		  {
		    s << '\t';
		    needSpace = false;
		    break;
		  }
		case 's':
		  {
		    s << ' ';
		    needSpace = false;
		    break;
		  }
		case '\\':
		  {
		    if (needSpace)
		      s << ' ';
		    s << '\\';
		    needSpace = true;
		    break;
		  }
		  //
		  //	ANSI stuff.
		  //
#define MACRO(m, t) \
case m: { s << Tty(Tty::t); ansiActive = true; break; }
#include "ansiEscapeSequences.cc"
#undef MACRO
		case 'o':
		  {
		    s << Tty(Tty::RESET);
		    ansiActive = false;
		    break;
		  }
		}
	      continue;
	    }
	  else if (n[1] == '`' &&  n[3] == 0)
	    {
	      switch (n[2])
		{
		case '(':
		case ')':
		case '[':
		case ']':
		case '{':
		case '}':
		case ',':
		  if (needSpace)
		    s << ' ';
		  s << n[2];
		  needSpace = true;
		  continue;
		}
	    }
	}
      //
      //	Normal case.
      //
      if (needSpace)
	s << ' ';
      s << n;
      needSpace = true;
    }
  if (ansiActive)
    s << Tty(Tty::RESET);
  s << '\n';
}
