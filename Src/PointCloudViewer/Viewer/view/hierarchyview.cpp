
#include "stdafx.h"
#include "hierarchyview.h"
#include "3dview.h"
#include <shlobj.h> // SHGetFolderPath

cHierarchyView::cHierarchyView(const StrId &name)
	: framework::cDockWindow(name)
	, m_keymapTexture(nullptr)
	, m_selCam(nullptr)
	, m_projEditMode(eProjectEditMode::None)
	, m_pinImg(nullptr)
{
}

cHierarchyView::~cHierarchyView()
{
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
			m_projName = "Project Name";
			m_projKeymapFileName = "C:\\PointCloud\\img.jpg";
			m_selCam = nullptr;

			// get current document directory path
			char my_documents[MAX_PATH];
			const HRESULT result = SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, my_documents);
			m_projDirPath = (result == S_OK) ? my_documents : "";
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
			m_editProj = g_global->m_pcDb.m_project;
			m_projName = m_editProj.name;
			m_projDirPath = m_editProj.dir;
			m_projKeymapFileName = m_editProj.keymapFileName;

			isOpenNewProj = true;
			m_projEditMode = eProjectEditMode::Modify;
		}
	}

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
		ImGui::InputText("##name", m_projName.m_str, m_projName.SIZE);

		// 프로젝트 디렉토리 설정
		ImGui::TextUnformatted("Directory Path :       ");
		ImGui::SameLine();
		ImGui::InputText("##directory", m_projDirPath.m_str, m_projDirPath.SIZE);

		ImGui::PushID((int)m_projDirPath.m_str);
		ImGui::SameLine();
		if (ImGui::Button("..."))
		{
			const StrPath path = common::BrowseFolder(m_owner->getSystemHandle()
				, "Select Project Directory"
				, m_projDirPath.c_str());
			if (!path.empty())
				m_projDirPath = path;
		}
		ImGui::PopID();

		// 도면 파일명 설정
		ImGui::TextUnformatted("KeyMap FileName : ");
		ImGui::SameLine();
		ImGui::InputText("##keymap", m_projKeymapFileName.m_str
			, m_projKeymapFileName.SIZE);

		ImGui::PushID((int)m_projKeymapFileName.m_str);
		ImGui::SameLine();
		if (ImGui::Button("..."))
		{
			const StrPath fileName = common::OpenFileDialog(m_owner->getSystemHandle()
				, { {L"Image File (*.bmp; *.jpg; *.png)", L"*.bmp;*.jpg;*.png"}
					, {L"All File (*.*)", L"*.*"} });
			if (!fileName.empty())
				m_projKeymapFileName = fileName;
		}
		ImGui::PopID();

		ImGui::Spacing();
		ImGui::SetCursorPosX(360);
		if (ImGui::Button("Edit Pin", ImVec2(100,0)))
		{
			isOpenEditLink = true;

			// read keymap texture file
			if (!m_projKeymapFileName.empty())
			{
				m_keymapTexture = graphic::cResourceManager::Get()->LoadTexture(
					GetRenderer(), m_projKeymapFileName);
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
				if (m_projDirPath.empty())
				{
					::MessageBoxA(m_owner->getSystemHandle()
						, "Fill Directory Path", "ERROR"
						, MB_OK | MB_ICONERROR);
				}
				else
				{
					const char c = m_projDirPath.m_str[m_projDirPath.size() - 1];
					if ((c != '\\') && (c != '/'))
						m_projDirPath += '\\';

					StrPath fileName = m_projDirPath + m_projName.c_str();
					const string ext = fileName.GetFileExt();
					if (ext.empty())
						fileName += ".prj";

					// point cloud data를 파일에 저장한다.
					// 임시객체를 만들어서 저장한다.
					m_editProj.name = m_projName.c_str();
					m_editProj.dir = m_projDirPath.c_str();
					m_editProj.keymapFileName = m_projKeymapFileName.c_str();

					cPointCloudDB pcd;
					pcd.m_project = m_editProj;
					if (pcd.Write(fileName))
					{
						// success write
						m_editProj = {}; // data clear
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
				pcd.m_project = m_editProj;
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
			m_editProj = {};
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
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		;

	const sf::Vector2u psize = m_owner->getSize();
	const ImVec2 size(600, 600);
	const ImVec2 pos(psize.x / 2.f - size.x / 2.f
		, psize.y / 2.f - size.y / 2.f);
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowBgAlpha(1.f);
	if (ImGui::Begin("Edit Pin", &open, flags))
	{
		// 도면 파일명 설정
		ImGui::TextUnformatted("KeyMap FileName : ");
		ImGui::SameLine();
		ImGui::InputText("##keymap", m_projKeymapFileName.m_str
			, m_projKeymapFileName.SIZE);

		ImGui::PushID((int)m_projKeymapFileName.m_str);
		ImGui::SameLine();
		if (ImGui::Button("..."))
		{
			const StrPath fileName = common::OpenFileDialog(m_owner->getSystemHandle()
				, { {L"Image File (*.bmp; *.jpg; *.png)", L"*.bmp;*.jpg;*.png"}
					, {L"All File (*.*)", L"*.*"} });
			if (!fileName.empty())
			{
				m_projKeymapFileName = fileName;

				m_keymapTexture = graphic::cResourceManager::Get()->LoadTexture(
					GetRenderer(), fileName);
			}
		}
		ImGui::PopID();
		ImGui::Spacing();

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
				if (m_pinImg)
				{
					for (auto &cam : m_editProj.cams)
					{
						// keymapPos is uv coordinate system
						const Vector2 keymapPos = (m_selCam == cam) ?
							m_camPos : cam->keymapPos;

						const Vector2 pos = offset +
							Vector2(size.x * keymapPos.x
								, size.y * keymapPos.y);

						ImGui::SetCursorPos(*(ImVec2*)&pos);
						ImGui::Image(m_pinImg->m_texSRV, ImVec2(10, 10));

						const Vector2 txtPos = pos + Vector2(0, 10.f);
						ImGui::SetCursorPos(*(ImVec2*)&txtPos);
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0,0,1,1));
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
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.1f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.8f, 0.1f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.2f, 0.1f, 1.f));
		if (ImGui::Button("Add Pin"))
		{
			cPointCloudDB::sCamera *cam = new cPointCloudDB::sCamera();
			cam->name = "Pin";
			cam->tessScale = 0.02f;
			m_editProj.cams.push_back(cam);
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
					common::removevector(m_editProj.cams, m_selCam);
					SAFE_DELETE(m_selCam);
				}
			}
		}
		ImGui::PopStyleColor(3);

		ImGui::Spacing();

		cPointCloudDB::sCamera *oldCam = m_selCam;

		const float h = 170;
		const float curY = ImGui::GetCursorPosY();
		if (ImGui::BeginChild("pin list", ImVec2(100, h), true))
		{
			for (auto &cam : m_editProj.cams)
			{
				ImGui::PushID(cam);
				const bool isSel = cam == m_selCam;
				if (ImGui::Selectable(cam->name.c_str(), isSel))
					m_selCam = cam;
				ImGui::PopID();
			}
		}
		ImGui::EndChild();

		// 선택된 카메라가 바꼈다면, 정보를 업데이트한다.
		if (oldCam != m_selCam)
		{
			m_camName = (m_selCam) ? m_selCam->name : "";
			m_camPcdFileName = (m_selCam) ? m_selCam->pc3dFileName : "";
			m_camTextureFileName = (m_selCam) ? m_selCam->pcTextureFileName : "";
			m_camPos = (m_selCam) ? m_selCam->keymapPos : Vector2(0, 0);
			m_camTessScale = (m_selCam) ? m_selCam->tessScale : 0.02f;
			oldCam = m_selCam;
		}

		ImGui::SetCursorPosX(120);
		ImGui::SetCursorPosY(curY);
		if (ImGui::BeginChild("pin info", ImVec2(470, h), true))
		{
			if (m_selCam)
			{
				ImGui::SetCursorPosX(400);

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.1f, 1.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0.1f, 1.f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.1f, 1.f));
				if (ImGui::Button("Apply"))
				{
					m_selCam->name = m_camName.c_str();
					m_selCam->pc3dFileName = m_camPcdFileName.c_str();
					m_selCam->pcTextureFileName = m_camTextureFileName.c_str();
					m_selCam->keymapPos = m_camPos;
					m_selCam->tessScale = m_camTessScale;
				}
				ImGui::PopStyleColor(3);

				//ImGui::SameLine();
				//if (ImGui::Button("Cancel"))
				//{
				//}
				ImGui::Spacing();

				ImGui::TextUnformatted("Name :                  ");
				ImGui::SameLine();
				ImGui::InputText("##name", m_camName.m_str, m_camName.SIZE);

				ImGui::TextUnformatted("PCD FileName :     ");
				ImGui::SameLine();
				ImGui::InputText("##pcd", m_camPcdFileName.m_str, m_camPcdFileName.SIZE);

				ImGui::PushID((int)m_camPcdFileName.m_str);
				ImGui::SameLine();
				if (ImGui::Button("..."))
				{
					const StrPath fileName = common::OpenFileDialog(m_owner->getSystemHandle()
						, { {L"pcd File (*.obj)", L"*.obj"}
							, {L"All File (*.*)", L"*.*"} });
					if (!fileName.empty())
					{
						m_camPcdFileName = fileName;
					}
				}
				ImGui::PopID();

				ImGui::TextUnformatted("Texture FileName :");
				ImGui::SameLine();
				ImGui::InputText("##texture", m_camTextureFileName.m_str, m_camTextureFileName.SIZE);

				ImGui::PushID((int)m_camTextureFileName.m_str);
				ImGui::SameLine();
				if (ImGui::Button("..."))
				{
					const StrPath fileName = common::OpenFileDialog(m_owner->getSystemHandle()
						, { {L"Image File (*.bmp; *.jpg; *.png)", L"*.bmp;*.jpg;*.png"}
							, {L"All File (*.*)", L"*.*"} });
					if (!fileName.empty())
					{
						m_camTextureFileName = fileName;
					}
				}
				ImGui::PopID();

				ImGui::TextUnformatted("Position :                ");
				ImGui::SameLine();
				ImGui::DragFloat2("##position", (float*)&m_camPos, 0.001f, 0.f, 1.f);

				ImGui::TextUnformatted("point scale :           ");
				ImGui::SameLine();
				ImGui::DragFloat("##tessScale", &m_camTessScale, 0.0001f, 0.f, 10.f);
			}
		}
		ImGui::EndChild();

	}
	ImGui::End();

	return open;
}
