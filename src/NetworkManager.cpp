//sgn
#include <unistd.h>

#include "NetworkManager.h"

namespace util {

	void NetworkManager::sendPacket(std::string pHost, unsigned int pPort, std::string strMsg) {
#ifdef __linux__
		sleep(1);
#endif
	}

	void NetworkManager::receiveThread(unsigned int pPort) {
		while (1) {
#ifdef __linux__
			sleep(1);
#endif
		}
	}

} // namespace util
