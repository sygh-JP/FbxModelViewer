// Only UTF-8 or ASCII is available.

class Xorshift128Random
{
	static const uint MaxVal = 10000;

	static uint4 CreateNext(uint4 random)
	{
		const uint t = (random.x ^ (random.x << 11));
		random.x = random.y;
		random.y = random.z;
		random.z = random.w;
		random.w = (random.w = (random.w ^ (random.w >> 19)) ^ (t ^ (t >> 8)));
		return random;
	}

	static uint GetRandomComponentUI(uint4 random)
	{
		return random.w;
	}

	static float GetRandomComponentUF(uint4 random)
	{
		return float(GetRandomComponentUI(random) % MaxVal) / float(MaxVal);
	}

	static float GetRandomComponentSF(uint4 random)
	{
		return 2.0f * GetRandomComponentUF(random) - 1.0f;
	}

	static uint4 CreateInitialNumber(uint seed)
	{
		if (seed == 0)
		{
			seed += 11;
		}

		uint4 temp;

		temp.w = seed;
		temp.x = (seed << 16) + (seed >> 16);
		temp.y = temp.w + temp.x;
		temp.z = temp.x ^ temp.y;

		return temp;
	}
};
