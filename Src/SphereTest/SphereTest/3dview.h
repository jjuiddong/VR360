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
	virtual void OnResizeEnd(const framework::eDockResize::Enum type, const sRectf &rect) override;
	virtual void OnEventProc(const sf::Event &evt) override;
	virtual void OnResetDevice() override;


protected:
	void UpdateLookAt(const POINT &mousePt);
	void OnWheelMove(const float delta, const POINT mousePt);
	void OnMouseMove(const POINT mousePt);
	void OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt);
	void OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt);


public:
	graphic::cSphere m_sphere;
	graphic::cGridLine m_gridLine;
	graphic::cQuad2D m_quad1;
	graphic::cQuad2D m_quad2;
	graphic::cQuad2D m_quad3;
	graphic::cShader11 m_shader;
	graphic::cRenderTarget m_renderTarget;

	Vector3 m_pickPos;
	Vector4 m_pickUV;
	Vector2 m_uv;
	bool m_isShowWireframe;
	bool m_isShowGridLine;

	// MouseMove Variable
	POINT m_viewPos;
	POINT m_mousePos; // window 2d mouse pos
	Vector3 m_mousePickPos; // mouse cursor pos in ground picking
	bool m_mouseDown[3]; // Left, Right, Middle
	float m_rotateLen;
	Plane m_groundPlane1;
	Plane m_groundPlane2;
};

