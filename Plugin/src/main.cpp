#include "DKUtil/Hook.hpp"

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

std::string n2hexstr(int w, size_t hex_len = sizeof(int)<<1) {
    static const char* digits = "0123456789ABCDEF";
    std::string rc(hex_len,'0');
    for (size_t i=0, j=(hex_len-1)*4 ; i<hex_len; ++i,j-=4)
        rc[i] = digits[(w>>j) & 0x0f];
    return rc;
}

static REL::Relocation<__int64 (*)(double, char*, ...)> ExecuteCommand{ REL::Offset(0x287DF04) };
int crtShip = 0;

static DWORD MainLoop(void* unused)
{
    (void)unused;
    for (;;)
    {
		int up = SFSE::WinAPI::GetKeyState(33);
		int down = SFSE::WinAPI::GetKeyState(34);
		auto* player = RE::Actor::PlayerCharacter();
		if (player){
			RE::TESObjectREFR* ship = player->GetAttachedSpaceship();
			if (ship){
				SFSE::log::info("CRT: {} {}", n2hexstr(ship->formID), std::to_string(down));
				crtShip = ship->formID;
			}
			else {
				crtShip = 0;
			}

		}
		if(crtShip) {
			std::string prefix = n2hexstr(crtShip);
			if (up < 0) {
				char* string1 = (prefix + std::string(".setav spaceshipenginepartmaxforwardspeed 10000")).data();
				char* string2 = (prefix + std::string(".setav spaceshipforwardspeedmult 10000")).data();
				char* string3 = (prefix + std::string(".setav spaceshipboostspeed 1500")).data();
				ExecuteCommand(0, string1);
				ExecuteCommand(0, string2);
				ExecuteCommand(0, string3);
			} else if (down < 0) {
				char* string1 = (prefix + std::string(".setav spaceshipenginepartmaxforwardspeed 300")).data();
				char* string2 = (prefix + std::string(".setav spaceshipforwardspeedmult 300")).data();
				char* string3 = (prefix + std::string(".setav spaceshipboostspeed 4")).data();
				ExecuteCommand(0, string1);
				ExecuteCommand(0, string2);
				ExecuteCommand(0, string3);
			}
		}
        Sleep(100);
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
				CreateThread(NULL, 4096, MainLoop, NULL, 0, NULL);
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
