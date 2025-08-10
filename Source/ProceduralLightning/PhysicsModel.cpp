#include "PhysicsModel.h"


PhysicsModel::PhysicsModel()
{
	// Default values.
	// *** //
	PressureMultiplier = 1.0f;
	Scale = 9.5f;
	Voltage = 300000000.0f;
	ConstantnV = 0.00000001; // nV = 10^-8
	ConstantA = 0.21f;
	ConstantADeviation = 0.02f;
	Length = 11.0f;
	LengthDeviation = 4.0f;
	Angle = 43.0f;
	AngleDeviation = 12.3f;
	StartHeight = 2000.0f;
	SeaLevelHeight = 0.0f;
	SeaLevelTemp = 310.0f;
	BranchChance = 0.8f;
	InitialAngleRange = 20.0f;
	MaxSegments = 500;
	bUseSegmentLimit = true;
	bPackagedBuildFix = true;
	Rand_Generator.seed(time(NULL));
	// *** //
}

PhysicsModel::~PhysicsModel()
{
}

// Procedurally generate lightning segments.
void PhysicsModel::GenerateSegments()
{
	SCOPE_CYCLE_COUNTER(STAT_Segments)
	{
		// Empty segment array if it has any values.
		if (!LightningSegments.IsEmpty())
		{
			LightningSegments.Empty();
		}

		// Height range for mapping temperatures.
		heightRange.X = SeaLevelHeight;
		heightRange.Y = StartHeight;

		// Calculate the temperature at the height the lightning is spawned at. This is working based on the assumption that temperature decreases by 6.5 degrees celsius by every 1000 metres of altitude.
		float startAltitudeTemp = SeaLevelTemp - (StartHeight / 1000.0f * 6.5f);

		// Temperature range for mapping to the height range.
		tempRange.X = SeaLevelTemp;
		tempRange.Y = startAltitudeTemp;

		// Normal distributions for the length of segments, angle of branching, and the constant A.
		std::normal_distribution<float> lenDistribution(Length, LengthDeviation);
		std::normal_distribution<float> angleDistribution(Angle, AngleDeviation);
		std::normal_distribution<float> constDistribution(ConstantA, ConstantADeviation);

		// Calculate A.
		float constA = constDistribution(Rand_Generator);

		// Don't allow the constant to drop too low.
		if (constA < 0.01)
		{
			constA = 0.01;
		}

		// Booleans for generating segments.
		bool bIsGenerating = true;
		bool firstSegment = true;
		bool secondSegment = false;

		// Arrays containing branching information - the direction the branch should head in (based on other segment's calculations), and the segment from which it should branch from.
		TArray<FVector> branchDirection;
		TArray<Segment> branchPoints;

		// Overarching while loop that creates the segments. The loop ends once there are no more branches left to be explored.
		while (bIsGenerating)
		{
			// Segments are generated one branch at a time.
			TArray<Segment> currentBranch;
			bool bIsBranchFinished = false;

			// While loop for generating a branch. This loop ends when it's not possible to branch any further.
			while (!bIsBranchFinished)
			{
				Segment segment;
				if (firstSegment) // The very first segment is generated slightly differently.
				{
					firstSegment = false;
					GenerateFirstSegment(segment, constA, lenDistribution); // generate the segment
					currentBranch.Add(segment); // add segment to branch.
					secondSegment = true; // next segment will be the second segment.
				}
				else
				{
					if (currentBranch.IsEmpty()) // First run for new branches. Pops the first point from branch stack and set it to be the parent. If there are no more branch points, it stops generating.
					{
						if (!branchPoints.IsEmpty())
						{
							segment.Parent = &branchPoints.Top();
							branchPoints.Pop();
						}
						else
						{
							bIsGenerating = false;
							break;
						}

					}
					else // If it's not a new branch, the last segment is the new segment's parent.
					{
						segment.Parent = &currentBranch.Top();
					}

					SCOPE_CYCLE_COUNTER(STAT_SingleSeg)
					{
						// Generate segment.
						// *** //

						// Start at the end of the last segment.
						segment.StartPos = segment.Parent->EndPos;

						// Calculate the pressure at this position, apply multiplier.
						segment.Pressure = CalculatePressure(segment.StartPos.Z) * PressureMultiplier;

						// Calculate the temperature based on the height and temp ranges and current height.
						
						segment.Temp = CalculateTemp(segment);
						
						// Calculate min diameter. dmin = (A * T) / p.
						segment.MinDiameter = CalculateMinDiameter(segment, constA);
						

						float diameter;
						// The new diameter is calculated using the equation d new = sqrt(1/2) * (d old / d min,old) * d min,new. The old diameters are taken from the parent segment.
						
						diameter = CalculateDiameter(segment);
						

						float branchAngle;
						FRotator rotation;
						float splitAngle;
						float splitAngleOffset;

						//CalculateAngles(angleDistribution, branchAngle, splitAngle, splitAngleOffset, rotation);
						branchAngle = CalculateAngle(angleDistribution);

						// The angle is split between this segment, and the other segment in the branch.
						splitAngle = branchAngle / 2;

						// Calculate the offset to add to the branching angle, so that the split is not always in the middle.
						splitAngleOffset = FMath::FRandRange(-branchAngle / 2, branchAngle / 2);

						if (FMath::RandBool()) { splitAngle = -splitAngle; }; // 50% chance to flip the angle.

						// Create rotator from angles and offsets.

						if (*bIs3DEnabled) // 3D - apply to X and Y dimensions. 2D - just X dimension.
						{
							rotation = FRotator(splitAngle + splitAngleOffset, splitAngle + splitAngleOffset, 0);
						}
						else
						{
							rotation = FRotator(splitAngle + splitAngleOffset, 0, 0);
						}
						

						// If this is the first segment in the branch, get the direction from the branch direction array.
						if (currentBranch.IsEmpty())
						{
							if (!branchDirection.IsEmpty())
							{
								segment.Direction = branchDirection.Pop();
							}
						}
						else // Otherwise, apply the calculated rotation to the parent's direction vector and use that as the new direction.
						{
							segment.Direction = rotation.RotateVector(segment.Parent->Direction);
						}


						BranchLogic(segment, branchPoints, rotation, splitAngle, splitAngleOffset, branchDirection, secondSegment, bIsBranchFinished, diameter);

						

						// Apply diameter, calculate length and set end position.
						segment.Diameter = diameter;
						
						segment.Length = CalculateLength(lenDistribution, segment);
						
						segment.EndPos = segment.StartPos + (segment.Direction * segment.Length);
						// *** //

						// Add the new segment to the branch.
						currentBranch.Add(segment);
					}
				}
			}

			// Once the segments in the branch have been generated, add them to the main array of segments.
			if (!currentBranch.IsEmpty())
			{
				for (Segment seg : currentBranch)
				{
					LightningSegments.Add(seg);
				}
			}

			// If the segment limit option is enabled, and the max number of segments has been exceeded, then stop generating.
			if (bUseSegmentLimit)
			{
				if (LightningSegments.Num() >= MaxSegments)
				{
					bIsGenerating = false;

					while (LightningSegments.Num() > MaxSegments)
					{
						LightningSegments.Pop();
					}
					return;
				}
			}

		}
	}
}

// Calculate the pressure at a specified height based on the general barometric formula. The exact equation used is provided here: https://www.engineeringtoolbox.com/air-altitude-pressure-d_462.html
float PhysicsModel::CalculatePressure(float height)
{
	SCOPE_CYCLE_COUNTER(STAT_Pressure) 
	{
		return ((101325.0f * FMath::Pow((1.0f - 2.25577f * FMath::Pow(10.0f, -5.0f) * height), 5.25588f)) / 100000.0f);
	}
}

// Generate the first lightning segment.
void PhysicsModel::GenerateFirstSegment(Segment& segment, float A, std::normal_distribution<float> lenDistribution)
{
	SCOPE_CYCLE_COUNTER(STAT_FirstSeg)
	{
		// Start at the specified height.
		segment.StartPos = FVector(0, 0, StartHeight);

		// Calculate the pressure, then apply the pressure multiplier.
		segment.Pressure = CalculatePressure(segment.StartPos.Z) * PressureMultiplier;

		// Calculate the temperature based on the current height mapped between the height and temperature ranges.
		
		segment.Temp = CalculateTemp(segment); // Unclamped, so even if height is out of bounds (i.e. below sea level), it will provide a temperature based on the trend.
		
		// Calculate the minimum diameter. The equation used here is: (p * dmin) / T = A [mm bar / 293K]. This can be re-arranged to dmin = (A * T) / p.
		
		segment.MinDiameter = CalculateMinDiameter(segment, A); // divide temperature by 293K to match A's units.
		

		// No equation for the initial direction of the lightning, so just create direction from a random angle in a specified range.
		float randomAngle = FMath::RandRange(-InitialAngleRange, InitialAngleRange);

		FRotator startRotation;

		// Apply angle in X and Y dimensions for 3D, only in X for 2D.
		if (*bIs3DEnabled)
		{
			startRotation = FRotator(randomAngle, randomAngle, 0);
		}
		else
		{
			startRotation = FRotator(randomAngle, 0, 0);
		}

		// Direction is calculated by rotating a vector that points straight down by the above calculated rotator.
		segment.Direction = startRotation.RotateVector(FVector(0, 0, -1));

		// Calculate the diameter based on the equation: d = nV * V.
		
		segment.Diameter = CalculateInitDiameter();
		

		// The length is calculated using the equation: L / d = Ld. Ld is the normally distributed length which can be adjusted by the user. In the original equation, this is 11+/-4. The final equation thus becomes L = Ld * d. Scaling is applied to the result.
	
		segment.Length = CalculateLength(lenDistribution, segment);
		

		// The end position of the segment is provided by adding the segment direction multiplied by the length on to the start position's vector.
		segment.EndPos = segment.StartPos + (segment.Direction * segment.Length);
	}
}

float PhysicsModel::CalculateTemp(Segment& segment)
{
	SCOPE_CYCLE_COUNTER(STAT_Temperature)
	{
		return FMath::GetMappedRangeValueUnclamped(heightRange, tempRange, segment.StartPos.Z);
	}
}

float PhysicsModel::CalculateMinDiameter(Segment& segment, float constA)
{
	SCOPE_CYCLE_COUNTER(STAT_MinDiameter)
	{
		return (constA * (segment.Temp / 293.0f)) / segment.Pressure;
	}
}

float PhysicsModel::CalculateDiameter(Segment& segment)
{
	SCOPE_CYCLE_COUNTER(STAT_Diameter)
	{
		return FMath::Sqrt(0.5f) * (segment.Parent->Diameter / segment.Parent->MinDiameter) * segment.MinDiameter;
	}
}

float PhysicsModel::CalculateInitDiameter()
{
	SCOPE_CYCLE_COUNTER(STAT_Diameter)
	{
		return  Voltage * ConstantnV;
	}
}

float PhysicsModel::CalculateLength(std::normal_distribution<float> lenDistribution, Segment& segment)
{
	SCOPE_CYCLE_COUNTER(STAT_Length)
	{
		return lenDistribution(Rand_Generator) * segment.Diameter * Scale;
	}
}

void PhysicsModel::CalculateAngles(std::normal_distribution<float> angleDistribution, float& branchAngle, float& splitAngle, float& splitAngleOffset, FRotator& rotation)
{
	// Get branching angle from normal distribution.
	SCOPE_CYCLE_COUNTER(STAT_Angle)
	{
		branchAngle = angleDistribution(Rand_Generator);

		// The angle is split between this segment, and the other segment in the branch.
		splitAngle = branchAngle / 2;

		// Calculate the offset to add to the branching angle, so that the split is not always in the middle.
		splitAngleOffset = FMath::FRandRange(-branchAngle / 2, branchAngle / 2);

		if (FMath::RandBool()) { splitAngle = -splitAngle; }; // 50% chance to flip the angle.

		// Create rotator from angles and offsets.

		if (*bIs3DEnabled) // 3D - apply to X and Y dimensions. 2D - just X dimension.
		{
			rotation = FRotator(splitAngle + splitAngleOffset, splitAngle + splitAngleOffset, 0);
		}
		else
		{
			rotation = FRotator(splitAngle + splitAngleOffset, 0, 0);
		}
	}
}

float PhysicsModel::CalculateAngle(std::normal_distribution<float> angleDistribution)
{
	SCOPE_CYCLE_COUNTER(STAT_Angle)
	{
		return angleDistribution(Rand_Generator);
	}
}

void PhysicsModel::BranchLogic(Segment& segment, TArray<Segment>& branchPoints, FRotator& rotation, float& splitAngle, float& splitAngleOffset, TArray<FVector>& branchDirection, bool& secondSegment, bool& bIsBranchFinished, float& diameter)
{
	//SCOPE_CYCLE_COUNTER(STAT_Branch)
	{
		// When the new segment's diameter exceed the minimum diameter, it can branch.
		if (diameter > segment.MinDiameter)
		{
			// Not physically based - branch chance to give a bit more variety in results instead of branching every single time.
			if (FMath::FRandRange(0.f, 1.f) < BranchChance)
			{
				// Save this segment as a branching point.
				branchPoints.Add(segment);

				// Negate split angle so the new branch goes in the opposite direction.
				if (*bIs3DEnabled)
				{
					rotation = FRotator(-splitAngle + splitAngleOffset, splitAngle + splitAngleOffset, 0);
				}
				else
				{
					rotation = FRotator(-splitAngle + splitAngleOffset, 0, 0);
				}

				// The direction for the branching segment is added to an array so that the next branch can access it.
				branchDirection.Add(rotation.RotateVector(segment.Parent->Direction));

				if (secondSegment && bPackagedBuildFix) // Fixes really strange bug only present when the game is packaged. Without this, the packaged build will never branch on the second segment for no discernable reason.
				{
					secondSegment = false;
					branchPoints.Add(segment); // add branch again since it seemingly ignores the first function call in the packaged build?
				}
			}
			else if (secondSegment)
			{
				secondSegment = false;
			}
		}
		else // If the diameter does not exceed the minimum diameter, it can no longer propogate so the branch is finished.
		{
			bIsBranchFinished = true;
		}
	}
}
