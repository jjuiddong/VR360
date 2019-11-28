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

	bool Init();
	virtual void OnUpdate(const float deltaSeconds) override;
	virtual void OnRender(const float deltaSeconds) override;


protected:
	void RenderMarkupList();
	void RenderPinHierarchy();
};
