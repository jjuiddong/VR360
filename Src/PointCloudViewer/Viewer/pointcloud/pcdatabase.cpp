
#include "stdafx.h"
#include "pcdatabase.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

using boost::property_tree::ptree;



cPointCloudDB::cPointCloudDB()
{
}

cPointCloudDB::~cPointCloudDB()
{
	Clear();
}


// read project file
// read point cloud information
//
// {
//	   "project" :
//		{
//			"name" : "project name",
//			"path" : "project file path",
//			"keymap filename" : "filename.jpg",
//			"cameras" : [
//				{
//					"name" : "camera1",
//					"pos" : "1 2 3",
//					"keymap pos" : "1 2",
//					"tess scale" : 1.f,
//					"point cloud 3d filename" : "",
//					"point cloud texture filename" : "",
//					"point cloud" : [
//						{
//							"name" : "name 1",
//							"pos" : "1 2 3",
//							"description" : "memo 1"
//						},
//						{
//							"name" : "name 2",
//							"pos" : "1 2 3",
//							"description" : "memo 2"
//						}
//					]
//				}
//			]
//		}
// }
//
bool cPointCloudDB::Read(const StrPath &fileName)
{
	Clear();

	if (!fileName.IsFileExist())
		return false;

	m_fileName = fileName;

	try
	{
		ptree props;
		boost::property_tree::read_json(fileName.c_str(), props);
		const string version = props.get<string>("version", "ver.1");

		// parse point cloud data
		ptree::assoc_iterator itor0 = props.find("project");
		if (props.not_found() != itor0)
		{
			//ptree &child_field0 = props.get_child("project");
			m_project.name = itor0->second.get<string>("name", "project name");
			m_project.dir = itor0->second.get<string>("path", "c:\\project\\");
			m_project.keymapFileName = itor0->second.get<string>(
				"keymap filename", "filename.jpg");

			ptree::assoc_iterator itor1 = itor0->second.find("cameras");
			if (props.not_found() != itor1)
			{
				ptree &child_field1 = itor0->second.get_child("cameras");
				for (ptree::value_type &vt0 : child_field1)
				{
					const string name = vt0.second.get<string>("name");
					const string fileName3d = vt0.second.get<string>("point cloud 3d filename", "");
					const string textureFileName = vt0.second.get<string>("point cloud texture filename", "");
					const string posStr = vt0.second.get<string>("pos");
					const Vector3 pos = ParseVector3(posStr);
					const string keymapPosStr = vt0.second.get<string>("keymap pos");
					const Vector2 kpos = ParseVector2(keymapPosStr);
					const float tessScale = vt0.second.get<float>("tess scale", 0.02f);

					sCamera *cam = AddCamera(name, pos);
					if (!cam)
						continue;

					cam->pc3dFileName = fileName3d;
					cam->pcTextureFileName = textureFileName;
					cam->keymapPos = kpos;
					cam->tessScale = tessScale;

					ptree::assoc_iterator itor2 = vt0.second.find("point cloud");
					if (vt0.second.not_found() != itor2)
					{
						ptree &child_field2 = vt0.second.get_child("point cloud");
						for (ptree::value_type &vt : child_field2)
						{
							sPCData pc;
							pc.name = vt.second.get<string>("name");

							const string posStr = vt.second.get<string>("pos");
							pc.pos = ParseVector3(posStr);

							const string wndPosStr = vt.second.get<string>("wndpos", "");
							pc.wndPos = ParseVector3(wndPosStr);
							if (pc.wndPos.IsEmpty())
								pc.wndPos = pc.pos;

							const string wndSizeStr = vt.second.get<string>("wndsize", "100 100 0");
							pc.wndSize = ParseVector3(wndSizeStr);

							pc.desc = vt.second.get<string>("description");

							AddData(name, pc);
						} //~point cloud
					}
				} //~camras
			} // find camras
		} //~project
	}
	catch (std::exception &e)
	{
 		common::Str128 msg;
		msg.Format("Read Error!!, Point Cloud Data File [ %s ]\n%s"
			, fileName.c_str(), e.what());
		MessageBoxA(NULL, msg.c_str(), "ERROR", MB_OK);
		return false;
	}

	return true;
}


bool cPointCloudDB::Write(const StrPath &fileName)
{
	try
	{
		ptree props;

		ptree proj;
		proj.put("name", m_project.name);
		proj.put("path", m_project.dir);
		proj.put("keymap filename", m_project.keymapFileName);

		ptree cams;
		for (auto &cam : m_project.cams)
		{
			ptree c;
			c.put("name", cam->name.c_str());
			c.put("point cloud 3d filename", cam->pc3dFileName.c_str());
			c.put("point cloud texture filename", cam->pcTextureFileName.c_str());
			common::Str128 text;
			text.Format("%f %f %f", cam->pos.x, cam->pos.y, cam->pos.z);
			c.put("pos", text.c_str());
			text.Format("%f %f", cam->keymapPos.x, cam->keymapPos.y);
			c.put("keymap pos", text.c_str());
			c.put("tess scale", cam->tessScale);

			ptree pcs;
			for (auto &pc : cam->pcds)
			{
				ptree z;
				z.put("name", pc->name.c_str());

				common::Str128 text;
				text.Format("%f %f %f", pc->pos.x, pc->pos.y, pc->pos.z);
				z.put("pos", text.c_str());

				if (pc->wndPos.IsEmpty())
					pc->wndPos = pc->pos;
				text.Format("%f %f %f", pc->wndPos.x, pc->wndPos.y, pc->wndPos.z);
				z.put("wndpos", text.c_str());

				text.Format("%f %f %f", pc->wndSize.x, pc->wndSize.y, pc->wndSize.z);
				z.put("wndsize", text.c_str());

				z.put("description", pc->desc.c_str());

				pcs.push_back(std::make_pair("", z));
			}
			c.add_child("point cloud", pcs);
			cams.push_back(std::make_pair("", c));
		}

		proj.push_back(std::make_pair("cameras", cams));
		props.add_child("project", proj);
		//props.add_child("cameras", cams);

		boost::property_tree::write_json(fileName.c_str(), props);
	}
	catch (std::exception &e)
	{
		common::Str128 msg;
		msg.Format("Write Error!!, Point Cloud File [ %s ]\n%s"
			, fileName.c_str(), e.what());
		MessageBoxA(NULL, msg.c_str(), "ERROR", MB_OK);
		return false;
	}
	return true;
}


// add camera, and return camera instance
cPointCloudDB::sCamera* cPointCloudDB::AddCamera(const string &name, const Vector3 &pos)
{
	sCamera *cam = FindCamera(name);
	if (cam)
		return NULL; // already exist

	cam = new sCamera;
	cam->name = name.c_str();
	cam->pos = pos;
	m_project.cams.push_back(cam);
	return cam;
}


bool cPointCloudDB::RemoveCamera(const string &name)
{
	sCamera *cam = FindCamera(name);
	if (!cam)
		return false; // not exist

	common::removevector(m_project.cams, cam);

	// remove all point cloud
	for (auto &pc : cam->pcds)
		delete pc;
	cam->pcds.clear();

	delete cam;
	return true;
}


cPointCloudDB::sCamera* cPointCloudDB::FindCamera(const string &name)
{
	for (auto &cam : m_project.cams)
		if (cam->name == name)
			return cam;
	return NULL;
}


// id: point cloud id
cPointCloudDB::sCamera* cPointCloudDB::FindCameraByPointId(const int pointId)
{
	for (auto &cam : m_project.cams)
		for (auto &pc : cam->pcds)
			if (pc->id == pointId)
				return cam;
	return NULL;
}


// create and return sPCData
// generate unique id
cPointCloudDB::sPCData* cPointCloudDB::CreateData(const string &cameraName)
{
	sCamera *cam = FindCamera(cameraName);
	RETV(!cam, NULL); // not exist camera

	const int id = common::GenerateId();
	sPCData *data = new sPCData;
	data->id = id;
	data->name = common::format("Memo-%d", id);
	data->pos = Vector3(0, 0, 0);
	cam->pcds.push_back(data);
	return data;
}


// add point cloud data, return id
// if fail, return -1
int cPointCloudDB::AddData(const string &cameraName, const sPCData &pc)
{
	sCamera *cam = FindCamera(cameraName);
	RETV(!cam, NULL); // not exist camera

	sPCData *data = FindData(cameraName, pc.pos);
	if (data)
		return -1; // already exist

	sPCData *n = new sPCData;
	*n = pc;
	n->id = common::GenerateId();
	cam->pcds.push_back(n);
	return n->id;
}


// remove point cloud data
// if success, return true or false
bool cPointCloudDB::RemoveData(const int pointId)
{
	sCamera *cam = FindCameraByPointId(pointId);
	RETV(!cam, NULL); // not exist camera

	sPCData *data = FindData(pointId);
	RETV(!data, NULL); // not exist point

	common::removevector(cam->pcds, data);
	delete data;
	return true;
}


cPointCloudDB::sPCData* cPointCloudDB::FindData(const int pointId)
{
	for (auto &cam : m_project.cams)
		for (auto &pc : cam->pcds)
			if (pc->id == pointId)
				return pc;
	return NULL;
}


cPointCloudDB::sPCData* cPointCloudDB::FindData(const string &cameraName, const Vector3 &pos)
{
	sCamera *cam = FindCamera(cameraName);
	RETV(!cam, NULL); // not exist camera

	for (auto &pc : cam->pcds)
		if (pc->pos.IsEqual(pos, 0.01f))
			return pc;
	return NULL;
}


// parse string to Vector3
// string format : x y z
Vector3 cPointCloudDB::ParseVector3(const string &str)
{
	vector<string> toks;
	common::tokenizer(str, " ", "", toks);
	if (toks.size() >= 3)
	{
		return Vector3((float)atof(toks[0].c_str())
			, (float)atof(toks[1].c_str())
			, (float)atof(toks[2].c_str()));
	}
	return Vector3::Zeroes;
}


// parse string to Vector2
// string format : x y
Vector2 cPointCloudDB::ParseVector2(const string &str)
{
	vector<string> toks;
	common::tokenizer(str, " ", "", toks);
	if (toks.size() >= 2)
	{
		return Vector2((float)atof(toks[0].c_str())
			, (float)atof(toks[1].c_str()));
	}
	return Vector2(0,0);
}


bool cPointCloudDB::IsLoad()
{
	return !m_project.name.empty();
}


void cPointCloudDB::Clear()
{
	for (auto &cam : m_project.cams)
	{
		for (auto &pc : cam->pcds)
			delete pc;
		delete cam;
	}
	m_project.cams.clear();
	m_project = {};
}
