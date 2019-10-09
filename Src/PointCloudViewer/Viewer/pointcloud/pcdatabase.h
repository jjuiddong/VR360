//
// 2019-10-08, jjuiddong
// point cloud database
//
//	- data structure
//		- camera[]
//		    - camera name
//			- camera pos
//			- point[]
//				- name
//				- pos
//				- description
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
		Vector3 pos;
		Str256 desc; // description
	};

	struct sCamera
	{
		string name; // unique name
		string pc3dFileName; // point cloud 3d data file name
		string pcTextureFileName; // point cloud texture file name
		Vector3 pos;
		vector<sPCData*> pcds; // point cloud data
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
	void Clear();


protected:
	Vector3 ParseVector3(const string &str);


public:
	vector<sCamera*> m_datas;
};
