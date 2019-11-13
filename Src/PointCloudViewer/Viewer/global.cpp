
#include "stdafx.h"
#include "global.h"
#include "view/3dview.h"


cGlobal::cGlobal()
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


void cGlobal::Clear()
{
	m_pcDb.Write("pointcloud_data.json");
	m_pcDb.Clear();
}
