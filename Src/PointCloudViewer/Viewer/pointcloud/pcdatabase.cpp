
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


// read point cloud information
//
// {
//	   "cameras" : [
//	       {
//             "name" : "camera1",
//             "pos" : "1 2 3",
//			   "point cloud 3d filename" : "",
//			   "point cloud texture filename" : "",
//             "point cloud" : [
//                 {
//                     "name" : "name 1",
//                     "pos" : "1 2 3",
//                     "description" : "memo 1"
//                 },
//                 {
//                     "name" : "name 2",
//                     "pos" : "1 2 3",
//                     "description" : "memo 2"
//                 }
//            ]
//         }
//     ]
// }
//
bool cPointCloudDB::Read(const StrPath &fileName)
{
	Clear();

	if (!fileName.IsFileExist())
		return false;

	try
	{
		ptree props;
		boost::property_tree::read_json(fileName.c_str(), props);
		const string version = props.get<string>("version", "ver.1");

		// parse point cloud data
		ptree::assoc_iterator itor0 = props.find("cameras");
		if (props.not_found() != itor0)
		{
			ptree &child_field0 = props.get_child("cameras");
			for (ptree::value_type &vt0 : child_field0)
			{
				const string name = vt0.second.get<string>("name");
				const string fileName3d = vt0.second.get<string>("point cloud 3d filename", "");
				const string textureFileName = vt0.second.get<string>("point cloud texture filename", "");
				const string posStr = vt0.second.get<string>("pos");
				const Vector3 pos = ParseVector3(posStr);
				sCamera *cam = AddCamera(name, pos);
				if (!cam)
					continue;

				cam->pc3dFileName = fileName3d;
				cam->pcTextureFileName = textureFileName;

				ptree::assoc_iterator itor = vt0.second.find("point cloud");
				if (vt0.second.not_found() != itor)
				{
					ptree &child_field = vt0.second.get_child("point cloud");
					for (ptree::value_type &vt : child_field)
					{
						sPCData pc;
						pc.name = vt.second.get<string>("name");

						const string posStr = vt.second.get<string>("pos");
						pc.pos = ParseVector3(posStr);
						pc.desc = vt.second.get<string>("description");

						AddData(name, pc);
					} //~point cloud
				}
			} //~camras
		}

	}
	catch (std::exception &e)
	{
		Str128 msg;
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

		ptree cams;
		for (auto &cam : m_datas)
		{
			ptree c;
			c.put("name", cam->name.c_str());
			c.put("point cloud 3d filename", cam->pc3dFileName.c_str());
			c.put("point cloud texture filename", cam->pcTextureFileName.c_str());
			Str128 text;
			text.Format("%f %f %f", cam->pos.x, cam->pos.y, cam->pos.z);
			c.put("pos", text.c_str());

			ptree pcs;
			for (auto &pc : cam->pcds)
			{
				ptree z;
				z.put("name", pc->name.c_str());

				Str128 text;
				text.Format("%f %f %f", pc->pos.x, pc->pos.y, pc->pos.z);
				z.put("pos", text.c_str());
				z.put("description", pc->desc.c_str());

				pcs.push_back(std::make_pair("", z));
			}
			c.add_child("point cloud", pcs);
			cams.push_back(std::make_pair("", c));
		}

		props.add_child("cameras", cams);

		boost::property_tree::write_json(fileName.c_str(), props);
	}
	catch (std::exception &e)
	{
		Str128 msg;
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
	m_datas.push_back(cam);
	return cam;
}


bool cPointCloudDB::RemoveCamera(const string &name)
{
	sCamera *cam = FindCamera(name);
	if (!cam)
		return false; // not exist

	common::removevector(m_datas, cam);

	// remove all point cloud
	for (auto &pc : cam->pcds)
		delete pc;
	cam->pcds.clear();

	delete cam;
	return true;
}


cPointCloudDB::sCamera* cPointCloudDB::FindCamera(const string &name)
{
	for (auto &cam : m_datas)
		if (cam->name == name)
			return cam;
	return NULL;
}


// id: point cloud id
cPointCloudDB::sCamera* cPointCloudDB::FindCameraByPointId(const int pointId)
{
	for (auto &cam : m_datas)
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
	for (auto &cam : m_datas)
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


void cPointCloudDB::Clear()
{
	for (auto &cam : m_datas)
	{
		for (auto &pc : cam->pcds)
			delete pc;
		delete cam;
	}
	m_datas.clear();
}
