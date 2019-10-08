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


public:
	c3DView *m_3dView;
	cInfoView *m_infoView;
};
