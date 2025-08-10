// L system rule class.
#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class PROCEDURALLIGHTNING_API LRule
{
public:
	// Constructors and destructors.
	LRule();
	LRule(FString v, FString r, float p);
	~LRule();

	// The rule string and the string that it replaces.
	FString Rule;
	FString ToReplace;

	// The probability of this rule occuring.
	float Probability;
};
