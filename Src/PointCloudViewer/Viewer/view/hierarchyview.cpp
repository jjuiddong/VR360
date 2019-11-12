
#include "stdafx.h"
#include "hierarchyview.h"
#include "3dview.h"
#include <shlobj.h> // SHGetFolderPath

cHierarchyView::cHierarchyView(const StrId &name)
	: framework::cDockWindow(name)
	, m_keymapTexture(nullptr)
	, m_selCam(nullptr)
	, m_selFloor(nullptr)
	, m_projEditMode(eProjectEditMode::None)
	, m_pinImg(nullptr)
	, m_hierarchy(nullptr)
{
}

cHierarchyView::~cHierarchyView()
{
	cPointCloudDB::RemoveProjectData(m_editProj);

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
			cPointCloudDB::RemoveProjectData(m_editProj);
			m_editProj.name = "Project Name";

			cPointCloudDB::sFloor *floor = new cPointCloudDB::sFloor;
			floor->name = "F1";
			floor->keymapFileName = "C:\\PointCloud\\img.jpg";
			m_editProj.floors.push_back(floor);

			m_selFloor = floor;
			m_selCam = nullptr;

			// get current document directory path
			char my_documents[MAX_PATH];
			const HRESULT result = SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, my_documents);
			m_editProj.dir = (result == S_OK) ? my_documents : "";
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
			cPointCloudDB::CopyProjectData(g_global->m_pcDb.m_project, m_editProj);

			m_selFloor = nullptr;
			m_selCam = nullptr;
			
			if (!m_editProj.floors.empty())
				m_selFloor = m_editProj.floors[0];

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
		ImGui::InputText("##name", m_editProj.name.m_str, m_editProj.name.SIZE);

		// 프로젝트 디렉토리 설정
		ImGui::TextUnformatted("Directory Path :       ");
		ImGui::SameLine();
		ImGui::InputText("##directory", m_editProj.dir.m_str, m_editProj.dir.SIZE);

		ImGui::PushID((int)m_editProj.dir.m_str);
		ImGui::SameLine();
		if (ImGui::Button("..."))
		{
			const StrPath path = common::BrowseFolder(m_owner->getSystemHandle()
				, "Select Project Directory"
				, m_editProj.dir.c_str());
			if (!path.empty())
				m_editProj.dir = path;
		}
		ImGui::PopID();

		// 도면 파일명 설정
		//ImGui::TextUnformatted("KeyMap FileName : ");
		//ImGui::SameLine();
		//ImGui::InputText("##keymap", m_projKeymapFileName.m_str
		//	, m_projKeymapFileName.SIZE);

		//ImGui::PushID((int)m_projKeymapFileName.m_str);
		//ImGui::SameLine();
		//if (ImGui::Button("..."))
		//{
		//	const StrPath fileName = common::OpenFileDialog(m_owner->getSystemHandle()
		//		, { {L"Image File (*.bmp; *.jpg; *.png)", L"*.bmp;*.jpg;*.png"}
		//			, {L"All File (*.*)", L"*.*"} });
		//	if (!fileName.empty())
		//		m_projKeymapFileName = fileName;
		//}
		//ImGui::PopID();

		ImGui::Spacing();
		ImGui::SetCursorPosX(360);
		if (ImGui::Button("Edit Pin", ImVec2(100,0)))
		{
			isOpenEditLink = true;

			// read keymap texture file
			//if (!m_projKeymapFileName.empty())
			//{
			//	m_keymapTexture = graphic::cResourceManager::Get()->LoadTexture(
			//		GetRenderer(), m_projKeymapFileName);
			//}
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
				if (m_editProj.dir.empty())
				{
					::MessageBoxA(m_owner->getSystemHandle()
						, "Fill Directory Path", "ERROR"
						, MB_OK | MB_ICONERROR);
				}
				else
				{
					const char c = m_editProj.dir.m_str[m_editProj.dir.size() - 1];
					if ((c != '\\') && (c != '/'))
						m_editProj.dir += '\\';

					StrPath fileName = m_editProj.dir + m_editProj.name.c_str();
					const string ext = fileName.GetFileExt();
					if (ext.empty())
						fileName += ".prj";

					// point cloud data를 파일에 저장한다.

					cPointCloudDB pcd;
					cPointCloudDB::CopyProjectData(m_editProj, pcd.m_project);
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
				cPointCloudDB pcd;
				cPointCloudDB::CopyProjectData(m_editProj, pcd.m_project);
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
				pcd.m_project = {}; // clear data
				open = false;
			}
			ImGui::PopStyleColor(3);
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(70,0)))
		{
			cPointCloudDB::RemoveProjectData(m_editProj);
			open = false;
		}
	}
	ImGui::End();

	// 카메라 연결 편집창 출력
	if (isOpenEditLink)
	{
		isOpenEditLink = RenderEditCameraDlg();
	}

	ImGui::PopStyleColor();

	return open;
}


// 카메라 연결 편집창
bool cHierarchyView::RenderEditCameraDlg()
{
	bool open = true;
	bool isApply = false;
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		;

	const sf::Vector2u psize = m_owner->getSize();
	const ImVec2 size(600, 675);
	const ImVec2 pos(psize.x / 2.f - size.x / 2.f
		, psize.y / 2.f - size.y / 2.f);
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowBgAlpha(1.f);

	if (ImGui::Begin("Edit Pin", &open, flags))
	{
		const StrId selFloorName = (m_selFloor) ? m_selFloor->name : "F1";

		ImGui::TextUnformatted("Select Floor :           ");
		ImGui::SameLine();
		ImGui::PushItemWidth(300);
		if (ImGui::BeginCombo("##floorcombo", selFloorName.c_str()))
		{
			for (auto &floor : m_editProj.floors)
			{
				if (ImGui::Selectable(floor->name.c_str()))
				{
					m_selFloor = floor;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.1f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.8f, 0.1f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.2f, 0.1f, 1.f));
		if (ImGui::Button("Add"))
		{
			cPointCloudDB::sFloor *floor = new cPointCloudDB::sFloor;
			floor->name.Format("F%d", m_editProj.floors.size() + 1);
			floor->keymapFileName = (m_selFloor) ? m_selFloor->keymapFileName : "";
			m_editProj.floors.push_back(floor);
			m_selFloor = floor;
		}
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0.1f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.1f, 0.1f, 1.f));
		if (ImGui::Button("Delete"))
		{
			if (m_selFloor)
			{
				if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
					, "Floor를 제거하시겠습니까?", "CONFIRM"
					, MB_YESNO | MB_ICONQUESTION))
				{
					cPointCloudDB::RemoveFloor(m_selFloor);
					common::removevector(m_editProj.floors, m_selFloor);

					m_selFloor = (!m_editProj.floors.empty()) ? 
						m_editProj.floors[0] : nullptr;
					m_selCam = nullptr;
				}
			}
		}
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
					for (auto &cam : m_selFloor->cams)
					{
						// keymapPos is uv coordinate system
						const Vector2 keymapPos = cam->keymapPos;

						const Vector2 pos = offset +
							Vector2(size.x * keymapPos.x
								, size.y * keymapPos.y);

						ImGui::SetCursorPos(*(ImVec2*)&pos);
						ImGui::Image(m_pinImg->m_texSRV, ImVec2(10, 10));

						const Vector2 txtPos = pos + Vector2(0, 10.f);
						ImGui::SetCursorPos(*(ImVec2*)&txtPos);
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 1, 1));
						ImGui::TextUnformatted(cam->name.c_str());
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

		if (m_selFloor)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.1f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.8f, 0.1f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.2f, 0.1f, 1.f));
			if (ImGui::Button("Add Pin"))
			{
				cPointCloudDB::sCamera *cam = new cPointCloudDB::sCamera();
				cam->name = "Pin";
				cam->tessScale = 0.02f;
				m_selFloor->cams.push_back(cam);
			}
			ImGui::PopStyleColor(3);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.1f, 0.1f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.1f, 0.1f, 1.f));
			ImGui::SameLine();
			if (ImGui::Button("Delete Pin"))
			{
				if (m_selCam)
				{
					if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
						, "Pin을 제거하시겠습니까?", "CONFIRM"
						, MB_YESNO | MB_ICONQUESTION))
					{
						g_global->m_pcDb.RemoveCamera(m_selFloor, m_selCam->name);
						m_selCam = nullptr;
					}
				}
			}
			ImGui::PopStyleColor(3);

			ImGui::Spacing();

			const float h = 170;
			const float curY = ImGui::GetCursorPosY();
			if (ImGui::BeginChild("pin list", ImVec2(100, h), true))
			{
				for (auto &cam : m_selFloor->cams)
				{
					ImGui::PushID(cam);
					const bool isSel = cam == m_selCam;
					if (ImGui::Selectable(cam->name.c_str(), isSel))
						m_selCam = cam;
					ImGui::PopID();
				}
			}
			ImGui::EndChild();

			ImGui::SetCursorPosX(120);
			ImGui::SetCursorPosY(curY);
			if (ImGui::BeginChild("pin info", ImVec2(470, h), true))
			{
				if (m_selCam)
				{
					ImGui::Spacing();

					ImGui::TextUnformatted("Name :                  ");
					ImGui::SameLine();
					ImGui::InputText("##name", m_selCam->name.m_str, m_selCam->name.SIZE);

					ImGui::TextUnformatted("PCD FileName :     ");
					ImGui::SameLine();
					ImGui::InputText("##pcd", m_selCam->pc3dFileName.m_str, m_selCam->pc3dFileName.SIZE);

					ImGui::PushID((int)m_selCam->pc3dFileName.m_str);
					ImGui::SameLine();
					if (ImGui::Button("..."))
					{
						const StrPath fileName = common::OpenFileDialog(m_owner->getSystemHandle()
							, { {L"pcd File (*.obj)", L"*.obj"}
								, {L"All File (*.*)", L"*.*"} });
						if (!fileName.empty())
						{
							m_selCam->pc3dFileName = fileName;
						}
					}
					ImGui::PopID();

					ImGui::TextUnformatted("Texture FileName :");
					ImGui::SameLine();
					ImGui::InputText("##texture", m_selCam->pcTextureFileName.m_str, m_selCam->pcTextureFileName.SIZE);

					ImGui::PushID((int)m_selCam->pcTextureFileName.m_str);
					ImGui::SameLine();
					if (ImGui::Button("..."))
					{
						const StrPath fileName = common::OpenFileDialog(m_owner->getSystemHandle()
							, { {L"Image File (*.bmp; *.jpg; *.png)", L"*.bmp;*.jpg;*.png"}
								, {L"All File (*.*)", L"*.*"} });
						if (!fileName.empty())
						{
							m_selCam->pcTextureFileName = fileName;
						}
					}
					ImGui::PopID();

					ImGui::TextUnformatted("Position :                ");
					ImGui::SameLine();
					ImGui::DragFloat2("##position", (float*)&m_selCam->keymapPos, 0.001f, 0.f, 1.f);

					ImGui::TextUnformatted("point scale :           ");
					ImGui::SameLine();
					ImGui::DragFloat("##tessScale", &m_selCam->tessScale, 0.0001f, 0.f, 10.f);
				}
			}
			ImGui::EndChild(); //~pin info
		}//~selFloor

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
		// recovery
		cPointCloudDB::RemoveProjectData(m_editProj);
		cPointCloudDB::CopyProjectData(g_global->m_pcDb.m_project, m_editProj);
		m_selFloor = nullptr;
		m_selCam = nullptr;

		if (!m_editProj.floors.empty())
			m_selFloor = m_editProj.floors[0];
	}

	return open;
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

	const string &selDateStr = g_global->m_currentDateName;
	const string &selFloorStr = g_global->m_currentFloorName;

	common::sFolderNode *dateNode = nullptr;

	// date list
	for (auto &kv : node->children)
	{
		common::Str128 text = kv.first;
		const bool isSelect = common::Str128(selDateStr) == text;
		if (ImGui::Selectable(text.utf8().c_str(), isSelect))
		{
			g_global->m_currentDateName = text.c_str();
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
			g_global->m_currentFloorName = text.c_str();
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
		//g_global->m_currentFloor = g_global->m_pcDb.FindFloor(selFloorStr);
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
