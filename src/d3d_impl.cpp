#include "../include/pch.hpp"
#include "../include/d3d_impl.hpp"
#include "../include/application.hpp"
#include "../include/log_helper.hpp"

namespace WinQuickUpdater
{
	// shared data
	static ImVec4 gs_v4ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// cotr & dotr
	CD3DManager::CD3DManager() :
		m_pD3DEx(nullptr), m_pD3DDevEx(nullptr), m_initialized(false), m_hWnd(nullptr)
	{
		ZeroMemory(&m_pD3DParams, sizeof(m_pD3DParams));
	}
	CD3DManager::~CD3DManager()
	{
		this->CleanupDevice();
	}

	void CD3DManager::CreateDevice()
	{
		auto hRet = Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3DEx);
		if (FAILED(hRet))
		{
			CLog::Instance().LogError(fmt::format("Direct3DCreate9Ex fail! Error: %u Ret: %p", GetLastError(), hRet), true);
			return;
		}

		// Create the D3DDevice
		ZeroMemory(&m_pD3DParams, sizeof(m_pD3DParams));
		m_pD3DParams.Windowed = TRUE;
		m_pD3DParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
		m_pD3DParams.hDeviceWindow = m_hWnd; 
		m_pD3DParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // enable vsync

		hRet = m_pD3DEx->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_pD3DParams, NULL, &m_pD3DDevEx);
		if (FAILED(hRet))
		{
			CLog::Instance().LogError(fmt::format("CreateDeviceEx fail! Error: %u Ret: %p", GetLastError(), hRet), true);
			return;
		}

		m_initialized = true;
	}

	void CD3DManager::CleanupDevice()
	{
		if (!this->m_initialized)
			return;
		this->m_initialized = false;

		if (this->m_pD3DDevEx)
		{
			this->m_pD3DDevEx->Release();
			this->m_pD3DDevEx = nullptr;
		}
		if (this->m_pD3DEx)
		{
			this->m_pD3DEx->Release();
			this->m_pD3DEx = nullptr;
		}
	}

	void CD3DManager::ResetDevice()
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();

		const auto hr = m_pD3DDevEx->Reset(&m_pD3DParams);
		if (hr == D3DERR_INVALIDCALL)
			IM_ASSERT(0);

		ImGui_ImplDX9_CreateDeviceObjects();
	}

	void CD3DManager::SetPosition(int width, int height)
	{
		if (this->m_pD3DDevEx)
		{
			this->m_pD3DParams.BackBufferWidth = width;
			this->m_pD3DParams.BackBufferHeight = height;

			this->ResetDevice();
		}
	}

	void CD3DManager::SetupImGui()
	{
		ImGui_ImplWin32_EnableDpiAwareness();

		// Setup Dear ImGui context
		ImGui::CreateContext();

		auto& io = ImGui::GetIO();
		io.IniFilename = nullptr;
		io.ConfigWindowsMoveFromTitleBarOnly = true;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui_ImplDX9_Init(m_pD3DDevEx);
		ImGui_ImplWin32_Init(m_hWnd);

		ImGui::StyleColorsDark();
		auto style = &ImGui::GetStyle();
		style->WindowTitleAlign = ImVec2(0.5f, 0.5f);
		style->WindowPadding = ImVec2(15, 8);
		style->WindowRounding = 5.0f;
		style->FramePadding = ImVec2(5, 5);
		style->FrameRounding = 4.0f;
		style->ItemSpacing = ImVec2(12, 8);
		style->ItemInnerSpacing = ImVec2(8, 6);
		style->IndentSpacing = 25.0f;
		style->ScrollbarSize = 15.0f;
		style->ScrollbarRounding = 9.0f;
		style->GrabMinSize = 5.0f;
		style->GrabRounding = 3.0f;
	}
	void CD3DManager::CleanupImGui()
	{
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}


	void CD3DManager::OnRenderFrame()
	{
		m_pD3DDevEx->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(255, 255, 255), 1.0f, 0);

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		static auto s_ui = CApplication::Instance().get_ui();

		s_ui->OnDraw();

		if (!s_ui->IsOpen())
			SendMessage(CApplication::Instance().get_window()->get_handle(), WM_DESTROY, 0, 0);

		ImGui::EndFrame();

		m_pD3DDevEx->SetRenderState(D3DRS_ZENABLE, FALSE);
		m_pD3DDevEx->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		m_pD3DDevEx->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

		ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * 255.0f), (int)(clear_color.y * 255.0f), (int)(clear_color.z * 255.0f), (int)(clear_color.w * 255.0f));
		m_pD3DDevEx->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);

		if (m_pD3DDevEx->BeginScene() >= 0)
		{
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			m_pD3DDevEx->EndScene();
		}

		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();

		HRESULT result = m_pD3DDevEx->Present(NULL, NULL, NULL, NULL);

		if (result == D3DERR_DEVICELOST && m_pD3DDevEx->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
			ResetDevice();
	}
};
