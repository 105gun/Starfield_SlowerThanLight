class HotkeyItem
{
public:
	std::vector<int> vHotkeyList;
	std::string      sName;

	bool IsPressed() {
		if (vHotkeyList.size() == 0)
			return false;
		for (auto& i : vHotkeyList) {
			if (SFSE::WinAPI::GetKeyState(i) >= 0) {
				return false;
			}
		}
		return true;
	}

	void Register(std::string name, std::string key, std::string defaultKey) {
		sName = name;
		_Register(key);
		if (vHotkeyList.size() == 0) {
			SFSE::log::info("Warning! Hotkey {} set fail. Using {}", key, defaultKey);
			_Register(defaultKey);
		}
		std::string logBuffer = fmt::format("Hotkey {}:", sName);
		for (auto& i : vHotkeyList) {
			logBuffer += fmt::format(" {}", i);
		}
		SFSE::log::info("{}", logBuffer.data());
	}

	void _Register(std::string key) {
		std::stringstream ss(key);
		int               i;
		while (ss >> i) {
			vHotkeyList.push_back(i);
			if (ss.peek() == '&') {
				ss.ignore();
			}
		}
	}
};

static HotkeyItem HotkeyUp;
static HotkeyItem HotkeyDown;
static HotkeyItem HotkeySlowMotion;
static HotkeyItem HotkeyReload;

class Config
{
public:
	class General
	{
	public:
		inline static DKUtil::Alias::Integer iReloadTime{ "iReloadTime", "General" };
		inline static DKUtil::Alias::String  sHotkeyUp{ "sHotkeyUp", "General" };
		inline static DKUtil::Alias::String  sHotkeyDown{ "sHotkeyDown", "General" };
		inline static DKUtil::Alias::String  sHotkeySlowMotion{ "sHotkeySlowMotion", "General" };
		inline static DKUtil::Alias::String  sHotkeyReload{ "sHotkeyReload", "General" };
		inline static DKUtil::Alias::Double  fSpaceshipEnginePartMaxForwardSpeed{ "fSpaceshipEnginePartMaxForwardSpeed", "General" };
		inline static DKUtil::Alias::Double  fSpaceshipForwardSpeedMult{ "fSpaceshipForwardSpeedMult", "General" };
	};

	static void Load()
	{
		static auto MainConfig = COMPILE_PROXY("SlowerThanLight.ini");
		MainConfig.Bind(General::iReloadTime, 0);
		MainConfig.Bind(General::sHotkeyUp, "49");
		MainConfig.Bind(General::sHotkeyDown, "50");
		MainConfig.Bind(General::sHotkeySlowMotion, "51");
		MainConfig.Bind(General::sHotkeyReload, "51&162");
		MainConfig.Bind(General::fSpaceshipEnginePartMaxForwardSpeed, 150);
		MainConfig.Bind(General::fSpaceshipForwardSpeedMult, 35);
		MainConfig.Load();
		HotkeyUp.Register("Up", *General::sHotkeyUp, "49");
		HotkeyDown.Register("Down", *General::sHotkeyDown, "50");
		HotkeySlowMotion.Register("SlowMotion", *General::sHotkeySlowMotion, "51");
		HotkeyReload.Register("Reload", *General::sHotkeyReload, "51&162");
	}
};