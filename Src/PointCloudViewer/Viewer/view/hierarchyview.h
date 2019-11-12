//
// 2019-11-09, jjuiddong
// Point Cloud Hierarchy View
//
#pragma once


class cHierarchyView : public framework::cDockWindow
{
public:
	cHierarchyView(const StrId &name);
	virtual ~cHierarchyView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;


protected:
	bool RenderNewProjectDlg();
	bool RenderEditCameraDlg();
	bool RenderHierarchy(common::sFolderNode *node);
	bool RenderHierarchy2(common::sFolderNode *node);
	bool UpdateDirectoryHierarchy(const StrPath &searchPath);


public:
	enum class eProjectEditMode {None, New, Modify};

	eProjectEditMode m_projEditMode;
	common::sFolderNode *m_hierarchy; // directory hierarchy

	// project creation variable
	graphic::cTexture *m_keymapTexture; // keymap texture
	cPointCloudDB::sFloor *m_selFloor;
	cPointCloudDB::sCamera *m_selCam;
	cPointCloudDB::sProject m_editProj;
	string m_selFileStr;
	graphic::cTexture *m_pinImg;
};
