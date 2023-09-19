#include "DKUtil/Hook.hpp"
#include "StateMachine.h"
#include <time.h>
#include <algorithm>

DLLEXPORT constinit auto SFSEPlugin_Version = []() noexcept {
	SFSE::PluginVersionData data{};

	data.PluginVersion(Plugin::Version);
	data.PluginName(Plugin::NAME);
	data.AuthorName(Plugin::AUTHOR);
	data.UsesSigScanning(true);
	//data.UsesAddressLibrary(true);
	data.HasNoStructUse(true);
	//data.IsLayoutDependent(true);
	data.CompatibleVersions({ SFSE::RUNTIME_LATEST });

	return data;
}();

static REL::Relocation<__int64 (*)(double, char*, ...)> ExecuteCommand{ REL::Offset(0x287DF04) };
auto*                                                   Logger = RE::ConsoleLog::GetSingleton();
const int                                               TimePerFrame = 50;
const int                                               TimePerFrameAnimation = 25;
// int StateMachine.GetInstance().iCurrentShip = 0;

std::string n2hexstr(int w, size_t hex_len = sizeof(int) << 1)
{
	static const char* digits = "0123456789ABCDEF";
	std::string        rc(hex_len, '0');
	for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
		rc[i] = digits[(w >> j) & 0x0f];
	return rc;
}

void SetActorValue(int formID, std::string name, int value)
{
	std::string result = fmt::format("{}.setav {} {}", n2hexstr(formID), name, value);
	ExecuteCommand(0, result.data());
}

void SetGS(std::string name, int value)
{
	std::string result = fmt::format("SetGS \"{}\" {}", name, value);
	ExecuteCommand(0, result.data());
}

inline int Interpolation(int totalFrames, int currentFrame, int beginValue, int endValue) {
	return (0.0 + endValue - beginValue) * currentFrame / totalFrames + beginValue;
}

DWORD Animation(LPVOID lpParam)
{

	AnimationInfo* hsInfo = (AnimationInfo*)lpParam;
	for (int frame = 0; frame < hsInfo->iTotalFrames; frame++) {
		SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipenginepartmaxforwardspeed", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->iBeginMFS, hsInfo->iEndMFS));
		//SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipenginepartmaxbackwardspeed", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->iBeginMFS, hsInfo->iEndMFS) / 2);
		SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipforwardspeedmult", std::max(hsInfo->iBeginFSM, hsInfo->iEndFSM));
		SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipboostspeed", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->iBeginBS, hsInfo->iEndBS));
		SetGS("fFlightCameraFOV:FlightCamera", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->iBeginFOV, hsInfo->iEndFOV));
		Sleep(TimePerFrameAnimation);
	}
	StateMachine::GetInstance().bLock = 0;
	hsInfo->target.Enter();
	return 0;
}

State::State(int idx, int mfs, int fsm, int bs, int fov)
{
	iIndex = idx;
	iMaxForwardSpeed = mfs;
	iForwardSpeedMult = fsm;
	iBoostSpeed = bs;
	iFov = fov;
}

void State::Enter()
{
	SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipenginepartmaxforwardspeed", iMaxForwardSpeed);
	//SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipenginepartmaxbackwardspeed", iMaxForwardSpeed / 2);
	SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipforwardspeedmult", iForwardSpeedMult);
	SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipboostspeed", iBoostSpeed);
	SetGS("fFlightCameraFOV:FlightCamera", iFov);
	StateMachine::GetInstance().iCurrentState = iIndex;
}

void State::Exit(State& target)
{
	StateMachine::GetInstance().bLock = 1;
	SFSE::log::info("CRT: change to {}", target.iIndex);
	const int animationTime = 1000;
	// Logger->Print(fmt::format("State {} -> {}", iIndex, target.iIndex).c_str(), 0);
	AnimationInfo* hsInfo = new AnimationInfo{ target, animationTime / TimePerFrameAnimation, iMaxForwardSpeed, target.iMaxForwardSpeed, iForwardSpeedMult, target.iForwardSpeedMult, iBoostSpeed, target.iBoostSpeed, iFov, target.iFov };
	CreateThread(NULL, 0, Animation, hsInfo, 0, NULL);
}

StateMachine::StateMachine()
{
	vStateList.push_back(State(0, 300, 300, 4, 90));
	vStateList.push_back(State(1, 10000, 20000, 15, 100));
	vStateList.push_back(State(2, 500000, 1000000, 150, 110));
	vStateList.push_back(State(3, 30000000, 30000000, 1500, 130));
	vStateList.push_back(State(4, 300000000, 300000000, 1500, 135));
	iCurrentState = 0;
	iCurrentShip = 0;
	bLock = 0;
}

State& StateMachine::GetCurrentState() { return vStateList[iCurrentState]; }

void StateMachine::ChangeStateMachine(InputType type)
{
	if (bLock)
		return;
	if (type == InputType::SpeedUp) {
		if (iCurrentState >= vStateList.size() - 1) {
			// skip
		} else {
			GetCurrentState().Exit(vStateList[iCurrentState + 1]);
		}
	}
	if (type == InputType::SpeedDown) {
		if (iCurrentState <= 0) {
			// skip
		} else {
			GetCurrentState().Exit(vStateList[iCurrentState - 1]);
		}
	}
}

void StateMachine::RegisterShip(int ship)
{
	iCurrentShip = ship;
	// TODO: Get origin value here
	// vStateList[0].Enter();
}

static DWORD MainLoop(void* unused)
{
	(void)unused;
	bool  upHoldFlag = false;
	bool    downHoldFlag = false;
    for (;;) {
		int   up = SFSE::WinAPI::GetKeyState(33);
		int   down = SFSE::WinAPI::GetKeyState(34);
		auto* player = RE::Actor::PlayerCharacter();
		if (player){
			RE::TESObjectREFR* ship = player->GetAttachedSpaceship();
			if (ship) {
				if (StateMachine::GetInstance().iCurrentShip == 0) {
					StateMachine::GetInstance().RegisterShip(ship->formID);
				}
			}
			else {
				StateMachine::GetInstance().iCurrentShip = 0;
			}

		}
		if(StateMachine::GetInstance().iCurrentShip != 0) {
			// SFSE::log::info("CRT: up{} down{} uf{} df{}", std::to_string(up), std::to_string(down), std::to_string(upHoldFlag), std::to_string(downHoldFlag));
			if (upHoldFlag && up >= 0) {
				upHoldFlag = 0;
			}
			if (downHoldFlag && down >= 0) {
				downHoldFlag = 0;
			}
			if (up < 0 && !upHoldFlag) {
				SFSE::log::info("CRT: up");
				upHoldFlag = 1;
				StateMachine::GetInstance().ChangeStateMachine(StateMachine::InputType::SpeedUp);
			} else if (down < 0 && !downHoldFlag) {
				SFSE::log::info("CRT: down");
				downHoldFlag = 1;
				StateMachine::GetInstance().ChangeStateMachine(StateMachine::InputType::SpeedDown);
			}
		}
		Sleep(TimePerFrame);
    }
    return 0;
}

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type) {
		case SFSE::MessagingInterface::kPostLoad:
			{
				CreateThread(NULL, 0, MainLoop, NULL, 0, NULL);
				break;
			}
		default:
			break;
		}
	}
}

/**
// for preload plugins
void SFSEPlugin_Preload(SFSE::LoadInterface* a_sfse);
/**/

DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
#ifndef NDEBUG
	while (!IsDebuggerPresent()) {
		Sleep(100);
	}
#endif

	SFSE::Init(a_sfse, false);

	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));

	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	// do stuff
	SFSE::AllocTrampoline(1 << 10);

	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	return true;
}
