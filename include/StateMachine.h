//sgn
#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>
//	#include "Protocol.h" // Circular Dependency, so commented
#include "Constants.h"
#include "FileLogger.h"

using namespace util;
class Protocol;
enum class States : unsigned char { Idle = 0, ServerPrecommit, ServerCommit, OpPerformed, ClientPrecommit, ClientCommit };

class StateMachine
{
public:
	static StateMachine* getInstance(States pState, std::shared_ptr<Protocol> pProtocol);
	~StateMachine() {}

	//	State transition calls
	virtual StateMachine* onWriteRequest() { return nullptr; }
	virtual StateMachine* onNegativeAckOrTimeout() { return nullptr; }
	virtual StateMachine* onAllAck() { return nullptr; }
	virtual StateMachine* onSuccess() { return nullptr; }	// Peer to Server
	virtual StateMachine* onUnsuccess() { return nullptr; }	// Peer to Server
	virtual StateMachine* onPrecommit(const std::string &pHost, unsigned int pPort) { return nullptr; }
	virtual StateMachine* onCommit(const std::string& pHost, unsigned int pPort) { return nullptr; }
	virtual StateMachine* onAbort(const std::string& pHost, unsigned int pPort) { return nullptr; }
	virtual StateMachine* onSuccessful(const std::string& pHost, unsigned int pPort) { return nullptr; }	// Server to Peer
	virtual StateMachine* onUnsuccessful(const std::string& pHost, unsigned int pPort) { return nullptr; }	// Server to Peer

	virtual States getStateEnum() = 0;
	static std::string getStateName(States pState);

protected:
	std::shared_ptr<Protocol> mpProtocol;
	Logger& mLogger;
	StateMachine(std::shared_ptr<Protocol> pProtocol) : mpProtocol{ pProtocol }, mLogger{ Logger::getInstance() } {}
};

//============================================= All State Classes ====================================

class IdleState : public StateMachine, public std::enable_shared_from_this<IdleState> {
public:
	StateMachine* onWriteRequest();
	StateMachine* onPrecommit(const std::string& pHost, unsigned int pPort);
	States getStateEnum() { return States::Idle; }

	virtual ~IdleState() {}
	static StateMachine* getInstance(std::shared_ptr<Protocol> pProtocol) {
		if (!pThis) pThis = new IdleState(pProtocol);
		return pThis;
	}

private:
	IdleState(std::shared_ptr<Protocol> pProtocol) : StateMachine(pProtocol) {}
	static IdleState* pThis;
};

class ServerPrecommitState : public StateMachine, public std::enable_shared_from_this<ServerPrecommitState> {
public:
	typedef std::shared_ptr<ServerPrecommitState> Ptr;
	StateMachine* onNegativeAckOrTimeout();
	StateMachine* onAllAck();
	States getStateEnum() { return States::ServerPrecommit; }

	virtual ~ServerPrecommitState() {}

	static StateMachine* getInstance(std::shared_ptr<Protocol> pProtocol) {
		if (!pThis) pThis = new ServerPrecommitState(pProtocol);
		return pThis;
	}

private:
	ServerPrecommitState(std::shared_ptr<Protocol> pProtocol) : StateMachine(pProtocol) {}
	static ServerPrecommitState* pThis;
};


class ServerCommitState : public StateMachine, public std::enable_shared_from_this<ServerCommitState> {
public:
	typedef std::shared_ptr<ServerCommitState> Ptr;
	StateMachine* onNegativeAckOrTimeout() { return onFailure(); }
	StateMachine* onUnsuccess() { return onFailure(); }
	StateMachine* onSuccess();
	States getStateEnum() { return States::ServerCommit; }

	static StateMachine* getInstance(std::shared_ptr<Protocol> pProtocol) {
		if (!pThis) pThis = new ServerCommitState(pProtocol);
		return pThis;
	}
	virtual ~ServerCommitState() {}

private:
	ServerCommitState(std::shared_ptr<Protocol> pProtocol)
		: StateMachine(pProtocol)
	{}
	StateMachine* onAllSuccess();
	StateMachine* onFailure();
	static ServerCommitState* pThis;
};

class ClientPrecommitState : public StateMachine, public std::enable_shared_from_this<ClientPrecommitState> {
public:
	typedef std::shared_ptr<ClientPrecommitState> Ptr;
	StateMachine* onNegativeAckOrTimeout();
	StateMachine* onCommit(const std::string& pHost, unsigned int pPort);
	StateMachine* onAbort(const std::string& pHost, unsigned int pPort);

	States getStateEnum() { return States::ClientPrecommit; }

	virtual ~ClientPrecommitState() {}

	static StateMachine* getInstance(std::shared_ptr<Protocol> pProtocol) {
		if (!pThis) pThis = new ClientPrecommitState(pProtocol);
		return pThis;
	}

private:
	ClientPrecommitState(std::shared_ptr<Protocol> pProtocol) : StateMachine(pProtocol) {}
	static ClientPrecommitState* pThis;
};

class ClientCommitState : public StateMachine, public std::enable_shared_from_this<ClientCommitState> {
public:
	typedef std::shared_ptr<ClientCommitState> Ptr;
	StateMachine* onNegativeAckOrTimeout();
	StateMachine* onSuccessful(const std::string& pHost, unsigned int pPort);
	StateMachine* onUnsuccessful(const std::string& pHost, unsigned int pPort);
	States getStateEnum() { return States::ClientCommit; }

	virtual ~ClientCommitState() {}

	static StateMachine* getInstance(std::shared_ptr<Protocol> pProtocol) {
		if (!pThis) pThis = new ClientCommitState(pProtocol);
		return pThis;
	}

private:
	ClientCommitState(std::shared_ptr<Protocol> pProtocol) : StateMachine(pProtocol) {}
	static ClientCommitState* pThis;
};


/*class OpPerformedState : public StateMachine, public std::enable_shared_from_this<OpPerformedState> {
public:
	StateMachine* onOpSuccess();
	StateMachine* onOpFailed();
	StateMachine* onNegativeAckOrTimeout();
	StateMachine* onUnsuccessful();
	States getStateEnum() { return States::OpPerformed; }

	virtual ~OpPerformedState() {}

	static StateMachine* getInstance(std::shared_ptr<Protocol> pProtocol) {
		if (!pThis) pThis = new OpPerformedState(pProtocol);
		return pThis;
	}

private:
	OpPerformedState(std::shared_ptr<Protocol> pProtocol) : StateMachine(pProtocol) {}
	static OpPerformedState* pThis;
};*/