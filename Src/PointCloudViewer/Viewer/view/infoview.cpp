
#include "stdafx.h"
#include "infoview.h"
#include "3dview.h"


cInfoView::cInfoView(const StrId &name)
	: framework::cDockWindow(name)
	, m_measureTex(nullptr)
	, m_captureTex(nullptr)
	, m_markupTex(nullptr)
	, m_shareTex(nullptr)
	, m_measureBtnSize(50, 30)
	, m_captureBtnSize(50, 30)	
	, m_markupBtnSize(50, 30)
	, m_shareBtnSize(50, 30)
	, m_isShowPopupMenu(false)
{
}

cInfoView::~cInfoView()
{
}


bool cInfoView::Init(graphic::cRenderer &renderer)
{
	// 측정 버튼 생성
	m_measureTex = graphic::cResourceManager::Get()->LoadTexture(
		renderer, "./media/icon/5.png");
	if (m_measureTex)
	{
		const float r = (float)m_measureTex->m_imageInfo.Height 
			/ (float)m_measureTex->m_imageInfo.Width;
		m_measureBtnSize.y = r * 50.f;
	}

	// 캡쳐 버튼 생성
	m_captureTex = graphic::cResourceManager::Get()->LoadTexture(
		renderer, "./media/icon/8.png");
	if (m_captureTex)
	{
		const float r = (float)m_measureTex->m_imageInfo.Height
			/ (float)m_measureTex->m_imageInfo.Width;
		m_captureBtnSize.y = r * 50.f;
	}

	// 마크업 버튼 생성
	m_markupTex = graphic::cResourceManager::Get()->LoadTexture(
		renderer, "./media/icon/markupbtn.png");
	if (m_markupTex)
	{
		const float r = (float)m_markupTex->m_imageInfo.Height
			/ (float)m_markupTex->m_imageInfo.Width;
		m_markupBtnSize.y = r * 50.f;
	}

	// 공유 버튼 생성
	m_shareTex = graphic::cResourceManager::Get()->LoadTexture(
		renderer, "./media/icon/sharebtn.png");
	if (m_shareTex)
	{
		const float r = (float)m_shareTex->m_imageInfo.Height
			/ (float)m_shareTex->m_imageInfo.Width;
		m_shareBtnSize.y = r * 50.f;
	}

	return true;
}


void cInfoView::OnUpdate(const float deltaSeconds)
{
}


void cInfoView::OnRender(const float deltaSeconds)
{
	RenderMarkupList();
	RenderMeasure();
	RenderCapture();
	RenderPinHierarchy();
	RenderPopupmenu();
}


// render markup list
void cInfoView::RenderMarkupList()
{
	static const char *totalStr = u8"---- 전체 ----";
	static StrId prevStr = totalStr;
	static eMarkup::Enum sortType = eMarkup::None;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Mark-up"))
	{
		const ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
		ImGui::PushStyleColor(ImGuiCol_Button, col);
		if (ImGui::ImageButton(m_markupTex->m_texSRV, m_markupBtnSize))
		{
			g_global->m_3dView->m_pointCloudPos = g_global->m_3dView->m_pickPos;
			g_global->m_state = eEditState::VR360;
			m_isShowPopupMenu = true;
		}
		ImGui::PopStyleColor();

		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("Mark-up");
			ImGui::EndTooltip();
		}

		ImGui::Text(u8"정렬:");
		ImGui::SameLine();
		ImGui::PushItemWidth(150);
		if (ImGui::BeginCombo("##markup combo", prevStr.c_str()))
		{
			if (ImGui::Selectable(totalStr))
			{
				prevStr = totalStr;
				sortType = eMarkup::None;
			}

			for (auto &markup : g_global->m_markups)
			{
				if (ImGui::Selectable(StrId(markup.name).utf8().c_str()))
				{
					prevStr = StrId(markup.name).utf8().c_str();
					sortType = markup.type;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		cPointCloudDB &pcDb = g_global->m_pcDb;
		set<int> rmPcs;

		for (auto &date : pcDb.m_project.dates)
		{
			ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
			ImGui::PushID((int)date->name.c_str());
			if (ImGui::TreeNode(date->name.utf8().c_str()))
			{
				for (auto &floor : date->floors)
				{
					ImGui::PushID((int)floor->name.c_str());
					if (ImGui::TreeNode(floor->name.utf8().c_str()))
					{
						for (auto &pin : floor->pins)
						{
							ImGui::PushID((int)pin->name.c_str());
							if (ImGui::TreeNode(pin->name.utf8().c_str()))
							{
								for (auto &pc : pin->pcds)
								{
									if (cPointCloudDB::sPCData::MEMO == pc->type)
										continue;

									if ((sortType != eMarkup::None)
										&& (pc->markup != sortType))
										continue;

									ImGui::PushID((int)pc->name.c_str());
									if (ImGui::TreeNode(pc->name.utf8().c_str()))
									{
										ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.f));
										ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.1f, 0.1f, 1.f));
										ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.1f, 0.1f, 1.f));
										if (ImGui::Button("Remove"))
										{
											common::Str128 msg;
											msg.Format("Remove Markup [ %s ]?", pc->name.c_str());
											if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
												, msg.c_str(), "CONFIRM", MB_YESNO))
											{
												rmPcs.insert(pc->id);
											}
										}
										ImGui::PopStyleColor(3);

										ImGui::TreePop();
									}//~point tree
									ImGui::PopID();
								}//~points
								ImGui::TreePop();
							}
							ImGui::PopID();
						}//~pins
						ImGui::TreePop();
					}//~floors tree node
					ImGui::PopID();
				}//~floors
				ImGui::TreePop();
			} //~date treenode
			ImGui::PopID();
		}//~dates

		// remove point
		for (auto &date : pcDb.m_project.dates)
			for (auto &floor : date->floors)
				for (auto &id : rmPcs)
					pcDb.RemoveData(floor, id);

		ImGui::Spacing();
		ImGui::Spacing();
	}//~collapsing header

}


void cInfoView::RenderMeasure()
{
	// equi rectangular로 출력할 때는 측정기능을 끈다.
	const bool isEquirectangular = g_global->m_3dView->m_isShowEquirectangular;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Measure"))
	{
		// measure button
		// 'Measure' toggle style button
		if (g_global->m_state == eEditState::Measure)
		{
			const ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
			ImGui::PushStyleColor(ImGuiCol_Button, col);
		}
		else
		{
			const ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
			ImGui::PushStyleColor(ImGuiCol_Button, col);
		}

		if (ImGui::ImageButton(m_measureTex->m_texSRV, m_measureBtnSize))
		{
			if (!isEquirectangular)
			{
				if (g_global->m_state == eEditState::Measure)
					g_global->m_state = eEditState::VR360;
				else
					g_global->m_state = eEditState::Measure;
			}
		}
		ImGui::PopStyleColor(1);

		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("Measure Mode");
			ImGui::EndTooltip();
		}
		//~button

		ImGui::SameLine();
		if (isEquirectangular)
		{
			ImGui::Text("Disable");
		}
		else
		{
			if (g_global->m_state == eEditState::Measure)
				ImGui::Text("Measure Mode");
			else
				ImGui::Text("Enable");			
		}

		ImGui::TextUnformatted("Show Measure");
		ImGui::SameLine();
		ImGui::Checkbox("##showmeasure", &g_global->m_3dView->m_isShowMeasure);

		ImGui::SameLine();

		vector<sMeasurePt> *measurePts = g_global->GetCurrentMeasurePts();

		if (ImGui::Button("Clear"))
		{
			if (measurePts)
				measurePts->clear();
		}

		if (measurePts)
		{
			int rmPt = -1;
			for (uint i = 0; i < measurePts->size(); ++i)
			{
				const sMeasurePt &p0 = measurePts->at(i);

				float indent = 0;
				if ((i % 2) == 0)
				{
					ImGui::PushID((int)&p0);
					if (ImGui::Button("X"))
						rmPt = (int)i;
					ImGui::PopID();
					ImGui::SameLine();
				}
				else
				{
					indent = 23;
				}

				common::Str128 text;
				text.Format("%.1f, %.1f, %.1f", p0.rpos.x, p0.rpos.y, p0.rpos.z);
				ImGui::Indent(indent);
				ImGui::Selectable(text.c_str());
				ImGui::Unindent(indent);
			}

			// remove measure point
			if (rmPt >= 0)
			{
				// remove pair point
				common::rotatepopvector(*measurePts, rmPt);
				if ((int)measurePts->size() > rmPt)
					common::rotatepopvector(*measurePts, rmPt);
			}
		}

		ImGui::Spacing();
		ImGui::Spacing();
	}
}


void cInfoView::RenderCapture()
{
	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Capture"))
	{
		// capture button
		const ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
		ImGui::PushStyleColor(ImGuiCol_Button, col);
		if (ImGui::ImageButton(m_captureTex->m_texSRV, m_captureBtnSize))
		{
			g_global->m_state = eEditState::Capture;
		}
		ImGui::PopStyleColor();

		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("Capture");
			ImGui::EndTooltip();
		}

		for (auto &fileName : g_global->m_captures)
			ImGui::Selectable(fileName.c_str());

		ImGui::Spacing();
		ImGui::Spacing();
	}
}


// render popup menu
void cInfoView::RenderPopupmenu()
{
	const Vector3 pointCloudPos = g_global->m_3dView->m_pointCloudPos;
	const Vector2 pointUV = g_global->m_3dView->m_pointUV;

	cPointCloudDB::sFloor *floor = g_global->m_pcDb.FindFloor(
		g_global->m_dateName, g_global->m_floorName);
	if (!floor || pointCloudPos.IsEmpty())
	{
		m_isShowPopupMenu = false;
		return;
	}

	if (m_isShowPopupMenu)
	{
		ImGui::OpenPopup("PopupMenu");
		m_isShowPopupMenu = false;
	}

	m_isBeginPopupMenu = false;
	if (ImGui::BeginPopup("PopupMenu"))
	{
		m_isBeginPopupMenu = true;

		if (ImGui::BeginMenu("Mark-up"))
		{
			for (auto &markup : g_global->m_markups)
			{
				if (ImGui::MenuItem(StrId(markup.name).utf8().c_str()))
				{
					// add markup
					cPointCloudDB::sPCData *pc = g_global->m_pcDb.CreateData(
						floor, g_global->m_pinName);
					if (pc)
					{
						pc->type = cPointCloudDB::sPCData::MARKUP;
						pc->name = markup.name;
						pc->markup = markup.type;
						pc->pos = pointCloudPos;
						pc->uvpos = pointUV;
					}
				}
			}

			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}
}


// render pin hierarchy
void cInfoView::RenderPinHierarchy()
{
	cPointCloudDB &pcDb = g_global->m_pcDb;
	const int iddummy = 1234;

	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	if (ImGui::CollapsingHeader("Memo"))
	{
		set<int> rmPcs;
		for (auto &date : pcDb.m_project.dates)
		{
			ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
			ImGui::PushID((int)date->name.c_str() + iddummy);
			if (ImGui::TreeNode(date->name.utf8().c_str()))
			{
				for (auto &floor : date->floors)
				{
					ImGui::PushID((int)floor->name.c_str() + iddummy);
					if (ImGui::TreeNode(floor->name.utf8().c_str()))
					{
						for (auto &pin : floor->pins)
						{
							ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
							ImGui::PushID((int)pin->name.c_str() + 10000);
							if (ImGui::TreeNode(pin->name.utf8().c_str()))
							{
								for (auto &pc : pin->pcds)
								{
									if (cPointCloudDB::sPCData::MARKUP == pc->type)
										continue;

									ImGui::PushID((int)pc->name.c_str());
									if (ImGui::TreeNode(pc->name.utf8().c_str()))
									{
										ImGui::PushID(pc + 1);
										common::Str128 text;
										//text.Format("Pos : %.2f, %.2f, %.2f", pc->pos.x, pc->pos.y, pc->pos.z);
										text.Format("Pos : %.2f,%.2f", pc->uvpos.x, pc->uvpos.y);
										ImGui::Selectable(text.c_str());
										ImGui::PopID();

										ImGui::PushID(pc + 2);
										ImGui::InputTextMultiline("", pc->desc.m_str, pc->desc.SIZE);
										ImGui::PopID();

										ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.f));
										ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.1f, 0.1f, 1.f));
										ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.1f, 0.1f, 1.f));
										if (ImGui::Button("Remove"))
										{
											common::Str128 msg;
											msg.Format("Remove Point [ %s ]?", pc->name.c_str());
											if (IDYES == ::MessageBoxA(m_owner->getSystemHandle()
												, msg.c_str(), "CONFIRM", MB_YESNO))
											{
												rmPcs.insert(pc->id);
											}
										}
										ImGui::PopStyleColor(3);

										ImGui::TreePop();
									}//~point tree
									ImGui::PopID();
								} //~points

								ImGui::TreePop();
							}//~pin tree
							ImGui::PopID();
						}//~pins
						ImGui::TreePop();
					}//~floor nodetree
					ImGui::PopID();
				} //~floors
				ImGui::TreePop();
			}//~date treenode
			ImGui::PopID();
		}//~dates

		// remove point
		for (auto &date : pcDb.m_project.dates)
			for (auto &floor : date->floors)
				for (auto &id : rmPcs)
					pcDb.RemoveData(floor, id);
	}//~collapsing header
}
