#pragma once

#include "Inline/BasicTypes.h"
#include "NFA.h"

namespace Regexp
{
	// Parses a regular expression from a string, and adds a recognizer for it to the given NFA
	// The recognizer will start from initialState, and end in finalState when the regular expression has been completely matched.
	void addToNFA(const char* regexpString,NFA::Builder* nfaBuilder,NFA::StateIndex initialState,NFA::StateIndex finalState);
};
