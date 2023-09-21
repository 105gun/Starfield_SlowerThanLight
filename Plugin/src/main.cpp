#include "DKUtil/Hook.hpp"
#include "StateMachine.h"
#include <algorithm>
#include <time.h>

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
const int                                               TimePerFrameAnimation = 16;
// int StateMachine.GetInstance().iCurrentShip = 0;

std::string n2hexstr(int w, size_t hex_len = sizeof(int) << 1)
{
	static const char* digits = "0123456789ABCDEF";
	std::string        rc(hex_len, '0');
	for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
		rc[i] = digits[(w >> j) & 0x0f];
	return rc;
}

void SetActorValue(int formID, std::string name, float value)
{
	std::string result = fmt::format("{}.setav {} {}", n2hexstr(formID), name, value);
	ExecuteCommand(0, result.data());
}

void SetGS(std::string name, float value)
{
	std::string result = fmt::format("SetGS \"{}\" {}", name, value);
	ExecuteCommand(0, result.data());
}

void CallCommand(std::string name, float value)
{
	std::string result = fmt::format("{} {}", name, value);
	ExecuteCommand(0, result.data());
}

inline float Interpolation(float totalFrames, float currentFrame, float beginValue, float endValue)
{
	return (0.0 + endValue - beginValue) * currentFrame / totalFrames + beginValue;
}

DWORD Animation(LPVOID lpParam)
{
	AnimationInfo* hsInfo = (AnimationInfo*)lpParam;
	for (int frame = 0; frame < hsInfo->iTotalFrames; frame++) {
		if (StateMachine::GetInstance().bShutingDown) {
			return 0;
		}

		SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipenginepartmaxforwardspeed", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->iBeginMFS, hsInfo->iEndMFS));
		//SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipenginepartmaxbackwardspeed", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->iBeginMFS, hsInfo->iEndMFS) / 2);
		SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipforwardspeedmult", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->iBeginFSM, hsInfo->iEndFSM));
		SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipboostspeed", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->iBeginBS, hsInfo->iEndBS));
		SetGS("fFlightCameraFOV:FlightCamera", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->fBeginFOV, hsInfo->fEndFOV));
		CallCommand("sgtm", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->fBeginTimeScale, hsInfo->fEndTimeScale));
		SetGS("fSpaceshipMaxAngularVelocityScale", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->fBeginAngularVelocityScale, hsInfo->fEndAngularVelocityScale));
		Sleep(TimePerFrameAnimation);
	}
	StateMachine::GetInstance().bLock = 0;
	hsInfo->target->Enter();
	return 0;
}

DWORD AnimationShutdown(LPVOID lpParam)
{
	AnimationInfo* hsInfo = (AnimationInfo*)lpParam;
	for (int frame = 0; frame < hsInfo->iTotalFrames; frame++) {
		SetGS("fFlightCameraFOV:FlightCamera", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->fBeginFOV, hsInfo->fEndFOV));
		CallCommand("sgtm", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->fBeginTimeScale, hsInfo->fEndTimeScale));
		SetGS("fSpaceshipMaxAngularVelocityScale", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->fBeginAngularVelocityScale, hsInfo->fEndAngularVelocityScale));
		Sleep(TimePerFrameAnimation);
	}
	return 0;
}

State::State(int idx, int mfs, int fsm, int bs, float fov, float timeScale = 1, float angularVelocityScale = 1.5)
{
	iIndex = idx;
	iMaxForwardSpeed = mfs;
	iForwardSpeedMult = fsm;
	iBoostSpeed = bs;
	fFov = fov;
	fTimeScale = timeScale;
	fAngularVelocityScale = angularVelocityScale;
}

void State::Enter()
{
	SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipenginepartmaxforwardspeed", iMaxForwardSpeed);
	//SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipenginepartmaxbackwardspeed", iMaxForwardSpeed / 2);
	SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipforwardspeedmult", iForwardSpeedMult);
	SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipboostspeed", iBoostSpeed);
	SetGS("fFlightCameraFOV:FlightCamera", fFov);
	CallCommand("sgtm", fTimeScale);
	SetGS("fSpaceshipMaxAngularVelocityScale", fAngularVelocityScale);
	StateMachine::GetInstance().iCurrentState = iIndex;
}

void State::Exit(State& target)
{
	bool      bSpeedUp = target.iIndex > iIndex;
	const int iAnimationTime = bSpeedUp ? 1000 : 500;
	/* if (StateMachine::GetInstance().bNeedShutDown && iIndex == 5 && !bSpeedUp) {
		SFSE::log::info("FTLShutDown", target.iIndex);
		StateMachine::GetInstance().bNeedShutDown = false;
		StateMachine::GetInstance().FTLShutDown();
		return;
	}*/
	StateMachine::GetInstance().bLock = 1;
	SFSE::log::info("Level change to {}", target.iIndex);
	// Logger->Print(fmt::format("State {} -> {}", iIndex, target.iIndex).c_str(), 0);
	AnimationInfo* hsInfo = new AnimationInfo{ &target,
		iAnimationTime / TimePerFrameAnimation,
		iMaxForwardSpeed, target.iMaxForwardSpeed,
		iForwardSpeedMult, target.iForwardSpeedMult,
		iBoostSpeed, target.iBoostSpeed,
		fFov, target.fFov,
		fTimeScale, target.fTimeScale,
		fAngularVelocityScale, target.fAngularVelocityScale };
	CreateThread(NULL, 0, Animation, hsInfo, 0, NULL);
}

StateMachine::StateMachine()
{
	vStateList.push_back(State(0, 300, 300, 4, 90));
	vStateList.push_back(State(1, 10000, 10000, 15, 92));
	vStateList.push_back(State(2, 300000, 1000000, 15, 95));
	vStateList.push_back(State(3, 4000000, 10000000, 150, 100));
	vStateList.push_back(State(4, 50000000, 30000000, 1500, 105));                  // 0.1c
	vStateList.push_back(State(5, 300000000, 300000000, 1500, 125));                // 1c
	vStateList.push_back(State(6, 450000000, 300000000, 1500, 130, 3, 0.1));        // x3
	vStateList.push_back(State(7, 600000000, 300000000, 1500, 132, 50, 0.001));     // x50
	vStateList.push_back(State(8, 1500000000, 300000000, 1500, 135, 100, 0.0001));  // x100
	iCurrentState = 0;
	iCurrentShip = 0;
	bLock = 0;
	bNeedShutDown = 0;
	bShutingDown = 0;
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
	vStateList[0].Enter();
}

void StateMachine::ShutdownUpdate()
{
	const int iAnimationTime = 100;
	auto&     current = StateMachine::GetInstance().GetCurrentState();

	bLock = 1;
	bShutingDown = 1;
	// ExecuteCommand(0, std::string("sgtm 0.25").data());
	SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipenginepartmaxforwardspeed", 300);
	SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipforwardspeedmult", 300000000);
	// SetGS("fFlightCameraFOV:FlightCamera", 80);
	
	AnimationInfo* hsInfo = new AnimationInfo{ NULL,
		iAnimationTime / TimePerFrameAnimation,
		0, 0,
		0, 0,
		0, 0,
		current.fFov, 80,
		current.fTimeScale, 0.25,
		current.fAngularVelocityScale, current.iIndex <= 4 ? 10 : current.fAngularVelocityScale };
	CreateThread(NULL, 0, AnimationShutdown, hsInfo, 0, NULL);
}

void StateMachine::ShutdownExit()
{
	const int iAnimationTime = 100;
	auto&     current = StateMachine::GetInstance().GetCurrentState();
	bLock = 0;
	bShutingDown = 0;
	AnimationInfo* hsInfo = new AnimationInfo{ NULL,
		iAnimationTime / TimePerFrameAnimation,
		0, 0,
		300, current.iForwardSpeedMult,
		0, 0,
		80, current.fFov,
		0.25, current.fTimeScale,
		10, current.fAngularVelocityScale };
	CreateThread(NULL, 0, AnimationShutdown, hsInfo, 0, NULL);
	GetCurrentState().Enter();
}

void StateMachine::FTLShutDown()
{
	ExecuteCommand(0, std::string("QuickSave").data());
	vStateList[0].Enter();
	ExecuteCommand(0, std::string("sgtm 0.1").data());
	bLock = 1;
	Sleep(5000);
	ExecuteCommand(0, std::string("sgtm 1").data());
	bLock = 0;
	ExecuteCommand(0, std::string("QuickLoad").data());
}

static DWORD MainLoop(void* unused)
{
	(void)unused;
	bool upHoldFlag = false;
	bool downHoldFlag = false;
	for (;;) {
		/* int up = SFSE::WinAPI::GetKeyState(33);
		int   down = SFSE::WinAPI::GetKeyState(34);
		int   stop = SFSE::WinAPI::GetKeyState(46);*/
		short up = SFSE::WinAPI::GetKeyState(SFSE::WinAPI::VKEnum::VK_1);
		short down = SFSE::WinAPI::GetKeyState(SFSE::WinAPI::VKEnum::VK_2);
		short ctrl = SFSE::WinAPI::GetKeyState(VK_LCONTROL);
		short stop = SFSE::WinAPI::GetKeyState(SFSE::WinAPI::VKEnum::VK_3);
		auto* player = RE::Actor::PlayerCharacter();
		if (player) {
			RE::TESObjectREFR* ship = player->GetAttachedSpaceship();
			if (ship) {
				if (StateMachine::GetInstance().iCurrentShip == 0) {
					StateMachine::GetInstance().RegisterShip(ship->formID);
				}
			} else {
				if (StateMachine::GetInstance().iCurrentShip != 0) {
					StateMachine::GetInstance().vStateList[0].Enter();
					StateMachine::GetInstance().iCurrentShip = 0;
				}
			}
		}
		if (StateMachine::GetInstance().iCurrentShip != 0) {
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
			if (stop < 0) {
				static int iCount = 0;
				if (ctrl < 0) {
					if (iCount++ > 2000 / TimePerFrame) {
						iCount = 0;
						StateMachine::GetInstance().FTLShutDown();
					}
				} else {
					iCount = 0;
					if (!StateMachine::GetInstance().bShutingDown)
						StateMachine::GetInstance().ShutdownUpdate();
				}
			} else {
				if (StateMachine::GetInstance().bShutingDown) {
					// Exit
					StateMachine::GetInstance().ShutdownExit();
				}
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
