//sgn
#include <iostream>
#include <thread>


#include "Protocol.h"
#include "Constants.h"
#include "StateMachine.h"

IdleState* IdleState::pThis = nullptr;
ServerPrecommitState* ServerPrecommitState::pThis = nullptr;
ServerCommitState* ServerCommitState::pThis = nullptr;
//OpPerformedState* OpPerformedState::pThis = nullptr;
ClientPrecommitState* ClientPrecommitState::pThis = nullptr;
ClientCommitState* ClientCommitState::pThis = nullptr;

StateMachine* StateMachine::getInstance(States pState, std::shared_ptr<Protocol> pProtocol) {
	switch (pState) {
	case States::ServerPrecommit:
		return ServerPrecommitState::getInstance(pProtocol);

	case States::ServerCommit:
		return ServerCommitState::getInstance(pProtocol);

	//case States::OpPerformed:
	//	return OpPerformedState::getInstance(pProtocol);

	case States::ClientPrecommit:
		return ClientPrecommitState::getInstance(pProtocol);

	case States::ClientCommit:
		return ClientCommitState::getInstance(pProtocol);

	case States::Idle:
	default:
		return IdleState::getInstance(pProtocol);
	}
	return nullptr;
}

StateMachine* IdleState::onWriteRequest() {
	mpProtocol->grabWriteLock();
	mpProtocol->broadcastMessage(PRECOMMIT);
	mpProtocol->addToTimer();
	return ServerPrecommitState::getInstance(mpProtocol);
}

StateMachine* IdleState::onPrecommit(const std::string& pHost, unsigned int pPort) {
	mpProtocol->grabWriteLock();
	mpProtocol->addToTimer();
	mpProtocol->sendMessage(pHost, pPort, POSITIVE_ACK);
	return ClientPrecommitState::getInstance(mpProtocol);
}

StateMachine* ServerPrecommitState::onNegativeAckOrTimeout() {
	mpProtocol->broadcastMessage(ABORT);
	mpProtocol->sendWriteResponse("ERROR WRITE Peers negative ack or timedout");
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ServerPrecommitState::onAllAck() {
	mpProtocol->removeActiveTimer();
	mpProtocol->broadcastMessage(COMMIT);
	mpProtocol->addToTimer();
	return ServerCommitState::getInstance(mpProtocol);;
}

//	All peers responded success
StateMachine* ServerCommitState::onSuccess() {
	auto pNewState = mpProtocol->writeMsg() ? onAllSuccess() : onFailure();
	return pNewState;
}

StateMachine* ServerCommitState::onAllSuccess() {
	mpProtocol->removeActiveTimer();
	mpProtocol->broadcastMessage(SUCCESSFUL);
	mpProtocol->sendWriteResponse("WROTE");
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ServerCommitState::onFailure() {
	mpProtocol->removeActiveTimer();
	mpProtocol->broadcastMessage(UNSUCCESSFUL);
	mpProtocol->sendWriteResponse("ERROR WRITE Peers write failed");
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ClientPrecommitState::onCommit(const std::string& pHost, unsigned int pPort) {
	mpProtocol->removeActiveTimer();
	mpProtocol->writeMsg()
		? mpProtocol->sendMessage(pHost, pPort, SUCCESS)
		: mpProtocol->sendMessage(pHost, pPort, UNSUCCESS);
	mpProtocol->addToTimer();
	return ClientCommitState::getInstance(mpProtocol);
}

StateMachine* ClientPrecommitState::onAbort(const std::string& pHost, unsigned int pPort) {
	mpProtocol->removeActiveTimer();
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ClientPrecommitState::onNegativeAckOrTimeout() {
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ClientCommitState::onSuccessful(const std::string& pHost, unsigned int pPort) {
	mpProtocol->removeActiveTimer();
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ClientCommitState::onUnsuccessful(const std::string& pHost, unsigned int pPort) {
	mpProtocol->removeActiveTimer();
	mpProtocol->undoWrite();
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ClientCommitState::onNegativeAckOrTimeout() {
	mpProtocol->removeActiveTimer();
	mpProtocol->undoWrite();
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

std::string StateMachine::getStateName(States pState) {
	switch (pState) {
	case States::ServerPrecommit:
		return "Server Precommit";

	case States::ServerCommit:
		return "Server Commit";

	case States::OpPerformed:
		return "OpPerformed";

	case States::ClientPrecommit:
		return "Client Precommit";

	case States::ClientCommit:
		return "Client Committed";

	case States::Idle:
	default:
		return "Idle";
	}
	return std::string();
}
