//
// 2019-11-28, jjuiddong
// point cloud global definition
//
#pragma once


DECLARE_ENUM(eMarkup
	, Issue
	//, Direction
	, Review
	, Water
	, ElectricProblem
	, Modify
	, Contract
	, Prohibit
	, FinishProcess
	, PlasterProcess
	, ElectricProcess
	, Labor
	, Change
	, Share
	, Cement
	, SteelFrame
	, I_Steel
	, Direction_Up
	, Direction_Down
	, Direction_Left
	, Direction_Right
	, None
);


struct sMarkup
{
	eMarkup::Enum type;
	string name;
	string desc;  // description
	string iconFileName;
	graphic::cTexture *icon;

	sMarkup() {}
	sMarkup(const eMarkup::Enum &type0, const string &name0
		, const string &desc0, const string &iconFileName0
		, graphic::cTexture *icon0=nullptr)
	 : type(type0), name(name0), desc(desc0), iconFileName(iconFileName0), icon(icon0) {}
};
