//
// 2019-10-08, jjuiddong
// global variable
//
#pragma once


class c3DView;
class cInfoView;
class cHierarchyView;

class cGlobal
{
public:
	cGlobal();
	virtual ~cGlobal();

	bool Init();
	bool ReadProjectFile(const StrPath &fileName);
	void Clear();


public:
	c3DView *m_3dView;
	cInfoView *m_infoView;
	cHierarchyView *m_hierarchyView;
	cPointCloudDB m_pcDb;
	string m_currentCameraName;
};
