
#include "stdafx.h"
#include "global.h"


cGlobal::cGlobal()
{
}

cGlobal::~cGlobal()
{
	Clear();
}


bool cGlobal::Init()
{
	m_pcDb.Read("pointcloud_data.json");

	m_currentCameraName = "camera1";

	return true;
}


void cGlobal::Clear()
{
	m_pcDb.Write("pointcloud_data.json");
	m_pcDb.Clear();
}
