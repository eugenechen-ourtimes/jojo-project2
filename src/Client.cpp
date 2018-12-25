const char *del = "--------------------";
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include "option.h"
using namespace std;
#include "SocketAddr.cpp"
#include "State.hpp"
#include "Command.hpp"
#include "CommandHelper.cpp"


static Option options[] = {
	{ OPT_STRING, "host", &host, "remote host"},
	{ OPT_UINT, "port", &port, "remote port"}
};

class Client {
	public:
		SocketAddr remote;
		int connFd;
		Client() {
			connFd = connect();
			if (connFd < 0) exit(-1);
		}
		Client(string host, unsigned port) {
			remote = SocketAddr(host, port);
			*this = Client();
		}
		void run()
		{
			CommandHelper helper(connFd, ::HOME);
			char line[1000];
			char arg0[1000];
			char arg1[1000];
			char arg2[1000];
			char arg3[1000];
			while (true) {
				fprintf(stderr, "> ");
				fgets(line, 1000, stdin);
				int ret = sscanf(line, "%s%s%s%s", arg0, arg1, arg2, arg3);

				if (ret <= 0) continue;
				string strCommand(arg0);

				if (strCommand == CommandHelper::HELP) {
					helper.help();
					continue;
				}

				if (strCommand == CommandHelper::REFRESH) {
					helper.refresh();
					continue;
				}

				/* home page */
				if (strCommand == CommandHelper::SIGN_UP) {
					helper.signUp();
					continue;
				}

				if (strCommand == CommandHelper::LOGIN) {
					helper.login();
					continue;
				}

				if (strCommand == CommandHelper::QUIT) {
					helper.quit();
					continue;
				}

				/* sign-up page */
				if (strCommand == CommandHelper::USERNAME) {
					helper.setUsername((ret == 2) ? string(arg1): "");
					continue;
				}

				if (strCommand == CommandHelper::PASSWORD) {
					helper.setPassword();
					continue;
				}

				if (strCommand == CommandHelper::CONFIRM_PASSWORD) {
					helper.confirmPassword();
					continue;
				}

				if (strCommand == CommandHelper::CANCEL) {
					helper.cancelSignUp();
					continue;
				}

				if (strCommand == CommandHelper::CREATE_ACCOUNT) {
					helper.createAccount();
					continue;
				}

				if (strCommand == CommandHelper::LIST) {
					helper.list();
					continue;
				}

				if (strCommand == CommandHelper::SEND) {
					helper.send(ret, string(arg1), string(arg2), string(arg3));
					continue;
				}

				if (strCommand == CommandHelper::LOGOUT) {
					helper.logout();
					continue;
				}
			}
		}

	private:
		int connect() {
			struct addrinfo hints;
			struct addrinfo *result, *rp;
			int status;
			memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_family = AF_INET;    
    		hints.ai_socktype = SOCK_STREAM; 
    		hints.ai_flags = 0;
    		hints.ai_addrlen = 0;
    		hints.ai_protocol = IPPROTO_TCP;
    		hints.ai_canonname = NULL;
    		hints.ai_addr = NULL;
    		hints.ai_next = NULL;
    		/* client do not need to know whether host is a hostname or ip address */
    		status = getaddrinfo(remote.host().c_str(), to_string(remote.port()).c_str(), &hints, &result);
			if (status < 0) {
				fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
				return -1;
			}

			for (rp = result; rp != NULL; rp = rp->ai_next) {
				int fd = socket(AF_INET, SOCK_STREAM, 0);
				if (fd < 0)	continue;
				if (::connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
					return fd;
				}
				
				fprintf(stderr, "connect: %s\n", strerror(errno));
				close(fd);
				return -2;
			}

    		return -3;
		}
};

int main(int argc, char *argv[])
{
	Opt_Parse(argc, argv, options, Opt_Number(options), 0);
	Client client(host, port);
	client.run();
}