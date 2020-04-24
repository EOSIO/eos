#include <vector>
#include <iostream>

struct point
{
	double x;
	double y;
};

int main()
{
	// Generate a lot of uniformly distributed 2d points in the range -1,-1 to +1,+1.
	enum { numXSamples = 10000 };
	enum { numYSamples = 10000 };
	std::vector<point> points;
	points.reserve(numXSamples * numYSamples);
	for(int x = 0;x < numXSamples;++x)
	{
		for(int y = 0;y < numXSamples;++y)
		{
			point p = {-1.0 + 2.0 * x / (numXSamples-1),-1.0 + 2.0 * y / (numYSamples-1)};
			points.push_back(p);
		}
	}

	// Count the ratio of points inside the unit circle.
	int numerator = 0;
	int denominator = 0;
	for(auto pointIt = points.begin();pointIt != points.end();++pointIt)
	{
		if(pointIt->x * pointIt->x + pointIt->y * pointIt->y < 1.0)
		{
			++numerator;
		}
		++denominator;
	}

	// Derive the area of the unit circle.
	auto circleArea = 4.0 * (double)numerator / denominator;
	std::cout << "result: " << circleArea << std::endl;

	return 0;
}
