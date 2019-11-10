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


public:

	enum class eProjectEditMode {None, New, Modify};

	eProjectEditMode m_projEditMode;
	common::Str64 m_projName; // project name
	StrPath m_projDirPath; // project directory path
	StrPath m_projKeymapFileName; // project keymap filename
	graphic::cTexture *m_keymapTexture; // keymap texture
	cPointCloudDB::sCamera *m_selCam = nullptr;
	common::Str64 m_camName; // camera name
	StrPath m_camPcdFileName; // camera point cloud filename
	StrPath m_camTextureFileName; // camera texture filename
	Vector2 m_camPos;
	float m_camTessScale;
	cPointCloudDB::sProject m_editProj;
	graphic::cTexture *m_pinImg;
};
