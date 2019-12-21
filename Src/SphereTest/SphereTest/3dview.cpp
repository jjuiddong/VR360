
#include "stdafx.h"
#include "3dview.h"

using namespace graphic;
using namespace framework;


c3DView::c3DView(const string &name)
	: framework::cDockWindow(name)
	, m_groundPlane1(Vector3(0, 1, 0), 0)
	, m_groundPlane2(Vector3(0, -1, 0), 0)
	, m_isShowWireframe(false)
	, m_isShowTexture(true)
	, m_isShowGridLine(false)
	, m_isShowPointCloud1(true)
	, m_isShowPointCloud2(false)
{
}

c3DView::~c3DView()
{
}


bool c3DView::Init(cRenderer &renderer)
{
	const Vector3 eyePos = Vector3(0, 0, 0);
	const Vector3 lookAt = Vector3(100, 0, 0);

	const sRectf viewRect = GetWindowSizeAvailible();
	m_camera.SetCamera(eyePos, lookAt, Vector3(0, 1, 0));
	m_camera.SetProjection(MATH_PI / 4.f, viewRect.Width() / viewRect.Height(), 1.f, 100000.f);
	m_camera.SetViewPort(viewRect.Width(), viewRect.Height());

	const Vector3 lightPos = Vector3(300, 1300, -400);
	graphic::GetMainLight().SetDirection((Vector3::Zeroes - lightPos).Normal());
	graphic::GetMainLight().Bind(renderer);

	cViewport vp = renderer.m_viewPort;
	vp.m_vp.Width = viewRect.Width();
	vp.m_vp.Height = viewRect.Height();
	m_renderTarget.Create(renderer, vp, DXGI_FORMAT_R8G8B8A8_UNORM, true, true
		, DXGI_FORMAT_D24_UNORM_S8_UINT);

	m_gridLine.Create(renderer, 600, 600, 1.f, 1.f
		, (eVertexType::POSITION | eVertexType::COLOR)
		, cColor(0.4f, 0.4f, 0.4f, 1.f)
		, cColor(0.6f, 0.6f, 0.6f, 1.f)
	);
	m_gridLine.m_transform.pos.y = 0.01f;

	m_sphere.Create(renderer, 1000, 100, 100
		, graphic::eVertexType::POSITION 
		//| graphic::eVertexType::NORMAL
		| graphic::eVertexType::TEXTURE0
	);


	const char* fileName = 
		//"test6.jpg";
		"workobj/1.bmp";

	m_sphere.m_texture = graphic::cResourceManager::Get()->LoadTexture(
		//renderer, "Big_ben_equirectangular.jpg");
		//renderer, "J6tcu.png");
		renderer, fileName);

	{
		const float offset = 0.03f;
		const Vector2 uvs[4] = {
			Vector2(-offset,-offset)
			, Vector2(offset,-offset)
			, Vector2(-offset,offset)
			, Vector2(offset,offset)
		};
		m_quad1.Create(renderer, 0, 100, 200, 200
			, (eVertexType::POSITION_RHW | eVertexType::COLOR | eVertexType::TEXTURE0)
			, fileName
			, uvs
		);
	}

	{
		const float offset = 0.003f;
		const Vector2 uvs[4] = {
			Vector2(-offset,-offset)
			, Vector2(offset,-offset)
			, Vector2(-offset,offset)
			, Vector2(offset,offset)
		};
		m_quad2.Create(renderer, 0, 310, 200, 200
			, (eVertexType::POSITION_RHW | eVertexType::COLOR | eVertexType::TEXTURE0)
			, fileName
			, uvs
		);
	}

	{
		const float offset = 0.0003f;
		const Vector2 uvs[4] = {
			Vector2(-offset,-offset)
			, Vector2(offset,-offset)
			, Vector2(-offset,offset)
			, Vector2(offset,offset)
		};
		m_quad3.Create(renderer, 0, 520, 200, 200
			, (eVertexType::POSITION_RHW | eVertexType::COLOR | eVertexType::TEXTURE0)
			, fileName
			, uvs
		);
	}

	m_shader.Create(renderer, 
		"./media/shader11/uvcolor.fxo"
		//"./media/shader11/tess-pos_rhw-uvcolor.fxo"
		, "Unlit"
		, graphic::eVertexType::POSITION_RHW | graphic::eVertexType::COLOR 
		| graphic::eVertexType::TEXTURE0);

	m_quad1.m_shader = &m_shader;
	m_quad2.m_shader = &m_shader;
	m_quad3.m_shader = &m_shader;

	m_pointShader.Create(renderer,
		"./media/shader11/tess-box.fxo"
		, "Unlit"
		, graphic::eVertexType::POSITION 
		| graphic::eVertexType::NORMAL
		| graphic::eVertexType::COLOR
		| graphic::eVertexType::TEXTURE0
	);

	m_pointCloud.Create(renderer, common::GenerateId(), "./media/workobj/test_1.obj");

	return true;
}


void c3DView::OnUpdate(const float deltaSeconds)
{
	m_camera.Update(deltaSeconds);
}


void c3DView::OnPreRender(const float deltaSeconds)
{
	cRenderer &renderer = GetRenderer();
	cAutoCam cam(&m_camera);

	renderer.UnbindTextureAll();
	GetMainCamera().Bind(renderer);
	GetMainLight().Bind(renderer);

	if (m_renderTarget.Begin(renderer))
	{
		CommonStates state(renderer.GetDevice());
		if (m_isShowWireframe)
		{
			renderer.GetDevContext()->RSSetState(state.CullNone());
			renderer.GetDevContext()->RSSetState(state.Wireframe());
		}
		else
		{
			//renderer.GetDevContext()->RSSetState(state.CullCounterClockwise());
			renderer.GetDevContext()->RSSetState(state.CullNone());
		}

		if (m_isShowGridLine)
			m_gridLine.Render(renderer);

		if (m_isShowTexture)
			m_sphere.Render(renderer);

		if (m_isShowPointCloud1)
		{
			Transform tfm;
			tfm.scale = Vector3(-1, -1, 1);
			m_pointCloud.SetShader(NULL);
			m_pointCloud.Render(renderer, tfm.GetMatrixXM());
		}

		if (m_isShowPointCloud2)
		{
			Transform tfm;
			tfm.scale = Vector3(-1, -1, 1);
			renderer.m_cbTessellation.m_v->size = Vector2(1, 1) * .02f;
			m_pointCloud.SetShader(&m_pointShader);
			m_pointCloud.RenderTessellation(renderer, 1, tfm.GetMatrixXM());
		}

		{
			Transform tfm;
			tfm.pos = m_pickPos;
			tfm.scale = Vector3::Ones * 0.01f;
			renderer.m_dbgBox.SetColor(cColor::YELLOW);
			renderer.m_dbgBox.SetBox(tfm);
			renderer.m_dbgBox.Render(renderer);
		}

		// render mouse point
		const Ray ray = GetMainCamera().GetRay(m_mousePos.x, m_mousePos.y);
		renderer.m_dbgLine.SetColor(cColor::RED);
		renderer.m_dbgLine.SetLine(Vector3(0,0,0), ray.orig + ray.dir * 100.f, 0.001f);
		renderer.m_dbgLine.Render(renderer);
		
		renderer.m_cbPerFrame.m_v->eyePosW = m_pickUV.GetVectorXM();
		m_quad1.Render(renderer);
		m_quad2.Render(renderer);
		m_quad3.Render(renderer);
	}
	m_renderTarget.End(renderer);
}


void c3DView::OnRender(const float deltaSeconds)
{
	ImVec2 pos = ImGui::GetCursorScreenPos();
	m_viewPos = { (int)(pos.x), (int)(pos.y) };
	const sRectf viewRect = GetWindowSizeAvailible();
	ImGui::Image(m_renderTarget.m_resolvedSRV, ImVec2(viewRect.Width(), viewRect.Height()));
	//ImGui::Image(m_renderTarget.m_resolvedSRV, ImVec2(100,100));

	// HUD
	// Render Menu Window
	const float MENU_WIDTH = 450.f;
	const float windowAlpha = 0.0f;
	bool isOpen = true;
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowBgAlpha(windowAlpha);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
	ImGui::SetNextWindowSize(ImVec2(min(viewRect.Width(), MENU_WIDTH), 100.f));
	if (ImGui::Begin("Information", &isOpen, flags))
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::Checkbox("Texture", &m_isShowTexture);
		ImGui::SameLine();
		ImGui::Checkbox("Wireframe", &m_isShowWireframe);
		ImGui::SameLine();
		ImGui::Checkbox("GridLine", &m_isShowGridLine);
		ImGui::SameLine();
		ImGui::Checkbox("PointCloud1", &m_isShowPointCloud1);
		ImGui::SameLine();
		ImGui::Checkbox("PointCloud2", &m_isShowPointCloud2);
		ImGui::Text("uv = %f, %f", m_uv.x, m_uv.y);
	}
	ImGui::PopStyleColor();
	ImGui::End();
}


void c3DView::OnResizeEnd(const framework::eDockResize::Enum type, const sRectf &rect)
{
	if (type == eDockResize::DOCK_WINDOW)
	{
		m_owner->RequestResetDeviceNextFrame();
	}
}


Vector3 c3DView::PickPointCloud(const POINT mousePt)
{
	const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
	const Line line(ray.dir, ray.orig, 10000.f);
	const Plane plane(ray.dir, ray.orig);
	
	sRawMeshGroup2 *rawMeshes = cResourceManager::Get()->LoadRawMesh(
		"./media/workobj/test_1.obj");
	RETV(!rawMeshes, Vector3::Zeroes);

	Transform tfm;
	tfm.scale = Vector3(-1, -1, 1);
	Matrix44 tm = tfm.GetMatrix();

	Vector3 nearPos;
	float minLen = FLT_MAX;
	for (auto &meshes : rawMeshes->meshes)
	{
		for (auto &vtx : meshes.vertices)
		{
			const Vector3 pos = vtx * tm;
			if (plane.Distance(pos) < 0.f)
				continue;

			const float dist = line.GetDistance(pos);
			if (minLen > dist)
			{
				minLen = dist;
				nearPos = pos;
			}
		}
	}
	return nearPos;
}


void c3DView::UpdateLookAt(const POINT &mousePt)
{
	GetMainCamera().MoveCancel();

	const float centerX = GetMainCamera().m_width / 2;
	const float centerY = GetMainCamera().m_height / 2;
	const Ray ray = GetMainCamera().GetRay((int)centerX, (int)centerY);
	const float distance = m_groundPlane1.Collision(ray.dir);
	if (distance < -0.2f)
	{
		//GetMainCamera().m_lookAt = m_groundPlane1.Pick(ray.orig, ray.dir);
	}
	else
	{ // horizontal viewing
		//const Vector3 lookAt = GetMainCamera().m_eyePos + GetMainCamera().GetDirection() * 20.f;
		//GetMainCamera().m_lookAt = lookAt;
	}

	//GetMainCamera().UpdateViewMatrix();
}


// 휠을 움직였을 때,
// 카메라 앞에 박스가 있다면, 박스 정면에서 멈춘다.
void c3DView::OnWheelMove(const float delta, const POINT mousePt)
{
	UpdateLookAt(mousePt);

	float len = 0;
	const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
	Vector3 lookAt = m_groundPlane1.Pick(ray.orig, ray.dir);
	len = (ray.orig - lookAt).Length();

	const int lv = 10;// m_quadTree.GetLevel(len);
	const float zoomLen = min(len * 0.1f, (float)(2 << (16 - lv)));

	//GetMainCamera().Zoom(ray.dir, (delta < 0) ? -zoomLen : zoomLen);
}


// Handling Mouse Move Event
void c3DView::OnMouseMove(const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;

	const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);

	if (m_mouseDown[0])
	{
		Vector3 dir = GetMainCamera().GetDirection();
		Vector3 right = GetMainCamera().GetRight();
		dir.y = 0;
		dir.Normalize();
		right.y = 0;
		right.Normalize();
		//GetMainCamera().MoveRight(-delta.x * m_rotateLen * 0.001f);
		//GetMainCamera().MoveFrontHorizontal(delta.y * m_rotateLen * 0.001f);
	}
	else if (m_mouseDown[1])
	{
		m_camera.Yaw(delta.x * 0.005f);
		m_camera.Pitch(delta.y * 0.005f);
	}
	else if (m_mouseDown[2])
	{
		const float len = GetMainCamera().GetDistance();
		GetMainCamera().MoveRight(-delta.x * len * 0.001f);
		GetMainCamera().MoveUp(delta.y * len * 0.001f);
	}

	//
	float dotH = 0.f;
	const Vector3 hdir = Vector3(ray.dir.x, 0, ray.dir.z).Normal();
	dotH = hdir.DotProduct(Vector3(1, 0, 0));

	float dotV = 0.f;
	const Vector3 vdir = Vector3(ray.dir.x, 0, ray.dir.z).Normal();
	dotV = vdir.DotProduct(ray.dir);

	float u = 0.f;
	{
		const float angle = acos(dotH) / MATH_PI;
		if (Vector3(1, 0, 0).CrossProduct(hdir).y > 0)
		{
			u = 1.f - (angle * 0.5f);
		}
		else
		{
			u = (angle * 0.5f);
		}
	}

	float v = 0.f;
	{
		float angle = acos(dotV) / MATH_PI;
		if (ray.dir.y > 0)
		{
			v = 0.5f - angle;
		}
		else
		{
			v = abs(angle) + 0.5f;
		}		
	}

	m_pickUV = Vector4(u, v, 0, 0);
	m_uv = Vector2(u, v);


	m_pickPos = PickPointCloud(mousePt);
}


// Handling Mouse Button Down Event
void c3DView::OnMouseDown(const sf::Mouse::Button &button, const POINT mousePt)
{
	m_mousePos = mousePt;
	UpdateLookAt(mousePt);
	SetCapture();

	switch (button)
	{
	case sf::Mouse::Left:
	{
		m_mouseDown[0] = true;
		const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
		Vector3 p1 = m_groundPlane1.Pick(ray.orig, ray.dir);
		m_rotateLen = (p1 - ray.orig).Length();// min(500.f, (p1 - ray.orig).Length());
	}
	break;

	case sf::Mouse::Right:
	{
		m_mouseDown[1] = true;

		//const Ray ray = GetMainCamera().GetRay(mousePt.x, mousePt.y);
		//Vector3 target = m_groundPlane1.Pick(ray.orig, ray.dir);
		//const float len = (GetMainCamera().GetEyePos() - target).Length();
	}
	break;

	case sf::Mouse::Middle:
		m_mouseDown[2] = true;
		break;
	}
}


void c3DView::OnMouseUp(const sf::Mouse::Button &button, const POINT mousePt)
{
	const POINT delta = { mousePt.x - m_mousePos.x, mousePt.y - m_mousePos.y };
	m_mousePos = mousePt;
	ReleaseCapture();

	switch (button)
	{
	case sf::Mouse::Left:
		m_mouseDown[0] = false;
		break;
	case sf::Mouse::Right:
		m_mouseDown[1] = false;
		break;
	case sf::Mouse::Middle:
		m_mouseDown[2] = false;
		break;
	}
}


void c3DView::OnEventProc(const sf::Event &evt)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (evt.type)
	{
	case sf::Event::KeyPressed:
		switch (evt.key.cmd)
		{
		case sf::Keyboard::Return: break;
		case sf::Keyboard::Space: break;

			//case sf::Keyboard::Left: m_camera.MoveRight(-0.5f); break;
			//case sf::Keyboard::Right: m_camera.MoveRight(0.5f); break;
			//case sf::Keyboard::Up: m_camera.MoveUp(0.5f); break;
			//case sf::Keyboard::Down: m_camera.MoveUp(-0.5f); break;
		}
		break;

	case sf::Event::MouseMoved:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		OnMouseMove(pos);
	}
	break;

	case sf::Event::MouseButtonPressed:
	case sf::Event::MouseButtonReleased:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		const POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		if (sf::Event::MouseButtonPressed == evt.type)
		{
			const sRectf viewRect = GetWindowSizeAvailible(true);
			if (viewRect.IsIn((float)curPos.x, (float)curPos.y))
				OnMouseDown(evt.mouseButton.button, pos);
		}
		else
		{
			OnMouseUp(evt.mouseButton.button, pos);
		}
	}
	break;

	case sf::Event::MouseWheelScrolled:
	{
		cAutoCam cam(&m_camera);

		POINT curPos;
		GetCursorPos(&curPos); // sf::event mouse position has noise so we use GetCursorPos() function
		ScreenToClient(m_owner->getSystemHandle(), &curPos);
		const POINT pos = { curPos.x - m_viewPos.x, curPos.y - m_viewPos.y };
		OnWheelMove(evt.mouseWheelScroll.delta, pos);
	}
	break;
	}
}


void c3DView::OnResetDevice()
{
	cRenderer &renderer = GetRenderer();

	// update viewport
	sRectf viewRect = { 0, 0, m_rect.Width() - 15, m_rect.Height() - 50 };
	m_camera.SetViewPort(viewRect.Width(), viewRect.Height());

	cViewport vp = GetRenderer().m_viewPort;
	vp.m_vp.Width = viewRect.Width();
	vp.m_vp.Height = viewRect.Height();
	m_renderTarget.Create(renderer, vp, DXGI_FORMAT_R8G8B8A8_UNORM, true, true, DXGI_FORMAT_D24_UNORM_S8_UINT);
}
