//
// 2019-09-04, jjuiddong
// 3D View
//
#pragma once

class cRobotRenderer;

class c3DView : public framework::cDockWindow
{
public:
	c3DView(const string &name);
	virtual ~c3DView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;
	virtual void OnPreRender(const float deltaSeconds) override;
	virtual void OnResizeEnd(const framework::eDockResize::Enum type, const common::sRectf &rect) override;
	virtual void OnEventProc(const sf::Event &evt) override;
	virtual void OnResetDevice() override;


protected:
	void RenderKeymap(const ImVec2 &pos);
	void RenderPopupmenu();
	void RenderPcMemo();
	void RenderShareFile();
	void RenderMarkup(graphic::cRenderer &renderer);
	void RenderMeasure(graphic::cRenderer &renderer);
	Vector3 PickPointCloud(const POINT mousePt);
	bool JumpPin(const string &pinName);
	bool MakeShareFile();
	
	void UpdateLookAt(const POINT &mousePt);
	void OnWheelMove(const float delta, const POINT mousePt);
	void OnMouseMove(const POINT mousePt);
	void OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt);
	void OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt);


public:
	graphic::cSphere2 m_sphere;
	graphic::cModel m_pointCloud;
	graphic::cGridLine m_gridLine;
	graphic::cCube m_cube;
	graphic::cQuad2D m_quad1;
	graphic::cQuad2D m_quad2;
	graphic::cQuad2D m_quad3;
	graphic::cQuad2D m_keyMap;
	graphic::cShader11 m_shader;
	graphic::cShader11 m_pointShader;
	graphic::cTexture *m_pinImg;
	graphic::cBillboard m_markup;
	graphic::cRenderTarget m_renderTarget;
	graphic::cTexture *m_keymapBtnTex;
	graphic::cTexture *m_shareBtnTex;
	ImVec2 m_keymapBtnSize;
	ImVec2 m_shareBtnSize;
	cPointCloudDB::sPin *m_curPinInfo; // current select camera

	Vector3 m_pickPos;
	Vector3 m_pointCloudPos;
	Vector4 m_pickUV;
	Vector2 m_uv;
	Vector2 m_pointUV;
	POINT m_popupMenuPos;
	common::Ray m_pointUVRay;
	float m_sphereRadius;
	float m_pickPosDistance;
	bool m_isShowWireframe;
	bool m_isShowTexture;
	bool m_isShowGridLine;
	bool m_isShowPointCloud1; // point cloud
	bool m_isShowPointCloud2; // tessellation
	bool m_isShowPopupMenu;
	bool m_isBeginPopupMenu;
	bool m_isShowKeymap;
	bool m_isShowMeasure;

	bool m_isUpdatePcWindowPos; // point information 창 위치를 재조정한다.

	// MouseMove Variable
	POINT m_viewPos;
	POINT m_mousePos; // window 2d mouse pos
	POINT m_clickPos; // window 2d mouse pos
	Vector3 m_mousePickPos; // mouse cursor pos in ground picking
	bool m_mouseDown[3]; // Left, Right, Middle
};
