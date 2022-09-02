//sgn
#pragma once
#include <iostream>

#define USER_CMD		(1001)
#define READ_CMD		(1002)
#define WRITE_CMD		(1003)
#define REPLACE_CMD		(1004)
#define QUIT_CMD		(1005)

#define PRECOMMIT_TIMEOUT_SECS	(3)


extern std::string PRECOMMIT;
extern std::string COMMIT;
extern std::string ABORT;
extern std::string SUCCESS;		//	From peer to server
extern std::string UNSUCCESS;	//	From peer to server
extern std::string SUCCESSFUL;	//	From server to peer
extern std::string UNSUCCESSFUL;//	From server to peer
extern std::string POSITIVE_ACK;
extern std::string NEGATIVE_ACK;

extern std::string USER;
extern std::string READ;
extern std::string WRITE;
extern std::string REPLACE;
extern std::string QUIT;

extern std::string ALPHA_CAPS;
extern std::string ALPHA_SMALL;
extern std::string NUMERIC;