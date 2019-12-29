//
// Point Cloud Viewer
//
#include "stdafx.h"
#include "view/3dview.h"
#include "view/infoview.h"
#include "view/hierarchyview.h"
#include <chrono>
#include <ctime>


class cViewer : public framework::cGameMain2
{
public:
	cViewer();
	virtual ~cViewer();
	virtual bool OnInit() override;
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnRenderMenuBar() override;
	virtual void OnRenderTitleBar() override;
	virtual void OnEventProc(const sf::Event &evt) override;
protected:
	bool ReadRegistry();
public:
	graphic::cTexture *m_titleIconTex; // 현대건설 아이콘
	graphic::cTexture *m_projectIconTex; // 프로젝트 아이콘
	ImVec2 m_titleIconSize;
	ImVec2 m_projectIconSize;
};


using namespace graphic;
using namespace framework;

INIT_FRAMEWORK3(cViewer);

cGlobal *g_global = NULL;

cViewer::cViewer()
	: m_titleIconTex(nullptr)
	, m_projectIconTex(nullptr)
	, m_titleIconSize(0, 45)
	, m_projectIconSize(0, 45)
{
	graphic::cResourceManager::Get()->SetMediaDirectory("./media/");

	m_windowName = L"Point Cloud Viewer";
	m_isMenuBar = true;
	m_isTitleBarOverriding = true;

	const float titleH = 50.f;
	m_titleBarHeight = titleH;
	m_titleBarHeight2 = titleH + 3.f;

	m_isLazyMode = true;
	const RECT r = { 0, 0, 1024, 768 };
	//const RECT r = { 0, 0, 1280, 960 };
	m_windowRect = r;
	//m_minDeltaTime = 1.f;
}

cViewer::~cViewer()
{
	SAFE_DELETE(g_global);
	//WSACleanup();
}


bool cViewer::OnInit()
{
	const float WINSIZE_X = float(m_windowRect.right - m_windowRect.left);
	const float WINSIZE_Y = float(m_windowRect.bottom - m_windowRect.top);
	GetMainCamera().SetCamera(Vector3(30, 30, -30), Vector3(0, 0, 0), Vector3(0, 1, 0));
	GetMainCamera().SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 0.1f, 10000.0f);
	GetMainCamera().SetViewPort(WINSIZE_X, WINSIZE_Y);

	m_camera.SetCamera(Vector3(-3, 10, -10), Vector3(0, 0, 0), Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 1.0f, 10000.f);
	m_camera.SetViewPort(WINSIZE_X, WINSIZE_Y);

	GetMainLight().Init(cLight::LIGHT_DIRECTIONAL,
		Vector4(0.3f, 0.3f, 0.3f, 1), Vector4(0.9f, 0.9f, 0.9f, 1),
		Vector4(0.2f, 0.2f, 0.2f, 1));
	const Vector3 lightPos(-300, 300, -300);
	const Vector3 lightLookat(0, 0, 0);
	GetMainLight().SetPosition(lightPos);
	GetMainLight().SetDirection((lightLookat - lightPos).Normal());

	// 타이틀 아이콘 생성
	m_titleIconTex = graphic::cResourceManager::Get()->LoadTexture(
		m_renderer, "./media/title_icon/현대건설_국문_가로.png");
	if (m_titleIconTex)
	{
		const float r = (float)m_titleIconTex->m_imageInfo.Width
			/ (float)m_titleIconTex->m_imageInfo.Height;
		m_titleIconSize.x = r * (float)m_titleIconSize.y;
	}

	m_projectIconTex = graphic::cResourceManager::Get()->LoadTexture(
		m_renderer, "./media/icon/3.png");
	if (m_projectIconTex)
	{
		const float r = (float)m_projectIconTex->m_imageInfo.Width
			/ (float)m_projectIconTex->m_imageInfo.Height;
		m_projectIconSize.x = r * (float)m_projectIconSize.y;
	}


	m_gui.SetContext();

	g_global = new cGlobal();
	g_global->Init();

	bool result = false;

	cHierarchyView *hierarchyView = new cHierarchyView("Hierarchy");
	hierarchyView->Create(eDockState::DOCKWINDOW, eDockSlot::TAB, this, NULL, 1.f
		, framework::eDockSizingOption::PIXEL);
	result = hierarchyView->Init(m_renderer);
	assert(result);

	c3DView *view = new c3DView("3D Map");
	view->Create(eDockState::DOCKWINDOW, eDockSlot::RIGHT, this, hierarchyView, 0.8f);
	result = view->Init(m_renderer);
	assert(result);

	cInfoView *infoView = new cInfoView("Information");
	infoView->Create(eDockState::DOCKWINDOW, eDockSlot::RIGHT, this, view, 0.25f
		, framework::eDockSizingOption::PIXEL);
	result = infoView->Init(m_renderer);
	assert(result);

	g_global->m_3dView = view;
	g_global->m_infoView = infoView;
	g_global->m_hierarchyView = hierarchyView;

	m_gui.SetContext();
	//m_gui.SetStyleColorsDark();
	m_gui.SetStyleColorsLight();

	if (!ReadRegistry())
	{
		dbg::Log("finish trial version\n");
		return false;
	}

	return true;
}


void cViewer::OnUpdate(const float deltaSeconds)
{
	__super::OnUpdate(deltaSeconds);
	cAutoCam cam(&m_camera);
	GetMainCamera().Update(deltaSeconds);
}


void cViewer::OnRender(const float deltaSeconds)
{
}


// registry read/write
// key : CurrentUser//Software//VR360//abc
// 사용기간이 30일 지나면 프로그램이 실행안되게하는 코드 추가
//
// 1. registry key가 없으면 추가하고, 그날 날짜를 저장한다.
//		format: 0xFF020191203 -> 20191203 ascii hexa 코드로 저장
// 2. 프로그램이 실행될 때마다 registry key 의 날짜를 검사해서 30일이 
//	  지나면 0xFF1~ 형태로 저장한다.
// 3. 이후 0xFF1로 값이 저장 되어있으면 프로그램 종료.
// 4. 0xFF0~ 형태이더라도 30일이 경과하면 프로그램 종료.
// 5. 프로그램을 새로 설치하더라도 registry key는 그대로 유지되기 때문에
//    프로그램이 실행 안됨
bool cViewer::ReadRegistry()
{
	const int DURATION = 90;
	bool reval = true;

	// open VR360 Regstry Key
	HKEY hKey;
	LONG lResult = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\VR360"
		, 0, KEY_ALL_ACCESS, &hKey);

	if (lResult != ERROR_SUCCESS) // not found registry key?
	{
		// check GVR registry key
		HKEY hKey0;
		lResult = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\GVR"
			, 0, KEY_ALL_ACCESS, &hKey0);
		if (lResult == ERROR_SUCCESS)
		{
			// 이미 Setup과정을 거친 프로그램일 경우, 새 Registry Key를 생성하는 
			// 과정은 취소된다.
			RegCloseKey(hKey0);
			return false;
		}
		else
		{
			DWORD dwDesc;
			lResult = RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\GVR", 0
				, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL
				, &hKey0, &dwDesc);
			if (lResult != ERROR_SUCCESS)
				return false; // fail
			RegCloseKey(hKey0);
		}

		// make VR360 registry key
		DWORD dwDesc;
		lResult = RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\VR360", 0
			, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL
			, &hKey, &dwDesc);
		if (lResult == ERROR_SUCCESS)
		{
			// store current date
			const string date = common::GetCurrentDateTime4();
			string date2 = "0xFF1";
			for (uint i = 0; i < date.length(); ++i)
			{
				char tmp[4];
				sprintf_s(tmp, "%2X", (int)date[i]);
				date2 += tmp;
			}
			RegSetValueExA(hKey, "abc", 0, REG_SZ, (BYTE*)date2.c_str(), date2.length());
			RegCloseKey(hKey);
		}
		else
		{
			reval = false; // fail
		}
	}
	else // found!!
	{
		// compare date and current
		char buffer[100];
		ZeroMemory(buffer, sizeof(buffer));
		DWORD dwType;
		DWORD dwBytes = sizeof(buffer);
		lResult = RegQueryValueExA(hKey, "abc", 0, &dwType, (LPBYTE)buffer, &dwBytes);
		if (lResult == ERROR_SUCCESS)
		{
			// 0xFF0~~
			if (buffer[4] != '1')
			{
				reval = false; // fail
			}
			else if ( (buffer[0] == '0') 
				&& (buffer[1] == 'x')
				&& (buffer[2] == 'F')
				&& (buffer[3] == 'F')
				&& (buffer[4] == '1')
				)
			{
				int year0, year1, year2, year3;
				int month0, month1;
				int day0, day1;
				const int len = sscanf_s(buffer, "0xFF1%2X%2X%2X%2X%2X%2X%2X%2X"
					, &year0, &year1, &year2, &year3
					, &month0, &month1
					, &day0, &day1);
				if (len == 8)
				{
					// registry key date
					string date;
					date += (char)year0;
					date += (char)year1;
					date += (char)year2;
					date += (char)year3;
					const int year = atoi(date.c_str());
					date.clear();

					date += (char)month0;
					date += (char)month1;
					const int month = atoi(date.c_str());
					date.clear();

					date += (char)day0;
					date += (char)day1;
					const int day = atoi(date.c_str());
					date.clear();

					// compare date
					const string curDate = common::GetCurrentDateTime4();
					date += curDate[0];
					date += curDate[1];
					date += curDate[2];
					date += curDate[3];
					const  int cyear = atoi(date.c_str());
					date.clear();

					date += curDate[4];
					date += curDate[5];
					const  int cmonth = atoi(date.c_str());
					date.clear();

					date += curDate[6];
					date += curDate[7];
					const  int cday = atoi(date.c_str());
					date.clear();

					const int duration = common::DateCompare2(cyear, cmonth, cday
						, year, month, day);
					if (duration > DURATION)
					{
						// store 0xFF0~~
						const string date = common::format("%4d%2d%2d", year, month, day);
						string date2 = "0xFF0";
						for (uint i = 0; i < date.length(); ++i)
						{
							char tmp[4];
							sprintf_s(tmp, "%2X", (int)date[i]);
							date2 += tmp;
						}
						RegSetValueExA(hKey, "abc", 0, REG_SZ, (BYTE*)date2.c_str(), date2.length());
						reval = false; // fail
					}
				}
				else
				{
					reval = false; // fail
				}
			}
			else
			{
				reval = false; // fail
			}
		}
		else
		{
			reval = false; // fail
		}

		RegCloseKey(hKey);
	}

	return reval;
}


void cViewer::OnRenderTitleBar()
{
	const ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoFocusOnAppearing
		;

	const ImVec4 winBg(0.93f, 0.93f, 0.93f, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, winBg);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0,0,0,0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(100, m_titleBarHeight));

	ImGui::SetNextWindowPos(ImVec2(1, 1));
	ImGui::SetNextWindowSize(ImVec2((float)getSize().x-2, m_titleBarHeight2));
	ImGui::Begin("##titlebar", NULL, flags);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14.0f, 3.0f));
	const ImVec4 childBg = ImGui::GetStyle().Colors[ImGuiCol_ChildWindowBg];

	ImGui::PushStyleColor(ImGuiCol_Button, childBg);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, childBg);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, childBg);

	ImGui::SetCursorPos(ImVec2(0, 1));
	const float systemBtnSize = 23.f;
	const float systemBtnSizeX3 = (systemBtnSize * 3) + 28;

	ImGui::ImageButton(m_titleIconTex->m_texSRV, m_titleIconSize);
	const bool clicked1 = ImGui::IsMouseDown(0) && ImGui::IsItemHovered() 
		&& (eState::NORMAL_DOWN == cRenderWindow::m_state);
	ImGui::SameLine(0, 2);
	ImGui::ImageButton(m_projectIconTex->m_texSRV, m_projectIconSize);
	const bool clicked2 = ImGui::IsMouseDown(0) && ImGui::IsItemHovered()
		&& (eState::NORMAL_DOWN == cRenderWindow::m_state);

	ImGui::PushFont(m_fontBig);
	const float w = m_titleIconSize.x + m_projectIconSize.x + 60;
	ImGui::SameLine(0, 0);
	ImGui::Button(m_title.c_str(), ImVec2((float)getSize().x - w - systemBtnSizeX3, m_titleBarHeight));
	ImGui::PopFont();
	const bool clicked3 = ImGui::IsMouseDown(0) && ImGui::IsItemHovered()
		&& (eState::NORMAL_DOWN == cRenderWindow::m_state);

	// TitleBar Click?
	if (ImGui::IsMouseDown(0)
		&& ((clicked1 || clicked2 || clicked3)
			|| cDockManager::Get()->IsMoveState())
		)
	{
		if (ImGui::IsMouseDoubleClicked(0)) // Double Click, Maximize Window
		{
			ChangeState(eState::NORMAL);
			ImGui::GetIO().MouseDown[0] = false; // maximize window move bug fix

			WINDOWPLACEMENT wndPl;
			GetWindowPlacement(getSystemHandle(), &wndPl);
			ShowWindow(getSystemHandle(), (wndPl.showCmd == SW_MAXIMIZE) ? SW_RESTORE : SW_MAXIMIZE);
			m_isFullScreen = (wndPl.showCmd != SW_MAXIMIZE);
		}
		else if (!m_isFullScreen)
		{
			ChangeState(eState::MOVE);

			// TitleBar Click and Move
			POINT mousePos;
			GetCursorPos(&mousePos);
			ScreenToClient(getSystemHandle(), &mousePos);
			sf::Vector2i windowPos = getPosition();
			const Vector2 delta = Vector2((float)mousePos.x, (float)mousePos.y) - m_clickPos;
			windowPos += sf::Vector2i((int)delta.x, (int)delta.y);
			setPosition(windowPos);
		}
	}
	else
	{
		// TitleBar Click Release?
		if (IsMoveState() && !ImGui::IsMouseDown(0))
		{
			ChangeState(eState::NORMAL);
		}
		else if (eState::NORMAL_DOWN == cRenderWindow::m_state)
		{
			ChangeState(eState::NORMAL_DOWN_ETC);
		}
	}

	ImGui::PopStyleColor(3); // button color

	// Title minimize, maximize, restore, close Button Color
	const float col_main_hue = 0.0f / 255.0f;
	const float col_main_sat = 0.0f / 255.0f;
	const float col_main_val = 80.0f / 255.0f;
	const ImVec4 col_main = ImColor::HSV(col_main_hue, col_main_sat, col_main_val);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(col_main.x, col_main.y, col_main.z, 0.44f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(col_main.x, col_main.y, col_main.z, 0.86f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(col_main.x, col_main.y, col_main.z, 1.00f));

	ImGui::SameLine();
	if (ImGui::ImageButton(m_titleBtn[0]->m_texSRV, ImVec2(systemBtnSize, systemBtnSize)
		, ImVec2(0, 0), ImVec2(1, 1), 0)) // Minimize Button
	{
		ShowWindow(getSystemHandle(), SW_MINIMIZE);
	}

	ImGui::SameLine();
	WINDOWPLACEMENT wndPl;
	GetWindowPlacement(getSystemHandle(), &wndPl); // Toggle Maximize or Restore
	if (wndPl.showCmd == SW_MAXIMIZE)
	{
		if (ImGui::ImageButton(m_titleBtn[3]->m_texSRV, ImVec2(systemBtnSize, systemBtnSize)
			, ImVec2(0, 0), ImVec2(1, 1), 0)) // Restore Button
		{
			ShowWindow(getSystemHandle(), SW_RESTORE);
			m_isFullScreen = false;
		}
	}
	else
	{
		if (ImGui::ImageButton(m_titleBtn[1]->m_texSRV, ImVec2(systemBtnSize, systemBtnSize)
			, ImVec2(0, 0), ImVec2(1, 1), 0)) // Maximize Button
		{
			ShowWindow(getSystemHandle(), SW_MAXIMIZE);
			m_isFullScreen = true;
		}
	}

	ImGui::SameLine();
	if (ImGui::ImageButton(m_titleBtn[2]->m_texSRV, ImVec2(systemBtnSize, systemBtnSize)
		, ImVec2(0, 0), ImVec2(1, 1), 0)) // Close Button
	{
		close();
	}
	ImGui::PopStyleColor(3); // button color
	ImGui::PopStyleVar(1); // frame padding

	ImGui::End();
	ImGui::PopStyleColor(2); // window bg, border col
	ImGui::PopStyleVar(1); // menubar min size
}


void cViewer::OnRenderMenuBar() 
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			const bool isLoad = g_global->m_pcDb.IsLoad();

			if (ImGui::MenuItem("New", NULL))
				g_global->m_hierarchyView->NewProject();
			if (ImGui::MenuItem("Open", NULL))
				g_global->m_hierarchyView->OpenProject();
			if (ImGui::MenuItem("Save", nullptr, nullptr, isLoad))
				g_global->m_hierarchyView->SaveProject();
			if (ImGui::MenuItem("Save As", nullptr, nullptr, isLoad))
				g_global->m_hierarchyView->SaveAsProject();
			if (ImGui::MenuItem("Project Setting", nullptr, nullptr, isLoad))
				g_global->m_hierarchyView->ProjectSetting();

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("About Point Cloud Viewer", NULL))
			{
				const char *msg = "이 프로그램의 어떠한 부분도 HYUNDAI E&C와 SEESAW의\n"\
"    사전 서면 허락없이 복제, 전송, 복사할 수 없으며, \n"\
"   프로그램의 사용권한은 HYUNDAI E&C 임직원으로\n"\
"                  제한되어 있습니다.\n\n"\
"    Copyright 2019. SEESAW All rights reserved.";

				::MessageBoxA(m_hWnd
					, msg
					, "CONFIRM"
					, MB_OK | MB_ICONINFORMATION);
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}
}


void cViewer::OnEventProc(const sf::Event &evt)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (evt.type)
	{
	case sf::Event::KeyPressed:
		if (sf::Keyboard::Escape == evt.key.cmd)
		{
			if (eEditState::Zoom == g_global->m_state)
			{
				g_global->m_state = eEditState::VR360;
				g_global->m_3dView->m_camera.SetEyePos(Vector3(0, 0, 0));
				g_global->m_3dView->m_isUpdatePcWindowPos = true;
			}
			else if (eEditState::Measure == g_global->m_state)
			{
				g_global->m_state = eEditState::VR360;
			}
		}
		//switch (evt.key.cmd)
		//{
		//case sf::Keyboard::Escape: close(); break;
		//}
		break;
	}
}
