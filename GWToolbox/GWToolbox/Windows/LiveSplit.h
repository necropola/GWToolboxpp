#pragma once

#include <vector>

#include <Defines.h>

#include <GWCA/GameContainers/GamePos.h>

#include "ToolboxWindow.h"

class LiveSplit : public ToolboxWindow {
	LiveSplit() {};
	~LiveSplit() {};
public:
	static LiveSplit& Instance() {
		static LiveSplit instance;
		return instance;
	}

	const char* Name() const override { return "LiveSplit"; }

	void Initialize() override;

	void Update(float delta) override;

	void Draw(IDirect3DDevice9* pDevice) override;
	void DrawSettingInternal() override;

	void LoadSettings(CSimpleIni* ini) override;
	void SaveSettings(CSimpleIni* ini) override;

private:
	struct Segment {
		enum Type {
			DoorOpen,
			QuestComplete,
			LevelDone,
			DoaAreadone, 
			ChatMessage
		} const type;
		const std::vector<uint32_t> ids; // quest id, door id, etc
		const char name[126];
		const int target = 0; // in seconds
		// ------ varying data below --------
		DWORD time = 0;
		float diff = 0; // in seconds
		bool done = false;
		void MarkDone(DWORD current_time);
	};

	void Reset();

	void AddDoaObjectives(GW::Vec2f spawn);

	void EventHappened(Segment::Type type, uint32_t id);

	std::vector<Segment> segments;
	size_t done = 0;

	void SaveHistory();
	void SaveSegmentInHistory();
	std::vector<std::vector<int>> history;
};
