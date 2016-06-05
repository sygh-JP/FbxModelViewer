#pragma once


// 前方宣言。
class MyUIProjectileAnimation;


class MyUIAnimCenter final : public IUIAnimationManagerEventHandler, public IServiceProvider
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
			QITABENT(MyUIAnimCenter, IUIAnimationManagerEventHandler),
			QITABENT(MyUIAnimCenter, IServiceProvider),
			{}, // sentinel
		};
#pragma warning(pop)
		return ::QISearch(this, qit, riid, ppv);
	}
#pragma endregion

	// IUIAnimationManagerEventHandler
public:
	IFACEMETHODIMP OnManagerStatusChanged(__in UI_ANIMATION_MANAGER_STATUS newStatus, __in UI_ANIMATION_MANAGER_STATUS previousStatus)
	{
		HRESULT hr = S_OK;
		if (newStatus == UI_ANIMATION_MANAGER_BUSY)
		{
			// 再描画すればよい？
		}
		return hr;
	}

	// IServiceProvider
public:
	IFACEMETHODIMP QueryService(__in REFGUID guidService, __in REFIID riid, __deref_out void** ppv);

public:
	HRESULT InitializeAnimationInterfaces();
	void FireProjectile(__in D2D1_POINT_2F launchPoint);
	size_t GetProjectileAnimCount() const
	{ return m_projectileArray.size(); }
	D2D1_POINT_2F GetProjectilePosition(size_t index);
	void RemoveDeadItemsInQueue();

	void UpdateAnimTimer();

public:
	MyUIAnimCenter()
	{}
	virtual ~MyUIAnimCenter()
	{}

private:
	std::vector<std::shared_ptr<MyUIProjectileAnimation>> m_projectileArray;

	// Animation components
	Microsoft::WRL::ComPtr<IUIAnimationManager> m_animationManager;
	Microsoft::WRL::ComPtr<IUIAnimationTimer> m_animationTimer;
	Microsoft::WRL::ComPtr<IUIAnimationTransitionLibrary> m_transitionLibrary;
	Microsoft::WRL::ComPtr<IUIAnimationTransitionFactory> m_transitionFactory;
};
