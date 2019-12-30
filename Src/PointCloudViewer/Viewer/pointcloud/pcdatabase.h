//
// 2019-10-08, jjuiddong
// point cloud database
//
//	- data structure
//		- project
//			- project name
//			- directory path
//			- dates[]
//				- name (YYYY-MM-DD)
//				- floors[]
//					- floor name
//					- keymap filename
//					- pins[]
//						- pin name
//						- pin pos
//						- points[]
//							- type
//							- name
//							- markup id
//							- pos
//							- epos
//							- wndpos
//							- wndsize
//							- description
//
#pragma once


// Point Cloude Database
class cPointCloudDB
{
public:
	struct sPCData // point cloud data
	{
		enum TYPE {MEMO, MARKUP};
		TYPE type;
		int id; // unique id, (auto setting)
		eMarkup::Enum markup;
		StrId name;
		Vector3 pos; // point cloud position
		Vector2 uvpos; // equirectangular position (uv coordinate)
		Vector3 wndPos; // information window poisition (ui)
		Vector3 wndSize; // information window size (ui) (z not use)
		common::Str256 desc; // description (utf8 format)
	};

	struct sPin
	{
		StrId name; // unique name
		StrPath pc3dFileName; // point cloud 3d data file name
		StrPath pcTextureFileName; // point cloud texture file name
		Vector3 pos;
		Vector2 keymapPos; // uv coord
		float tessScale; // point cloud tessellation scale
		graphic::Quaternion rot; // point cloud rotation (not used)
		vector<sPCData*> pcds; // point cloud data
	};

	struct sFloor
	{
		StrId name;
		StrPath keymapFileName; // keymap filename
		vector<sPin*> pins;
	};

	struct sDate
	{
		StrId name; // date string (YYYY-MM-DD)
		vector<sFloor*> floors;
	};

	struct sProject
	{
		StrId name; // project name
		StrPath dir; // project directory
		vector<sDate*> dates;
	};

	cPointCloudDB();
	cPointCloudDB(const cPointCloudDB &rhs);
	virtual ~cPointCloudDB();

	bool Read(const StrPath &fileName);
	bool Write(const StrPath &fileName);

	sDate* AddDate(const StrId &name);
	bool RemoveDate(const StrId &name);
	sDate* FindDate(const StrId &name);

	sFloor* AddFloor(sDate* date, const StrId &name);
	bool RemoveFloor(sDate* date, const StrId &name);
	sFloor* FindFloor(sDate* date, const StrId &name);
	sFloor* FindFloor(const StrId &dateName, const StrId &floorName);

	sPin* AddPin(sFloor *floor, const StrId &name, const Vector3 &pos);
	bool RemovePin(sFloor *floor, const StrId &name);
	sPin* FindPin(sFloor *floor, const StrId &name);
	sPin* FindPin(const StrId &dateName, const StrId &floorName, const StrId &pinName);
	sPin* FindPinByPointId(sFloor *floor, const int pointId);

	sPCData* CreateData(sFloor *floor, const StrId &pinName);
	int AddData(sFloor *floor, const StrId &pinName, const sPCData &pc);
	bool RemoveData(sFloor *floor, const int pointId);
	sPCData* FindData(sFloor *floor, const int pointId);
	sPCData* FindData(const StrId dateName, const StrId &floorName, const int pointId);
	sPCData* FindData(sFloor *floor, const StrId &pinName, const Vector3 &pos);
	bool MakeShareFile(const StrId &dateStr, const StrId &floorStr, const StrId &pinStr
		, OUT sProject &out);

	cPointCloudDB& operator=(const cPointCloudDB &rhs);

	static bool CopyProjectData(const sProject &src, sProject &dst);
	static bool RemoveProjectData(sProject &proj);

	bool IsLoad();
	void Clear();


protected:
	Vector3 ParseVector3(const string &str);
	Vector2 ParseVector2(const string &str);


public:
	StrPath m_fileName; // project filename
	sProject m_project;
};
