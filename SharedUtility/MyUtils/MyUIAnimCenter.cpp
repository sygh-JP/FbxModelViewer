#include "stdafx.h"
#include "MyUIAnimCenter.h"
#include "MyUIProjectileAnim.h"


// The main window creates 1 or more projectile objects.  The projectile objects need access to
// the animation primitives.  Using IServiceProvider is simply one way to allow the main window
// to cache these primitives.  When the projectiles are created, they are passed a reference to
// the main window's service provider interface and then probe that interface for these primitives
// as required.
IFACEMETHODIMP MyUIAnimCenter::QueryService(__in REFGUID guidService, __in REFIID riid, __deref_out void** ppv)
{
	HRESULT hr = E_NOINTERFACE;
	if (guidService == CLSID_UIAnimationManager)
	{
		hr = m_animationManager.CopyTo(riid, ppv);
	}
	else if (guidService == CLSID_UIAnimationTimer)
	{
		hr = m_animationTimer.CopyTo(riid, ppv);
	}
	else if (guidService == CLSID_UIAnimationTransitionLibrary)
	{
		hr = m_transitionLibrary.CopyTo(riid, ppv);
	}
	else if (guidService == CLSID_UIAnimationTransitionFactory)
	{
		hr = m_transitionFactory.CopyTo(riid, ppv);
	}
	return hr;
}

// Initialize all of the animation primitives necessary to perform drawing and projectile animation
HRESULT MyUIAnimCenter::InitializeAnimationInterfaces()
{
	HRESULT hr = S_OK;
	{
		// Create Animation Manager
		hr = CoCreateInstance(CLSID_UIAnimationManager, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_animationManager));
		if (SUCCEEDED(hr))
		{
			// Create Animation Timer
			hr = CoCreateInstance(CLSID_UIAnimationTimer, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_animationTimer));
			if (SUCCEEDED(hr))
			{
				// Create the Transition Factory to wrap interpolators in transitions
				hr = CoCreateInstance(CLSID_UIAnimationTransitionFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_transitionFactory));
				if (SUCCEEDED(hr))
				{
					// Create Animation Transition Library
					hr = CoCreateInstance(CLSID_UIAnimationTransitionLibrary, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_transitionLibrary));
					if (SUCCEEDED(hr))
					{
						hr = m_animationManager->SetManagerEventHandler(static_cast<IUIAnimationManagerEventHandler*>(this));
					}
				}
			}
		}
	}
	return hr;
}

void MyUIAnimCenter::FireProjectile(__in D2D1_POINT_2F launchPoint)
{
	auto projectile = std::make_shared<MyUIProjectileAnimation>();
	{
		m_projectileArray.push_back(projectile);

		projectile->Fire(static_cast<IServiceProvider*>(this), launchPoint);
	}
}

D2D1_POINT_2F MyUIAnimCenter::GetProjectilePosition(size_t index)
{
	_ASSERTE(index < m_projectileArray.size());
	D2D1_POINT_2F point = {};
	_ASSERTE(m_projectileArray[index] != nullptr);
	m_projectileArray[index]->GetPosition(point);
	return point;
}

void MyUIAnimCenter::RemoveDeadItemsInQueue()
{
	auto newEnd = std::remove_if(m_projectileArray.begin(), m_projectileArray.end(),
		//[](decltype(m_projectileArray[0]) val) { return val.get() == nullptr; }
		//[](decltype(m_projectileArray[0]) val) { return !val->IsAnimating(); }
		[](decltype(m_projectileArray[0]) val) { return val.get() == nullptr || !val->IsAnimating(); }
	);
	m_projectileArray.erase(newEnd, m_projectileArray.end());
}

// VC++ 2010 MFC の WAM ラッパーサンプルでは、アニメーションの巻き戻しも実装されている。
// http://msdn.microsoft.com/ja-jp/library/gg466500(v=vs.100).aspx

void MyUIAnimCenter::UpdateAnimTimer()
{
	// Update the animation manager with the current time
	UI_ANIMATION_SECONDS secondsNow = 0;
	if (SUCCEEDED(m_animationTimer->GetTime(&secondsNow)))
	{
		if (SUCCEEDED(m_animationManager->Update(secondsNow)))
		{
			// Continue redrawing the client area as long as there are animations scheduled
			auto status = UI_ANIMATION_MANAGER_STATUS();
			if (SUCCEEDED(m_animationManager->GetStatus(&status)))
			{
				if (status == UI_ANIMATION_MANAGER_BUSY)
				{
					// 再描画する？
				}
			}
		}
	}
}
