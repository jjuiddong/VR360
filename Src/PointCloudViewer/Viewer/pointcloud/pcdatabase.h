//
// 2019-10-08, jjuiddong
// point cloud database
//
//	- data structure
//		- project
//			- project name
//			- directory path
//			- keymap filename
//			- camera[]
//				- camera name
//				- camera pos
//				- point[]
//					- name
//					- pos
//					- description
//
#pragma once


// Point Cloude DataBase
class cPointCloudDB
{
public:
	struct sPCData // point cloud data
	{
		int id; // unique id, (auto setting)
		string name;
		Vector3 pos; // point cloud position
		Vector3 wndPos; // information window poisition (ui)
		Vector3 wndSize; // information window size (ui) (z not use)
		common::Str256 desc; // description
	};

	struct sCamera
	{
		string name; // unique name
		string pc3dFileName; // point cloud 3d data file name
		string pcTextureFileName; // point cloud texture file name
		Vector3 pos;
		Vector2 keymapPos;
		float tessScale; // point cloud tessellation scale
		graphic::Quaternion rot; // point cloud rotation
		vector<sPCData*> pcds; // point cloud data
	};

	struct sProject
	{
		string name; // project name
		string dir; // file directory
		string keymapFileName; // keymap filename
		vector<sCamera*> cams;
	};

	cPointCloudDB();
	virtual ~cPointCloudDB();

	bool Read(const StrPath &fileName);
	bool Write(const StrPath &fileName);
	sCamera* AddCamera(const string &name, const Vector3 &pos);
	bool RemoveCamera(const string &name);
	sCamera* FindCamera(const string &name);
	sCamera* FindCameraByPointId(const int pointId);
	sPCData* CreateData(const string &cameraName);
	int AddData(const string &cameraName, const sPCData &pc);
	bool RemoveData(const int pointId);
	sPCData* FindData(const int pointId);
	sPCData* FindData(const string &cameraName, const Vector3 &pos);
	bool IsLoad();
	void Clear();


protected:
	Vector3 ParseVector3(const string &str);
	Vector2 ParseVector2(const string &str);


public:
	StrPath m_fileName; // project filename
	sProject m_project;
};
