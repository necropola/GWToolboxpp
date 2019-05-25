#include "LiveSplit.h"

#include <fstream>

#include <GWCA/Managers/StoCMgr.h>
#include <GWCA/Packets/StoC.h>
#include <GWCA/Constants/Maps.h>
#include <GWCA/Constants/Constants.h>
#include <GWCA/GameContainers/Array.h>
#include <GWCA/Managers/MapMgr.h>

#include <Modules/Resources.h>

namespace {
	enum class DoaAreaID : uint32_t {
		Foundry = 0x273F,
		Veil,
		Gloom,
		City
	};
}

void LiveSplit::Initialize() {
	ToolboxWindow::Initialize();

	GW::StoC::AddCallback<GW::Packet::StoC::PartyDefeated>(
		[this](GW::Packet::StoC::PartyDefeated *packet) -> bool {
		return false;
	});

	GW::StoC::AddCallback<GW::Packet::StoC::GameSrvTransfer>(
		[this](GW::Packet::StoC::GameSrvTransfer *packet) -> bool {
		return false;
	});

	GW::StoC::AddCallback<GW::Packet::StoC::InstanceLoadFile>(
		[this](GW::Packet::StoC::InstanceLoadFile *packet) -> bool {
		// We would want to have a default type that can handle objective by using name in Guild Wars
		// The only thing we miss is how to determine wether this map has a mission objectives.
		// We could use packet 187, but this can be a little bit hairy to do. Ask Ziox for more info.
		
		switch (packet->map_fileID) {
		case 219215: 
			Reset();
			AddDoaObjectives(packet->spawn_point); 
			break;
		default:
			break;
		}
		return false;
	});

	GW::StoC::AddCallback<GW::Packet::StoC::ObjectiveAdd>(
		[this](GW::Packet::StoC::ObjectiveAdd *packet) -> bool {
		// type 12 is the "title" of the mission objective, should we ignore it or have a "title" objective ?
		return false;
	});

	GW::StoC::AddCallback<GW::Packet::StoC::ObjectiveUpdateName>(
		[this](GW::Packet::StoC::ObjectiveUpdateName* packet) -> bool {
		//Objective *obj = GetCurrentObjective(packet->objective_id);
		//if (obj) obj->SetStarted();
		return false;
	});

	GW::StoC::AddCallback<GW::Packet::StoC::ObjectiveDone>(
		[this](GW::Packet::StoC::ObjectiveDone* packet) -> bool {
		//Objective *obj = GetCurrentObjective(packet->objective_id);
		//if (obj) {
		//	obj->SetDone();
		//	objective_sets.back()->CheckSetDone();
		//}
		return false;
	});

	GW::StoC::AddCallback<GW::Packet::StoC::AgentUpdateAllegiance>(
		[this](GW::Packet::StoC::AgentUpdateAllegiance* packet) -> bool {
		return false;
	});

	GW::StoC::AddCallback<GW::Packet::StoC::DoACompleteZone>(
		[this](GW::Packet::StoC::DoACompleteZone* packet) -> bool {
		if (packet->message[0] != 0x8101) return false;
		uint32_t id = packet->message[1];
		EventHappened(Segment::Type::DoaAreadone, id);
		return false;
	});

	GW::StoC::AddCallback<GW::Packet::StoC::ManipulateMapObject>(
		[this](GW::Packet::StoC::ManipulateMapObject* packet) -> bool {
			if (GW::Map::GetInstanceType() != GW::Constants::InstanceType::Explorable) return false;
			if (packet->unk1 != 0 || packet->unk2 != 16) return false;
			EventHappened(Segment::Type::DoorOpen, packet->object_id);
			//printf("0x111: obj %d, %d - %d\n", packet->object_id, packet->unk1, packet->unk2);
			return false;
		});

	// todo: remove
	AddDoaObjectives(GW::Vec2f(0, 0));
}

void LiveSplit::Update(float delta) {

}

void LiveSplit::Draw(IDirect3DDevice9* pDevice) {
	if (!visible) return;

	ImGui::SetNextWindowPosCenter(ImGuiSetCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiSetCond_FirstUseEver);
	if (ImGui::Begin(Name(), GetVisiblePtr(), GetWinFlags())) {

		for (size_t i = 0; i < segments.size(); ++i) {
			if (i == done && i > 0) {
				ImVec2 pos = ImGui::GetCursorScreenPos();
				ImGui::GetWindowDrawList()->AddLine(
					ImVec2(pos.x, pos.y - 1), 
					ImVec2(pos.x + 200, pos.y - 1), 
					0xAAFFFFFF, 1.0f);
			}

			const Segment& s = segments[i];
			ImGui::PushID(i);
			ImGui::Text(s.name);
			
			if (s.done) {
				ImGui::SameLine(120.0f);
				if (s.diff > 0) {
					ImGui::TextColored(ImColor(1.0f, 0.0f, 0.0f), "+ %.1f", s.diff);
				} else {
					ImGui::TextColored(ImColor(0.0f, 1.0f, 0.0f), "- %.1f", -s.diff);
				}
			}
			
			ImGui::SameLine(180.0f);
			ImGui::Text("%2d:%02d", (s.target / 60), (s.target % 60));

			ImGui::PopID();
		}

	}
	if (ImGui::SmallButton("Export history as .csv")) {
		SaveHistory();
	}
	if (ImGui::IsItemHovered()) ImGui::SetTooltip("In GWToolbox settings folder / location logs");
	ImGui::End();
}

void LiveSplit::DrawSettingInternal() {

}

void LiveSplit::LoadSettings(CSimpleIni* ini) {
	ToolboxWindow::LoadSettings(ini);
}

void LiveSplit::SaveSettings(CSimpleIni* ini) {
	ToolboxWindow::SaveSettings(ini);
}

void LiveSplit::AddDoaObjectives(GW::Vec2f spawn) {
	//static const GW::Vec2f area_spawns[] = {
	//{ -10514, 15231 },  // foundry
	//{ -18575, -8833 },  // city
	//{ 364, -10445 },    // veil
	//{ 16034, 1244 },    // gloom
	//};
	//const GW::Vec2f mallyx_spawn(-3931, -6214);
	//
	//constexpr int n_areas = 4;
	//double best_dist = GW::GetDistance(spawn, mallyx_spawn);
	//int area = -1;
	//for (int i = 0; i < n_areas; ++i) {
	//	float dist = GW::GetDistance(spawn, area_spawns[i]);
	//	if (best_dist > dist) {
	//		best_dist = dist;
	//		area = i;
	//	}
	//}
	//if (area == -1) return; // we're doing mallyx, not doa!

	segments.push_back({ Segment::DoorOpen, { 6356 }, "Room 1" , 30});
	segments.push_back({ Segment::DoorOpen, { 45276 }, "Room 2" , 50});
	segments.push_back({ Segment::DoorOpen, { 55421 }, "Room 3" , 75});
	segments.push_back({ Segment::DoorOpen, { 17955 }, "Room 4" , 135});
	segments.push_back({ Segment::DoorOpen, { 45667 }, "Black beast gate", 190});
	segments.push_back({ Segment::DoaAreadone, { (uint32_t)DoaAreaID::Foundry }, "Foundry" , 230 });
	segments.push_back({ Segment::DoorOpen, { 54727 }, "City outside" , 320 });
	segments.push_back({ Segment::DoaAreadone, { (uint32_t)DoaAreaID::City }, "City", 390});
	segments.push_back({ Segment::DoorOpen, { 13005, 11772, 28851 }, "360 Take" });
	segments.push_back({ Segment::DoorOpen, { 56510, 4753 }, "360 Done", 545 });
	segments.push_back({ Segment::DoorOpen, { 46650, 29594, 49742, 55680, 28961 }, "Underlords", 590 });
	// todo: 6-0 take
	segments.push_back({ Segment::DoaAreadone, { (uint32_t)DoaAreaID::Veil}, "Veil" , 715 });
	// todo: more splits in gloom. Use npc dialogs. 
	segments.push_back({ Segment::DoaAreadone, { (uint32_t)DoaAreaID::Gloom}, "Gloom", 959 });
}

void LiveSplit::EventHappened(Segment::Type type, uint32_t id) {
	for (size_t i = 0; i < segments.size(); ++i) {
		auto& s = segments[i];
		if (s.type != type) continue;
		if (s.done) continue;
		for (const auto s_id : s.ids) {
			if (s_id == id) {
				s.MarkDone(GW::Map::GetInstanceTime());
				done = i + 1;
				break;
			}
		}
	}
}

void LiveSplit::Segment::MarkDone(DWORD current_time) {
	done = true;
	time = current_time / 1000;
	// note: current_time is in milliseconds
	diff = (current_time - (target * 1000)) / 1000.0f;
}

void LiveSplit::Reset() {
	SaveSegmentInHistory();
	segments.clear();
	done = 0;
}

void LiveSplit::SaveSegmentInHistory() {
	history.push_back(std::vector<int>());
	for (const auto& segment : segments) {
		if (segment.done) {
			history.back().push_back(segment.time);
		}
	}
}

void LiveSplit::SaveHistory() {

	if (!segments.empty()) SaveSegmentInHistory();

	SYSTEMTIME localtime;
	GetLocalTime(&localtime);
	std::wstring filename = std::wstring(L"runs ")
		+ L"-" + std::to_wstring(localtime.wYear)
		+ L"-" + std::to_wstring(localtime.wMonth)
		+ L"-" + std::to_wstring(localtime.wDay)
		+ L" - " + std::to_wstring(localtime.wHour)
		+ L"-" + std::to_wstring(localtime.wMinute)
		+ L"-" + std::to_wstring(localtime.wSecond)
		+ L".csv";

	std::ofstream file;
	const std::wstring path = Resources::GetPath(L"location logs", filename);
	file.open(path);

	file << "room 1, room 2, room 3, room 4, bb gate, foundry, city outside, city, 360 take, 360 done, underlords, 6-0 take, veil, gloom \n";
	
	for (const auto& run : history) {
		size_t i = 0;
		for (; i < run.size(); ++i) {
			file << std::to_string(run[i]) << ", ";
		}
		for (; i < 14; ++i) {
			file << ", ";
		}
		file << "\n";
	}
	file.close();

	history.clear();
}
