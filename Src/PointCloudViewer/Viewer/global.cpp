
#include "stdafx.h"
#include "global.h"
#include "view/3dview.h"


cGlobal::cGlobal()
	: m_pcMap(nullptr)
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
