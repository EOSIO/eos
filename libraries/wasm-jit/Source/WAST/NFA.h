#pragma once

#include "Inline/BasicTypes.h"
#include "Inline/DenseStaticIntSet.h"

#include <string>

namespace NFA
{
	// A set of characters.
	typedef DenseStaticIntSet<U8,256> CharSet;

	// An index of a DFA state. A negative index indicates an "accepting" or terminal state.
	typedef I16 StateIndex;

	enum
	{
		edgeDoesntConsumeInputFlag = (StateIndex)0x4000, // A flag that's set on terminal DFA state transitions that don't consume any input
		unmatchedCharacterTerminal = (StateIndex)0x8000, // An implicit terminal state that indicates the DFA didn't recognize the input
		maximumTerminalStateIndex = (StateIndex)0xbfff, // Should be the largest negative number that doesn't have edgeDoesntConsumeInputFlag set.
	};

	// Creates an abstract object that holds the state of an under-construction BFA.
	struct Builder;
	Builder* createBuilder();

	// Functions to add states and edges to the under-construction DFA.
	StateIndex addState(Builder* builder);
	void addEdge(Builder* builder,StateIndex initialState,const CharSet& predicate,StateIndex nextState);
	void addEpsilonEdge(Builder* builder,StateIndex initialState,StateIndex nextState);
	StateIndex getNonTerminalEdge(Builder* builder,StateIndex initialState,char c);

	// Dumps the NFA's states and edges to the GraphViz .dot format.
	std::string dumpNFAGraphViz(const Builder* builder);

	// Encapsulates a NFA that has been translated into a DFA that can be efficiently executed.
	struct Machine
	{
		Machine(): stateAndOffsetToNextStateMap(nullptr), numClasses(0), numStates(0) {}
		~Machine();
		
		Machine(Machine&& inMachine) { moveFrom(std::move(inMachine)); }
		void operator=(Machine&& inMachine) { moveFrom(std::move(inMachine)); }

		// Constructs a DFA from the abstract builder object (which is destroyed).
		Machine(Builder* inBuilder);
		
		// Feeds characters into the DFA until it reaches a terminal state.
		// Upon reaching a terminal state, the state is returned, and the nextChar pointer
		// is updated to point to the first character not consumed by the DFA.
		inline StateIndex feed(const char*& nextChar) const
		{
			Iptr state = 0;
			do
			{
				state = stateAndOffsetToNextStateMap[state + charToOffsetMap[(U8)nextChar[0]]];
				if(state < 0) { nextChar += 1; break; }
				state = stateAndOffsetToNextStateMap[state + charToOffsetMap[(U8)nextChar[1]]];
				if(state < 0) { nextChar += 2; break; }
				state = stateAndOffsetToNextStateMap[state + charToOffsetMap[(U8)nextChar[2]]];
				if(state < 0) { nextChar += 3; break; }
				state = stateAndOffsetToNextStateMap[state + charToOffsetMap[(U8)nextChar[3]]];
				nextChar += 4;
			}
			while(state >= 0);
			if(state & edgeDoesntConsumeInputFlag)
			{
				--nextChar;
				state &= ~edgeDoesntConsumeInputFlag;
			}
			return (StateIndex)state;
		}

		// Dumps the DFA's states and edges to the GraphViz .dot format.
		std::string dumpDFAGraphViz() const;

	private:

		typedef I16 InternalStateIndex;
		enum { internalMaxStates = INT16_MAX };

		U32 charToOffsetMap[256];
		InternalStateIndex* stateAndOffsetToNextStateMap;
		Uptr numClasses;
		Uptr numStates;

		void moveFrom(Machine&& inMachine);
	};
}