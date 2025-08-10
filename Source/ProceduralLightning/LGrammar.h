// A stochastic grammar for use in the L system.
#pragma once

#include "CoreMinimal.h"
#include "LRule.h"

DECLARE_STATS_GROUP(TEXT("LSystem"), STATGROUP_LSystem, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Iterations"), STAT_Iterations, STATGROUP_LSystem);
DECLARE_CYCLE_STAT(TEXT("Iterate"), STAT_Iterate, STATGROUP_LSystem);
/**
 * 
 */
class PROCEDURALLIGHTNING_API LGrammar
{
public:
	// Constructor and destructor.
	LGrammar(FString axiom, TArray<FString> rules);
	~LGrammar();

	// Iterate through the string.
	void Iterate(int its);

	// Returns the resulting string.
	FString GetResult();
protected:
	// The string to be iterated through.
	FString Condition;

	// An array to store the rules.
	TArray<LRule> RulesArray;

	// Combined probability values of the rules array.
	float TotalProbability;
};
