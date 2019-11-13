
#include "stdafx.h"
#include "hierarchyview.h"
#include "3dview.h"
#include <shlobj.h> // SHGetFolderPath

cHierarchyView::cHierarchyView(const StrId &name)
	: framework::cDockWindow(name)
	, m_keymapTexture(nullptr)
	, m_selPin(nullptr)
	, m_selFloor(nullptr)
	, m_projEditMode(eProjectEditMode::None)
	, m_pinImg(nullptr)
	, m_hierarchy(nullptr)
{
}

cHierarchyView::~cHierarchyView()
{
	m_editPc.Clear();
	m_tempPc.Clear();
	if (m_hierarchy)
	{
		common::DeleteFolderNode(m_hierarchy);
		m_hierarchy = nullptr;
	}
}


bool cHierarchyView::Init(graphic::cRenderer &renderer)
{
	m_pinImg = graphic::cResourceManager::Get()->LoadTexture(renderer
		, "./media/pin.png");

	return true;
}


void cHierarchyView::OnUpdate(const float deltaSeconds)
{
}


void cHierarchyView::OnRender(const float deltaSeconds)
{
	static bool isOpenNewProj = false;
	if (ImGui::Button("New Project"))
	{
		if (!isOpenNewProj)
		{
			m_editPc.Clear();
			m_editPc.m_project.name = "Project Name";
			m_selDate = nullptr;
			m_selFloor = nullptr;
			m_selPin = nullptr;

			// get current document directory path
			char my_documents[MAX_PATH];
			const HRESULT result = SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, my_documents);
			m_editPc.m_project.dir = (result == S_OK) ? my_documents : "";
			m_tempPc = m_editPc;
		}
		isOpenNewProj = true;
		m_projEditMode = eProjectEditMode::New;
	}

	if (ImGui::Button("Open Project"))
	{
		const StrPath fileName = common::OpenFileDialog(m_owner->getSystemHandle()
			, { {L"Project File (*.prj)", L"*.prj"}
				, {L"All File (*.*)", L"*.*"} });
		if (!fileName.empty())
		{
			if (!g_global->ReadProjectFile(fileName))
			{
				::MessageBoxA(m_owner->getSystemHandle()
					, "Error!! Read Project File (Internal Error)", "ERROR"
					, MB_OK | MB_ICONERROR);
			}
		}
	}

	if (ImGui::Button("Save Project"))
	{
		if (g_global->m_pcDb.IsLoad())//null project return
		{
			StrPath fileName = common::SaveFileDialog(m_owner->getSystemHandle()
				, { {L"Project File (*.prj)", L"*.prj"}
					, {L"All File (*.*)", L"*.*"} });
			if (!fileName.empty())
			{
				if (g_global->m_pcDb.Write(fileName))
				{
					::MessageBoxA(m_owner->getSystemHandle()
						, "Success!! Write Project File", "CONFIRM"
						, MB_OK | MB_ICONINFORMATION);
				}
				else
				{
					::MessageBoxA(m_owner->getSystemHandle()
						, "Error!! Write Project File (Internal Error)", "ERROR"
						, MB_OK | MB_ICONERROR);
				}
			}
		}
	}

	if (ImGui::Button("Project Information")) // Modify Button
	{
		if (g_global->m_pcDb.IsLoad())//null project return
		{
			m_editPc = g_global->m_pcDb;
			m_tempPc = m_editPc;
			m_selDate = nullptr;
			m_selFloor = nullptr;
			m_selPin = nullptr;

			isOpenNewProj = true;
			m_projEditMode = eProjectEditMode::Modify;
		}
	}

	if (ImGui::Button("Refresh"))
	{
		if (g_global->m_pcDb.IsLoad())//null project return
		{
			UpdateDirectoryHierarchy(g_global->m_pcDb.m_project.dir);
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
	RenderHierarchy2(m_hierarchy);

	if (isOpenNewProj)
	{
		isOpenNewProj = RenderNewProjectDlg();
		if (!isOpenNewProj)
			m_projEditMode = eProjectEditMode::None;
	}
}


// New Project Dialog
bool cHierarchyView::RenderNewProjectDlg()
{
	static bool isOpenEditLink = false;
	cPointCloudDB::sProject &proj = m_editPc.m_project;

	bool open = true;
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		;

	// change background color (default window too transparent)
	const ImVec4 tc = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
	Vector4 tc2(tc.x, tc.y, tc.z, tc.w);
	Vector4 tc3 = tc2 * 1.4f;
	tc3.w = 0.98f;
	ImVec4 bgCol = *(ImVec4*)&tc3;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, bgCol);

	const sf::Vector2u psize = m_owner->getSize();
	const ImVec2 size(500, 250);
	const ImVec2 pos(psize.x / 2.f - size.x / 2.f
		, psize.y / 2.f - size.y / 2.f);
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowBgAlpha(1.f);

	const StrId titleBarName = (eProjectEditMode::New == m_projEditMode) ?
		"New Project" : "Modify Project";
	if (ImGui::Begin(titleBarName.c_str(), &open, flags))
	{
		// 프로젝트명 설정
		ImGui::TextUnformatted("Project Name :        ");
		ImGui::SameLine();
		ImGui::InputText("##name", proj.name.m_str, proj.name.SIZE);

		// 프로젝트 디렉토리 설정
		ImGui::TextUnformatted("Directory Path :       ");
		ImGui::SameLine();
		ImGui::InputText("##directory", proj.dir.m_str, proj.dir.SIZE);

		ImGui::PushID((int)proj.dir.m_str);
		ImGui::SameLine();
		if (ImGui::Button("..."))
		{
			const StrPath path = common::BrowseFolder(m_owner->getSystemHandle()
				, "Select Project Directory"
				, proj.dir.c_str());
			if (!path.empty())
				proj.dir = path;
		}
		ImGui::PopID();

		ImGui::Spacing();
		ImGui::SetCursorPosX(360);
		if (ImGui::Button("Edit Pin", ImVec2(100,0)))
		{
			isOpenEditLink = true;

			if (!m_selDate && !proj.dates.empty())
				m_selDate = proj.dates[0];
			if (!m_selFloor && m_selDate && !m_selDate->floors.empty())
				m_selFloor = m_selDate->floors[0];

			// read keymap texture file
			if (m_selFloor && !m_selFloor->keymapFileName.empty())
			{
				m_keymapTexture = graphic::cResourceManager::Get()->LoadTexture(
					GetRenderer(), m_selFloor->keymapFileName);
			}
		}

		ImGui::Spacing();

		ImGui::SetCursorPosX(330);
		ImGui::SetCursorPosY(210);
		if (eProjectEditMode::New == m_projEditMode)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.1f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.8f, 0.1f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.1f, 1.f));
			if (ImGui::Button("Create", ImVec2(70, 0)))
			{
				if (proj.dir.empty())
				{
					::MessageBoxA(m_owner->getSystemHandle()
						, "Fill Directory Path", "ERROR"
						, MB_OK | MB_ICONERROR);
				}
				else
				{
					const char c = proj.dir.m_str[proj.dir.size() - 1];
					if ((c != '\\') && (c != '/'))
						proj.dir += '\\';

					StrPath fileName = proj.dir + proj.name.c_str();
					const string ext = fileName.GetFileExt();
					if (ext.empty())
						fileName += ".prj";

					// point cloud data를 파일에 저장한다.
					cPointCloudDB pcd = m_editPc;
					if (pcd.Write(fileName))
					{
						// success write
						open = false; // close dialog

						// open new project
						if (!g_global->ReadProjectFile(fileName))
						{
							::MessageBoxA(m_owner->getSystemHandle()
								, "Error!! New Project File (Internal Error)", "ERROR"
								, MB_OK | MB_ICONERROR);
						}
					}
					else
					{
						::MessageBoxA(m_owner->getSystemHandle()
							, "Error Save Project File (Internal Error)"
							, "ERROR", MB_OK | MB_ICONERROR);
					}
				} //~path empty?
			}//~button create
			ImGui::PopStyleColor(3);
		}
		else if (eProjectEditMode::Modify == m_projEditMode)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.1f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0.1f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.1f, 1.f));
			if (ImGui::Button("Modify", ImVec2(70,0)))
			{
				cPointCloudDB pcd = m_editPc;
				StrPath fileName = g_global->m_pcDb.m_fileName;
				if (pcd.Write(fileName))
				{
					::MessageBoxA(m_owner->getSystemHandle()
						, "Success!! Write Project File", "CONFIRM"
						, MB_OK | MB_ICONINFORMATION);

					// open new project
					if (!g_global->ReadProjectFile(fileName))
					{
						::MessageBoxA(m_owner->getSystemHandle()
							, "Error!! Reed Project File (Internal Error)", "ERROR"
							, MB_OK | MB_ICONERROR);
					}
				}
				else
				{
					::MessageBoxA(m_owner->getSystemHandle()
						, "Error!! Write Project File (Internal Error)", "ERROR"
						, MB_OK | MB_ICONERROR);
				}
				open = false;
			}
			ImGui::PopStyleColor(3);
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(70,0)))
		{
			m_editPc.Clear();
			open = false;
		}
	}
	ImGui::End();

	// 카메라 연결 편집창 출력
	if (isOpenEditLink)
	{
		isOpenEditLink = RenderEditPinDlg();
	}

	ImGui::PopStyleColor();

	return open;
}


// 카메라 연결 편집창
bool cHierarchyView::RenderEditPinDlg()
{
	bool open = true;
	bool isApply = false;
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		;

	const sf::Vector2u psize = m_owner->getSize();
	const ImVec2 size(600, 700);
	const ImVec2 pos(psize.x / 2.f - size.x / 2.f
		, psize.y / 2.f - size.y / 2.f);
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowBgAlpha(1.f);

	if (ImGui::Begin("Edit Pin", &open, flags))
	{
		RenderDateEdit();
		RenderFloorEdit();

		// 도면 출력
		const ImVec2 keymapPos = ImGui::GetCursorPos();
		if (ImGui::BeginChild("image", ImVec2(320, 320), true))
		{
			const Vector2 size(300, 300); // keymap image size
			const Vector2 offset(2, 2); // dummy offset

			if (m_keymapTexture)
			{
				// render keymap
				ImGui::Image(m_keymapTexture->m_texSRV, *(ImVec2*)&size);

				// render pin image
				ImVec2 oldPos = ImGui::GetCursorPos();
				if (m_pinImg && m_selFloor)
				{
					for (auto &pin : m_selFloor->pins)
					{
						// keymapPos is uv coordinate system
						const Vector2 keymapPos = pin->keymapPos;

						const Vector2 pos = offset +
							Vector2(size.x * keymapPos.x
								, size.y * keymapPos.y);

						ImGui::SetCursorPos(*(ImVec2*)&pos);
						ImGui::Image(m_pinImg->m_texSRV, ImVec2(10, 10));

						const Vector2 txtPos = pos + Vector2(0, 10.f);
						ImGui::SetCursorPos(*(ImVec2*)&txtPos);
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 1, 1));
						ImGui::TextUnformatted(pin->name.c_str());
						ImGui::PopStyleColor();
					}
				}
				ImGui::SetCursorPos(oldPos); // recovery
			}
			else
			{
				// nothing to render
			}
		}
		ImGui::EndChild();

		//ImGui::Spacing();
		ImGui::Spacing();

		// 카메라 속성 편집 화면
		ImGui::Separator();

		RenderPinEdit();

		ImGui::SetCursorPosX(480);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.1f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0.1f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.1f, 1.f));
		if (ImGui::Button("Apply"))
		{
			isApply = true;
			open = false;
		}
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			open = false; // cancel setting
		}
	}
	ImGui::End();

	if (!open && !isApply)
	{
		m_editPc = m_tempPc;// recovery
		m_selDate = nullptr;
		m_selFloor = nullptr;
		m_selPin = nullptr;
	}

	return open;
}


bool cHierarchyView::RenderDateEdit()
{
	cPointCloudDB::sProject &proj = m_editPc.m_project;

	const StrId selDateName = (m_selDate) ? m_selDate->name : "";

	ImGui::TextUnformatted("Select Date :           ");
	ImGui::SameLine();
	ImGui::PushItemWidth(300);
	if (ImGui::BeginCombo("##datecombo", selDateName.c_str()))
	{
		for (auto &date : proj.dates)
		{
			if (ImGui::Selectable(date->name.c_str()))
			{
				m_selDate = date;
				m_selFloor = (!date->floors.empty()) ?
					date->floors[0] : nullptr;
				m_selPin = nullptr;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();

	ImGui::SameLine();
	//ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.1f, 1.f));
	//ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.8f, 0.1f, 1.f));
	//ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.2f, 0.1f, 1.f));
	ImGui::PushID((int)m_selDate + 1);
	if (ImGui::Button("Add"))
	{
		const StrPath path = common::BrowseFolder(m_owner->getSystemHandle()
			, "Select Project Directory"
			, proj.dir.c_str());
		if (!path.empty())
		{
			cPointCloudDB::sDate *date = new cPointCloudDB::sDate;
			date->name = path.GetFileName();//YYYY-MM-DD
			proj.dates.push_back(date);
			m_selDate = date;
			m_selFloor = nullptr;
			m_selPin = nullptr;
		}
	}
	ImGui::PopID();
	//ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0.1f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.1f, 0.1f, 1.f));
	ImGui::PushID((int)m_selDate + 2);
	if (ImGui::Button("Delete"))
	{
		if (m_selDate)
		{
			if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
				, "Date를 제거하시겠습니까?", "CONFIRM"
				, MB_YESNO | MB_ICONQUESTION))
			{
				m_editPc.RemoveDate(m_selDate->name);
				m_selDate = (!proj.dates.empty()) ? 
					proj.dates[0] : nullptr;
				m_selFloor = nullptr;
				m_selPin = nullptr;
			}
		}
	}
	ImGui::PopID();
	ImGui::PopStyleColor(3);

	return true;
}


bool cHierarchyView::RenderFloorEdit()
{
	const StrId selFloorName = (m_selFloor) ? m_selFloor->name : "";

	ImGui::TextUnformatted("Select Floor :           ");
	ImGui::SameLine();
	ImGui::PushItemWidth(300);
	if (ImGui::BeginCombo("##floorcombo", selFloorName.c_str()))
	{
		if (m_selDate)
		{
			for (auto &floor : m_selDate->floors)
			{
				if (ImGui::Selectable(floor->name.c_str()))
				{
					m_selFloor = floor;
				}
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();

	ImGui::SameLine();
	//ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.1f, 1.f));
	//ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.8f, 0.1f, 1.f));
	//ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.2f, 0.1f, 1.f));
	ImGui::PushID((int)m_selFloor + 1);
	if (ImGui::Button("Add"))
	{
		if (m_selDate)
		{
			cPointCloudDB::sFloor *floor = new cPointCloudDB::sFloor;
			floor->name.Format("%dF", m_selDate->floors.size() + 1);
			floor->keymapFileName = (m_selFloor) ? m_selFloor->keymapFileName : "";
			m_selDate->floors.push_back(floor);
			m_selFloor = floor;
		}
	}
	ImGui::PopID();
	//ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0.1f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.1f, 0.1f, 1.f));
	ImGui::PushID((int)m_selFloor + 2);
	if (ImGui::Button("Delete"))
	{
		if (m_selFloor)
		{
			if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
				, "Floor를 제거하시겠습니까?", "CONFIRM"
				, MB_YESNO | MB_ICONQUESTION))
			{
				if (m_selDate)
				{
					m_editPc.RemoveFloor(m_selDate, m_selFloor->name);
					m_selFloor = (!m_selDate->floors.empty()) ?
						m_selDate->floors[0] : nullptr;
					m_selPin = nullptr;
				}
			}
		}
	}
	ImGui::PopID();
	ImGui::PopStyleColor(3);

	if (m_selFloor)
	{
		ImGui::TextUnformatted("Edit Floor Name :    ");
		ImGui::SameLine();
		ImGui::InputText("##floor name", m_selFloor->name.m_str
			, m_selFloor->name.SIZE);

		// 도면 파일명 설정
		ImGui::TextUnformatted("KeyMap FileName : ");
		ImGui::SameLine();
		ImGui::InputText("##keymap", m_selFloor->keymapFileName.m_str
			, m_selFloor->keymapFileName.SIZE);

		ImGui::PushID((int)m_selFloor->keymapFileName.c_str());
		ImGui::SameLine();
		if (ImGui::Button("..."))
		{
			const StrPath fileName = common::OpenFileDialog(m_owner->getSystemHandle()
				, { {L"Image File (*.bmp; *.jpg; *.png)", L"*.bmp;*.jpg;*.png"}
					, {L"All File (*.*)", L"*.*"} });
			if (!fileName.empty())
			{
				m_selFloor->keymapFileName = fileName;

				m_keymapTexture = graphic::cResourceManager::Get()->LoadTexture(
					GetRenderer(), fileName);
			}
		}
		ImGui::PopID();
		ImGui::Spacing();
	}

	return true;
}


bool cHierarchyView::RenderPinEdit()
{
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.1f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.8f, 0.1f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.2f, 0.1f, 1.f));
	if (ImGui::Button("Add Pin"))
	{
		if (m_selFloor)
		{
			cPointCloudDB::sPin *pin = new cPointCloudDB::sPin;
			pin->name = "Pin";
			pin->tessScale = 0.02f;
			pin->keymapPos = Vector2(0, 0);
			m_selFloor->pins.push_back(pin);
		}
	}
	ImGui::PopStyleColor(3);

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0.1f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.1f, 0.1f, 1.f));
	ImGui::SameLine();
	if (ImGui::Button("Delete Pin"))
	{
		if (m_selPin)
		{
			if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
				, "Pin을 제거하시겠습니까?", "CONFIRM"
				, MB_YESNO | MB_ICONQUESTION))
			{
				if (m_selFloor)
					g_global->m_pcDb.RemovePin(m_selFloor, m_selPin->name);
				m_selPin = nullptr;
			}
		}
	}
	ImGui::PopStyleColor(3);

	ImGui::Spacing();

	const float h = 170;
	const float curY = ImGui::GetCursorPosY();
	if (ImGui::BeginChild("pin list", ImVec2(100, h), true))
	{
		if (m_selFloor)
		{
			for (auto &pin : m_selFloor->pins)
			{
				ImGui::PushID(pin);
				const bool isSel = pin == m_selPin;
				if (ImGui::Selectable(pin->name.c_str(), isSel))
					m_selPin = pin;
				ImGui::PopID();
			}
		}
	}
	ImGui::EndChild();

	ImGui::SetCursorPosX(120);
	ImGui::SetCursorPosY(curY);
	if (ImGui::BeginChild("pin info", ImVec2(470, h), true))
	{
		if (m_selPin)
		{
			ImGui::Spacing();

			ImGui::TextUnformatted("Name :                  ");
			ImGui::SameLine();
			ImGui::InputText("##name", m_selPin->name.m_str, m_selPin->name.SIZE);

			ImGui::TextUnformatted("PCD FileName :     ");
			ImGui::SameLine();
			ImGui::InputText("##pcd", m_selPin->pc3dFileName.m_str, m_selPin->pc3dFileName.SIZE);

			ImGui::PushID((int)m_selPin->pc3dFileName.m_str);
			ImGui::SameLine();
			if (ImGui::Button("..."))
			{
				const StrPath fileName = common::OpenFileDialog(m_owner->getSystemHandle()
					, { {L"pcd File (*.obj)", L"*.obj"}
						, {L"All File (*.*)", L"*.*"} });
				if (!fileName.empty())
				{
					m_selPin->pc3dFileName = fileName;
				}
			}
			ImGui::PopID();

			ImGui::TextUnformatted("Texture FileName :");
			ImGui::SameLine();
			ImGui::InputText("##texture", m_selPin->pcTextureFileName.m_str, m_selPin->pcTextureFileName.SIZE);

			ImGui::PushID((int)m_selPin->pcTextureFileName.m_str);
			ImGui::SameLine();
			if (ImGui::Button("..."))
			{
				const StrPath fileName = common::OpenFileDialog(m_owner->getSystemHandle()
					, { {L"Image File (*.bmp; *.jpg; *.png)", L"*.bmp;*.jpg;*.png"}
						, {L"All File (*.*)", L"*.*"} });
				if (!fileName.empty())
				{
					m_selPin->pcTextureFileName = fileName;
				}
			}
			ImGui::PopID();

			ImGui::TextUnformatted("Position :                ");
			ImGui::SameLine();
			ImGui::DragFloat2("##position", (float*)&m_selPin->keymapPos, 0.001f, 0.f, 1.f);

			ImGui::TextUnformatted("point scale :           ");
			ImGui::SameLine();
			ImGui::DragFloat("##tessScale", &m_selPin->tessScale, 0.0001f, 0.f, 10.f);
		}
	}
	ImGui::EndChild(); //~pin info

	return true;
}


// render directory hierarch tree view
bool cHierarchyView::RenderHierarchy(common::sFolderNode *node)
{
	RETV(!node, false);

	for (auto &kv : node->children)
	{
		auto &p = kv.second;

		common::Str128 text = kv.first;
		if (ImGui::TreeNode(text.utf8().c_str()))
		{
			RenderHierarchy(p);
			ImGui::TreePop();
		}
	}

	for (auto &str : node->files)
	{
		common::Str128 text = str;
		ImGui::Selectable(text.utf8().c_str());
	}

	return true;
}


// render directory hierarch list view
bool cHierarchyView::RenderHierarchy2(common::sFolderNode *node)
{
	RETV(!node, false);

	const string &selDateStr = g_global->m_cDateStr;
	const string &selFloorStr = g_global->m_cFloorStr;

	common::sFolderNode *dateNode = nullptr;

	// date list
	for (auto &kv : node->children)
	{
		common::Str128 text = kv.first;
		const bool isSelect = common::Str128(selDateStr) == text;
		if (ImGui::Selectable(text.utf8().c_str(), isSelect))
		{
			g_global->m_cDateStr = text.c_str();
			dateNode = kv.second;
		}
		if (isSelect)
			dateNode = kv.second;
	}

	if (!dateNode)
		return true;

	ImGui::Spacing();
	ImGui::Spacing();

	common::sFolderNode *floorNode = nullptr;
	bool isFloorClicked = false;

	// floor list
	for (auto &kv : dateNode->children)
	{
		common::Str128 text = kv.first;
		const bool isSelect = common::Str128(selFloorStr) == text;
		ImGui::Indent(20);
		if (ImGui::Selectable(text.utf8().c_str(), isSelect))
		{
			g_global->m_cFloorStr = text.c_str();
			floorNode = kv.second;
			isFloorClicked = true;
		}
		ImGui::Unindent(20);
		if (isSelect)
			floorNode = kv.second;
	}

	if (!floorNode)
		return true;

	// select floor node
	if (isFloorClicked)
	{
		// load keymap file
		cPointCloudDB::sFloor *floor 
			= g_global->m_pcDb.FindFloor(selDateStr, selFloorStr);
		if (floor)
		{
			g_global->m_3dView->m_keyMap.m_texture
				= graphic::cResourceManager::Get()->LoadTexture(
					g_global->m_3dView->GetRenderer()
					, floor->keymapFileName);
		}
	}
	
	ImGui::Spacing();
	ImGui::Spacing();

	string selectFileName;
	bool isFileClicked = false;

	// file list
	for (auto &fileName : floorNode->files)
	{
		common::Str128 text = fileName;
		const bool isSelect = common::Str128(m_selFileStr) == text;
		ImGui::Indent(40);
		if (ImGui::Selectable(text.utf8().c_str(), isSelect))
		{
			m_selFileStr = text.c_str();
			isFileClicked = true;
		}
		ImGui::Unindent(40);
		if (isSelect)
			selectFileName = fileName;
	}

	if (isFileClicked)
	{
		const string fileName = selDateStr + "/" + selFloorStr + "/" + m_selFileStr;
		// search 
	}

	return true;
}


// initialize directory hierarchy information
bool cHierarchyView::UpdateDirectoryHierarchy(
	const StrPath &searchPath)
{
	if (m_hierarchy)
		common::DeleteFolderNode(m_hierarchy);

	list<string> files;
	list<string> exts;
	exts.push_back(".bmp"); exts.push_back(".BMP");
	exts.push_back(".jpg"); exts.push_back(".JPG");
	exts.push_back(".png"); exts.push_back(".PNG");
	exts.push_back(".obj"); exts.push_back(".OBJ");
	exts.push_back(".txt"); exts.push_back(".TXT");
	common::CollectFiles2(exts, searchPath.c_str(), searchPath.c_str(), files);
	m_hierarchy = common::CreateFolderNode(files, true);

	return true;
}
