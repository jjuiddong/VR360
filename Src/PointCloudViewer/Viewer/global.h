//
// 2019-10-08, jjuiddong
// global variable
//
#pragma once


class c3DView;
class cInfoView;
class cHierarchyView;
class cPointCloudMap;

class cGlobal
{
public:
	cGlobal();
	virtual ~cGlobal();

	bool Init();
	bool ReadProjectFile(const StrPath &fileName);
	bool LoadPCMap(const StrPath &pcMapFileName);
	void ClearMap();
	void Clear();


public:
	c3DView *m_3dView;
	cInfoView *m_infoView;
	cHierarchyView *m_hierarchyView;
	cPointCloudDB m_pcDb;
	cPointCloudMap *m_pcMap;
	map<string, cPointCloudMap*> m_maps;

	string m_cDateStr; // current date Name
	string m_cFloorStr; // current Floor Name
	string m_cPinStr; // current Pin Name
};
