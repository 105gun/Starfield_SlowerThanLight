#include "DKUtil/Hook.hpp"
#include "StateMachine.h"
#include <algorithm>
#include <time.h>
#include <Config.h>
#include <History.h>
#include <ConsoleMonitor.h>

// GetPlayerHomeSpaceShip
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

static REL::Relocation<__int64 (*)(double, char*, ...)> ExecuteCommand{ REL::ID(166307) };  // From Console-Command-Runner-SF
auto*                                                   Logger = RE::ConsoleLog::GetSingleton();
const int                                               TimePerFrame = 50;
const int                                               TimePerFrameAnimation = 16;
bool                                                    SHUTDOWN_FLAG = 0;
std::queue<std::string>                                 PrintBuffer;
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

void GetActorValue(int formID, std::string name)
{
	std::string result = fmt::format("{}.getav {}", n2hexstr(formID), name);
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

void PrintInfo(std::string info)
{
	PrintBuffer.push(info);
}

void PrintInfoUpdte()
{
	if (PrintBuffer.empty())
		return;
	std::string targetString = PrintBuffer.front();
	PrintBuffer.pop();
	std::string result = fmt::format("cgf \"Debug.Notification\" \"{}\"", targetString);
	SFSE::log::info("{}", targetString);
	ExecuteCommand(0, result.data());
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
		//SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipboostspeed", Interpolation(hsInfo->iTotalFrames, frame, hsInfo->iBeginBS, hsInfo->iEndBS));
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
	PrintInfo(fmt::format("Gear {}", iIndex));
	SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipenginepartmaxforwardspeed", iMaxForwardSpeed);
	//SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipenginepartmaxbackwardspeed", iMaxForwardSpeed / 2);
	SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipforwardspeedmult", iForwardSpeedMult);
	//SetActorValue(StateMachine::GetInstance().iCurrentShip, "spaceshipboostspeed", iBoostSpeed);
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


class hkQuicksave // From BakaQuickFullSaves
{
public:
	static void Install()
	{
		static REL::Relocation<std::uintptr_t> target{ REL::Offset(0x023B634C), 0xCF };
		auto&                                  trampoline = SFSE::GetTrampoline();
		_Quicksave = trampoline.write_call<5>(target.address(), Quicksave);
	}

private:
	static void Quicksave(void* a_this)
	{
		_Quicksave(a_this);
		// SFSE::log::info("Quicksave! {}", a_this);
		if (SHUTDOWN_FLAG) {
			SHUTDOWN_FLAG = 0;
			ExecuteCommand(0, std::string("sgtm 1").data());
			// StateMachine::GetCurrentState().bLock = 0;
			LoadMostRecent();
		}
	}

	static bool LoadMostRecent()
	{
		using func_t = decltype(&LoadMostRecent);
		static REL::Relocation<func_t> func{ REL::Offset(0x023A7040) };
		return func();
	}

	inline static REL::Relocation<decltype(&Quicksave)> _Quicksave;
};

StateMachine::StateMachine()
{
	vStateList.push_back(State(0, 300, 300, 4, 90));
	vStateList.push_back(State(1, 5000, 5000, 15, 92));
	vStateList.push_back(State(2, 100000, 500000, 15, 95));
	vStateList.push_back(State(3, 2000000, 10000000, 150, 100));
	vStateList.push_back(State(4, 50000000, 30000000, 1500, 105));                  // 0.1c
	vStateList.push_back(State(5, 300000000, 300000000, 1500, 125));                // 1c
	vStateList.push_back(State(6, 450000000, 300000000, 1500, 130, 3, 0.1));        // x3
	vStateList.push_back(State(7, 1500000000, 300000000, 1500, 132, 50, 0.001));     // x50
	vStateList.push_back(State(8, 1500000000, 300000000, 1500, 135, 75, 0.0001));  // x75
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
			GetCurrentState().Exit(vStateList[vStateList.size() - 1]);
		} else {
			GetCurrentState().Exit(vStateList[iCurrentState + 1]);
		}
	}
	if (type == InputType::SpeedDown) {
		if (iCurrentState <= 0) {
			GetCurrentState().Exit(vStateList[0]);
		} else {
			GetCurrentState().Exit(vStateList[iCurrentState - 1]);
		}
	}
}

void StateMachine::RegisterShip(RE::TESObjectREFR* ship)
{
	PrintInfo(fmt::format("Registering {}", ship->GetDisplayFullName()));
	iCurrentShip = ship->formID;
	pCurrentShip = ship;
	// TODO: Get origin value here
	// SpaceShipHistory::GetInstance().GetAndSet(iCurrentShip);
	bLock = 1;
	target = iCurrentShip;
	GetActorValue(iCurrentShip, "SpaceshipEnginePartMaxForwardSpeed");
	GetActorValue(iCurrentShip, "SpaceshipForwardSpeedMult");
	GetActorValue(iCurrentShip, "SpeedMult");
	Sleep(2000);
	if (MFS != 0 && FSM != 0 && MFS < 300) {
		PrintInfo(fmt::format("Get {}: {}-{}", ship->GetDisplayFullName(), FSM, MFS));
		vStateList[0].iForwardSpeedMult = FSM;
		vStateList[0].iMaxForwardSpeed = MFS;
		vStateList[0].Enter();
		SetActorValue(iCurrentShip, "SpeedMult", 1024*MFS + FSM);
		SpaceShipHistory::GetInstance().Write(iCurrentShip, ship->GetDisplayFullName(), MFS, FSM);
	} else {
		if (SM > 1024) {
			float MFSFromSave = int(SM / 1024);
			float FSMFromSave = SM - MFSFromSave * 1024;
			PrintInfo(fmt::format("Get from save {}: {}-{}", ship->GetDisplayFullName(), FSMFromSave, MFSFromSave));
			vStateList[0].iForwardSpeedMult = FSMFromSave;
			vStateList[0].iMaxForwardSpeed = MFSFromSave;
			vStateList[0].Enter();
		} else {
			PrintInfo(fmt::format("Get {} Fail: Use default values", ship->GetDisplayFullName()));
			vStateList[0].iForwardSpeedMult = *Config::General::fSpaceshipForwardSpeedMult;
			vStateList[0].iMaxForwardSpeed = *Config::General::fSpaceshipEnginePartMaxForwardSpeed;
			vStateList[0].Enter();
		}
	}
	target = 0;
	FSM = MFS = SM = 0;
	bLock = 0;
}

void StateMachine::ExitShip()
{
	PrintInfo(fmt::format("Exiting {}", pCurrentShip->GetDisplayFullName()));
	iCurrentShip = 0;
	pCurrentShip = 0;
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
	if (*Config::General::iReloadTime > 0) {
		ExecuteCommand(0, std::string("QuickSave").data());
		vStateList[0].Enter();
		ExecuteCommand(0, std::string("sgtm 0.1").data());
		bLock = 1;
		Sleep(*Config::General::iReloadTime);
		ExecuteCommand(0, std::string("sgtm 1").data());
		bLock = 0;
		ExecuteCommand(0, std::string("QuickLoad").data());
	} else {
		SHUTDOWN_FLAG = 1;
		vStateList[0].Enter();
		ExecuteCommand(0, std::string("sgtm 0.1").data());
		//bLock = 1;
		ExecuteCommand(0, std::string("QuickSave").data());
	}
}



void ExtraTest(RE::TESObjectREFR* ship)
{
	;
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
		auto* player = RE::Actor::PlayerCharacter();
		//SFSE::log::info("WTF2 {}", (int)player);
		if (player) {
			RE::TESObjectREFR* ship = player->GetSpaceship();
			SFSE::log::info("WTF2 {}", (int)ship);
			//RE::TESObjectREFR* ship = 0;
			if (ship) {
				// On some ship or station
				// SFSE::log::info("CRT_SHIP {} {} {} {}", n2hexstr(ship->formID), RE::PlayerCamera::GetSingleton()->IsInFirstPerson(), RE::PlayerCamera::GetSingleton()->IsInThirdPerson(), ship->GetActorValue(*RE::ActorValue::GetSingleton()->spaceshipCustomized));
				if (StateMachine::GetInstance().pCurrentShip != ship) {
					// Get into a ship not in statemachine
					if (RE::PlayerCamera::GetSingleton()->QCameraEquals(RE::CameraState::kFlightCamera)) {
						// On new ship's pilot seat
						StateMachine::GetInstance().RegisterShip(ship);
					} else if (StateMachine::GetInstance().iCurrentShip != 0) {
						// Exit the old one
						StateMachine::GetInstance().ExitShip();
					}
				} else {
					// On last ship
					// Do nothing				
				}
			} else {
				// Not on a ship
				if (StateMachine::GetInstance().iCurrentShip != 0) {
					StateMachine::GetInstance().ExitShip();
				}
			}
			if (StateMachine::GetInstance().iCurrentShip != 0) {
				// SFSE::log::info("CRT: up{} down{} uf{} df{}", std::to_string(up), std::to_string(down), std::to_string(upHoldFlag), std::to_string(downHoldFlag));
				if (upHoldFlag && !HotkeyUp.IsPressed()) {
					upHoldFlag = 0;
				}
				if (downHoldFlag && !HotkeyDown.IsPressed()) {
					downHoldFlag = 0;
				}
				if (HotkeyUp.IsPressed() && !upHoldFlag) {
					SFSE::log::info("CRT: up");
					upHoldFlag = 1;
					StateMachine::GetInstance().ChangeStateMachine(StateMachine::InputType::SpeedUp);
				} else if (HotkeyDown.IsPressed() && !downHoldFlag) {
					SFSE::log::info("CRT: down");
					downHoldFlag = 1;
					StateMachine::GetInstance().ChangeStateMachine(StateMachine::InputType::SpeedDown);
				}
				if (HotkeySlowMotion.IsPressed()) {
					/* static int iCount = 0;
					if (ctrl < 0) {
						if (iCount++ > 2000 / TimePerFrame) {
							iCount = 0;
							StateMachine::GetInstance().FTLShutDown();
						}
					} else {
						iCount = 0;
						if (!StateMachine::GetInstance().bShutingDown)
							StateMachine::GetInstance().ShutdownUpdate();
					}*/
					if (!StateMachine::GetInstance().bShutingDown)
						StateMachine::GetInstance().ShutdownUpdate();
				} else {
					if (StateMachine::GetInstance().bShutingDown) {
						// Exit
						StateMachine::GetInstance().ShutdownExit();
					}
				}
				static int iCount = 0;
				if (HotkeyReload.IsPressed()) {
					if (iCount++ > 2000 / TimePerFrame) {
						iCount = 0;
						StateMachine::GetInstance().FTLShutDown();
					}
				} else {
					iCount = 0;
				}
			}
			PrintInfoUpdte();
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
				hkQuicksave::Install();
				Config::Load();
				//SpaceShipHistory::GetInstance().ReadFromFile();
				CreateThread(NULL, 0, MainLoop, NULL, 0, NULL);
				CreateThread(NULL, 0, ConsoleMonitorThread, NULL, 0, NULL);
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
