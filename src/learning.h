#pragma once

// Training position stores the position and the target value.
struct TrainingPosition
{
	TrainingPosition() {}
	TrainingPosition(const SBoard& inBoard, float inTargetVal) : board(inBoard), targetVal(inTargetVal) {}

	SBoard board;
	float targetVal;
};

struct MatchResults {
	int wins = 0;
	int draws = 0;
	int losses = 0;

	float Percent() {
		return (wins * 1.0f + draws * 0.5f) / float( wins + draws + losses);
	}
	float EloDiff() {
		const float LN10 = 2.302585092994046f;
		if (Percent() == 0.0f || Percent() == 1.0f) return 0.0f;
		return -400.0f * logf(1.0f / Percent() - 1.0f) / LN10; 
	}
};

void CreateTrainingSet();
int ImportLatestMatches(MatchResults& results);