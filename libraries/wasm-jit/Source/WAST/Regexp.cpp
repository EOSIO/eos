#include "Inline/BasicTypes.h"
#include "Inline/Errors.h"
#include "Regexp.h"
#include "NFA.h"

#include <assert.h>

enum class NodeType : U8
{
	lit,
	zeroOrMore,
	oneOrMore,
	optional,
	alt,
	seq,
};
struct Node
{
	const NodeType type;
	Node(NodeType inType): type(inType) {}
	virtual ~Node() {}
};
struct Lit : Node
{
	NFA::CharSet charSet;
	Lit(const NFA::CharSet& inCharSet): Node(NodeType::lit), charSet(inCharSet) {}
};
template<NodeType inType>
struct Unary : Node
{
	Node* child;
	Unary(Node* inChild): Node(inType), child(inChild) {}
	~Unary() { delete child; }
};
template<NodeType inType>
struct Binary : Node
{
	Node* firstChild;
	Node* secondChild;
	Binary(Node* inFirstChild,Node* inSecondChild)
		: Node(inType)
		, firstChild(inFirstChild)
		, secondChild(inSecondChild)
	{}
	~Binary()
	{
		delete firstChild;
		delete secondChild;
	}
};
typedef Unary<NodeType::zeroOrMore> ZeroOrMore;
typedef Unary<NodeType::oneOrMore> OneOrMore;
typedef Unary<NodeType::optional> Optional;
typedef Binary<NodeType::alt> Alt;
typedef Binary<NodeType::seq> Seq;

template<bool inSet>
static bool isMetachar(char c)
{
	switch(c)
	{
	case '^': case '\\':
		return true;
	case '-': case ']':
		return inSet;
	case '$': case '.':
	case '|': case '*': case '+': case '?':
	case '(': case ')': case '[': case '{':
		return !inSet;
	default:
		return false;
	};
}

template<bool inSet>
static char parseChar(const char*& nextChar)
{
	if(*nextChar == '\\')
	{
		++nextChar;
		if(!isMetachar<inSet>(*nextChar))
		{
			Errors::fatalf("'%c' is not a metachar in this context",*nextChar);
		}
	}
	else if(isMetachar<inSet>(*nextChar))
	{
		Errors::fatalf("'%c' is a metachar in this context",*nextChar);
	}
	else if(*nextChar == 0)
	{
		Errors::fatalf("unexpected end of string");
	}
	return *nextChar++;
}

template<bool inSet>
static NFA::CharSet parseLit(const char*& nextChar)
{
	NFA::CharSet result;
	const char c = parseChar<inSet>(nextChar);
	if(inSet && *nextChar == '-')
	{
		++nextChar;
		const char d = parseChar<inSet>(nextChar);
		result.addRange((U8)c,(U8)d);
	}
	else { result.add((U8)c); }
	return result;
}

template<bool inSet>
static NFA::CharSet parseCharClass(const char*& nextChar)
{
	if(*nextChar != '\\') { return parseLit<inSet>(nextChar); }
	else
	{
		NFA::CharSet result;
		switch(nextChar[1])
		{
		case 'd': result.addRange('0','9'); break;
		case 'w': result.addRange('a','z'); result.addRange('A','Z'); result.addRange('0','9'); result.add('_'); break;
		case 's': result.add(' '); result.add('\t'); result.add('\f'); result.add('\r'); result.add('\n'); break;
		default:
			if(!isMetachar<inSet>(nextChar[1]))
			{
				Errors::fatalf("'%c' is not a metachar in this context",nextChar[1]);
			}
			result.add(nextChar[1]);
			break;
		};
		nextChar += 2;
		return result;
	}
}

static NFA::CharSet parseSet(const char*& nextChar)
{
	NFA::CharSet result;

	WAVM_ASSERT_THROW(*nextChar == '[');
	++nextChar;

	bool isNegative = false;
	if(*nextChar == '^')
	{
		isNegative = true;
		++nextChar;
	}

	while(*nextChar && *nextChar != ']')
	{
		result = result | parseCharClass<true>(nextChar); 
	};
	++nextChar;

	if(isNegative)
	{
		result = ~result;
		result.remove(0);
	}

	return result;
}

static Node* parseElementary(const char*& nextChar,Uptr groupDepth)
{
	NFA::CharSet charSet;
	switch(*nextChar)
	{
	case '[':
	{
		charSet = parseSet(nextChar);
		break;
	}
	case '.':
	{
		charSet = ~NFA::CharSet(0);
		++nextChar;
		break;
	}
	case '$':
	{
		charSet.add('\n');
		++nextChar;
		break;
	}
	default:
	{
		charSet = parseCharClass<false>(nextChar);
		break;
	}
	};
	return new Lit(charSet);
}
	
static Node* parseUnion(const char*& nextChar,Uptr groupDepth);
static Node* parseGroup(const char*& nextChar,Uptr groupDepth)
{
	if(*nextChar != '(') { return parseElementary(nextChar,groupDepth); }
	else
	{
		++nextChar;
		Node* result = parseUnion(nextChar,groupDepth+1);
		WAVM_ASSERT_THROW(*nextChar == ')');
		++nextChar;
		return result;
	}
}

static Node* parseQuantifier(const char*& nextChar,Uptr groupDepth)
{
	Node* result = parseGroup(nextChar,groupDepth);

	switch(*nextChar)
	{
	case '+': ++nextChar; result = new OneOrMore(result); break;
	case '*': ++nextChar; result = new ZeroOrMore(result); break;
	case '?': ++nextChar; result = new Optional(result); break;
	};

	return result;
}

static Node* parseSeq(const char*& nextChar,Uptr groupDepth)
{
	Node* result = nullptr;
	while(true)
	{
		Node* newNode = parseQuantifier(nextChar,groupDepth);
		result = result ? new Seq(result,newNode) : newNode;

		switch(*nextChar)
		{
		case ')':
			if(groupDepth == 0) { Errors::fatalf("unexpected ')'"); }
			return result;
		case 0:
			if(groupDepth != 0) { Errors::fatalf("unexpected end of string"); }
			return result;
		case '|':
			return result;
		};
	};
}

static Node* parseUnion(const char*& nextChar,Uptr groupDepth)
{
	Node* result = nullptr;
	while(true)
	{
		Node* newNode = parseSeq(nextChar,groupDepth);
		result = result ? new Alt(result,newNode) : newNode;

		switch(*nextChar)
		{
		case ')':
			if(groupDepth == 0) { Errors::fatalf("unexpected ')'"); }
			return result;
		case 0:
			if(groupDepth != 0) { Errors::fatalf("unexpected end of string"); }
			return result;
		case '|':
			++nextChar;
			break;
		default:
			Errors::fatalf("unrecognized input");
		};
	};
}

static Node* parse(const char* string)
{
	const char* nextChar = string;
	Node* node = parseUnion(nextChar,0);
	if(*nextChar != 0) { Errors::fatalf("failed to parse entire regexp"); }
	return node;
}

static void createNFA(NFA::Builder* nfaBuilder,Node* node,NFA::StateIndex initialState,NFA::StateIndex finalState)
{
	switch(node->type)
	{
	case NodeType::lit:
	{
		auto lit = (Lit*)node;
		NFA::addEdge(nfaBuilder,initialState,lit->charSet,finalState);
		break;
	}
	case NodeType::zeroOrMore:
	{
		auto zeroOrMore = (ZeroOrMore*)node;
		createNFA(nfaBuilder,zeroOrMore->child,initialState,initialState);
		NFA::addEpsilonEdge(nfaBuilder,initialState,finalState);
		break;
	};
	case NodeType::oneOrMore:
	{
		auto oneOrMore = (OneOrMore*)node;
		auto intermediateState = NFA::addState(nfaBuilder);
		createNFA(nfaBuilder,oneOrMore->child,initialState,intermediateState);
		createNFA(nfaBuilder,oneOrMore->child,intermediateState,intermediateState);
		NFA::addEpsilonEdge(nfaBuilder,intermediateState,finalState);
		break;
	}
	case NodeType::optional:
	{
		auto optional = (Optional*)node;
		createNFA(nfaBuilder,optional->child,initialState,finalState);
		NFA::addEpsilonEdge(nfaBuilder,initialState,finalState);
		break;
	}
	case NodeType::alt:
	{
		auto alt = (Alt*)node;
		createNFA(nfaBuilder,alt->firstChild,initialState,finalState);
		createNFA(nfaBuilder,alt->secondChild,initialState,finalState);
		break;
	}
	case NodeType::seq:
	{
		auto seq = (Seq*)node;
		auto intermediateState = NFA::addState(nfaBuilder);
		createNFA(nfaBuilder,seq->firstChild,initialState,intermediateState);
		createNFA(nfaBuilder,seq->secondChild,intermediateState,finalState);
		break;
	}
	default:
		Errors::unreachable();
	};
}

namespace Regexp
{
	void addToNFA(const char* regexpString,NFA::Builder* nfaBuilder,NFA::StateIndex initialState,NFA::StateIndex finalState)
	{
		Node* rootNode = parse(regexpString);
		createNFA(nfaBuilder,rootNode,initialState,finalState);
		delete rootNode;
	}
};
