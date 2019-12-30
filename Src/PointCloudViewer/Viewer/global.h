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
	vector<sMeasurePt>* GetCurrentMeasurePts();
	string GetCurrentUniquePinId();
	void ClearMap();
	void Clear();


public:
	eEditState m_state;
	cPointCloudDB m_pcDb;
	cPointCloudMap *m_pcMap; // current point cloud map, reference
	map<string, cPointCloudMap*> m_maps; // key: filename
	vector<sMarkup> m_markups;

	map<string, vector<sMeasurePt>> m_measures; // point cloud position array, key: unique pin id
	vector<string> m_captures; // cpature file array

	// current selection date-floor-pin
	string m_dateName; // current date Name
	string m_floorName; // current floor Name
	string m_pinName; // current Pin Name

	// view
	c3DView *m_3dView;
	cInfoView *m_infoView;
	cHierarchyView *m_hierarchyView;
};
