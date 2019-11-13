
#include "stdafx.h"
#include "pcdatabase.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

using boost::property_tree::ptree;



cPointCloudDB::cPointCloudDB()
{
}

cPointCloudDB::cPointCloudDB(const cPointCloudDB &rhs)
{
	operator=(rhs);
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
//			"floors" : [
//				{
//				"keymap filename" : "filename.jpg",
//				"pins" : [
//					{
//						"name" : "pin1",
//						"pos" : "1 2 3",
//						"keymap pos" : "1 2",
//						"tess scale" : 1.f,
//						"point cloud 3d filename" : "",
//						"point cloud texture filename" : "",
//						"point cloud" : [
//							{
//								"name" : "name 1",
//								"pos" : "1 2 3",
//								"description" : "memo 1"
//							},
//							{
//									"name" : "name 2",
//									"pos" : "1 2 3",
//									"description" : "memo 2"
//							}
//						]
//					}
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
			m_project.name = itor0->second.get<string>("name", "project name");
			m_project.dir = itor0->second.get<string>("path", "c:\\project\\");

			ptree::assoc_iterator itor1 = itor0->second.find("dates");
			if (props.not_found() != itor1)
			{
				ptree &child_field0 = itor0->second.get_child("dates");
				for (ptree::value_type &vt3 : child_field0)
				{
					const string dateName = vt3.second.get<string>("name");
					sDate *date = AddDate(dateName);
					if (!date)
						throw std::exception();

					ptree::assoc_iterator itor2 = vt3.second.find("floors");
					if (props.not_found() != itor2)
					{
						ptree &child_field1 = vt3.second.get_child("floors");
						for (ptree::value_type &vt0 : child_field1)
						{
							const string floorName = vt0.second.get<string>("name");
							sFloor *floor = AddFloor(date, floorName);
							if (!floor)
								throw std::exception();

							floor->keymapFileName = vt0.second.get<string>(
								"keymap filename", "filename.jpg");

							ptree::assoc_iterator itor3 = vt0.second.find("pins");
							if (props.not_found() != itor3)
							{
								ptree &child_field2 = vt0.second.get_child("pins");
								for (ptree::value_type &vt1 : child_field2)
								{
									const string name = vt1.second.get<string>("name");
									const string fileName3d = vt1.second.get<string>("point cloud 3d filename", "");
									const string textureFileName = vt1.second.get<string>("point cloud texture filename", "");
									const string posStr = vt1.second.get<string>("pos");
									const Vector3 pos = ParseVector3(posStr);
									const string keymapPosStr = vt1.second.get<string>("keymap pos");
									const Vector2 kpos = ParseVector2(keymapPosStr);
									const float tessScale = vt1.second.get<float>("tess scale", 0.02f);

									sPin *pin = AddPin(floor, name, pos);
									if (!pin)
										continue;

									pin->pc3dFileName = fileName3d;
									pin->pcTextureFileName = textureFileName;
									pin->keymapPos = kpos;
									pin->tessScale = tessScale;

									ptree::assoc_iterator itor4 = vt1.second.find("point cloud");
									if (vt1.second.not_found() != itor4)
									{
										ptree &child_field3 = vt1.second.get_child("point cloud");
										for (ptree::value_type &vt : child_field3)
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

											AddData(floor, name, pc);
										} //~point cloud
									}//find point cloud
								} //~pins
							} // find pins
						}//~floors
					}//find floors
				} //~dates
			}//~find dates
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
		proj.put("name", m_project.name.c_str());
		proj.put("path", m_project.dir.c_str());

		ptree dates;
		for (auto &date : m_project.dates)
		{
			ptree d;
			d.put("name", date->name.c_str());

			ptree floors;
			for (auto &floor : date->floors)
			{
				ptree fl;
				fl.put("name", floor->name.c_str());
				fl.put("keymap filename", floor->keymapFileName.c_str());

				ptree pins;
				for (auto &pin : floor->pins)
				{
					ptree c;
					c.put("name", pin->name.c_str());
					c.put("point cloud 3d filename", pin->pc3dFileName.c_str());
					c.put("point cloud texture filename", pin->pcTextureFileName.c_str());
					common::Str128 text;
					text.Format("%f %f %f", pin->pos.x, pin->pos.y, pin->pos.z);
					c.put("pos", text.c_str());
					text.Format("%f %f", pin->keymapPos.x, pin->keymapPos.y);
					c.put("keymap pos", text.c_str());
					c.put("tess scale", pin->tessScale);

					ptree pcs;
					for (auto &pc : pin->pcds)
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
					pins.push_back(std::make_pair("", c));
				}//~pins

				fl.add_child("pins", pins);
				floors.push_back(std::make_pair("", fl));
			}//~floors

			d.add_child("floors", floors);
			dates.push_back(std::make_pair("", d));
		}//~dates

		proj.add_child("dates", dates);
		props.add_child("project", proj);

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


cPointCloudDB::sDate* cPointCloudDB::AddDate(const StrId &name)
{
	sDate *date = FindDate(name);
	if (date)
		return NULL; // already exist

	date = new sDate;
	date->name = name.c_str();
	m_project.dates.push_back(date);
	return date;
}


bool cPointCloudDB::RemoveDate(const StrId &name)
{
	sDate *date = FindDate(name);
	if (!date)
		return false; // not exist

	common::removevector(m_project.dates, date);

	// remove all point cloud
	while (!date->floors.empty())
	{
		sFloor *floor = date->floors.back();
		RemoveFloor(date, floor->name);
	}
	date->floors.clear();
	delete date;
	return true;
}


cPointCloudDB::sDate* cPointCloudDB::FindDate(const StrId &name)
{
	for (auto &date : m_project.dates)
		if (date->name == name)
			return date;
	return NULL;
}


cPointCloudDB::sFloor* cPointCloudDB::AddFloor(sDate* date, const StrId &name)
{
	sFloor *floor = FindFloor(date, name);
	if (floor)
		return NULL; // already exist

	floor = new sFloor;
	floor->name = name.c_str();
	date->floors.push_back(floor);
	return floor;
}


bool cPointCloudDB::RemoveFloor(sDate* date, const StrId &name)
{
	sFloor *floor = FindFloor(date, name);
	if (!floor)
		return false; // not exist

	common::removevector(date->floors, floor);

	// remove all point cloud
	for (auto &pin : floor->pins)
	{
		for (auto &pc : pin->pcds)
			delete pc;
		pin->pcds.clear();
		delete pin;
	}
	floor->pins.clear();
	delete floor;
	return true;
}


cPointCloudDB::sFloor* cPointCloudDB::FindFloor(sDate* date, const StrId &name)
{
	for (auto &floor : date->floors)
		if (floor->name == name)
			return floor;
	return NULL;
}


cPointCloudDB::sFloor* cPointCloudDB::FindFloor(
	const StrId &dateName, const StrId &floorName)
{
	sDate *date = FindDate(dateName);
	RETV(!date, nullptr);
	return FindFloor(date, floorName);
}


// add pin, and return pin instance
cPointCloudDB::sPin* cPointCloudDB::AddPin(sFloor *floor
	, const StrId &name, const Vector3 &pos)
{
	sPin *pin = FindPin(floor, name);
	if (pin)
		return NULL; // already exist

	pin = new sPin;
	pin->name = name.c_str();
	pin->pos = pos;
	floor->pins.push_back(pin);
	return pin;
}


bool cPointCloudDB::RemovePin(sFloor *floor
	, const StrId &name)
{
	sPin *pin = FindPin(floor, name);
	if (!pin)
		return false; // not exist

	common::removevector(floor->pins, pin);

	// remove all point cloud
	for (auto &pc : pin->pcds)
		delete pc;
	pin->pcds.clear();

	delete pin;
	return true;
}


cPointCloudDB::sPin* cPointCloudDB::FindPin(sFloor *floor
	, const StrId &name)
{
	for (auto &pin : floor->pins)
		if (pin->name == name)
			return pin;
	return NULL;
}


cPointCloudDB::sPin* cPointCloudDB::FindPin(const StrId dateName
	, const StrId &floorName, const StrId &pinName)
{
	sFloor *floor = FindFloor(dateName, floorName);
	RETV(!floor, nullptr);
	sPin *pin = FindPin(floor, pinName);
	return pin;
}


// id: point cloud id
cPointCloudDB::sPin* cPointCloudDB::FindPinByPointId(sFloor *floor
	, const int pointId)
{
	for (auto &pin : floor->pins)
		for (auto &pc : pin->pcds)
			if (pc->id == pointId)
				return pin;
	return NULL;
}


// create and return sPCData
// generate unique id
cPointCloudDB::sPCData* cPointCloudDB::CreateData(sFloor *floor
	, const StrId &pinName)
{
	sPin *pin = FindPin(floor, pinName);
	RETV(!pin, NULL); // not exist Pin

	const int id = common::GenerateId();
	sPCData *data = new sPCData;
	data->id = id;
	data->name = common::format("Memo-%d", id);
	data->pos = Vector3(0, 0, 0);
	pin->pcds.push_back(data);
	return data;
}


// add point cloud data, return id
// if fail, return -1
int cPointCloudDB::AddData(sFloor *floor, const StrId &pinName, const sPCData &pc)
{
	sPin *pin = FindPin(floor, pinName);
	RETV(!pin, NULL); // not exist pin

	sPCData *data = FindData(floor, pinName, pc.pos);
	if (data)
		return -1; // already exist

	sPCData *n = new sPCData;
	*n = pc;
	n->id = common::GenerateId();
	pin->pcds.push_back(n);
	return n->id;
}


// remove point cloud data
// if success, return true or false
bool cPointCloudDB::RemoveData(sFloor *floor, const int pointId)
{
	sPin *pin = FindPinByPointId(floor, pointId);
	RETV(!pin, NULL); // not exist pin

	sPCData *data = FindData(floor, pointId);
	RETV(!data, NULL); // not exist point

	common::removevector(pin->pcds, data);
	delete data;
	return true;
}


cPointCloudDB::sPCData* cPointCloudDB::FindData(sFloor *floor, const int pointId)
{
	for (auto &pin : floor->pins)
		for (auto &pc : pin->pcds)
			if (pc->id == pointId)
				return pc;
	return NULL;
}


cPointCloudDB::sPCData* cPointCloudDB::FindData(
	const StrId dateName, const StrId &floorName, const int pointId)
{
	sFloor *floor = FindFloor(dateName, floorName);
	RETV(!floor, nullptr);
	return FindData(floor, pointId);
}


cPointCloudDB::sPCData* cPointCloudDB::FindData(sFloor *floor
	, const StrId &pinName, const Vector3 &pos)
{
	sPin *pin = FindPin(floor, pinName);
	RETV(!pin, NULL); // not exist pin

	for (auto &pc : pin->pcds)
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


cPointCloudDB& cPointCloudDB::operator=(const cPointCloudDB &rhs)
{
	if (this != &rhs)
	{
		Clear();
		CopyProjectData(rhs.m_project, m_project);
	}
	return *this;
}

// deep copy project data structure
// src -> dst
bool cPointCloudDB::CopyProjectData(const sProject &src, sProject &dst)
{
	dst = src;
	dst.dates.clear(); // remove before copy

	for (auto &date : src.dates)
	{
		sDate *d = new sDate;
		*d = *date;
		d->floors.clear();//remove before copy

		for (auto &floor : date->floors)
		{
			sFloor *f = new sFloor;
			*f = *floor;
			f->pins.clear();// remove before copy

			for (auto &pin : floor->pins)
			{
				sPin *c = new sPin;
				*c = *pin;
				c->pcds.clear(); // remove before copy

				for (auto &pc : pin->pcds)
				{
					sPCData *p = new sPCData;
					*p = *pc;
					c->pcds.push_back(p);
				}
				f->pins.push_back(c);
			}
			d->floors.push_back(f);
		}
		dst.dates.push_back(d);
	}

	return true;
}


// remove project data
bool cPointCloudDB::RemoveProjectData(sProject &proj)
{
	for (auto &date : proj.dates)
	{
		for (auto &floor : date->floors)
		{
			for (auto &pin : floor->pins)
			{
				for (auto &pc : pin->pcds)
					delete pc;
				delete pin;
			}
			floor->pins.clear();
			delete floor;
		}
		delete date;
	}
	proj = {};
	return true;
}


//bool cPointCloudDB::RemoveDate(sFloor *floor)
//{
//	// remove all point cloud
//	for (auto &pin : floor->pins)
//	{
//		for (auto &pc : pin->pcds)
//			delete pc;
//		pin->pcds.clear();
//		delete pin;
//	}
//	floor->pins.clear();
//	delete floor;
//	return true;
//}


void cPointCloudDB::Clear()
{
	RemoveProjectData(m_project);
}
