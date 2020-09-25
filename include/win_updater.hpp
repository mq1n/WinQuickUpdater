#pragma once
#include "abstract_singleton.hpp"
#include "pch.hpp"

namespace WinQuickUpdater
{
	using TProgressCallback = std::function<void(LONG, LONG, LONG)>;
	using TNotificationCallback = std::function<void()>;

	inline const auto ComErrorMessage(HRESULT hr)
	{
		_com_error err(hr);
		LPCTSTR errMsg = err.ErrorMessage();
		return errMsg;
	}
	inline const auto ComStrToStdStr(BSTR bszName)
	{
#pragma warning(push) 
#pragma warning(disable: 4242 4244)
		std::wstring wszName(bszName, SysStringLen(bszName));
		//std::string szName(wszName.begin(), wszName.end());
#pragma warning(push) 
		return wszName;
	}

	class ComStr
	{
	public:
		ComStr(const std::string& in) : m_com_str(nullptr)
		{
			Initialize(std::wstring(in.begin(), in.end()));
		}
		ComStr(const std::wstring& in) : m_com_str(nullptr)
		{
			Initialize(in);
		}
		~ComStr()
		{
			if (m_com_str) SysFreeString(m_com_str);
		}

		operator BSTR ()
		{
			return m_com_str;
		}

	protected:
		void Initialize(const std::wstring& in)
		{
			if (!in.empty()) m_com_str = SysAllocString(in.c_str());
		}

	private:
		BSTR m_com_str;
	};

	template <class I>
	class IUNK : public I
	{
	public:
		STDMETHODIMP QueryInterface(REFIID riid, void __RPC_FAR* __RPC_FAR* ppvObject)
		{
			if (ppvObject == NULL)
				return E_POINTER;

			if (riid == __uuidof(IUnknown) || riid == __uuidof(I))
			{
				*ppvObject = this;
				return S_OK;
			}

			return E_NOINTERFACE;
		}

		STDMETHODIMP_(ULONG) AddRef(void) { return 1; }
		STDMETHODIMP_(ULONG) Release(void) { return 1; }
	};
	class IDPC : public IUNK<IDownloadProgressChangedCallback>
	{
	public:
		IDPC(TProgressCallback cb) :
			m_cb(cb)
		{
		}

		STDMETHODIMP Invoke(IDownloadJob* job, IDownloadProgressChangedCallbackArgs* args)
		{
			IDownloadProgress* prg = NULL;
			if (SUCCEEDED(args->get_Progress(&prg)))
			{
				LONG total_percent = 0;
				LONG current_percent = 0;
				LONG current_idx = 0;

				if (SUCCEEDED(prg->get_PercentComplete(&total_percent)) && SUCCEEDED(prg->get_CurrentUpdatePercentComplete(&current_percent)) && SUCCEEDED(prg->get_CurrentUpdateIndex(&current_idx)))
				{
					m_cb(current_idx, current_percent, total_percent);
				}

				prg->Release();
			}
			return S_OK;
		}

	private:
		TProgressCallback m_cb;
	};
	class IDCC : public IUNK<IDownloadCompletedCallback>
	{
	public:
		IDCC(TNotificationCallback cb)
		{
			m_cb = cb;
			m_Event = CreateEvent(NULL, FALSE, FALSE, NULL);
		}
		~IDCC()
		{
			CloseHandle(m_Event);
		}

		STDMETHODIMP Invoke(IDownloadJob* job, IDownloadCompletedCallbackArgs* args)
		{
			m_cb();
			SetEvent(m_Event);
			return S_OK;
		}

		auto GetEvent() const { return m_Event; };

	private:
		TNotificationCallback m_cb;
		HANDLE m_Event;
	};
	class IIPC : public IUNK<IInstallationProgressChangedCallback>
	{
	public:	
		IIPC(TProgressCallback cb) :
			m_cb(cb)
		{
		}

		STDMETHODIMP Invoke(IInstallationJob* job, IInstallationProgressChangedCallbackArgs* args)
		{
			IInstallationProgress* prg = NULL;
			if (SUCCEEDED(args->get_Progress(&prg)))
			{
				LONG total_percent = 0;
				LONG current_percent = 0;
				LONG current_idx = 0;

				if (SUCCEEDED(prg->get_PercentComplete(&total_percent)) && SUCCEEDED(prg->get_CurrentUpdatePercentComplete(&current_percent)) && SUCCEEDED(prg->get_CurrentUpdateIndex(&current_idx)))
				{
					m_cb(current_idx, current_percent, total_percent);
				}
				prg->Release();
			}
			return S_OK;
		}
	private:
		TProgressCallback m_cb;
	};
	class IICC : public IUNK<IInstallationCompletedCallback>
	{
	public:
		IICC(TNotificationCallback cb)
		{
			m_cb = cb;
			m_Event = CreateEvent(NULL, FALSE, FALSE, NULL);
		}
		~IICC()
		{
			CloseHandle(m_Event);
		}

		STDMETHODIMP Invoke(IInstallationJob* job, IInstallationCompletedCallbackArgs* args)
		{
			m_cb();
			SetEvent(m_Event);
			return S_OK;
		}

		auto GetEvent() const { return m_Event; };

	private:
		TNotificationCallback m_cb;
		HANDLE m_Event;
	};


	class win_updater : public CSingleton <win_updater>
	{
	public:
		win_updater();
		~win_updater();

		bool initialize();
		void destroy();

		void start_update();
		void start_redist_installer();
		void update_worker();

	private:
		std::atomic <bool> m_start_update;
		std::thread m_worker;
	};
};
