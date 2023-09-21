

class State
{
public:
	int iIndex;
	int iMaxForwardSpeed;
	int iForwardSpeedMult;
	int iBoostSpeed;
	float fFov;
	float fTimeScale;
	float fAngularVelocityScale;

	State(int idx, int mfs, int fsm, int bs, float fov, float timeScale, float angularVelocityScale);

	void Enter();

	void Exit(State& target);
};

class StateMachine
{
public:
	bool bLock;
	bool bShutingDown;
	bool bNeedShutDown;
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
	void FTLShutDown();
	void ShutdownUpdate();
	void ShutdownExit();
};

struct AnimationInfo
{
	State* target;
	int    iTotalFrames;
	int    iBeginMFS;
	int    iEndMFS;
	int    iBeginFSM;
	int    iEndFSM;
	int    iBeginBS;
	int    iEndBS;
	float    fBeginFOV;
	float    fEndFOV;
	float    fBeginTimeScale;
	float    fEndTimeScale;
	float    fBeginAngularVelocityScale;
	float    fEndAngularVelocityScale;
};