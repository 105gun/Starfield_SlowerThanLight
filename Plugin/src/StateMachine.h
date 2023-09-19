

class State
{
public:
	int iIndex;
	int iMaxForwardSpeed;
	int iForwardSpeedMult;
	int iBoostSpeed;
	int iFov;
	State(int idx, int mfs, int fsm, int bs, int fov);

	void Enter();

	void Exit(State& target);
};

class StateMachine
{
public:
	bool bLock;
	enum class InputType
	{
		SpeedUp,
		SpeedDown,
	};

	static StateMachine& GetInstance()
	{
		static StateMachine singleton;
		return singleton;
	}

	unsigned int       iCurrentShip = 0;
	int                iCurrentState = 0;
	std::vector<State> vStateList;

	StateMachine();

	State& GetCurrentState();

	void ChangeStateMachine(InputType type);
	void RegisterShip(int ship);
};

struct AnimationInfo
{
	State& target;
	int    iTotalFrames;
	int    iBeginMFS;
	int    iEndMFS;
	int    iBeginFSM;
	int    iEndFSM;
	int    iBeginBS;
	int    iEndBS;
	int    iBeginFOV;
	int    iEndFOV;
};