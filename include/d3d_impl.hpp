#pragma once
#include "abstract_singleton.hpp"

namespace WinQuickUpdater
{
	class CD3DManager : public CSingleton <CD3DManager>
	{
	public:
		CD3DManager();
		virtual ~CD3DManager();

		void CreateDevice();
		void CleanupDevice();
		void ResetDevice();

		void SetupImGui();
		void CleanupImGui();

		void OnRenderFrame();

		void SetWindow(HWND hWnd) { m_hWnd = hWnd; };
		void SetPosition(int width, int height);

		auto GetD3D() const { return m_pD3DEx; };
		auto GetDevice() const { return m_pD3DDevEx; };
		auto GetParameters() const { return m_pD3DParams; };
		HWND GetWindow() const { return m_hWnd; };

	private:
		bool m_initialized;
		HWND m_hWnd;

		LPDIRECT3D9EX		m_pD3DEx;
		LPDIRECT3DDEVICE9EX	m_pD3DDevEx;
		D3DPRESENT_PARAMETERS m_pD3DParams;
	};
};
