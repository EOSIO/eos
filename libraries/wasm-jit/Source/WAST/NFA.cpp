#include "Inline/BasicTypes.h"
#include "Inline/Errors.h"
#include "Inline/Timing.h"
#include "Platform/Platform.h"
#include "NFA.h"
#include <set>
#include <map>
#include <string>
#include <string.h>
#include <algorithm>

namespace NFA
{
	typedef std::vector<StateIndex> StateSet;
	
	template<typename Element>
	void addUnique(std::vector<Element>& vector,const Element& element)
	{
		for(const auto& existingElement : vector)
		{
			if(existingElement == element)
			{
				return;
			}
		}
		vector.push_back(element);
	}

	struct NFAState
	{
		std::map<StateIndex,CharSet> nextStateToPredicateMap;
		std::vector<StateIndex> epsilonNextStates;
	};
	
	struct Builder
	{
		std::vector<NFAState> nfaStates;
	};

	Builder* createBuilder()
	{
		auto builder = new Builder();
		addState(builder);
		return builder;
	}

	StateIndex addState(Builder* builder)
	{
		WAVM_ASSERT_THROW(builder->nfaStates.size() < INT16_MAX);
		builder->nfaStates.emplace_back();
		return StateIndex(builder->nfaStates.size() - 1);
	}

	void addEdge(Builder* builder,StateIndex initialState,const CharSet& predicate,StateIndex nextState)
	{
		CharSet& transitionPredicate = builder->nfaStates[initialState].nextStateToPredicateMap[nextState];
		transitionPredicate = transitionPredicate | predicate;
	}

	void addEpsilonEdge(Builder* builder,StateIndex initialState,StateIndex nextState)
	{
		addUnique(builder->nfaStates[initialState].epsilonNextStates,nextState);
	}

	StateIndex getNonTerminalEdge(Builder* builder,StateIndex initialState,char c)
	{
		for(const auto& nextStateToPredicatePair : builder->nfaStates[initialState].nextStateToPredicateMap)
		{
			if(nextStateToPredicatePair.first >= 0 && nextStateToPredicatePair.second.contains((U8)c))
			{
				return nextStateToPredicatePair.first;
			}
		}

		return unmatchedCharacterTerminal;
	}

	struct DFAState
	{
		StateIndex nextStateByChar[256];
		DFAState()
		{
			for(Uptr charIndex = 0;charIndex < 256;++charIndex)
			{
				nextStateByChar[charIndex] = unmatchedCharacterTerminal | edgeDoesntConsumeInputFlag;
			}
		}
	};

	std::vector<DFAState> convertToDFA(Builder* builder)
	{
		Timing::Timer timer;

		std::vector<DFAState> dfaStates;
		std::map<StateSet,StateIndex> nfaStateSetToDFAStateMap;
		std::vector<StateSet> dfaStateToNFAStateSetMap;
		std::vector<StateIndex> pendingDFAStates;

		nfaStateSetToDFAStateMap.emplace(StateSet {0},(StateIndex)0);
		dfaStateToNFAStateSetMap.emplace_back(StateSet {0});
		dfaStates.emplace_back();
		pendingDFAStates.push_back((StateIndex)0);

		Uptr maxLocalStates = 0;
		Uptr maxDFANextStates = 0;

		while(pendingDFAStates.size())
		{
			const StateIndex currentDFAStateIndex = pendingDFAStates.back();
			pendingDFAStates.pop_back();
		
			const StateSet currentStateSet = dfaStateToNFAStateSetMap[currentDFAStateIndex];

			// Expand the set of current states to include all states reachable by epsilon transitions from the current states.
			StateSet epsilonClosureCurrentStateSet = currentStateSet;
			for(Uptr scanIndex = 0;scanIndex < epsilonClosureCurrentStateSet.size();++scanIndex)
			{
				StateIndex scanState = epsilonClosureCurrentStateSet[scanIndex];
				if(scanState >= 0)
				{
					for(auto epsilonNextState : builder->nfaStates[scanState].epsilonNextStates)
					{
						addUnique(epsilonClosureCurrentStateSet,epsilonNextState);
					}
				}
			}

			// Find the subset of the non-terminal states in the current set.
			StateSet nonTerminalCurrentStateSet;
			StateIndex currentTerminalState = unmatchedCharacterTerminal | edgeDoesntConsumeInputFlag;
			bool hasCurrentTerminalState = false;
			for(auto stateIndex : epsilonClosureCurrentStateSet)
			{
				if(stateIndex >= 0)
				{
					addUnique(nonTerminalCurrentStateSet,stateIndex);
				}
				else
				{
					if(hasCurrentTerminalState)
					{
						Errors::fatalf("NFA has multiple possible terminal states for the same input");
					}
					hasCurrentTerminalState = true;
					currentTerminalState = stateIndex | edgeDoesntConsumeInputFlag;
				}
			}

			// Build a compact index of the states following all states in the current set.
			std::map<StateIndex,StateIndex> stateIndexToLocalStateIndexMap;
			std::vector<StateIndex> localStateIndexToStateIndexMap;
			for(auto stateIndex : nonTerminalCurrentStateSet)
			{
				const NFAState& nfaState = builder->nfaStates[stateIndex];
				for(auto transition : nfaState.nextStateToPredicateMap)
				{
					if(!stateIndexToLocalStateIndexMap.count(transition.first))
					{
						stateIndexToLocalStateIndexMap.emplace(transition.first,(StateIndex)localStateIndexToStateIndexMap.size());
						localStateIndexToStateIndexMap.emplace_back(transition.first);
					}
				}
			}

			if(!stateIndexToLocalStateIndexMap.count(currentTerminalState))
			{
				stateIndexToLocalStateIndexMap.emplace(currentTerminalState,(StateIndex)localStateIndexToStateIndexMap.size());
				localStateIndexToStateIndexMap.emplace_back(currentTerminalState);
			}
		
			enum { numSupportedLocalStates = 64 };
			typedef DenseStaticIntSet<StateIndex,numSupportedLocalStates> LocalStateSet;

			const Uptr numLocalStates = stateIndexToLocalStateIndexMap.size();
			WAVM_ASSERT_THROW(numLocalStates <= numSupportedLocalStates);
			maxLocalStates = std::max<Uptr>(maxLocalStates,numLocalStates);

			// Combine the [nextState][char] transition maps for current states and transpose to [char][nextState]
			// After building the compact index of referenced states, the nextState set can be represented as a 64-bit mask.
			LocalStateSet charToLocalStateSet[256];
			for(auto stateIndex : nonTerminalCurrentStateSet)
			{
				const NFAState& nfaState = builder->nfaStates[stateIndex];
				for(auto transition : nfaState.nextStateToPredicateMap)
				{
					for(Uptr charIndex = 0;charIndex < 256;++charIndex)
					{
						if(transition.second.contains((char)charIndex))
						{
							charToLocalStateSet[charIndex].add(stateIndexToLocalStateIndexMap.at(transition.first));
						}
					}
				}
			}
		
			const LocalStateSet currentTerminalStateLocalSet(stateIndexToLocalStateIndexMap.at(currentTerminalState));
			for(Uptr charIndex = 0;charIndex < 256;++charIndex)
			{
				if(charToLocalStateSet[charIndex].isEmpty())
				{
					charToLocalStateSet[charIndex] = currentTerminalStateLocalSet;
				}
			}

			// Find the set of unique local state sets that follow this state set.
			std::vector<LocalStateSet> uniqueLocalNextStateSets;
			for(Uptr charIndex = 0;charIndex < 256;++charIndex)
			{
				const LocalStateSet localStateSet = charToLocalStateSet[charIndex];
				if(!localStateSet.isEmpty())
				{
					addUnique(uniqueLocalNextStateSets,localStateSet);
				}
			}

			// For each unique local state set that follows this state set, find or create a corresponding DFA state.
			std::map<LocalStateSet,StateIndex> localStateSetToDFAStateIndexMap;
			for(auto localNextStateSet : uniqueLocalNextStateSets)
			{
				// Convert the local state set bit mask to a global NFA state set.
				StateSet nextStateSet;
				{
					LocalStateSet residualLocalStateSet = localNextStateSet;
					while(true)
					{
						const StateIndex localStateIndex = residualLocalStateSet.getSmallestMember();
						if(localStateIndex == numSupportedLocalStates) { break; }

						nextStateSet.push_back(localStateIndexToStateIndexMap.at(localStateIndex));
						residualLocalStateSet.remove(localStateIndex);
					};
				}

				if(nextStateSet.size() == 1 && *nextStateSet.begin() < 0)
				{
					localStateSetToDFAStateIndexMap.insert(std::make_pair(localNextStateSet,*nextStateSet.begin()));
				}
				else
				{
					// Find an existing DFA state corresponding to this NFA state set.
					auto nextDFAStateIt = nfaStateSetToDFAStateMap.find(nextStateSet);
					StateIndex nextDFAStateIndex;
					if(nextDFAStateIt != nfaStateSetToDFAStateMap.end())
					{
						localStateSetToDFAStateIndexMap.emplace(localNextStateSet,nextDFAStateIt->second);
					}
					else
					{
						// If no corresponding DFA state existing yet, create a new one and add it to the queue
						// of pending states to process.
						nextDFAStateIndex = (StateIndex)dfaStates.size();
						localStateSetToDFAStateIndexMap.emplace(localNextStateSet,nextDFAStateIndex);
						nfaStateSetToDFAStateMap.insert(std::make_pair(nextStateSet,nextDFAStateIndex));
						dfaStateToNFAStateSetMap.emplace_back(nextStateSet);
						dfaStates.emplace_back();
						pendingDFAStates.push_back(nextDFAStateIndex);
					}
				}			
			}

			// Set up the DFA transition map.
			DFAState& dfaState = dfaStates[nfaStateSetToDFAStateMap[currentStateSet]];
			for(auto localStateSetToDFAStateIndex : localStateSetToDFAStateIndexMap)
			{
				const LocalStateSet localStateSet = localStateSetToDFAStateIndex.first;
				const StateIndex nextDFAStateIndex = localStateSetToDFAStateIndex.second;
				for(Uptr charIndex = 0;charIndex < 256;++charIndex)
				{
					if(charToLocalStateSet[charIndex] == localStateSet)
					{
						dfaState.nextStateByChar[charIndex] = nextDFAStateIndex;
					}
				}
			}
			maxDFANextStates = std::max(maxDFANextStates,(Uptr)uniqueLocalNextStateSets.size());
		};
		
		Timing::logTimer("translated NFA->DFA",timer);
		Log::printf(Log::Category::metrics,"  translated NFA with %u states to DFA with %u states\n",builder->nfaStates.size(),dfaStates.size());
		Log::printf(Log::Category::metrics,"  maximum number of states following a NFA state set: %u\n",maxLocalStates);
		Log::printf(Log::Category::metrics,"  maximum number of states following a DFA state: %u\n",maxDFANextStates);

		return dfaStates;
	}

	struct StateTransitionsByChar
	{
		U8 c;
		StateIndex* nextStateByInitialState;
		Uptr numStates;
		StateTransitionsByChar(U8 inC,Uptr inNumStates): c(inC), nextStateByInitialState(nullptr), numStates(inNumStates) {}
		StateTransitionsByChar(StateTransitionsByChar&& inMove)
		: c(inMove.c), nextStateByInitialState(inMove.nextStateByInitialState), numStates(inMove.numStates)
		{
			inMove.nextStateByInitialState = nullptr;
		}
		~StateTransitionsByChar() { if(nextStateByInitialState) { delete [] nextStateByInitialState; } }

		void operator=(StateTransitionsByChar&& inMove)
		{
			c = inMove.c;
			nextStateByInitialState = inMove.nextStateByInitialState;
			numStates = inMove.numStates;
			inMove.nextStateByInitialState = nullptr;
		}

		bool operator<(const StateTransitionsByChar& right) const
		{
			WAVM_ASSERT_THROW(numStates == right.numStates);
			return memcmp(nextStateByInitialState,right.nextStateByInitialState,sizeof(StateIndex)*numStates) < 0;
		}
		bool operator!=(const StateTransitionsByChar& right) const
		{
			WAVM_ASSERT_THROW(numStates == right.numStates);
			return memcmp(nextStateByInitialState,right.nextStateByInitialState,sizeof(StateIndex)*numStates) != 0;
		}
	};
	
	Machine::Machine(Builder* builder)
	{
		// Convert the NFA constructed by the builder to a DFA.
		std::vector<DFAState> dfaStates = convertToDFA(builder);
		WAVM_ASSERT_THROW(dfaStates.size() <= internalMaxStates);
		delete builder;
		
		Timing::Timer timer;

		// Transpose the [state][character] transition map to [character][state].
		std::vector<StateTransitionsByChar> stateTransitionsByChar;
		for(Uptr charIndex = 0;charIndex < 256;++charIndex)
		{
			stateTransitionsByChar.push_back(StateTransitionsByChar((U8)charIndex,dfaStates.size()));
			stateTransitionsByChar[charIndex].nextStateByInitialState = new StateIndex[dfaStates.size()];
			for(Uptr stateIndex = 0;stateIndex < dfaStates.size();++stateIndex)
			{ stateTransitionsByChar[charIndex].nextStateByInitialState[stateIndex] = dfaStates[stateIndex].nextStateByChar[charIndex]; }
		}

		// Sort the [character][state] transition map by the [state] column.
		numStates = dfaStates.size();
		std::sort(stateTransitionsByChar.begin(),stateTransitionsByChar.end());

		// Build a minimal set of character equivalence classes that have the same transition across all states.
		U8 characterToClassMap[256];
		U8 representativeCharsByClass[256];
		numClasses = 1;
		characterToClassMap[stateTransitionsByChar[0].c] = 0;
		representativeCharsByClass[0] = stateTransitionsByChar[0].c;
		for(Uptr charIndex = 1;charIndex < stateTransitionsByChar.size();++charIndex)
		{
			if(stateTransitionsByChar[charIndex] != stateTransitionsByChar[charIndex-1])
			{
				representativeCharsByClass[numClasses] = stateTransitionsByChar[charIndex].c;
				++numClasses;
			}
			characterToClassMap[stateTransitionsByChar[charIndex].c] = U8(numClasses - 1);
		}

		// Build a [charClass][state] transition map.
		stateAndOffsetToNextStateMap = new InternalStateIndex[numClasses * numStates];
		for(Uptr classIndex = 0;classIndex < numClasses;++classIndex)
		{
			for(Uptr stateIndex = 0;stateIndex < numStates;++stateIndex)
			{
				stateAndOffsetToNextStateMap[stateIndex + classIndex * numStates] = InternalStateIndex(dfaStates[stateIndex].nextStateByChar[representativeCharsByClass[classIndex]]);
			}
		}

		// Build a map from character index to offset into [charClass][initialState] transition map.
		WAVM_ASSERT_THROW((numClasses - 1) * (numStates - 1) <= UINT32_MAX);
		for(Uptr charIndex = 0;charIndex < 256;++charIndex)
		{
			charToOffsetMap[charIndex] = U32(numStates * characterToClassMap[charIndex]);
		}

		Timing::logTimer("reduced DFA character classes",timer);
		Log::printf(Log::Category::metrics,"  reduced DFA character classes to %u\n",numClasses);
	}

	Machine::~Machine()
	{
		if(stateAndOffsetToNextStateMap)
		{
			delete [] stateAndOffsetToNextStateMap;
			stateAndOffsetToNextStateMap = nullptr;
		}
	}

	void Machine::moveFrom(Machine&& inMachine)
	{
		memcpy(charToOffsetMap,inMachine.charToOffsetMap,sizeof(charToOffsetMap));
		stateAndOffsetToNextStateMap = inMachine.stateAndOffsetToNextStateMap;
		inMachine.stateAndOffsetToNextStateMap = nullptr;
		numClasses = inMachine.numClasses;
		numStates = inMachine.numStates;
	}

	char nibbleToHexChar(U8 value) { return value < 10 ? ('0' + value) : 'a' + value - 10; }

	std::string escapeString(const std::string& string)
	{
		std::string result;
		for(Uptr charIndex = 0;charIndex < string.size();++charIndex)
		{
			auto c = string[charIndex];
			if(c == '\\') { result += "\\\\"; }
			else if(c == '\"') { result += "\\\""; }
			else if(c == '\n') { result += "\\n"; }
			else if(c < 0x20 || c > 0x7e)
			{
				result += "\\\\";
				result += nibbleToHexChar((c & 0xf0) >> 4);
				result += nibbleToHexChar((c & 0x0f) >> 0);
			}
			else { result += c; }
		}
		return result;
	}

	std::string getGraphEdgeCharLabel(Uptr charIndex)
	{
		switch(charIndex)
		{
		case '^': return "\\^";
		case '\f': return "\\f";
		case '\r': return "\\r";
		case '\n': return "\\n";
		case '\t': return "\\t";
		case '\'': return "\\\'";
		case '\"': return "\\\"";
		case '\\': return "\\\\";
		case '-': return "\\-";
		default: return std::string(1,(char)charIndex);
		};
	}

	std::string getGraphEdgeLabel(const CharSet& charSet)
	{
		std::string edgeLabel;
		U8 numSetChars = 0;
		for(Uptr charIndex = 0;charIndex < 256;++charIndex)
		{
			if(charSet.contains((char)charIndex)) { ++numSetChars; }
		}
		if(numSetChars > 1) { edgeLabel += "["; }
		const bool isNegative = numSetChars >= 100;
		if(isNegative) { edgeLabel += "^"; }
		for(Uptr charIndex = 0;charIndex < 256;++charIndex)
		{
			if(charSet.contains((char)charIndex) != isNegative)
			{
				edgeLabel += getGraphEdgeCharLabel(charIndex);
				if(charIndex + 2 < 256
				&& charSet.contains((char)charIndex+1) != isNegative
				&& charSet.contains((char)charIndex+2) != isNegative)
				{
					edgeLabel += "-";
					charIndex += 2;
					while(charIndex + 1 < 256 && charSet.contains((char)charIndex+1) != isNegative) { ++charIndex; }
					edgeLabel += getGraphEdgeCharLabel(charIndex);
				}
			}
		}
		if(numSetChars > 1) { edgeLabel += "]"; }
		return edgeLabel;
	}

	std::string dumpNFAGraphViz(const Builder* builder)
	{
		std::string result;
		result += "digraph {\n";
		std::set<StateIndex> terminalStates;
		for(Uptr stateIndex = 0;stateIndex < builder->nfaStates.size();++stateIndex)
		{
			const NFAState& nfaState = builder->nfaStates[stateIndex];

			result += "state" + std::to_string(stateIndex) + "[shape=square label=\"" + std::to_string(stateIndex) + "\"];\n";

			for(const auto& statePredicatePair : nfaState.nextStateToPredicateMap)
			{
				std::string edgeLabel = getGraphEdgeLabel(statePredicatePair.second);
				std::string nextStateName = statePredicatePair.first < 0
					? "terminal" + std::to_string(-statePredicatePair.first)
					: "state" + std::to_string(statePredicatePair.first);
				result += "state" + std::to_string(stateIndex)
					+ " -> "
					+ nextStateName + "[label=\"" + escapeString(edgeLabel) + "\"];\n";
				if(statePredicatePair.first < 0)
				{
					terminalStates.emplace(statePredicatePair.first);
				}
			}

			for(auto epsilonNextState : nfaState.epsilonNextStates)
			{
				std::string nextStateName = epsilonNextState < 0
					? "terminal" + std::to_string(-epsilonNextState)
					: "state" + std::to_string(epsilonNextState);
				result += "state" + std::to_string(stateIndex) + " -> " + nextStateName + "[label=\"&epsilon;\"];\n";
			}
		}
		for(auto terminalState : terminalStates)
		{
			result += "terminal" + std::to_string(-terminalState)
				+ "[shape=octagon label=\"" + std::to_string(maximumTerminalStateIndex - terminalState) + "\"];\n";
		}
		result += "}\n";
		return result;
	}

	std::string Machine::dumpDFAGraphViz() const
	{
		std::string result;
		result += "digraph {\n";
		std::set<StateIndex> terminalStates;

		CharSet* classCharSets = (CharSet*)alloca(sizeof(CharSet) * numClasses);
		memset(classCharSets,0,sizeof(CharSet) * numClasses);
		for(Uptr charIndex = 0;charIndex < 256;++charIndex)
		{
			const Uptr classIndex = charToOffsetMap[charIndex] / numStates;
			classCharSets[classIndex].add((U8)charIndex);
		}
	
		{
			std::map<StateIndex,CharSet> transitions;
			for(Uptr classIndex = 0;classIndex < numClasses;++classIndex)
			{
				const InternalStateIndex nextState = stateAndOffsetToNextStateMap[0 + classIndex * numStates];
				CharSet& transitionPredicate = transitions[nextState];
				transitionPredicate = classCharSets[classIndex] | transitionPredicate;
			}

			Uptr startIndex = 0;
			for(auto transitionPair : transitions)
			{
				if((transitionPair.first & ~edgeDoesntConsumeInputFlag) != unmatchedCharacterTerminal)
				{
					result += "start" + std::to_string(startIndex) + "[shape=triangle label=\"\"];\n";

					std::string edgeLabel = getGraphEdgeLabel(transitionPair.second);
					std::string nextStateName = transitionPair.first < 0
						? "terminal" + std::to_string(-(transitionPair.first & ~edgeDoesntConsumeInputFlag))
						: "state" + std::to_string(transitionPair.first);
					result += "start" + std::to_string(startIndex)
						+ " -> "
						+ nextStateName + "[label=\""
						+ (transitionPair.first < 0 && (transitionPair.first & edgeDoesntConsumeInputFlag) != 0 ? "&epsilon; " : "")
						+ escapeString(edgeLabel) + "\"];\n";

					if(transitionPair.first < 0)
					{
						terminalStates.emplace(StateIndex(transitionPair.first & ~edgeDoesntConsumeInputFlag));
					}

					++startIndex;
				}
			}
		}

		for(Uptr stateIndex = 1;stateIndex < numStates;++stateIndex)
		{
			result += "state" + std::to_string(stateIndex) + "[shape=square label=\"" + std::to_string(stateIndex) + "\"];\n";

			std::map<StateIndex,CharSet> transitions;
			for(Uptr classIndex = 0;classIndex < numClasses;++classIndex)
			{
				const InternalStateIndex nextState = stateAndOffsetToNextStateMap[stateIndex + classIndex * numStates];
				CharSet& transitionPredicate = transitions[nextState];
				transitionPredicate = classCharSets[classIndex] | transitionPredicate;
			}

			for(auto transitionPair : transitions)
			{
				if((transitionPair.first & ~edgeDoesntConsumeInputFlag) != unmatchedCharacterTerminal)
				{
					std::string edgeLabel = getGraphEdgeLabel(transitionPair.second);
					std::string nextStateName = transitionPair.first < 0
						? "terminal" + std::to_string(-(transitionPair.first & ~edgeDoesntConsumeInputFlag))
						: "state" + std::to_string(transitionPair.first);
					result += "state" + std::to_string(stateIndex)
						+ " -> "
						+ nextStateName + "[label=\""
						+ (transitionPair.first < 0 && (transitionPair.first & edgeDoesntConsumeInputFlag) != 0 ? "&epsilon; " : "")
						+ escapeString(edgeLabel) + "\"];\n";

					if(transitionPair.first < 0)
					{
						terminalStates.emplace(transitionPair.first);
					}
				}
			}
		}
		for(auto terminalState : terminalStates)
		{
			result += "terminal" + std::to_string(-(terminalState & ~edgeDoesntConsumeInputFlag))
				+ "[shape=octagon label=\"" + std::to_string(maximumTerminalStateIndex - (terminalState & ~edgeDoesntConsumeInputFlag)) + "\"];\n";
		}
		result += "}\n";
		return result;
	}
}