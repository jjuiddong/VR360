
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
	cPointCloudDB &pcDb = g_global->m_pcDb;

	set<int> rmPcs;

	for (auto &floor : pcDb.m_project.floors)
	{
		for (auto &cam : floor->cams)
		{
			ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode(cam->name.c_str()))
			{
				for (auto &pc : cam->pcds)
				{
					ImGui::PushID(pc->name.c_str());
					if (ImGui::TreeNode(pc->name.c_str()))
					{
						ImGui::PushID(pc + 1);
						common::Str128 text;
						text.Format("Pos : %.2f, %.2f, %.2f", pc->pos.x, pc->pos.y, pc->pos.z);
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
			}//~camera tree
		}//~cameras
	} //~floors

	// remove point
	for (auto &floor : pcDb.m_project.floors)
		for (auto &id : rmPcs)
			pcDb.RemoveData(floor, id);
}
