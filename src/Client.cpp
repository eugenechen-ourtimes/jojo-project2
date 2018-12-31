const char *del = "--------------------";
const char *reset = "\033[0m";
const char *arrow[2] = {"=>", "->"};
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
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
#include "DataType.hpp"
#include "CommandHelper.cpp"
#include <sys/select.h>
#include <dirent.h>
#include <sys/stat.h>

static Option options[] = {
	{ OPT_STRING, "host", &host, "remote host"},
	{ OPT_UINT, "port", &port, "remote port"}
};

class Client {
	public:
		static const State nowstate = ::HOME;
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
			CommandHelper helper(connFd, nowstate);
			
			fd_set read_set;
			fd_set working_set;
			FD_ZERO(&read_set);
			FD_SET(connFd, &read_set);
			FD_SET(STDIN_FILENO, &read_set);

			fprintf(stderr, "> ");

			while (1) {

				memcpy(&working_set, &read_set, sizeof(fd_set));
				select(connFd + 1 , &working_set, NULL, NULL, NULL);
				
				if (FD_ISSET(STDIN_FILENO, &working_set)) {
					handleStdin(helper);
					fprintf(stderr, "> ");
					continue;
				}

				if (FD_ISSET(connFd, &working_set)) {
					fprintf(stderr, "\n");
					handleDataFromServer(connFd, helper);
					fprintf(stderr, "> ");
				}
			}
		}

		void handleStdin(CommandHelper &helper)
		{
			static char line[1000];
			static char arg0[1000];
			static char arg1[1000];
			static char arg2[1000];
			static char arg3[1000];

			fgets(line, 1000, stdin);
			
			int ret = sscanf(line, "%s%s%s%s", arg0, arg1, arg2, arg3);
			
			if (ret <= 0) return;
			string strCommand(arg0);

			if (strCommand == CommandHelper::HELP) {
				helper.help();
				return;
			}

			if (strCommand == CommandHelper::REFRESH) {
				helper.refresh();
				return;
			}

			/* home page */
			if (strCommand == CommandHelper::SIGN_UP) {
				helper.signUp();
				return;
			}

			if (strCommand == CommandHelper::LOGIN) {
				helper.login();
				return;
			}

			if (strCommand == CommandHelper::QUIT) {
				helper.quit();
				return;
			}

			/* sign-up page */
			if (strCommand == CommandHelper::USERNAME) {
				if (ret == 1) {
					fprintf(stderr, "missing input string\n");
					return;
				}

				helper.inputUsername(string(arg1));
				return;
			}

			if (strCommand == CommandHelper::PASSWORD) {
				helper.inputPassword();
				return;
			}

			if (strCommand == CommandHelper::CONFIRM_PASSWORD) {
				helper.confirmPassword();
				return;
			}

			if (strCommand == CommandHelper::CANCEL) {
				helper.cancelSignUp();
				return;
			}

			if (strCommand == CommandHelper::CREATE_ACCOUNT) {
				helper.createAccount();
				return;
			}
			
			if (strCommand == CommandHelper::LIST) {
				helper.list();
				return;
			}

			if (strCommand == CommandHelper::SEND) {
				/*	send	-m/-f	username	message/filename
					arg0	arg1	arg2		arg3

					send	username	message
					arg0	arg1		arg2	

					sendData(option, targetUserName, content) */

				if (ret < 3) {
					/* TODO: modify this sentence */
					fprintf(stderr, "format error! Please input \033[33m\033[1m\\help\033[0m to make sure the format.\n");
					return;
				}

				if (ret == 3) {
					helper.sendData("-m", string(arg1), string(arg2));
					return;
				}

				helper.sendData(string(arg1), string(arg2), string(arg3));
				return;
			}

			if (strCommand == CommandHelper::LOGOUT) {
				helper.logout();
				return;
			}

			if (strCommand == CommandHelper::HISTORY) {
				helper.history();
				return;
			}

			if (strCommand == CommandHelper::DOWNLOAD) {
				if (helper.getState() != ::ONLINE) {
					helper.promptStateIncorrect();
					return;
				}

				if (ret < 2) {
					fprintf(stderr, "format error! \033[33m\033[1m\\download\033[0m should be followed by one parameter [%s], or you can input \033[33m\033[1m\\help\033[0m to see advanced instruction\n", "%s");
					return;
				}

				helper.download(string(arg1), (ret == 2) ? "": string(arg2));
				return;
			}

			if (strCommand == CommandHelper::DOWNLOADLIST) {
				helper.showDownloadList();
				return;
			}
		}

		void handleDataFromServer(int fd, CommandHelper &helper)
		{
			int numOfLines = -1;
			char fromUserName[64], content[256];
			char time_cstr[32];
			int fromUserNameLen, contentLen;
			int time_len;

			int type = Zero;
			int ret = recv(fd, &numOfLines, sizeof(int), 0);
			if (ret == 0) {
				fprintf(stderr, "server disconnected\n");
				exit(0);
			}

			bool hasFile = false;

			while (numOfLines--) {
				recv(fd, &fromUserNameLen, sizeof(int), 0);
				recv(fd, fromUserName, fromUserNameLen, 0);

				recv(fd, &contentLen, sizeof(int), 0);
				recv(fd, content, contentLen, 0);

				recv(fd, &type, sizeof(int), 0);

				recv(fd, &time_len, sizeof(int), 0);
				recv(fd, time_cstr, time_len, 0);

				if (type != Message && type != File) {
					fprintf(stderr, "Unknown type %d\n", type);
					continue;
				}

				fromUserName[fromUserNameLen] = '\0';
				content[contentLen] = '\0';
				time_cstr[time_len] = '\0';

				bool received = true;
				send(fd, &received, sizeof(bool), 0);

				int isFile = (type == File) ? 1: 0;

				#define COLOR "\033[31m\033[1m"
				
				fprintf(stderr, COLOR "%s" RESET " %s %s\t%s\n",
					fromUserName,
					arrow[isFile],
					content,
					time_cstr
				);

				#undef COLOR

				if (isFile) {
					hasFile = true;
					string path = CommandHelper::downloadListFolder + helper.getUsername();
					FILE *fp = fopen(path.c_str(), "a");
					if (fp == NULL) {
						perror(path.c_str());
						exit(-1);
					}
					fprintf(fp, "%s %s\n", content, time_cstr);
					fclose(fp);
				}
			}

			if (hasFile) {
				fprintf(stderr, "Input \033[33m\033[1m\\download [filename]\033[0m to download files.\n");
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
