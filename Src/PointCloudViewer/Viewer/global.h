//
// 2019-10-08, jjuiddong
// global variable
//
#pragma once


class c3DView;
class cInfoView;

class cGlobal
{
public:
	cGlobal();
	virtual ~cGlobal();

	bool Init();
	void Clear();


public:
	c3DView *m_3dView;
	cInfoView *m_infoView;
	cPointCloudDB m_pcDb;
	string m_currentCameraName;
};
