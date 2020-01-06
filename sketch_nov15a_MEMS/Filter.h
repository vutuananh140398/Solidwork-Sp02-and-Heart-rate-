#include <math.h>
#define MAXSAMPLE 100

struct FilterT
{
	int arr[MAXSAMPLE];

	int maxF = 0;
	int minF = 0;
	int avgF = 0;
	int offSet = 0, stack = 0;

	void init(int _offSet, int _stack, int _max, int _min)
	{
		for (int i = 0; i < MAXSAMPLE; i++)
		{
			this->arr[i] = 0;
		}
		this->offSet = _offSet;
		this->stack = _stack;
		this->maxF = _max;
		this->minF = _min;
	}
	void add(int value)
	{
		if (value >= this->offSet)
		{
			this->arr[value - this->offSet]++;
		}
	}
	void filtering(int *maxV, int *minV, int *avgV)
	{
		int sumF = 0;
		int countF = 0;
    int trueValue = 0;
		for (short i = 0; i < MAXSAMPLE; i++)
		{
    trueValue = i + this->offSet;
			if (this->arr[i] >= stack)
			{
				if (trueValue > this->maxF)
				{
					this->maxF = trueValue;
				}
				if (trueValue < this->minF)
				{
					this->minF = trueValue;
				}
				countF += this->arr[i];
				sumF += trueValue * this->arr[i];
			}
		}
		*maxV = this->maxF;
		*minV = this->minF;
		*avgV = round(sumF/countF);
	}
	void log()
	{
		for (int i = 0; i < MAXSAMPLE; i++)
		{
			if (arr[i] > 0)
			{
				Serial.print(i + this->offSet);
				Serial.print("-");
				Serial.println(arr[i]);
			}

		}
		Serial.println();
	}
};
