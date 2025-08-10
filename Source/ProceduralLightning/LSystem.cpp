#include "LSystem.h"

LSystem::LSystem()
{
}

LSystem::~LSystem()
{
}

// Build the L system.
void LSystem::Build(FString axiom, TArray<FString> rules, int iterations)
{
	// Create the L system's stochastic grammar using the provided rules and axiom.
	LGrammar grammar(axiom, rules);

	// Iterate through the string the specified number of times.
	grammar.Iterate(iterations);

	// Save the result.
	Result = grammar.GetResult();
}

// Returns the resulting string.
FString LSystem::GetResult()
{
	return Result;
}
