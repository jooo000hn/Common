//
// 매프레임 마다 호출되는 Update() 함수 이벤트를 받기 위한 옵져버 객체다.
//
//
#pragma once


class iUpdateObserver
{
public:
	virtual bool Init() { return true; }
	virtual void Update(const float deltaSeconds) = 0;
};


class cUpdateObservable
{
public:
	cUpdateObservable();
	virtual ~cUpdateObservable();

	void AddUpdateObserver(iUpdateObserver* observer);
	void RemoveUpdateObserver(iUpdateObserver* observer);
	bool NotifyInitObserver();
	void NotifyUpdateObserver(const float deltaSeconds);


protected:
	vector<iUpdateObserver*> m_updatObservers; // reference
};




inline cUpdateObservable::cUpdateObservable()
{
	m_updatObservers.reserve(16);
}

inline cUpdateObservable::~cUpdateObservable()
{
	m_updatObservers.clear();
}

inline void cUpdateObservable::AddUpdateObserver(iUpdateObserver* observer)
{
	RET(!observer);
	RemoveUpdateObserver(observer);
	m_updatObservers.push_back(observer);
}

inline void cUpdateObservable::RemoveUpdateObserver(iUpdateObserver* observer)
{
	common::removevector(m_updatObservers, observer);
}

inline void cUpdateObservable::NotifyUpdateObserver(const float deltaSeconds)
{
	for each (auto &observer in m_updatObservers)
		observer->Update(deltaSeconds);
}

inline bool cUpdateObservable::NotifyInitObserver()
{
	for each(auto &observer in m_updatObservers)
	{
		if (!observer->Init())
			return false;
	}
	return true;
}
