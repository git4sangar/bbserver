//sgn
#include <iostream>
#include <thread>
#include <unistd.h>

#include "Protocol.h"
#include "Constants.h"
#include "StateMachine.h"

#define MODULE_NAME "StateMachine : "

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
	mLogger << std::endl << MODULE_NAME << "--- IdleState::onWriteRequest ---" << std::endl;
	mLogger << MODULE_NAME << "Acq Lk, Brd Msg, Add Tmr" << std::endl;
	mpProtocol->grabWriteLock();
	mpProtocol->broadcastMessage(PRECOMMIT);
	mpProtocol->addToTimer();
	return ServerPrecommitState::getInstance(mpProtocol);
}

StateMachine* IdleState::onPrecommit(std::string pHost, uint32_t pPort) {
	mLogger << std::endl << MODULE_NAME << "--- IdleState::onPrecommit ---" << std::endl;
	mLogger << MODULE_NAME << "Acq Lk, Add Tmr, Snd +Ak" << std::endl;
	mpProtocol->grabWriteLock();
	usleep(1000 * 100);
	mpProtocol->respondBackToPeer(pHost, pPort, POSITIVE_ACK);
	mpProtocol->addToTimer();
	return ClientPrecommitState::getInstance(mpProtocol);
}

StateMachine* ServerPrecommitState::onNegativeAckOrTimeout() {
	mLogger << std::endl << MODULE_NAME << "--- ServerPrecommitState::onNegativeAckOrTimeout ---" << std::endl;
	mLogger << MODULE_NAME << "Brd Msg, Wrt Resp, Rel Lk" << std::endl;
	mpProtocol->broadcastMessage(ABORT);
	mpProtocol->sendWriteResponse("3.2 ERROR WRITE Peers negative ack or timedout");
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ServerPrecommitState::onAllAck() {
	mLogger << std::endl << MODULE_NAME << "--- ServerPrecommitState::onAllAck ---" << std::endl;
	mLogger << MODULE_NAME << "Rmv Tmr, Brd Msg, Add Tmr" << std::endl;
	mpProtocol->removeActiveTimer();
	usleep(1000 * 100);
	mpProtocol->broadcastMessage(COMMIT);
	mpProtocol->addToTimer();
	return ServerCommitState::getInstance(mpProtocol);;
}

//	All peers responded success
StateMachine* ServerCommitState::onSuccess() {
	mLogger << std::endl << MODULE_NAME << "--- ServerCommitState::onSuccess ---" << std::endl;
	mLogger << MODULE_NAME << "Wrt Msg" << std::endl;
	size_t msgNo = mpProtocol->writeOrReplaceMsg();
	auto pNewState = (msgNo > 0) ? onAllSuccess(msgNo) : onFailure();
	return pNewState;
}

StateMachine* ServerCommitState::onAllSuccess(size_t pMsgNo) {
	mLogger << std::endl << MODULE_NAME << "--- ServerCommitState::onAllSuccess ---" << std::endl;
	mLogger << MODULE_NAME << "Rmv Tmr, Brd Msg, Wrt Resp, Rel Lk" << std::endl;
	mpProtocol->removeActiveTimer();
	mpProtocol->broadcastMessage(SUCCESSFUL);
	mpProtocol->sendWriteResponse(std::string("3.0 WROTE ") + std::to_string(pMsgNo), pMsgNo);
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ServerCommitState::onFailure() {
	mLogger << std::endl << MODULE_NAME << "--- ServerCommitState::onFailure ---" << std::endl;
	mLogger << MODULE_NAME << "Rmv Tmr, Brd Msg, Wrt Resp, Rel Lk" << std::endl;
	mpProtocol->removeActiveTimer();
	mpProtocol->broadcastMessage(UNSUCCESSFUL);
	mpProtocol->sendWriteResponse("3.1 UNKNOWN ");
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ClientPrecommitState::onCommit(std::string pHost, uint32_t pPort) {
	mLogger << std::endl << MODULE_NAME << "--- ClientPrecommitState::onCommit ---" << std::endl;
	mLogger << MODULE_NAME << "Rmv Tmr, Wrt Msg, Brd Msg, Add Tmr" << std::endl;
	mpProtocol->removeActiveTimer();
	if(mpProtocol->writeOrReplaceMsg() > 0) {
		usleep(1000 * 100);
		mpProtocol->respondBackToPeer(pHost, pPort, SUCCESS);
		mpProtocol->addToTimer();
		return ClientCommitState::getInstance(mpProtocol);
	}
	usleep(1000 * 100);
	mpProtocol->respondBackToPeer(pHost, pPort, UNSUCCESS);
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ClientPrecommitState::onAbort(std::string pHost, uint32_t pPort) {
	mLogger << std::endl << MODULE_NAME << "--- ClientPrecommitState::onAbort ---" << std::endl;
	mLogger << MODULE_NAME << "Rmv Tmr, Rel Lk" << std::endl;
	mpProtocol->removeActiveTimer();
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ClientPrecommitState::onNegativeAckOrTimeout() {
	mLogger << std::endl << MODULE_NAME << "--- ClientPrecommitState::onNegativeAckOrTimeout ---" << std::endl;
	mLogger << MODULE_NAME << "Rel Lk" << std::endl;
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ClientCommitState::onSuccessful(std::string pHost, uint32_t pPort) {
	mLogger << std::endl << MODULE_NAME << "--- ClientCommitState::onSuccessful ---" << std::endl;
	mLogger << MODULE_NAME << "Rmv Tmr, Rel Lk" << std::endl;
	mpProtocol->removeActiveTimer();
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ClientCommitState::onUnsuccessful(std::string pHost, uint32_t pPort) {
	mLogger << std::endl << MODULE_NAME << "--- ClientCommitState::onUnsuccessful ---" << std::endl;
	mLogger << MODULE_NAME << "Rmv Tmr, Undo Wrt, Rel Lk" << std::endl;
	mpProtocol->removeActiveTimer();
	mpProtocol->undoWrite();
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

StateMachine* ClientCommitState::onNegativeAckOrTimeout() {
	mLogger << std::endl << MODULE_NAME << "--- ClientCommitState::onNegativeAckOrTimeout ---" << std::endl;
	mLogger << MODULE_NAME << "Rmv Tmr, Undo Wrt, Rel Lk" << std::endl;
	mpProtocol->removeActiveTimer();
	mpProtocol->undoWrite();
	mpProtocol->releaseWriteLock();
	return IdleState::getInstance(mpProtocol);
}

std::string StateMachine::getStateName() {
	/*switch (getStateEnum()) {
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
	}*/
	return std::string();
}
