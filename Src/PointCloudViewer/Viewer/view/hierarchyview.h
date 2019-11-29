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

	bool NewProject();
	bool OpenProject();
	bool SaveProject();
	bool ProjectSetting();


protected:
	bool RenderNewProjectDlg();
	bool RenderEditPinDlg();
	bool RenderDateEdit();
	bool RenderFloorEdit();
	bool RenderPinEdit();
	bool RenderHierarchy(common::sFolderNode *node);
	bool RenderHierarchy2(common::sFolderNode *node);
	bool UpdateDirectoryHierarchy(const StrPath &searchPath);


public:
	enum class eProjectEditMode {None, New, Modify};

	eProjectEditMode m_projEditMode;
	common::sFolderNode *m_hierarchy; // directory hierarchy

	// project creation variable
	graphic::cTexture *m_keymapTexture; // keymap texture
	cPointCloudDB m_editPc; // edit point cloud
	cPointCloudDB m_tempPc; // copy editPc
	cPointCloudDB::sDate *m_selDate;
	cPointCloudDB::sFloor *m_selFloor;
	cPointCloudDB::sPin *m_selPin;
	string m_selFileStr;
	graphic::cTexture *m_pinImg;
	bool m_isOpenNewProj;

	graphic::cTexture *m_folderTex;
	ImVec2 m_folderIconSize;
};
