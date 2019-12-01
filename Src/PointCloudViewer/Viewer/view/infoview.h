//
// 2019-10-08, jjuiddong
// Point Cloud Information View
//
#pragma once


class cInfoView : public framework::cDockWindow
{
public:
	cInfoView(const StrId &name);
	virtual ~cInfoView();

	bool Init(graphic::cRenderer &renderer);
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;


protected:
	void RenderMarkupList();
	void RenderMeasure();
	void RenderCapture();
	void RenderPopupmenu();
	void RenderPinHierarchy();

public:
	graphic::cTexture *m_measureTex;
	graphic::cTexture *m_captureTex;
	graphic::cTexture *m_markupTex;
	graphic::cTexture *m_shareTex;
	ImVec2 m_measureBtnSize;
	ImVec2 m_captureBtnSize;
	ImVec2 m_markupBtnSize;
	ImVec2 m_shareBtnSize;

	bool m_isShowPopupMenu;
	bool m_isBeginPopupMenu;
};
