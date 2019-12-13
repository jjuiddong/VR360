
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
	m_markups.push_back({ eMarkup::Issue, "�̽� �߻�", "�̽� �߻�", "./media/icon/M1.png" });
	m_markups.push_back({ eMarkup::Direction, "���� ǥ��", "���� ǥ��", "./media/icon/M2.png" });
	m_markups.push_back({ eMarkup::Review, "���� ����", "���� ���� �ʿ�", "./media/icon/M3.png" });
	m_markups.push_back({ eMarkup::Water, "����, ���", "����, ���", "./media/icon/M4.png" });
	m_markups.push_back({ eMarkup::ElectricProblem, "���°��� ����", "������ ���°��� ���� ����", "./media/icon/M5.png" });
	m_markups.push_back({ eMarkup::Modify, "���� �ʿ�", "���� �ʿ�", "./media/icon/M6.png" });
	m_markups.push_back({ eMarkup::Contract, "�⼺, �ϵ�", "�⼺, �ϵ� ����", "./media/icon/M7.png" });
	m_markups.push_back({ eMarkup::Prohibit, "����", "����", "./media/icon/M8.png" });
	m_markups.push_back({ eMarkup::FinishProcess, "���� ����", "���� ����", "./media/icon/M9.png" });
	m_markups.push_back({ eMarkup::PlasterProcess, "���� ����", "���� ����", "./media/icon/M10.png" });
	m_markups.push_back({ eMarkup::ElectricProcess, "���� ����", "���� ����", "./media/icon/M11.png" });
	m_markups.push_back({ eMarkup::Labor, "�η� ����", "�η� ���� ��", "./media/icon/M12.png" });
	m_markups.push_back({ eMarkup::Change, "��ü �ʿ�", "��ü �ʿ�", "./media/icon/M13.png" });
	m_markups.push_back({ eMarkup::Share, "���� �� ����", "���� �� ����", "./media/icon/M14.png" });
	m_markups.push_back({ eMarkup::Cement, "�ø�Ʈ �̽�", "�ø�Ʈ ���� �̽�", "./media/icon/M15.png" });
	m_markups.push_back({ eMarkup::SteelFrame, "ö�� (ö����)", "ö�� (ö����)", "./media/icon/M16.png" });
	m_markups.push_back({ eMarkup::I_Steel, "I�� ����, �������", "I�� ����, ������� �ʿ�", "./media/icon/M17.png" });

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
