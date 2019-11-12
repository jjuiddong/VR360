//
// 2019-10-08, jjuiddong
// point cloud database
//
//	- data structure
//		- project
//			- project name
//			- directory path
//			- floor[]
//				- keymap filename
//				- camera[]
//					- camera name
//					- camera pos
//					- point[]
//						- name
//						- pos
//						- description
//
#pragma once


// Point Cloude DataBase
class cPointCloudDB
{
public:
	struct sPCData // point cloud data
	{
		int id; // unique id, (auto setting)
		StrId name;
		Vector3 pos; // point cloud position
		Vector3 wndPos; // information window poisition (ui)
		Vector3 wndSize; // information window size (ui) (z not use)
		common::Str256 desc; // description
	};

	struct sCamera
	{
		StrId name; // unique name
		StrPath pc3dFileName; // point cloud 3d data file name
		StrPath pcTextureFileName; // point cloud texture file name
		Vector3 pos;
		Vector2 keymapPos;
		float tessScale; // point cloud tessellation scale
		graphic::Quaternion rot; // point cloud rotation
		vector<sPCData*> pcds; // point cloud data
	};

	struct sFloor
	{
		StrId name;
		StrPath keymapFileName; // keymap filename
		vector<sCamera*> cams;
	};

	struct sProject
	{
		StrId name; // project name
		StrPath dir; // project directory
		vector<sFloor*> floors;
	};

	cPointCloudDB();
	virtual ~cPointCloudDB();

	bool Read(const StrPath &fileName);
	bool Write(const StrPath &fileName);
	sFloor* AddFloor(const StrId &name);
	bool RemoveFloor(const StrId &name);
	sFloor* FindFloor(const StrId &name);
	sCamera* AddCamera(sFloor *floor, const StrId &name, const Vector3 &pos);
	bool RemoveCamera(sFloor *floor, const StrId &name);
	sCamera* FindCamera(sFloor *floor, const StrId &name);
	sCamera* FindCameraByPointId(sFloor *floor, const int pointId);

	sPCData* CreateData(sFloor *floor, const StrId &cameraName);
	int AddData(sFloor *floor, const StrId &cameraName, const sPCData &pc);
	bool RemoveData(sFloor *floor, const int pointId);
	sPCData* FindData(sFloor *floor, const int pointId);
	sPCData* FindData(sFloor *floor, const StrId &cameraName, const Vector3 &pos);

	static bool CopyProjectData(const sProject &src, sProject &dst);
	static bool RemoveProjectData(sProject &proj);
	static bool RemoveFloor(sFloor *floor);

	bool IsLoad();
	void Clear();


protected:
	Vector3 ParseVector3(const string &str);
	Vector2 ParseVector2(const string &str);


public:
	StrPath m_fileName; // project filename
	sProject m_project;
};
