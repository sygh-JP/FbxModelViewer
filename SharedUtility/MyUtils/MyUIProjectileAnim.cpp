// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "stdafx.h"
#include "MyUIProjectileAnim.h"

using Microsoft::WRL::ComPtr;

// The storyboard for a projectile animation includes a constant (linear) horizontal transition
// and a transition based on a custom interpolator that simulates gravity for the vertical
// transition.  The vertical transition could probably be handled by one of the default transitions
// from the transition library but this sample uses a custom interpolator to demonstrate how it works.
// The transitions simply update the values of animation variable objects (m_animationVariableX and
// m_animationVariableY). The values from those animation variables are used during the render pass to
// draw the projectile at the correct coordinates.
HRESULT MyUIProjectileAnimation::InitializeStoryboard(__in IServiceProvider *site, Microsoft::WRL::ComPtr<IUIAnimationStoryboard>& out)
{
	ComPtr<IUIAnimationManager> animationManager;
	HRESULT hr = IUnknown_QueryService(site, CLSID_UIAnimationManager, IID_PPV_ARGS(&animationManager));
	if (SUCCEEDED(hr))
	{
		ComPtr<IUIAnimationStoryboard> storyBoard;
		hr = animationManager->CreateStoryboard(&storyBoard);
		if (SUCCEEDED(hr))
		{
			ComPtr<IUIAnimationTransitionLibrary> transitionLibrary;
			hr = IUnknown_QueryService(site, CLSID_UIAnimationTransitionLibrary, IID_PPV_ARGS(&transitionLibrary));
			if (SUCCEEDED(hr))
			{
				ComPtr<IUIAnimationTransition> horizontalTransition;
				hr = transitionLibrary->CreateLinearTransition(2.0, 0.0, &horizontalTransition);
				if (SUCCEEDED(hr))
				{
					hr = storyBoard->AddTransition(m_animationVariableX.Get(), horizontalTransition.Get());
					if (SUCCEEDED(hr))
					{
						ComPtr<IUIAnimationTransitionFactory> transitionFactory;
						hr = IUnknown_QueryService(site, CLSID_UIAnimationTransitionFactory, IID_PPV_ARGS(&transitionFactory));
						if (SUCCEEDED(hr))
						{
							m_gravityInterpolator = std::make_shared<MyUIGravityInterpolator>(2000.0, 0.0);
							//if (SUCCEEDED(hr))
							{
								ComPtr<IUIAnimationTransition> verticalTransition;
								// IUIAnimationTransitionFactory::CreateTransition() の第1引数に渡した IUIAnimationInterpolator の参照カウントが増加するらしい。
								// 今回はインターフェイスをエセ実装しているので、インスタンスの寿命をきちんと C++ ユーザーコード側で管理しておく必要がある。
								hr = transitionFactory->CreateTransition(m_gravityInterpolator.get(), &verticalTransition);
								if (SUCCEEDED(hr))
								{
									hr = storyBoard->AddTransition(m_animationVariableY.Get(), verticalTransition.Get());
									if (SUCCEEDED(hr))
									{
										out = storyBoard;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return hr;
}

HRESULT MyUIProjectileAnimation::CreateAnimationVariables(__in IUIAnimationManager *animationManager, __in D2D1_POINT_2F launchPoint)
{
	HRESULT hr = animationManager->CreateAnimationVariable(launchPoint.x, m_animationVariableX.ReleaseAndGetAddressOf());
	if (SUCCEEDED(hr))
	{
		hr = animationManager->CreateAnimationVariable(launchPoint.y, m_animationVariableY.ReleaseAndGetAddressOf());
	}
	return hr;
}

HRESULT MyUIProjectileAnimation::StartStoryboard(__in IServiceProvider *site, __in IUIAnimationStoryboard *storyBoard)
{
	ComPtr<IUIAnimationTimer> animationTimer;
	HRESULT hr = IUnknown_QueryService(site, CLSID_UIAnimationTimer, IID_PPV_ARGS(&animationTimer));
	if (SUCCEEDED(hr))
	{
		UI_ANIMATION_SECONDS now;
		hr = animationTimer->GetTime(&now);
		if (SUCCEEDED(hr))
		{
			hr = storyBoard->SetStoryboardEventHandler(static_cast<IUIAnimationStoryboardEventHandler*>(this));
			if (SUCCEEDED(hr))
			{
				hr = storyBoard->Schedule(now);
				if (SUCCEEDED(hr))
				{
					m_isAnimating = true;
				}
				else
				{
					storyBoard->SetStoryboardEventHandler(nullptr);
				}
			}
		}
	}
	return hr;
}

// The service provider must provide all of the animation primitives necessary to create storyboards,
// transitions, and animation timers.  Note that this implementation is simply a design choice that allows
// the main window to cache these primitives so that each projectile does not have to create and manage
// them.  See CMainWindow::QueryService for additional comments.
//
// The basic steps for this animation are:
// - Create animation variables.  These simply keep track of the value of a double over time
// - Create the storyboard, which contains the transitions.  The transitions are associated
//   with the animation variables and update their values based on either a standard
//   interpolation (e.g. CreateLinearTransition) or a custom interpolation.  The projectile
//   animation uses a standard transtion for the horizontal axis and a custom interpolation
//   for the vertical axis
// - Schedule the storyboard.
HRESULT MyUIProjectileAnimation::Fire(__in IServiceProvider *site, __in D2D1_POINT_2F launchPoint)
{
	ComPtr<IUIAnimationManager> animationManager;
	HRESULT hr = IUnknown_QueryService(site, CLSID_UIAnimationManager, IID_PPV_ARGS(&animationManager));
	if (SUCCEEDED(hr))
	{
		hr = this->CreateAnimationVariables(animationManager.Get(), launchPoint);
		if (SUCCEEDED(hr))
		{
			ComPtr<IUIAnimationStoryboard> storyBoard;
			hr = this->InitializeStoryboard(site, storyBoard);
			if (SUCCEEDED(hr))
			{
				hr = this->StartStoryboard(site, storyBoard.Get());
			}
		}
	}
	return hr;
}

HRESULT MyUIProjectileAnimation::GetPosition(__out D2D1_POINT_2F& position)
{
	_ASSERTE(m_animationVariableX && m_animationVariableY);
	HRESULT hr = S_OK;
	double x = 0, y = 0;
	hr = m_animationVariableX->GetValue(&x);
	if (FAILED(hr))
	{
		return hr;
	}
	hr = m_animationVariableY->GetValue(&y);
	if (FAILED(hr))
	{
		return hr;
	}
	position.x = float(x);
	position.y = float(y);
	return S_OK;
}
