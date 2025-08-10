#include "LGrammar.h"

// The constructor. Here the axiom and rules are set.
LGrammar::LGrammar(FString axiom, TArray<FString> rules)
{
	// Set starting string.
	Condition = axiom;

	// Starting value for probability.
	TotalProbability = 0;

	// Iterate through the rule strings to create LRules.
	for (int i = 0; i < rules.Num(); i++)
	{
		// The three parts of the rule strings. To the left of " => " is the string to be replaced. Between " => " and " (" is the rule itself. The probability is between " (" and ")".
		int pos1 = rules[i].Find(" => ");
		int pos2 = rules[i].Find(" (");
		int pos3 = rules[i].Find(")");

		// If rule separator and brackets are present in the string...
		if (pos1 != -1 && pos2 != -1)
		{
			// Separate out the string to be replaced, the string to act as the rule, and the probability.
			FString stringToReplace = rules[i].Mid(0, pos1);
			FString rule = rules[i].Mid(pos1 + 4, pos2 - pos1 - 4); // 4 is length of " => "
			FString probability = rules[i].Mid(pos2 + 3, pos3); // 3 is position after " ("

			// Create an L system rule with these properties.
			LRule r(stringToReplace, rule, FCString::Atof(*probability));

			// Add the rules to an array and increase total probability.
			RulesArray.Add(r);
			TotalProbability += r.Probability;
		}
	}

}

LGrammar::~LGrammar()
{
}

// Returns the resulting string.
FString LGrammar::GetResult()
{
	return Condition;
}

// Iterate through the string.
void LGrammar::Iterate(int its)
{
	SCOPE_CYCLE_COUNTER(STAT_Iterations)
	{
		int loopCount = 0;

		// For the specified number of iterations...
		for (int i = 0; i < its; i++)
		{
			SCOPE_CYCLE_COUNTER(STAT_Iterate)
			{

				// String to store the result of this iteration.
				FString newCondition;

				// Goes through each position in the string...
				for (int j = 0; j < Condition.Len(); j++)
				{
					loopCount++;

					// Random number used to select a rule.
					float random = FMath::RandRange(0.0f, TotalProbability);
					LRule randomRule;

					// Check each rule in the rule array's probability, and select based on the random number.
					// Example - random number is 0.75. Rule 1 and 2 both have 0.5 probability. In first rule check, 0.75 is more than 0.5 so the probability is taken away from 0.75 resulting in 0.25. This value is then lower than rule 2's probability of 0.5, resulting in that rule being selected.
					for (auto rule : RulesArray)
					{
						// If the random number is lower than that rules probability, use that rule.
						if (random <= rule.Probability)
						{
							randomRule = rule;
							break;
						}
						else // If not, decrease the random number by the rules probability.
						{
							random -= rule.Probability;
						}
					}

					// Add current position in string to a new string.
					FString current;
					current += Condition[j];

					// String to be added to the condition. This will be changed if the rule replacement criteria is met, otherwise the same string will be added to the final string.
					FString replacement = current;

					// If the string matches the rule's replacement string, replace the string with the rule.
					if (randomRule.ToReplace == current)
					{
						//replacement = randomRule.Rule;
						replacement = "F[+F]";
					}

					// Add to the condition.
					newCondition += replacement;
				}

				// Once each part of the string has been iterated through, set the condition to the new string.
				Condition = newCondition;

				
			}
		}

		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Loop count: %d"), loopCount));

	}
}
