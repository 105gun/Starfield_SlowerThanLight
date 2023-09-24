//#include <StateMachine.h>
//#include<ConsoleMonitor.h>

class SpaceShipHistory
{
public:
	// TODO: add a lock?
	struct SpaceShipHistoryItem
	{
		int   target;
		std::string name;
		float MFS;
		float FSM;
	};
	std::vector<SpaceShipHistoryItem> list;

	static SpaceShipHistory& GetInstance()
	{
		static SpaceShipHistory singleton;
		return singleton;
	}

	SpaceShipHistoryItem* Find(int formID)
	{
		for (int i = 0; i < list.size(); i++) {
			if (list[i].target == formID) {
				return &list[i];
			}
		}
		return 0;
	}
	void Write(int formID, std::string name, float MFS, float FSM)
	{
		auto item = Find(formID);
		if (item) {
			item->FSM = FSM;
			item->MFS = MFS;
		} else {
			list.push_back({ formID, name, MFS, FSM });
		}
		UpdateFile();
	}
	void ConsoleMonitorCallBack(int formID, std::string name, float MFS, float FSM)
	{
		Write(formID, name, MFS, FSM);
		StateMachine::GetInstance().vStateList[0].iForwardSpeedMult = FSM;
		StateMachine::GetInstance().vStateList[0].iMaxForwardSpeed = MFS;
		StateMachine::GetInstance().vStateList[0].Enter();
		StateMachine::GetInstance().bLock = 0;
	}

	void GetAndSet(int formID)
	{
		 StateMachine::GetInstance().bLock = 1;
		auto item = Find(formID);
		 if (item) {
			StateMachine::GetInstance().vStateList[0].iForwardSpeedMult = item->FSM;
			StateMachine::GetInstance().vStateList[0].iMaxForwardSpeed = item->MFS;
			StateMachine::GetInstance().vStateList[0].Enter();
			StateMachine::GetInstance().bLock = 0;
		} else {
			//ConsoleMonitor::callback = &SpaceShipHistory::ConsoleMonitorCallBack;
			//ConsoleMonitor::target = formID;
		}
	}
	void UpdateFile()
	{
		std::string   logFile = "Data\\SFSE\\plugins\\STL_SpaceShipHistory";
		std::ofstream out(logFile, std::ios::out);
		for (auto& item : list) {
			out << item.target << ' ' << item.name << ' ' << item.MFS << ' ' << item.FSM << std::endl;
		}
		out.close();
	}
	void ReadFromFile()
	{
		// It's not working since formID changes when game restart
		/* std::string logFile = "Data\\SFSE\\plugins\\STL_SpaceShipHistory";
		std::ifstream in(logFile, std::ios::in);
		std::string   line;
		while (getline(in, line)) {
			std::stringstream ss(line);
			int               _target;
			std::string       _name;
			float             _MFS, _WSM;
			ss >> _target >> _name >> _MFS >> _WSM;
			// list.push_back({ _target, _name, _MFS, _WSM });
		}
		in.close();*/
	}
};