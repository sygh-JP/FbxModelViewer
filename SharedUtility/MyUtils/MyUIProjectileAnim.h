// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once


// http://msdn.microsoft.com/ja-jp/library/windows/desktop/ff934859.aspx
// http://blogs.msdn.com/b/jpwin/archive/2011/06/16/simulating-projectile-motion-with-animation-manager.aspx

// 下記はすべて参照カウントをきちんと管理していないエセのカスタム COM インターフェイス実装なので注意。
// ポインタを Microsoft::WRL::ComPtr などで管理しないこと。代わりに std::shared_ptr を使う。
// 本来は参照カウントがゼロになったときメモリーを解放したりしないといけない。


// 前方宣言。
class MyUIGravityInterpolator;


class MyUIProjectileAnimation final : public IUIAnimationStoryboardEventHandler
{
#pragma region // IUnknown
public:
	IFACEMETHODIMP_(ULONG) AddRef()
	{ return 1; }
	IFACEMETHODIMP_(ULONG) Release()
	{ return 0; }
	IFACEMETHODIMP QueryInterface(__in REFIID riid, __deref_out void** ppv)
	{
#pragma warning(push)
#pragma warning(disable : 4838) // VC 2015 で発生するようになった警告。バグ？
		static const QITAB qit[] =
		{
			QITABENT(MyUIProjectileAnimation, IUIAnimationStoryboardEventHandler),
			{}, // sentinel
		};
#pragma warning(pop)
		// QISearch() 内で自動的に AddRef() が呼ばれるらしい。
		return ::QISearch(this, qit, riid, ppv);
	}
#pragma endregion

public:
	HRESULT Fire(__in IServiceProvider *site, __in D2D1_POINT_2F launchPoint);
	HRESULT GetPosition(__out D2D1_POINT_2F& position);
	bool IsAnimating() const
	{ return m_isAnimating; }

#pragma region // IUIAnimationStoryboardEventHandler
public:
	IFACEMETHODIMP OnStoryboardStatusChanged(
		__in IUIAnimationStoryboard *storyboard,
		__in UI_ANIMATION_STORYBOARD_STATUS newStatus,
		__in UI_ANIMATION_STORYBOARD_STATUS previousStatus)
	{
		if ((newStatus == UI_ANIMATION_STORYBOARD_CANCELLED) ||
			(newStatus == UI_ANIMATION_STORYBOARD_TRUNCATED) ||
			(newStatus == UI_ANIMATION_STORYBOARD_FINISHED))
		{
			m_isAnimating = false;
		}
		else if (newStatus == UI_ANIMATION_STORYBOARD_READY)
		{
			// Remove the event handler from the storyboard so that it releases
			// its reference to the projectile object.  See http://msdn.microsoft.com/en-us/library/dd371971(v=VS.85).aspx
			// for details on the storyboard states.
			storyboard->SetStoryboardEventHandler(nullptr);
		}
		return S_OK;
	}
	IFACEMETHODIMP OnStoryboardUpdated(__in IUIAnimationStoryboard *storyboard)
	{
		return S_OK;
	}
#pragma endregion

public:
	MyUIProjectileAnimation()
		: m_isAnimating(false)
	{}
	virtual ~MyUIProjectileAnimation()
	{}

private:
	HRESULT CreateAnimationVariables(__in IUIAnimationManager *animationManager, __in D2D1_POINT_2F launchPoint);
	HRESULT InitializeStoryboard(__in IServiceProvider *site, Microsoft::WRL::ComPtr<IUIAnimationStoryboard>& out);
	HRESULT StartStoryboard(__in IServiceProvider *site, __in IUIAnimationStoryboard *storyBoard);

private:
	bool m_isAnimating;

	std::shared_ptr<MyUIGravityInterpolator> m_gravityInterpolator;

	// X and Y Animation Variables
	Microsoft::WRL::ComPtr<IUIAnimationVariable> m_animationVariableX;
	Microsoft::WRL::ComPtr<IUIAnimationVariable> m_animationVariableY;
};


// Custom interpolator
class MyUIGravityInterpolator final : public IUIAnimationInterpolator
{
#pragma region // IUnknown
public:
	IFACEMETHODIMP_(ULONG) AddRef()
	{ return 1; }
	IFACEMETHODIMP_(ULONG) Release()
	{ return 0; }
	IFACEMETHODIMP QueryInterface(__in REFIID riid, __deref_out void** ppv)
	{
#pragma warning(push)
#pragma warning(disable : 4838) // VC 2015 で発生するようになった警告。バグ？
		static const QITAB qit[] =
		{
			QITABENT(MyUIGravityInterpolator, IUIAnimationInterpolator),
			{}, // sentinel
		};
#pragma warning(pop)
		return ::QISearch(this, qit, riid, ppv);
	}
#pragma endregion

#pragma region // IUIAnimationInterpolator
public:
	IFACEMETHODIMP SetInitialValueAndVelocity(__in double initialValue, __in double initialVelocity)
	{
		m_initialValue = initialValue;
		m_initialVelocity = -1000.0;
		return S_OK;
	}

	// Sets the interpolator's duration
	IFACEMETHODIMP SetDuration(__in UI_ANIMATION_SECONDS duration)
	{
		// No duration dependency is declared so this will not be called
		return E_NOTIMPL;            
	}

	// Gets the interpolator's duration
	IFACEMETHODIMP GetDuration(__out UI_ANIMATION_SECONDS *pDuration)
	{   
		*pDuration = 2.0f;
		return S_OK;
	}

	// Gets the final value to which the interpolator leads
	IFACEMETHODIMP GetFinalValue(__out double *pValue)
	{        
		*pValue = m_finalValue;
		return S_OK;
	}

	// Interpolates the value at a given offset
	IFACEMETHODIMP InterpolateValue(__in UI_ANIMATION_SECONDS offset, __out double *pValue)
	{
		*pValue = m_initialValue + (m_initialVelocity * offset) + (0.5 * m_acceleration * (offset * offset));
		return S_OK;
	}

	// Interpolates the velocity at a given offset
	IFACEMETHODIMP InterpolateVelocity(__in UI_ANIMATION_SECONDS offset, __out double *pVelocity)
	{
		*pVelocity = m_initialVelocity + (m_acceleration * offset);
		return S_OK;
	}

	// Gets the interpolator's dependencies
	IFACEMETHODIMP GetDependencies(__out UI_ANIMATION_DEPENDENCIES *initialValues,
		__out UI_ANIMATION_DEPENDENCIES *initialVelocity,
		__out UI_ANIMATION_DEPENDENCIES *duration)
	{
		// The final value of the interpolator is not affected by the initial value or velocity, but
		// the intermediate values, final velocity and duration all are affected
		*initialValues =
			UI_ANIMATION_DEPENDENCY_INTERMEDIATE_VALUES |
			UI_ANIMATION_DEPENDENCY_FINAL_VELOCITY |
			UI_ANIMATION_DEPENDENCY_DURATION;

		*initialVelocity =
			UI_ANIMATION_DEPENDENCY_INTERMEDIATE_VALUES |
			UI_ANIMATION_DEPENDENCY_FINAL_VELOCITY |
			UI_ANIMATION_DEPENDENCY_DURATION;

		// This interpolator does not have a duration parameter, so SetDuration should not be called on it
		*duration = UI_ANIMATION_DEPENDENCY_NONE;

		return S_OK;
	}
#pragma endregion

public:
	MyUIGravityInterpolator(__in double acceleration, __in double finalValue)
		: m_duration()
		, m_initialValue(0.0)
		, m_finalValue(finalValue)
		, m_initialVelocity(0.0)
		, m_acceleration(acceleration)
	{}
	virtual ~MyUIGravityInterpolator()
	{}

private:
	UI_ANIMATION_SECONDS m_duration;
	double m_initialValue;
	double m_finalValue;
	double m_initialVelocity;
	double m_acceleration;
};
