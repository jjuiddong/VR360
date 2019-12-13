
#include "stdafx.h"
#include "global.h"
#include "view/3dview.h"


cGlobal::cGlobal()
	: m_pcMap(nullptr)
	, m_state(eEditState::VR360)
{
}

cGlobal::~cGlobal()
{
	Clear();
}


bool cGlobal::Init()
{
	//m_pcDb.Read("pointcloud_data.json");
	m_cPinStr = "camera1";

	// initialize markup 
	m_markups.push_back({ eMarkup::Issue, "이슈 발생", "이슈 발생", "./media/icon/M1.png" });
	m_markups.push_back({ eMarkup::Direction, "방향 표시", "방향 표시", "./media/icon/M2.png" });
	m_markups.push_back({ eMarkup::Review, "추후 검토", "추후 검토 필요", "./media/icon/M3.png" });
	m_markups.push_back({ eMarkup::Water, "누수, 방수", "누수, 방수", "./media/icon/M4.png" });
	m_markups.push_back({ eMarkup::ElectricProblem, "전력공급 문제", "공사중 전력공급 관련 문제", "./media/icon/M5.png" });
	m_markups.push_back({ eMarkup::Modify, "수정 필요", "수정 필요", "./media/icon/M6.png" });
	m_markups.push_back({ eMarkup::Contract, "기성, 하도", "기성, 하도 관련", "./media/icon/M7.png" });
	m_markups.push_back({ eMarkup::Prohibit, "금지", "금지", "./media/icon/M8.png" });
	m_markups.push_back({ eMarkup::FinishProcess, "마감 공정", "마감 공정", "./media/icon/M9.png" });
	m_markups.push_back({ eMarkup::PlasterProcess, "미장 공정", "미장 공정", "./media/icon/M10.png" });
	m_markups.push_back({ eMarkup::ElectricProcess, "전기 공정", "전기 공정", "./media/icon/M11.png" });
	m_markups.push_back({ eMarkup::Labor, "인력 보충", "인력 보충 등", "./media/icon/M12.png" });
	m_markups.push_back({ eMarkup::Change, "교체 필요", "교체 필요", "./media/icon/M13.png" });
	m_markups.push_back({ eMarkup::Share, "전달 및 공유", "전달 및 공유", "./media/icon/M14.png" });
	m_markups.push_back({ eMarkup::Cement, "시멘트 이슈", "시멘트 관련 이슈", "./media/icon/M15.png" });
	m_markups.push_back({ eMarkup::SteelFrame, "철근 (철골조)", "철근 (철골조)", "./media/icon/M16.png" });
	m_markups.push_back({ eMarkup::I_Steel, "I형 형강, 자재공급", "I형 형강, 자재공급 필요", "./media/icon/M17.png" });

	return true;
}


// read project file
bool cGlobal::ReadProjectFile(const StrPath &fileName)
{
	if (!m_pcDb.Read(fileName))
		return false;

	// load keymap file
	//m_3dView->m_keyMap.m_texture
	//	= graphic::cResourceManager::Get()->LoadTexture(
	//		m_3dView->GetRenderer(), m_pcDb.m_project.keymapFileName);

	m_3dView->m_curPinInfo = nullptr;

	return true;
}


// load point cloud map file
bool cGlobal::LoadPCMap(const StrPath &pcMapFileName)
{
	auto it = m_maps.find(pcMapFileName.c_str());
	if (m_maps.end() != it)
	{
		m_pcMap = it->second;
		return true;
	}

	cPointCloudMap *pcMap = new cPointCloudMap();
	if (!pcMap->Read(pcMapFileName, 333 * 2, 200 * 2))
	{
		m_pcMap = nullptr;
		return false;
	}

	m_maps.insert({ pcMapFileName.c_str(), pcMap });
	m_pcMap = pcMap;

	// to display pcmap, load vertex
	using namespace graphic;
	sRawMeshGroup2 *model = cResourceManager::Get()->FindModel2(pcMapFileName.c_str());
	if (model)
		return true; // already loaded

	// create point cloud vertex
	sRawMeshGroup2 *rawMeshes = new sRawMeshGroup2;
	rawMeshes->name = pcMapFileName.c_str();

	rawMeshes->meshes.push_back({});
	sRawMesh2 *mesh = &rawMeshes->meshes.back();
	mesh->name = "mesh1";
	mesh->renderFlags = eRenderFlag::VISIBLE | eRenderFlag::NOALPHABLEND;
	for (uint i = 0; i < pcMap->m_pcCount; ++i)
	{
		auto &vtx = pcMap->m_pcd[pcMap->m_pcCount - i - 1];
		mesh->vertices.push_back(vtx);
	}

	rawMeshes->nodes.push_back({});
	sRawNode *node = &rawMeshes->nodes.back();
	node->name = "node1";
	node->meshes.push_back(0);
	node->localTm = Matrix44::Identity;

	cResourceManager::Get()->InsertRawMesh(pcMapFileName, rawMeshes);
	return true;
}


void cGlobal::ClearMap()
{
	for (auto &kv : m_maps)
		delete kv.second;
	m_maps.clear();
}


void cGlobal::Clear()
{
	m_pcDb.Write("pointcloud_data.json");
	m_pcDb.Clear();
	ClearMap();
}
