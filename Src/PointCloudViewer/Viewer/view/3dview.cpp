
#include "stdafx.h"
#include "3dview.h"
#include "hierarchyview.h"

using namespace graphic;
using namespace framework;


c3DView::c3DView(const string &name)
	: framework::cDockWindow(name)
	, m_isShowWireframe(false)
	, m_isShowEquirectangular(true)
	, m_isShowPointCloud(false)
	, m_isShowPointCloudMesh(false)
	, m_isShowPcMap(false)
	, m_isShowPopupMenu(false)
	, m_isBeginPopupMenu(false)
	, m_isUpdatePcWindowPos(false)
	, m_curPinInfo(nullptr)
	, m_pinImg(nullptr)
	, m_sphereRadius(1000)
	, m_pickPosDistance(10.f)
	, m_keymapBtnTex(nullptr)
	, m_shareBtnTex(nullptr)
	, m_keymapBtnSize(40, 20)
	, m_shareBtnSize(40, 20)
	, m_isShowKeymap(true)
	, m_isShowMeasure(true)
	, m_markupScale(0.35f)
{
}

c3DView::~c3DView()
{
}


bool c3DView::Init(cRenderer &renderer)
{
	const Vector3 eyePos = Vector3(0, 0, 0);
	const Vector3 lookAt = Vector3(100, 0, 0);

	const common::sRectf viewRect = GetWindowSizeAvailible();
	m_camera.SetCamera(eyePos, lookAt, Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, viewRect.Width() / viewRect.Height(), 1.f, 100000.f);
	m_camera.SetViewPort(viewRect.Width(), viewRect.Height());

	const Vector3 lightPos = Vector3(300, 1300, -400);
	graphic::GetMainLight().SetDirection((Vector3::Zeroes - lightPos).Normal());
	graphic::GetMainLight().Bind(renderer);

	cViewport vp = renderer.m_viewPort;
	vp.m_vp.Width = viewRect.Width();
	vp.m_vp.Height = viewRect.Height();
	m_renderTarget.Create(renderer, vp, DXGI_FORMAT_R8G8B8A8_UNORM, true, true
		, DXGI_FORMAT_D24_UNORM_S8_UINT);

	m_sphere.Create(renderer, m_sphereRadius, 100, 100
		, graphic::eVertexType::POSITION 
		//| graphic::eVertexType::NORMAL
		| graphic::eVertexType::TEXTURE0
	);


	m_keyMap.Create(renderer, 0, 50, 200, 200
		, (eVertexType::POSITION_RHW | eVertexType::COLOR | eVertexType::TEXTURE0));

	m_shader.Create(renderer, 
		"./media/shader11/uvcolor.fxo"
		//"./media/shader11/tess-pos_rhw-uvcolor.fxo"
		, "Unlit"
		, graphic::eVertexType::POSITION_RHW | graphic::eVertexType::COLOR 
		| graphic::eVertexType::TEXTURE0);

	m_quad1.m_shader = &m_shader;
	m_quad2.m_shader = &m_shader;
	m_quad3.m_shader = &m_shader;

	m_pointShader.Create(renderer,
		"./media/shader11/tess-box.fxo"
		, "Unlit"
		, graphic::eVertexType::POSITION 
		| graphic::eVertexType::NORMAL
		| graphic::eVertexType::COLOR
		| graphic::eVertexType::TEXTURE0
	);

	m_markup.Create(renderer, graphic::BILLBOARD_TYPE::ALL_AXIS, 1.f, 1.f, Vector3(0, 0, 0));

	m_pinImg = graphic::cResourceManager::Get()->LoadTexture(renderer
		, "./media/pin.png");

	cBoundingBox bbox(Vector3(0, 0, 0), Vector3(1, 1, 1)*0.2f, Quaternion());
	m_cube.Create(renderer, bbox
		, (graphic::eVertexType::POSITION | graphic::eVertexType::COLOR));

	// load markup icon texture file
	for (auto &markup : g_global->m_markups)
		markup.icon = cResourceManager::Get()->LoadTexture(renderer, markup.iconFileName.c_str());

	// 키맵 버튼 생성
	m_keymapBtnTex = graphic::cResourceManager::Get()->LoadTexture(
		renderer, "./media/icon/2.png");
	if (m_keymapBtnTex)
	{
		const float r = (float)m_keymapBtnTex->m_imageInfo.Height
		/ (float)m_keymapBtnTex->m_imageInfo.Width;
		m_keymapBtnSize.y = r * (float)m_keymapBtnSize.x;
	}

	// 공유 버튼 생성
	m_shareBtnTex = graphic::cResourceManager::Get()->LoadTexture(
		renderer, "./media/icon/sharebtn.png");
	if (m_shareBtnTex)
	{
		const float r = (float)m_shareBtnTex->m_imageInfo.Height
			/ (float)m_shareBtnTex->m_imageInfo.Width;
		m_shareBtnSize.y = r * m_shareBtnSize.x;
	}

	return true;
}


void c3DView::OnUpdate(const float deltaSeconds)
{
	m_camera.Update(deltaSeconds);
}


void c3DView::OnPreRender(const float deltaSeconds)
{
	cRenderer &renderer = GetRenderer();
	cAutoCam cam(&m_camera);

	renderer.UnbindTextureAll();
	GetMainCamera().Bind(renderer);
	GetMainLight().Bind(renderer);

	//const Vector4 bgColor(0.90f, 0.90f, 0.90f, 1.00f);
	const Vector4 bgColor(0.87f, 0.87f, 0.87f, 1.00f);
	if (m_renderTarget.Begin(renderer, bgColor))
	{
		CommonStates state(renderer.GetDevice());
		if (m_isShowWireframe)
		{
			renderer.GetDevContext()->RSSetState(state.Wireframe());
		}
		else
		{
			renderer.GetDevContext()->RSSetState(state.CullCounterClockwise());
		}

		if (m_isShowEquirectangular
			&& m_sphere.m_texture
			&& (string(m_sphere.m_texture->m_fileName.GetFileName()) != "white.dds")
			)
			m_sphere.Render(renderer);

		const float tessScale = m_curPinInfo ? m_curPinInfo->tessScale : 0.02f;

		if (m_isShowPointCloud)
		{
			if (m_isShowPointCloudMesh)
			{
				m_pointCloud.SetShader(nullptr);
				m_pointCloud.SetTechnique("Unlit");
				m_pointCloud.Render(renderer);

				renderer.GetDevContext()->OMSetDepthStencilState(state.DepthNone(), 0);
				m_pointCloud.SetShader(&m_pointShader);
				m_pointCloud.SetTechnique("Unlit_White");
				renderer.m_cbTessellation.m_v->size = Vector2(1, 1) * tessScale;
				m_pointCloud.RenderTessellation(renderer, 1);
				renderer.GetDevContext()->OMSetDepthStencilState(state.DepthDefault(), 0);
			}
			else
			{
				m_pointCloud.SetShader(&m_pointShader);
				m_pointCloud.SetTechnique("Unlit");
				renderer.m_cbTessellation.m_v->size = Vector2(1, 1) * tessScale;
				m_pointCloud.RenderTessellation(renderer, 1);
			}
		}
		
		if (m_isShowPcMap)
		{
			Transform tfm;
			renderer.m_cbTessellation.m_v->size = Vector2(1, 1) * tessScale;
			m_pcMapModel.SetShader(&m_pointShader);
			m_pcMapModel.RenderTessellation(renderer, 1, tfm.GetMatrixXM());
		}

		{
			Transform tfm;
			tfm.pos = m_pickPos;
			//tfm.scale = Vector3::Ones * 3.01f;
			tfm.scale = Vector3::Ones * 0.03f;
			m_cube.SetColor(cColor::YELLOW);
			m_cube.SetCube(tfm);
			m_cube.Render(renderer);
		}

		if (eEditState::Zoom != g_global->m_state)
			RenderMarkup(renderer);

		// render mouse point
		if (eEditState::Zoom != g_global->m_state)
		{
			const Ray ray = GetMainCamera().GetRay(m_mousePos.x, m_mousePos.y);
			renderer.m_dbgLine.SetColor(cColor::RED);
			renderer.m_dbgLine.SetLine(Vector3(0,0,0), ray.orig + ray.dir * 100.f, 0.001f);
			renderer.m_dbgLine.Render(renderer);
		}
		
		// Render Line Point to Memo Window
		cPointCloudDB::sPin *pin = g_global->m_pcDb.FindPin(
			g_global->m_dateName, g_global->m_floorName, g_global->m_pinName);
		if (pin && (eEditState::Zoom != g_global->m_state))
		{
			renderer.GetDevContext()->OMSetDepthStencilState(state.DepthNone(), 0);

			const Ray ray = GetMainCamera().GetRay();
			const Plane plane(ray.dir, ray.orig);
			for (auto &pc : pin->pcds)
			{
				if (plane.Distance(pc->pos) < 0.f)
					continue;
				if (pc->type != cPointCloudDB::sPCData::MEMO)
					continue;

				renderer.m_dbgLine.SetLine(pc->pos, pc->wndPos, 0.001f);
				renderer.m_dbgLine.Render(renderer);
			}

			renderer.GetDevContext()->OMSetDepthStencilState(state.DepthDefault(), 0);
		}

		// Render Line to PopupMenu
		if (m_isBeginPopupMenu)
		{
			const Ray ray = GetMainCamera().GetRay(m_popupMenuPos.x, m_popupMenuPos.y);
			renderer.m_dbgLine.SetColor(cColor::YELLOW);
			renderer.m_dbgLine.SetLine(m_pickPos, ray.orig + ray.dir * 100.f, 0.001f);
			renderer.m_dbgLine.Render(renderer);
		}

		if (!m_isShowEquirectangular
			&& m_isShowMeasure
			&& (eEditState::Zoom != g_global->m_state)
			)
			RenderMeasure(renderer);
		
		if (0) // uv zoom image
		{
			renderer.m_cbPerFrame.m_v->eyePosW = m_pickUV.GetVectorXM();
			m_quad1.Render(renderer);
			m_quad2.Render(renderer);
			m_quad3.Render(renderer);
		}
	}
	m_renderTarget.End(renderer);

	// capture vr360
	if (g_global->m_state == eEditState::Capture)
	{
		cPointCloudDB::sPin *pin = g_global->m_pcDb.FindPin(
			g_global->m_dateName, g_global->m_floorName, g_global->m_pinName);
		if (pin) // check load pin
		{
			// save capture file to project directory/capture folder
			// filename = datetime string + date + floor + pin
			const string dts = common::GetCurrentDateTime();
			string fileName;
			fileName = g_global->m_pcDb.m_project.dir.GetFullFileName().c_str();
			fileName += "\\capture";
			if (!common::IsFileExist(fileName)) // check capture folder
				CreateDirectoryA(fileName.c_str(), NULL); // make capture folder

			fileName += string("\\") + dts + " [" + g_global->m_dateName + "][" 
				+ g_global->m_floorName + "][" + g_global->m_pinName + "]";
			fileName += ".jpg";

			if (S_OK == DirectX::SaveWICTextureToFile(renderer.m_devContext
				, renderer.m_backBuffer, GUID_ContainerFormatJpeg
				, common::str2wstr(fileName).c_str()))
			{
				StrPath path = fileName;
				g_global->m_captures.push_back(path.GetFileName());
			}
		}

		g_global->m_state = eEditState::VR360;
	}
}


void c3DView::RenderMeasure(graphic::cRenderer &renderer)
{
	vector<sMeasurePt> *measurePts = g_global->GetCurrentMeasurePts();
	if (!measurePts)
		return;

	CommonStates state(renderer.GetDevice());
	renderer.GetDevContext()->OMSetDepthStencilState(state.DepthNone(), 0);

	float totLen = 0.f;
	for (uint i = 1; i < measurePts->size(); i+=2)
	{
		const sMeasurePt &p0 = measurePts->at(i - 1);
		const sMeasurePt &p1 = measurePts->at(i);

		renderer.m_dbgLine.SetColor(cColor::RED);
		//renderer.m_dbgLine.SetLine(p0.epos, p1.epos, 0.01f);
		renderer.m_dbgLine.SetLine(p0.rpos, p1.rpos, 1.f);
		renderer.m_dbgLine.Render(renderer);

		const float len = p0.rpos.Distance(p1.rpos);
		totLen += len;

		Transform tfm;
		tfm.pos = p1.epos + Vector3(0, 0.15f, 0);
		tfm.scale = Vector3::Ones * 0.1f;

		WStr128 text;
		text.Format(L"%.1f", len * 0.1f);
		renderer.m_textMgr.AddTextRender(renderer, i, text.c_str()
			, cColor::WHITE, cColor::BLACK, graphic::BILLBOARD_TYPE::ALL_AXIS
			, tfm, true
		);
	}

	renderer.GetDevContext()->OMSetDepthStencilState(state.DepthDefault(), 0);
}


void c3DView::OnRender(const float deltaSeconds)
{
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	m_viewPos = { (int)(pos.x), (int)(pos.y) };
	const common::sRectf viewRect = GetWindowSizeAvailible();
	ImGui::Image(m_renderTarget.m_resolvedSRV, ImVec2(viewRect.Width(), viewRect.Height()));

	// HUD
	// Render Menu Window
	const float MENU_WIDTH = 320.f;
	const float windowAlpha = 0.0f;
	bool isOpen = true;
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoBackground
		;
	ImGui::SetNextWindowPos(pos);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	ImGui::SetNextWindowSize(ImVec2(min(viewRect.Width(), MENU_WIDTH), 100.f));
	if (ImGui::Begin("Information", &isOpen, flags))
	{
		if (ImGui::Checkbox("Equirectangular", &m_isShowEquirectangular))
		{
			m_isShowPointCloud = !m_isShowEquirectangular;

			// equirectangular view에서는 측정기능을 끈다.
			if (m_isShowEquirectangular && (eEditState::Measure == g_global->m_state))
				g_global->m_state = eEditState::VR360;
		}

		ImGui::SameLine();
		if (ImGui::Checkbox("3D      ", &m_isShowPointCloud))
		{
			m_isShowEquirectangular = !m_isShowPointCloud;

			// equirectangular view에서는 측정기능을 끈다.
			if (m_isShowEquirectangular && (eEditState::Measure == g_global->m_state))
				g_global->m_state = eEditState::VR360;
		}

		if (m_isShowPointCloud)
		{
			ImGui::SameLine();
			ImGui::Checkbox("Mesh", &m_isShowPointCloudMesh);
		}
	
	}
	ImGui::End();
	ImGui::PopStyleColor();

	RenderShareFile();
	RenderKeymap(pos);
	RenderPopupmenu();
	RenderPcMemo();
}


// render keymap
void c3DView::RenderKeymap(const ImVec2 &pos)
{
	cPointCloudDB::sFloor *floor = g_global->m_pcDb.FindFloor(
		g_global->m_dateName, g_global->m_floorName);
	if (!floor)
		return;

	const common::sRectf viewRect = GetWindowSizeAvailible();
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoBackground
		;

	bool isOpen = true;
	const float btnH = m_keymapBtnSize.y + 20.f; // keymap button height
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + viewRect.Height() - 200.f - btnH));
	ImGui::SetNextWindowSize(ImVec2(min(viewRect.Width(), 200.f), 200.f + btnH));
	if (ImGui::Begin("keymap window", &isOpen, flags))
	{
		// keymap toggle button
		const ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
		ImGui::PushStyleColor(ImGuiCol_Button, col);
		if (ImGui::ImageButton(m_keymapBtnTex->m_texSRV, m_keymapBtnSize))
		{
			m_isShowKeymap = !m_isShowKeymap;
		}
		ImGui::PopStyleColor();

		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("Keymap On/Off");
			ImGui::EndTooltip();
		}

		if (g_global->m_pcDb.IsLoad()
			&& m_keyMap.m_texture
			&& m_isShowKeymap)
		{
			auto *srv = m_keyMap.m_texture->m_texSRV;
			const Vector2 keymapSize(200, 200); // keymap image size
			const Vector2 keymapPos(0, btnH);

			ImGui::SetCursorPos(*(ImVec2*)&keymapPos);
			ImGui::Image(srv, *(ImVec2*)&keymapSize);

			// render pin image
			ImVec2 oldPos = ImGui::GetCursorPos();
			if (m_pinImg)
			{
				const Vector2 offset(keymapPos.x - 5, keymapPos.y - 5); // dummy offset

				cPointCloudDB::sFloor *floor = g_global->m_pcDb.FindFloor(
					g_global->m_dateName, g_global->m_floorName);
				if (floor)
				{
					for (auto &pin : floor->pins)
					{
						// keymapPos is uv coordinate system
						const Vector2 uv = pin->keymapPos;
						const Vector2 pos = offset +
							Vector2(keymapSize.x * uv.x
								, keymapSize.y * uv.y);

						const ImVec4 pinColor = (pin == m_curPinInfo) ?
							ImVec4(1, 0, 0, 1) : ImVec4(0.4f, 0.2f, 0.2f, 1.f);

						ImGui::SetCursorPos(*(ImVec2*)&pos);
						ImGui::PushStyleColor(ImGuiCol_Button, pinColor);
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 0.2f, 0.2f, 1.f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.1f, 0.1f, 1.f));
						ImGui::PushID(pin);
						if (ImGui::Button(" ", ImVec2(10, 10)))
							JumpPin(pin->name.c_str());
						ImGui::PopID();
						ImGui::PopStyleColor(3);

						const Vector2 txtPos = pos + Vector2(0, 10.f);
						const ImVec4 txtColor = (pin == m_curPinInfo) ?
							ImVec4(1, 0, 0, 1) : ImVec4(0, 0, 1, 1);
						ImGui::SetCursorPos(*(ImVec2*)&txtPos);
						ImGui::PushStyleColor(ImGuiCol_Text, txtColor);
						ImGui::TextUnformatted(pin->name.utf8().c_str());
						ImGui::PopStyleColor();
					}
				}
			}
			ImGui::SetCursorPos(oldPos); // recovery
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();
}


// render popup menu
void c3DView::RenderPopupmenu()
{
	cPointCloudDB::sFloor *floor = g_global->m_pcDb.FindFloor(
		g_global->m_dateName, g_global->m_floorName);
	if (!floor || m_pointCloudPos.IsEmpty())
	{
		m_isShowPopupMenu = false;
		return;
	}

	if (m_isShowPopupMenu)
	{
		ImGui::OpenPopup("PopupMenu");
		m_isShowPopupMenu = false;
	}

	m_isBeginPopupMenu = false;
	if (ImGui::BeginPopup("PopupMenu"))
	{
		m_isBeginPopupMenu = true;
		if (ImGui::MenuItem("Memo"))
		{
			cPointCloudDB::sPCData *pc = g_global->m_pcDb.CreateData(
				floor, g_global->m_pinName);
			if (pc)
			{
				pc->type = cPointCloudDB::sPCData::MEMO;
				pc->pos = m_pointCloudPos;
				pc->uvpos = m_pointUV;

				// point cloud의 약간 오른쪽에 창을 위치시킨다.
				pc->wndPos = m_pointCloudPos + m_camera.GetRight() * 0.2f;
				pc->wndSize = Vector3(200, 150, 0);

				// point cloud information창 위치를 조정한다.
				m_isUpdatePcWindowPos = true;
			}
		}

		if (ImGui::BeginMenu("Mark-up"))
		{
			for (auto &markup : g_global->m_markups)
			{
				if (ImGui::MenuItem(StrId(markup.name).utf8().c_str()))
				{
					// add markup
					cPointCloudDB::sPCData *pc = g_global->m_pcDb.CreateData(
						floor, g_global->m_pinName);
					if (pc)
					{
						pc->type = cPointCloudDB::sPCData::MARKUP;
						pc->name = markup.name;
						pc->markup = markup.type;
						pc->pos = m_pointCloudPos;
						pc->uvpos = m_pointUV;
					}
				}
			}

			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}
}


// render point cloud memo
void c3DView::RenderPcMemo()
{
	if (eEditState::Zoom == g_global->m_state)
		return;

	// Render Point Cloud Information Window
	cPointCloudDB::sPin *pin = g_global->m_pcDb.FindPin(
		g_global->m_dateName, g_global->m_floorName, g_global->m_pinName);
	if (!pin)
		return;

	ImGuiWindowFlags wndFlags = 0;
	const Ray ray = m_camera.GetRay();
	const Plane plane(ray.dir, ray.orig);
	bool isUpdateWindowPos = false;
	set<int> rmPcs;

	for (auto &pc : pin->pcds)
	{
		if (plane.Distance(pc->pos) < 0.f)
			continue;
		if (cPointCloudDB::sPCData::MEMO != pc->type)
			continue;

		if (m_isUpdatePcWindowPos || m_mouseDown[1])
		{
			const Vector2 screenPos = m_camera.GetScreenPos(pc->wndPos);
			const ImVec2 wndPos(screenPos.x + m_rect.left
				, screenPos.y + m_rect.top + MENUBAR_HEIGHT2);
			ImGui::SetNextWindowPos(wndPos);
			ImGui::SetNextWindowSize(ImVec2(pc->wndSize.x, pc->wndSize.y));
		}

		ImGui::PushID(pc);
		bool isOpen = true;
		if (ImGui::Begin(pc->name.utf8().c_str(), &isOpen, wndFlags))
		{
			bool isStoreWndPos = false;

			// 메모 내용의 가장 윗줄을 타이틀 이름으로 설정한다.
			// 편집 도중에 윈도우 타이틀 이름을 바꾸면 포커스를 잃기 때문에
			// 포커스가 바뀔 때 업데이트 한다.
			static StrId focusWndName;
			StrId changeTitleWnd;
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
			{
				if (focusWndName != pc->name)
				{
					changeTitleWnd = focusWndName;
					focusWndName = pc->name;
				}
			}
			else
			{
				if (focusWndName == pc->name)
				{
					changeTitleWnd = focusWndName;
					focusWndName.clear();
				}
			}

			if (!changeTitleWnd.empty())
			{
				for (auto &p : pin->pcds)
				{
					if (changeTitleWnd == p->name)
					{
						vector<string> toks;
						common::tokenizer(p->desc.c_str(), "\n", "", toks);
						if (!toks.empty() && (p->name != toks[0]))
						{
							p->name = StrId(toks[0]).ansi();
							isUpdateWindowPos = true;
							break;
						}
					}
				}
			}
			//~ change window title name

			const ImVec2 parentWndSize = ImGui::GetWindowSize();
			const ImVec2 wndSize(parentWndSize.x - 20, parentWndSize.y - 50);
			ImGui::PushID(pc + 1);
			ImGui::InputTextMultiline("", pc->desc.m_str, pc->desc.SIZE, wndSize);
			ImGui::PopID();

			// 창이 마우스로 선택되면, 위치정보를 업데이트 한다.
			// 창은 사용자가 임의로 위치를 이동할 수 있다.
			if (m_mouseDown[0] && ImGui::IsWindowFocused() && ImGui::IsWindowHovered())
			{
				ImVec2 wndPos = ImGui::GetWindowPos();
				// 2d 좌표를 3d로 좌표로 변환해서 저장한다.
				const Ray r = m_camera.GetRay((int)(wndPos.x - m_rect.left)
					, (int)(wndPos.y - (m_rect.top + MENUBAR_HEIGHT2)));
				const float len = r.orig.Distance(pc->pos);
				Vector3 pos = r.orig + r.dir * len * 0.9f;
				pc->wndPos = pos;
			}
			else
			{
				const ImVec2 wndSize = ImGui::GetWindowSize();
				pc->wndSize = Vector3(wndSize.x, wndSize.y, 0);
			}
		}

		// close window -> show remove messagebox
		if (!isOpen)
		{
			Str128 msg;
			msg.Format("[ %s ] 메모를 제거하시겠습니까?", pc->name.c_str());
			if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
				, msg.c_str(), "CONFIRM", MB_YESNO))
			{
				rmPcs.insert(pc->id);
			}
		}

		ImGui::End();
		ImGui::PopID();
	}
	m_isUpdatePcWindowPos = isUpdateWindowPos;

	// remove point
	if (cPointCloudDB::sFloor *floor = g_global->m_pcDb.FindFloor(
		g_global->m_dateName, g_global->m_floorName))
	{
		for (auto &id : rmPcs)
			g_global->m_pcDb.RemoveData(floor, id);
	}
}


// render sharefile button
void c3DView::RenderShareFile()
{
	if (!m_curPinInfo)
		return;

	const common::sRectf viewRect = GetWindowSizeAvailible();
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoBackground
		;

	bool isOpen = true;
	ImGui::SetNextWindowPos(ImVec2((float)m_viewPos.x + viewRect.Width() - 60.f, (float)m_viewPos.y));
	ImGui::SetNextWindowSize(ImVec2(80, 80));
	if (ImGui::Begin("##sharebtn window", &isOpen, flags))
	{
		// keymap toggle button
		const ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
		ImGui::PushStyleColor(ImGuiCol_Button, col);
		if (ImGui::ImageButton(m_shareBtnTex->m_texSRV, m_shareBtnSize))
		{
			MakeShareFile();
		}
		ImGui::PopStyleColor();

		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("Share");
			ImGui::EndTooltip();
		}
	}
	ImGui::End();
}


// render markup icon in 3d space
void c3DView::RenderMarkup(graphic::cRenderer &renderer)
{
	// Render Point Cloud Information Window
	cPointCloudDB::sPin *pin = g_global->m_pcDb.FindPin(
		g_global->m_dateName, g_global->m_floorName, g_global->m_pinName);
	if (!pin)
		return;

	const Ray ray = m_camera.GetRay();
	const Plane plane(ray.dir, ray.orig);
	for (auto &pc : pin->pcds)
	{
		if (plane.Distance(pc->pos) < 0.f)
			continue;
		if (cPointCloudDB::sPCData::MARKUP != pc->type)
			continue;
		if (g_global->m_markups.size() <= (uint)pc->markup)
			continue;

		Transform tfm;
		tfm.pos = pc->pos;
		tfm.scale = Vector3::Ones * m_markupScale;
		m_markup.m_transform = tfm;
		m_markup.m_texture = g_global->m_markups[pc->markup].icon;
		m_markup.Render(renderer);
	}
}


void c3DView::OnResizeEnd(const framework::eDockResize::Enum type, const common::sRectf &rect)
{
	if (type == eDockResize::DOCK_WINDOW)
	{
		m_owner->RequestResetDeviceNextFrame();
	}
}


Vector3 c3DView::PickPointCloud(const POINT mousePt)
{
	const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
	const Line line(ray.dir, ray.orig, 10000.f);
	const Plane plane(ray.dir, ray.orig);
	
	sRawMeshGroup2 *rawMeshes = cResourceManager::Get()->LoadRawMesh(
		m_pointCloud.m_fileName.c_str());
	RETV(!rawMeshes, Vector3::Zeroes);

	Transform tfm; // no transform now
	Matrix44 tm = tfm.GetMatrix();

	Vector3 nearPos;
	float minLen = FLT_MAX;
	for (auto &meshes : rawMeshes->meshes)
	{
		for (auto &vtx : meshes.vertices)
		{
			const Vector3 pos = vtx * tm;

			// 카메라 방향의 반대쪽 버텍스는 제외
			if (plane.Distance(pos) < 0.f)
				continue;

			const float dist = line.GetDistance(pos);
			if (minLen > dist)
			{
				minLen = dist;
				nearPos = pos;
			}
		}
	}
	return nearPos;
}


bool c3DView::JumpPin(const string &pinName)
{
	g_global->m_pinName = pinName;

	// update window title name
	common::Str128 title;
	title.Format("[%s]-[%s]-[%s]-[%s]"
		, g_global->m_pcDb.m_project.name.c_str()
		, g_global->m_dateName.c_str(), g_global->m_floorName.c_str(), pinName.c_str());
	g_application->m_title = title.utf8();

	cPointCloudDB::sFloor *floor = g_global->m_pcDb.FindFloor(
		g_global->m_dateName, g_global->m_floorName);
	if (!floor)
	{
		m_sphere.m_texture = nullptr;
		m_pointCloud.Clear();
		return false;
	}
	cPointCloudDB::sPin *pin = g_global->m_pcDb.FindPin(floor, pinName);
	if (!pin)
	{
		m_sphere.m_texture = nullptr;
		m_pointCloud.Clear();
		return false;
	}

	// load equirectangular file
	m_sphere.m_texture = graphic::cResourceManager::Get()->LoadTexture(GetRenderer()
		, pin->pcTextureFileName);
	if ((!m_sphere.m_texture)
		|| ((m_sphere.m_texture) && (m_sphere.m_texture->m_fileName.find("white.dds"))))
	{
		Str128 msg;
		msg.Format("에러 발생!!\n[ %s ] 이미지 파일을 찾지 못했습니다.\nProject Setting에서 파일명을 확인하세요."
			, pin->pcTextureFileName.c_str());
		::MessageBoxA(m_owner->getSystemHandle(), msg.c_str(), "ERROR"
			, MB_OK | MB_ICONERROR);
	}

	// load 3d point cloud file
	if (!pin->pc3dFileName.empty())
		m_pointCloud.Create(GetRenderer(), common::GenerateId(), pin->pc3dFileName);

	// load point cloud map file
	{
		StrPath pcMapFileName = StrPath(pin->pcTextureFileName).GetFileNameExceptExt2();
		pcMapFileName += ".pcmap";
		if (g_global->LoadPCMap(pcMapFileName))
		{
			// load point cloud vertex
			m_pcMapModel.Create(GetRenderer(), common::GenerateId(), pcMapFileName);
		}
	}

	m_isUpdatePcWindowPos = true; // use OnRender() function
	m_curPinInfo = pin;

	return true;
}


// make share file
// 현재 보고있는 장면만 공유한다.
// date, floor, pin, markup, memo 정보
bool c3DView::MakeShareFile()
{
	if (!m_curPinInfo)
	{
		::MessageBoxA(m_owner->getSystemHandle(), "Error ShareFile1", "ERROR", MB_OK | MB_ICONERROR);
		return false;
	}

	// 현재 보고있는 장면만 복사한다.
	cPointCloudDB::sProject shareProj;
	if (!g_global->m_pcDb.MakeShareFile(g_global->m_dateName, g_global->m_floorName
		, g_global->m_pinName, shareProj))
	{
		::MessageBoxA(m_owner->getSystemHandle(), "Error ShareFile2", "ERROR", MB_OK | MB_ICONERROR);
		return false;
	}

	const string dateTimeStr = common::GetCurrentDateTime();

	// 공유용 폴더를 하나 생성하고, (project dir/share/today date/)
	// 공유할 파일만 공유폴더에 복사한다.
	// keymap, vr360 경로 정보를 바뀐 경로로 수정한다.
	for (auto &date : shareProj.dates)
	{
		for (auto &floor : date->floors)
		{
			string dstFileName;
			dstFileName = shareProj.dir.c_str();
			dstFileName += "share";

			// make share folder
			StrPath checkDir(dstFileName);
			if (!checkDir.IsFileExist())
			{
				if (!CreateDirectoryA(checkDir.c_str(), NULL))
				{
					::MessageBoxA(m_owner->getSystemHandle()
						, "Error ShareFile3", "ERROR", MB_OK | MB_ICONERROR);
					return false;
				}
			}

			// make share/current date folder
			dstFileName += "\\" + dateTimeStr;
			StrPath checkDir2(dstFileName);
			if (!checkDir2.IsFileExist())
			{
				if (!CreateDirectoryA(checkDir2.c_str(), NULL))
				{
					::MessageBoxA(m_owner->getSystemHandle()
						, "Error ShareFile4", "ERROR", MB_OK | MB_ICONERROR);
					return false;
				}
			}

			// make share/current date/date folder
			dstFileName += "\\" + g_global->m_dateName;
			StrPath checkDir3(dstFileName);
			if (!checkDir3.IsFileExist())
			{
				if (!CreateDirectoryA(checkDir3.c_str(), NULL))
				{
					::MessageBoxA(m_owner->getSystemHandle()
						, "Error ShareFile5", "ERROR", MB_OK | MB_ICONERROR);
					return false;
				}
			}

			// make share/current date/date/floor folder
			dstFileName += "\\" + g_global->m_floorName;
			StrPath checkDir4(dstFileName);
			if (!checkDir4.IsFileExist())
			{
				if (!CreateDirectoryA(checkDir4.c_str(), NULL))
				{
					::MessageBoxA(m_owner->getSystemHandle()
						, "Error ShareFile6", "ERROR", MB_OK | MB_ICONERROR);
					return false;
				}
			}

			// copy keymapfile to share folder
			const StrPath keymapFileName = dstFileName + "\\" + floor->keymapFileName.GetFileName();
			if (floor->keymapFileName.IsFileExist())
			{
				// copy to share folder
				if (!::CopyFileA(floor->keymapFileName.c_str(), keymapFileName.c_str(), FALSE))
				{
					::MessageBoxA(m_owner->getSystemHandle()
						, "Error ShareFile7", "ERROR", MB_OK | MB_ICONERROR);
					return false;
				}
				floor->keymapFileName = keymapFileName;
			}

			// copy point cloud data file
			for (auto &pin : floor->pins)
			{
				const StrPath pcdFileName = dstFileName + "\\" + pin->pc3dFileName.GetFileName();
				const StrPath textureFileName = dstFileName + "\\" + pin->pcTextureFileName.GetFileName();
				const StrPath srcPcmapFileName = pin->pc3dFileName.GetFileNameExceptExt2() + ".pcmap";
				const StrPath pcmapFileName = pcdFileName.GetFileNameExceptExt2() + ".pcmap";
				const StrPath srcMtlFileName = pin->pcTextureFileName.GetFilePathExceptFileName()
					+ "\\" + GetMtlFileName(pin->pc3dFileName).GetFileName();
				const StrPath mtlFileName = dstFileName + "\\" + srcMtlFileName.GetFileName();

				if (pin->pc3dFileName.IsFileExist())
				{
					// obj file copy to share folder
					if (!::CopyFileA(pin->pc3dFileName.c_str(), pcdFileName.c_str(), FALSE))
					{
						::MessageBoxA(m_owner->getSystemHandle()
							, "Error ShareFile8", "ERROR", MB_OK | MB_ICONERROR);
						return false;
					}

					pin->pc3dFileName = pcdFileName;
				}

				if (pin->pcTextureFileName.IsFileExist())
				{
					// texture file copy to share folder
					if (!::CopyFileA(pin->pcTextureFileName.c_str(), textureFileName.c_str(), FALSE))
					{
						::MessageBoxA(m_owner->getSystemHandle()
							, "Error ShareFile9", "ERROR", MB_OK | MB_ICONERROR);
						return false;
					}

					pin->pcTextureFileName = textureFileName;
				}

				if (srcPcmapFileName.IsFileExist())
				{
					// pcmap file copy to share folder
					if (!::CopyFileA(srcPcmapFileName.c_str(), pcmapFileName.c_str(), FALSE))
					{
						::MessageBoxA(m_owner->getSystemHandle()
							, "Error ShareFile10", "ERROR", MB_OK | MB_ICONERROR);
						return false;
					}
				}

				// *.mtl file copy to share folder
				if (srcMtlFileName.IsFileExist())
				{
					// pcmap file copy to share folder
					if (!::CopyFileA(srcMtlFileName.c_str(), mtlFileName.c_str(), FALSE))
					{
						::MessageBoxA(m_owner->getSystemHandle()
							, "Error ShareFile11", "ERROR", MB_OK | MB_ICONERROR);
						return false;
					}
				}

			}//~pins
		}//~floors
	}//~dates


	string dstFileName;
	dstFileName = shareProj.dir.c_str();
	dstFileName += "share";
	dstFileName += string("\\") + dateTimeStr + "\\" + "share_" + shareProj.name.c_str();
	dstFileName += ".prj";

	cPointCloudDB pcDb;
	pcDb.m_project = shareProj;
	pcDb.m_project.dir = shareProj.dir + "share\\" + dateTimeStr; // update project directory
	if (pcDb.Write(dstFileName))
	{
		Str128 text;
		text.Format("Success Save Share File\n\n %s", dstFileName.c_str());
		::MessageBoxA(m_owner->getSystemHandle()
			, text.c_str(), "CONFIRM", MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		::MessageBoxA(m_owner->getSystemHandle()
			, "Error ShareFile(0)", "ERROR", MB_OK | MB_ICONERROR);
	}

	return true;
}


// get mtl filename from obj file
StrPath c3DView::GetMtlFileName(const StrPath &objFileName)
{
	std::ifstream ifs(objFileName.c_str());
	if (!ifs.is_open())
		return "";

	int cnt = 0;
	string line;
	while (getline(ifs, line) && (cnt++ < 100))
	{
		if (string::npos != line.find("mtllib"))
		{
			vector<string> toks;
			common::tokenizer(line, " ", "", toks);
			if (toks.size() >= 2)
			{
				return toks[1];
			}
			else
			{
				break; // error occurred
			}
		}
	}

	return "";
}


void c3DView::UpdateLookAt(const POINT &mousePt)
{
	GetMainCamera().MoveCancel();
}


// 휠을 움직였을 때,
// 카메라 앞에 박스가 있다면, 박스 정면에서 멈춘다.
void c3DView::OnWheelMove(const float delta, const POINT mousePt)
{
	UpdateLookAt(mousePt);

	//float len = 0;
	//const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
	//Vector3 lookAt = m_groundPlane1.Pick(ray.orig, ray.dir);
	//len = (ray.orig - lookAt).Length();
	//const int lv = 10;// m_quadTree.GetLevel(len);
	//const float zoomLen = min(len * 0.1f, (float)(2 << (16 - lv)));
	//GetMainCamera().Zoom(ray.dir, (delta < 0) ? -zoomLen : zoomLen);

	g_global->m_state = eEditState::Zoom;
	GetMainCamera().MoveFront(delta*20.f);
}


// Handling Mouse Move Event
void c3DView::OnMouseMove(const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;

	const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);

	if (m_mouseDown[0])
	{
	}
	else if (m_mouseDown[1])
	{
		m_camera.Yaw(delta.x * 0.005f);
		m_camera.Pitch(delta.y * 0.005f);
	}
	else if (m_mouseDown[2])
	{
		const float len = GetMainCamera().GetDistance();
		GetMainCamera().MoveRight(-delta.x * len * 0.001f);
		GetMainCamera().MoveUp(delta.y * len * 0.001f);
	}

	const Vector2 uv = m_sphere.GetDirectionUV(ray);
	m_uv = uv;
	m_pickUV = uv;
}


// Handling Mouse Button Down Event
void c3DView::OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt)
{
	m_mousePos = mousePt;
	m_clickPos = mousePt;
	UpdateLookAt(mousePt);
	SetCapture();

	// if edit project info, ignore mouse event
	if (g_global->m_hierarchyView->m_isOpenNewProj)
		return;

	switch (button)
	{
	case sf::Mouse::Left:
		m_mouseDown[0] = true;
		break;
	case sf::Mouse::Right:
		m_mouseDown[1] = true;
		break;
	case sf::Mouse::Middle:
		m_mouseDown[2] = true;
		break;
	}
}


void c3DView::OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;
	ReleaseCapture();

	// if edit project info, ignore mouse event
	if (g_global->m_hierarchyView->m_isOpenNewProj)
		return;

	switch (button)
	{
	case sf::Mouse::Left:
	{
		m_mouseDown[0] = false;

		if (!m_isBeginPopupMenu)
		{
			// 체크박스 근처에서 클릭할 때, 무시
			const sRectf rect = sRectf::Rect(0, 0, 400, 50);
			if (rect.IsIn((float)mousePt.x, (float)mousePt.y))
				break;

			const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
			const Vector2 uv = m_sphere.GetDirectionUV(ray);
			m_pointUV = uv;
			m_pointUVRay = ray;
			const Vector3 pos = ray.dir * m_pickPosDistance + ray.orig;
			m_pickPos = pos;

			// pcmap picking
			//if ((eEditState::Measure == g_global->m_state)
			//	&& g_global->m_pcMap)
			//{
			//	// uv 위치의 point cloud position을 얻는다.
			//	const Vector3 pcPos = g_global->m_pcMap->GetPosition(uv);
			//	
			//	sMeasurePt measure;
			//	measure.epos = pos;
			//	measure.rpos = pcPos;
			//	measure.uv = uv;

			//	vector<sMeasurePt> *measurePts = g_global->GetCurrentMeasurePts();
			//	if (measurePts)
			//		measurePts->push_back(measure);
			//}

			if (eEditState::Measure == g_global->m_state)
			{
				const Vector3 pcPos = PickPointCloud(mousePt);

				sMeasurePt measure;
				measure.epos = pos;
				measure.rpos = pcPos;
				measure.uv = uv;

				vector<sMeasurePt> *measurePts = g_global->GetCurrentMeasurePts();
				if (measurePts)
					measurePts->push_back(measure);
			}
		}
	}
	break;

	case sf::Mouse::Right:
	{
		m_mouseDown[1] = false;

		// check popup menu
		const Vector2 dist((float)(m_clickPos.x - mousePt.x)
			, (float)(m_clickPos.y - mousePt.y));
		if (dist.Length() < 5)
		{
			// 마우스를 거의 움직이지 않은 상태에서
			// 마우스 오른쪽 버튼을 눌렀다 떼야 팝업메뉴가 출력된다.
			m_isShowPopupMenu = true;
			m_pointCloudPos = m_pickPos;
			m_popupMenuPos = mousePt;
		}
	}
	break;

	case sf::Mouse::Middle:
		m_mouseDown[2] = false;
		break;
	}
}


void c3DView::OnEventProc(const sf::Event &evt)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (evt.type)
	{
	case sf::Event::KeyPressed:
		switch (evt.key.cmd)
		{
		case sf::Keyboard::Return: break;
		case sf::Keyboard::Space: break;

			//case sf::Keyboard::Left: m_camera.MoveRight(-0.5f); break;
			//case sf::Keyboard::Right: m_camera.MoveRight(0.5f); break;
			//case sf::Keyboard::Up: m_camera.MoveUp(0.5f); break;
			//case sf::Keyboard::Down: m_camera.MoveUp(-0.5f); break;
		}
		break;

	case sf::Event::MouseMoved:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		OnMouseMove(pos);
	}
	break;

	case sf::Event::MouseButtonPressed:
	case sf::Event::MouseButtonReleased:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		const POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		const sRectf viewRect = GetWindowSizeAvailible(true);

		if (sf::Event::MouseButtonPressed == evt.type)
		{
			if (viewRect.IsIn((float)pos.x, (float)pos.y))
				OnMouseDown(evt.mouseButton.button, pos);
		}
		else
		{
			// 화면밖에 마우스가 있더라도 Capture 상태일 경우 Up 이벤트는 받게한다.
			if (viewRect.IsIn((float)pos.x, (float)pos.y)
				|| (this == GetCapture()))
				OnMouseUp(evt.mouseButton.button, pos);
		}
	}
	break;

	case sf::Event::MouseWheelScrolled:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		const POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		OnWheelMove(evt.mouseWheelScroll.delta, pos);
	}
	break;
	}
}


void c3DView::OnResetDevice()
{
	m_isUpdatePcWindowPos = true; // use OnRender() function

	cRenderer &renderer = GetRenderer();

	// update viewport
	common::sRectf viewRect = { 0, 0, m_rect.Width() - 15, m_rect.Height() - 50 };
	m_camera.SetViewPort(viewRect.Width(), viewRect.Height());

	cViewport vp = GetRenderer().m_viewPort;
	vp.m_vp.Width = viewRect.Width();
	vp.m_vp.Height = viewRect.Height();
	m_renderTarget.Create(renderer, vp, DXGI_FORMAT_R8G8B8A8_UNORM, true, true, DXGI_FORMAT_D24_UNORM_S8_UINT);
}
