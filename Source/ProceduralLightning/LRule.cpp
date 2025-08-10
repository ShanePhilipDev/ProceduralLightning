#include "LRule.h"

// Constructors and destructors.
// *** //
LRule::LRule()
{
}

LRule::LRule(FString v, FString r, float p)
{
	Rule = r;
	ToReplace = v;
	Probability = p;
}

LRule::~LRule()
{
}
// *** //