//
// Point Cloud Viewer
//
#include "stdafx.h"
#include "view/3dview.h"
#include "view/infoview.h"
#include "view/hierarchyview.h"


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
	ImGui::SameLine(0, 2);
	ImGui::ImageButton(m_projectIconTex->m_texSRV, m_projectIconSize);

	ImGui::PushFont(m_fontBig);
	const float w = m_titleIconSize.x + m_projectIconSize.x + 60;
	ImGui::SameLine(0, 0);
	ImGui::Button(m_title.c_str(), ImVec2((float)getSize().x - w - systemBtnSizeX3, m_titleBarHeight));
	ImGui::PopFont();

	// TitleBar Click?
	if (ImGui::IsMouseDown(0)
		&& ((ImGui::IsItemHovered() && (eState::NORMAL_DOWN == cRenderWindow::m_state))
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
			if (ImGui::MenuItem("New", NULL))
				g_global->m_hierarchyView->NewProject();
			if (ImGui::MenuItem("Open", NULL))
				g_global->m_hierarchyView->OpenProject();
			if (ImGui::MenuItem("Save", NULL))
				g_global->m_hierarchyView->SaveProject();
			if (ImGui::MenuItem("Project Setting", NULL))
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
		switch (evt.key.cmd)
		{
		case sf::Keyboard::Escape: close(); break;
		}
		break;
	}
}
