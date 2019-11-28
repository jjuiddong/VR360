
#include "stdafx.h"
#include "infoview.h"


cInfoView::cInfoView(const StrId &name)
	: framework::cDockWindow(name)
{
}

cInfoView::~cInfoView()
{
}


bool cInfoView::Init()
{
	return true;
}


void cInfoView::OnUpdate(const float deltaSeconds)
{
}


void cInfoView::OnRender(const float deltaSeconds)
{
	RenderMarkupList(); // markup btn
	RenderPinHierarchy();
}


// render markup list
void cInfoView::RenderMarkupList()
{
	static const char *totalStr = u8"---- 전체 ----";
	static const char *prevStr = totalStr;
	static eMarkup::Enum sortType = eMarkup::None;

	ImGui::Text("Markup List");

	ImGui::Spacing();
	ImGui::Text(u8"정렬:");
	ImGui::SameLine();
	ImGui::PushItemWidth(150);
	if (ImGui::BeginCombo("##markup combo", prevStr))
	{
		if (ImGui::Selectable(totalStr))
		{
			prevStr = totalStr;
			sortType = eMarkup::None;
		}

		for (auto &markup : g_global->m_markups)
		{
			if (ImGui::Selectable(markup.name.c_str()))
			{
				prevStr = markup.name.c_str();
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
		for (auto &floor : date->floors)
		{
			for (auto &pin : floor->pins)
			{
				ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
				ImGui::PushID(pin->name.c_str());
				if (ImGui::TreeNode(pin->name.c_str()))
				{
					for (auto &pc : pin->pcds)
					{
						if (cPointCloudDB::sPCData::MEMO == pc->type)
							continue;

						if ((sortType != eMarkup::None)
							&& (pc->markup != sortType))
							continue;

						ImGui::PushID((int)pc->name.c_str());
						if (ImGui::TreeNode(pc->name.c_str()))
						{
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.1f, 0.1f, 1.f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.1f, 0.1f, 1.f));
							if (ImGui::Button("Remove"))
							{
								common::Str128 msg;
								msg.Format("Remove Markup [ %s ]?", pc->name.ansi().c_str());
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
		}//~floors
	}//~dates

	// remove point
	for (auto &date : pcDb.m_project.dates)
		for (auto &floor : date->floors)
			for (auto &id : rmPcs)
				pcDb.RemoveData(floor, id);
}


// render pin hierarchy
void cInfoView::RenderPinHierarchy()
{
	cPointCloudDB &pcDb = g_global->m_pcDb;

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("Memo List");

	set<int> rmPcs;
	for (auto &date : pcDb.m_project.dates)
	{
		for (auto &floor : date->floors)
		{
			for (auto &pin : floor->pins)
			{
				ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
				if (ImGui::TreeNode(pin->name.c_str()))
				{
					for (auto &pc : pin->pcds)
					{
						if (cPointCloudDB::sPCData::MARKUP == pc->type)
							continue;

						ImGui::PushID((int)pc->name.c_str());
						if (ImGui::TreeNode(pc->name.c_str()))
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
			}//~pins
		} //~floors
	}//~dates

	// remove point
	for (auto &date : pcDb.m_project.dates)
		for (auto &floor : date->floors)
			for (auto &id : rmPcs)
				pcDb.RemoveData(floor, id);
}
