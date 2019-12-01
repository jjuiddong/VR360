//
// 2019-10-08, jjuiddong
// global variable
//
#pragma once


class c3DView;
class cInfoView;
class cHierarchyView;
class cPointCloudMap;

enum eEditState {
	VR360, Measure, Capture
};


// measure information
struct sMeasurePt 
{
	Vector3 epos; // equirectangular pos
	Vector3 rpos; // real pos, point cloud pos
	Vector2 uv;
};


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
	eEditState m_state;
	c3DView *m_3dView;
	cInfoView *m_infoView;
	cHierarchyView *m_hierarchyView;
	cPointCloudDB m_pcDb;
	cPointCloudMap *m_pcMap;
	map<string, cPointCloudMap*> m_maps;
	vector<sMarkup> m_markups;

	string m_cDateStr; // current date Name
	string m_cFloorStr; // current Floor Name
	string m_cPinStr; // current Pin Name

	vector<sMeasurePt> m_measures; // point cloud position array
	vector<string> m_captures; // cpature file array
};
