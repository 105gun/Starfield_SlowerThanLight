int target;
float MFS;
float FSM;
float SM;

	static DWORD ConsoleMonitorThread(void* unused)
	{
		(void)unused;
		std::string   logFile = "Data\\SFSE\\plugins\\sfse_plugin_console.log";
		std::ifstream in(logFile, std::ios::in);

		long long   lastSize = 0;
		std::string line;

		target = 0;
		MFS = FSM = SM = 0;

		while (true) {
			in.open(logFile, std::ios::in);
			in.seekg(0, std::ios::end);
			long long fileSize = in.tellg();
			if (fileSize > 0) {
				in.close();
				break;
			}
			in.close();
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
		SFSE::log::info("ConsoleMonitorThread Start");

		in.open(logFile, std::ios::in);
		while (true) {
			if (in.peek() == EOF) {
				in.clear();
				in.seekg(lastSize, std::ios::beg);
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}

			getline(in, line);
			//SFSE::log::info("Return value: {}", line);
			lastSize = in.tellg();

			if (target && line.find("GetActorValue: SpaceshipEnginePartMaxForwardSpeed") != std::string::npos) {
				size_t      start = line.find(">>");
				std::string _value = line.substr(start + 2);
				MFS = std::stof(_value);
			}

			if (target && line.find("GetActorValue: SpaceshipForwardSpeedMult") != std::string::npos) {
				size_t      start = line.find(">>");
				std::string _value = line.substr(start + 2);
				FSM = std::stof(_value);
			}

			if (target && line.find("GetActorValue: SpeedMult") != std::string::npos) {
				size_t      start = line.find(">>");
				std::string _value = line.substr(start + 2);
				SM = std::stof(_value);
				// SFSE::log::info("Return value: {} {}", _value, SM);
			}

			if (target && MFS != 0 && FSM != 0) {
				/* SFSE::log::info("Register {} {} {}", target);
				//TODO
				//SpaceShipHistory::ConsoleMonitorCallBack(target,MFS,FSM);
				target = 0;
				MFS = 0;
				FSM = 0;*/
			}
		}

		in.close();
		SFSE::log::info("ConsoleMonitorThread Exit");

		return 0;
	}
