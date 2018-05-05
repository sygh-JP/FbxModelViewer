#pragma once

// <Windows.h>, <cfloat> は別途インクルードしてください。

// 高分解能ストップウォッチ クラスおよびマルチメディア ストップウォッチ クラスの定義。
// 
// .NET の System.Diagnostics.Stopwatch が内部で使用している API と同じ高分解能パフォーマンス カウンターを使う。
// なお、Windows XP およびそれ以前の OS では、計測開始時の CPU クロック周波数を取得して、それを基準に計算を行なうので、
// Intel Turbo Boost や AMD Turbo Core などの動的オーバークロック機能を持つ CPU では正しく計測できない。
// Windows Vista 以降の OS と HPET をサポートする M/B の組み合わせでは、
// 同 API は HPET (High Precision Event Timer) で実時間取得するようになっているらしく、
// 特に大きな問題にはならないらしい。

namespace MyUtils
{
	//! @brief  Wrapper for high-performance counter.<br>
	//! 
	//! cf. System.Diagnostics.Stopwatch class of .NET Framework.<br>
	class HRStopwatch
	{
	private:
		//static LARGE_INTEGER m_freq;

		LARGE_INTEGER m_start;
		INT64 m_accumulatedValue;
		bool m_isStarted;
	public:
		HRStopwatch()
			: m_start(), m_accumulatedValue(), m_isStarted()
		{}

		void Start()
		{
			if (!m_isStarted)
			{
				::QueryPerformanceCounter(&m_start);
				m_isStarted = true;
			}
		}

		void Stop()
		{
			if (m_isStarted)
			{
				LARGE_INTEGER now = {};
				::QueryPerformanceCounter(&now);
				m_accumulatedValue += now.QuadPart - m_start.QuadPart;
				m_isStarted = false;
			}
		}

		void Reset()
		{
			m_start = LARGE_INTEGER();
			m_accumulatedValue = 0;
			m_isStarted = false;
		}

		void Restart()
		{
			this->Reset();
			this->Start();
		}

		double GetElapsedTimeInSeconds() const
		{ return double(m_accumulatedValue) / double(GetFrequency()); }

		INT64 GetElapsedTimeInMilliseconds() const
		{ return (GetFrequency() != 0) ? (INT64(1000) * m_accumulatedValue / GetFrequency()) : INT64_MAX; }

		static bool Initialize()
		{
			return !!::QueryPerformanceFrequency(&GetFrequencyRef());
		}

		static INT64 GetFrequency()
		{
			return GetFrequencyRef().QuadPart;
		}

	private:
		static LARGE_INTEGER& GetFrequencyRef()
		{
			static LARGE_INTEGER freq;
			return freq;
		}
	};

#ifdef _MSC_VER
	//__declspec(selectany) LARGE_INTEGER HRStopwatch::m_freq;
#endif


#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

	//! @brief  Wrapper for multimedia timer.<br>
	//! 
	//! cf. System.Diagnostics.Stopwatch class of .NET Framework.<br>
	class MMStopwatch
	{
	private:
		static const UINT Precision = 1;
		DWORD m_start;
		INT64 m_accumulatedValue;
		bool m_isStarted;
	public:
		MMStopwatch()
			: m_start(), m_accumulatedValue(), m_isStarted()
		{}

		void Start()
		{
			if (!m_isStarted)
			{
				m_start = ::timeGetTime();
				m_isStarted = true;
			}
		}

		void Stop()
		{
			if (m_isStarted)
			{
				const DWORD now = ::timeGetTime();
				m_accumulatedValue += now - m_start;
				m_isStarted = false;
			}
		}

		void Reset()
		{
			m_start = 0;
			m_accumulatedValue = 0;
			m_isStarted = false;
		}

		void Restart()
		{
			this->Reset();
			this->Start();
		}

		double GetElapsedTimeInSeconds() const
		{ return double(m_accumulatedValue) * 1.0e-3; }

		INT64 GetElapsedTimeInMilliseconds() const
		{ return m_accumulatedValue; }

		static bool Initialize()
		{
			return (::timeBeginPeriod(Precision) == TIMERR_NOERROR);
		}

		static bool Uninitialize()
		{
			return (::timeEndPeriod(Precision) == TIMERR_NOERROR);
		}
	};

#endif


#if 0
	template<typename TStopwatch> void DoTimerTestImpl()
	{
		TStopwatch stopwatch;
		printf("Timer started.\n");
		stopwatch.Start();
		::Sleep(1000);
		stopwatch.Stop();
		::Sleep(500); // Not counted.
		printf("Elapsed time[sec] = %010.3f\n", stopwatch.GetElapsedTimeInSeconds());
		printf("Timer re-started.\n");
		stopwatch.Start();
		::Sleep(1000);
		stopwatch.Stop();
		printf("Elapsed time[sec] = %010.3f\n", stopwatch.GetElapsedTimeInSeconds());
		printf("Timer is reset.\n");
		stopwatch.Reset();
		printf("Timer started.\n");
		stopwatch.Start();
		::Sleep(1);
		stopwatch.Stop();
		printf("Elapsed time[sec] = %010.3f\n", stopwatch.GetElapsedTimeInSeconds());
	}

	inline void DoTimerTest1()
	{
		// timeBeginPeriod() and timeEndPeriod() is originally for timeGetTime(),
		// however it also seems to affect the results of QueryPerformanceCounter().
		MMStopwatch::Initialize();
		const bool isTimerAvailable = HRStopwatch::Initialize();
		_ASSERTE(isTimerAvailable);
		DoTimerTestImpl<HRStopwatch>();
		MMStopwatch::Uninitialize();
	}

	inline void DoTimerTest2()
	{
		MMStopwatch::Initialize();
		DoTimerTestImpl<MMStopwatch>();
		MMStopwatch::Uninitialize();
	}
#endif
}
