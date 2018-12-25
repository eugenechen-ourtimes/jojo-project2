#include "option.h"
#include "Command.hpp"
#include "State.hpp"
#include "CreateAccountResult.hpp"
#include "LoginResult.hpp"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <memory.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <map>
#include <string>
#include <algorithm>
#include <assert.h>
#include "SignUpUtils.hpp"

using namespace std;

/* { OPT_STRING, "text", &textFileName, "text file to disambiguate" } */
#include "SocketAddr.cpp"
static Option options[] = {
	{ OPT_UINT, "port", &port, "port number"}
};

template<typename KeyType, typename ValueType> 
pair<KeyType,ValueType> maxValue(const map<KeyType,ValueType> &x)
{
	return *max_element(x.begin(), x.end(),
	[] (const pair<KeyType,ValueType> & p1, const pair<KeyType,ValueType> & p2)
	{
		return p1.second < p2.second;
	}); 
}

class Server {
	public:
		static const int maxAccount = 1000;
		static const int maxConnection = 256;
		static const int maxLoginUsers = 30;
		

		static const string usersFileName;
		FILE *usersFile;

		fd_set readFds, copy;
		int listenFd;
		unsigned listenPort;

		map < SocketAddr, int > connections;	/* SocketAddr -> connFd */

		map < string, int > onlineUsers;		/* username -> connFd */
		map < int, State> states;				/* connFd -> State */				
		
		map < string, string > credentials;		/* username -> password */

		Server();
		Server(unsigned listenPort);

		class CommandHandler {
			public:
				Server &server;
				CommandHandler(Server &ref): server(ref) {}
				void handleSignUpRequest(int connFd);
				void checkUsernameTaken(int connFd);
				void createAccount(int connFd);
				void acceptCancelSignUp(int connFd);
				void acceptQuit(const SocketAddr &socketAddr, int connFd);
				void handleLogin(int connFd);
				void handleLogout(int connFd);
				void handleListUsers(int connFd);
		};

		void init() {
			char username[64];
			char password[1024];
			FILE *fp = fopen(usersFileName.c_str(), "r");
			if (fp != NULL) {
				while (fscanf(fp, "%s%s", username, password) != EOF) {
					credentials[string(username)] = string(password);
				}
				fclose(fp);
			}

			usersFile = fopen(usersFileName.c_str(), "a");
			if (usersFile == NULL) {
				perror(usersFileName.c_str());
				exit(-1);
			}
		}

		void run() {
			CommandHandler handler(*this);
			FD_SET(listenFd, &readFds);

			while (true) {
				struct timeval timeout;
				memcpy(&copy, &readFds, sizeof(fd_set));
				unsigned maxFd = getMaxFd();
				timeout.tv_sec = 0; timeout.tv_usec = 200000;
				int ret = select(maxFd + 1, &copy, NULL, NULL, &timeout);
				if (ret < 0) {
					perror("select");
					continue;
				}

				if (FD_ISSET(listenFd, &copy)) {
					addConnection();
					continue;
				}

				/* connections: socketAddr -> connFd */
				/* onlineUsers:  username -> socketAddr */

				checkConnections(handler);
			}
		}

	private:
		inline State getStateByFd(int connFd);
		unsigned getMaxFd();
		void addConnection();
		void removeUserFromOnlineList(int connFd);
		void checkConnections(CommandHandler &handler);
};




const string Server::usersFileName = "../data/server/user.txt";





int main(int argc, char *argv[])
{
	Opt_Parse(argc, argv, options, Opt_Number(options), 0);
	Server server(port);
	server.init();
	server.run();
	return 0;
}

void Server::removeUserFromOnlineList(int connFd)
{
	map < string, int >::iterator it;
	for (it = onlineUsers.begin(); it != onlineUsers.end(); it++) {
		if (connFd == it->second) {
			fprintf(stderr, "remove %s from online user list\n", it->first.c_str());
			onlineUsers.erase(it);
			return;
		}
	}
}


void Server::checkConnections(CommandHandler &handler)
{
	map < SocketAddr, int >::iterator c_it; /* c: connection */
	for (c_it = connections.begin(); c_it != connections.end(); c_it++) {
		if (FD_ISSET(c_it->second, &copy)) {
			int command = 0;
			int connFd = c_it->second;
			int ret = recv(connFd, &command, sizeof(int), 0);
			if (ret == 0) {
				/* connection close */
				if (getStateByFd(connFd) == ::ONLINE)
					removeUserFromOnlineList(connFd);
				/* remove states */
				states.erase(connFd);

				/* remove connection */
				fprintf(stderr, "remove connection to %s:%u\n",
					c_it->first.host().c_str(),
					c_it->first.port()
					);

				connections.erase(c_it);
				close(connFd);
				FD_CLR(connFd, &readFds);
				return;
			} 

			switch (command) {
				case ::signUp:
					handler.handleSignUpRequest(connFd);
					return;
				case ::checkUsernameTaken:
					handler.checkUsernameTaken(connFd);
					return;
				case ::createAccount:
					handler.createAccount(connFd);
					return;
				case ::cancelSignUp:
					handler.acceptCancelSignUp(connFd);
					return;
				case ::quit:
					handler.acceptQuit(c_it->first, connFd);
					return;
				case ::login:
					handler.handleLogin(connFd);
					return;
				case ::logout:
					handler.handleLogout(connFd);
					return;
				case ::listUsers:
					handler.handleListUsers(connFd);
					return;
			}
		}
	}
}

Server::Server()
{
	listenFd = 0;
	listenPort = 0;
	FD_ZERO(&readFds);
	FD_ZERO(&copy);
}

Server::Server(unsigned listenPort)
{
	this->listenPort = listenPort;

	listenFd = socket(PF_INET, SOCK_STREAM, 0);
	if (listenFd < 0) {
		fprintf(stderr, "socket open error\n");
		exit(-1);
	}

	struct sockaddr_in addr;
 	addr.sin_family = AF_INET;
 	addr.sin_addr.s_addr = htonl(INADDR_ANY);
 	addr.sin_port = htons(port);

 	if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
 		close(listenFd);
 		fprintf(stderr, "port bind error\n");
    	exit(-1);
 	}

	if (listen(listenFd, 1024) < 0) {
		close(listenFd);
		fprintf(stderr, "port listen error\n");
		exit(-1);
	}

	FD_ZERO(&readFds);
	FD_ZERO(&copy);

	fprintf(stderr, "server init success\n");
}

void Server::addConnection()
{
	if (connections.size() == maxConnection) {
		fprintf(stderr, "connection full\n");
		return;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	const size_t connLen = sizeof(addr);

	int connFd = accept(listenFd,
					(struct sockaddr*)&addr,
					(socklen_t*)&connLen);
	if (connFd < 0) {
		if (errno == ENFILE) {
			fprintf(stderr, "out of file descriptor table\n");
			return;
		}

		if (errno == EINTR) return;

		fprintf(stderr, "accept err\n");
		fprintf(stderr, "code: %s\n", strerror(errno));
		return;
	}

	char ip[40];

	if (inet_ntop(addr.sin_family, &(addr.sin_addr), ip, 40) == NULL) {
		close(connFd);
		fprintf(stderr, "inet_ntop: %s\n", strerror(errno));
	}

	FD_SET(connFd, &readFds);
	SocketAddr socketAddr(string(ip), addr.sin_port);
	fprintf(stderr, "add %s:%u to connection\n", ip, addr.sin_port);
	connections[socketAddr] = connFd;
	states[connFd] = ::HOME;
}

inline State Server::getStateByFd(int connFd)
{
	map < int, State >::iterator it = states.find(connFd);
	if (it == states.end()) fprintf(stderr, "error\n");
	return (it->second);
}

unsigned Server::getMaxFd()
{
	if (connections.empty()) return listenFd;
	auto max = maxValue(connections);/* max in client's fd */
	int maxFd = listenFd;			/* server's listen fd */
	if (max.second > maxFd) maxFd = max.second;
	return maxFd;
}

void Server::CommandHandler::handleSignUpRequest(int connFd)
{
	map < string, string > &credentials = server.credentials;
	bool permit = (credentials.size() < maxAccount);
	send(connFd, &permit, sizeof(bool), 0);
	if (permit) server.states[connFd] = ::REGISTER;
}

void Server::CommandHandler::checkUsernameTaken(int connFd)
{
	char username[20];
	int usernameLen = -1;
	recv(connFd, &usernameLen, sizeof(int), 0);
	recv(connFd, username, usernameLen, 0);
	username[usernameLen] = '\0';
	map < string, string > &credentials = server.credentials;
	bool usernameTaken = credentials.find(string(username)) != credentials.end();
	send (connFd, &usernameTaken, sizeof(bool), 0);
}

void Server::CommandHandler::createAccount(int connFd)
{
	CreateAccountResult result = OK;
	SignUpUtils utils;

	map < int, State > &states = server.states;

	char username[20];
	char password[256];
	int usernameLen = -1;
	recv(connFd, &usernameLen, sizeof(int), 0);
	recv(connFd, username, usernameLen, 0);
	username[usernameLen] = '\0';
	map < string, string > &credentials = server.credentials;

	int passwordLen = -1;
	recv(connFd, &passwordLen, sizeof(int), 0);
	recv(connFd, password, passwordLen, 0);
	password[passwordLen] = '\0';

	utils.setUsername(string(username));
	utils.setPassword(string(password));

	if (!utils.usernameValid()) {
		result = UsernameInvalid;
	} else {
		if (!utils.passwordValid()) {
			result = PasswordInvalid;
		} else {
			if (credentials.find(utils.getUsername()) != credentials.end())
				result = UsernameTaken;
			else {
				if (credentials.size() >= maxAccount) {
					result = FullAccount;
					states[connFd] = ::HOME;
				}
			}
		}
	}

	if (result == OK) {
		states[connFd] = ::HOME;
		credentials[utils.getUsername()] = utils.getPassword();
		fprintf(stderr, "create account <%s, %s>\n", username, password);
		fprintf(server.usersFile, "%s %s\n", username, password);
		fflush(server.usersFile);
	}

	send(connFd, &result, sizeof(int), 0);
}

void Server::CommandHandler::acceptCancelSignUp(int connFd)
{
	bool ack = true;
	send(connFd, &ack, sizeof(bool), 0);
	server.states[connFd] = ::HOME;
}

void Server::CommandHandler::acceptQuit(const SocketAddr &socketAddr, int connFd)
{
	bool ack = true;
	send(connFd, &ack, sizeof(bool), 0);
	fprintf(stderr, "accept quit from %s:%u\n",
		socketAddr.host().c_str(),
		socketAddr.port()
		);

	/* remove states */
	server.states.erase(connFd);
	
	/* remove connection */
	server.connections.erase(socketAddr);
	close(connFd);
	FD_CLR(connFd, &server.readFds);
}

void Server::CommandHandler::handleLogin(int connFd)
{
	/* AlreadyOnline */
	/* UsernameDoesNotExist */
	/* PasswordIncorrectt */
	/* ChatroomFull */
	map < int, State > &states = server.states;

	char username[20];
	char password[256];
	int usernameLen = -1;
	recv(connFd, &usernameLen, sizeof(int), 0);
	recv(connFd, username, usernameLen, 0);
	username[usernameLen] = '\0';
	
	map < string, string > &credentials = server.credentials;
	map < string, int > &onlineUsers = server.onlineUsers;

	int passwordLen = -1;
	recv(connFd, &passwordLen, sizeof(int), 0);
	recv(connFd, password, passwordLen, 0);
	password[passwordLen] = '\0';

	map < string, string >::iterator it = credentials.find(string(username));
	LoginResult result = Login;

	if (it == credentials.end()) {
		result = UsernameDoesNotExist;
	} else {
		if (string(password) != it->second) 
			result = PasswordIncorrect;
		else {
			if (states.find(connFd)->second == ::ONLINE)
				result = AlreadyOnline;
			else {
				if (onlineUsers.find(string(username)) != onlineUsers.end())
					result = LoginByAnotherProcess;
				else {
					if (onlineUsers.size() >= maxLoginUsers)
						result = ChatroomFull;
				}
			}
		}
	}

	if (result == Login) {
		server.states[connFd] = ::ONLINE;
		server.onlineUsers[string(username)] = connFd;
		fprintf(stderr, "%s login\n", username);
	}
	send(connFd, &result, sizeof(LoginResult), 0);
}

void Server::CommandHandler::handleLogout(int connFd)
{
	char username[20];
	int usernameLen = -1;
	recv(connFd, &usernameLen, sizeof(int), 0);
	recv(connFd, username, usernameLen, 0);
	username[usernameLen] = '\0';
	map < string, int > &onlineUsers = server.onlineUsers;
	map < int, State >  &states = server.states;
	map < string, int >::iterator it = onlineUsers.find(string(username));
	bool ack = true;
	assert(states.find(connFd) != states.end());
	if (states.find(connFd)->second == ONLINE) {
		if (it == onlineUsers.end() || it->second != connFd)
			ack = false;
	}

	send(connFd, &ack, sizeof(bool), 0);
	if (ack) {
		fprintf(stderr, "%s logout\n", username);
		server.removeUserFromOnlineList(connFd);
		states[connFd] = ::HOME;
	}
}

void Server::CommandHandler::handleListUsers(int connFd)
{
	/* only allow when state is ::ONLINE */
	map < int, State >  &states = server.states;
	bool permit = states.find(connFd)->second == ::ONLINE;
	send(connFd, &permit, sizeof(bool), 0);
	if (!permit) return;
	
	map < string, string > &credentials = server.credentials;
	int usersMapSize = credentials.size();
	
	send(connFd, &usersMapSize, sizeof(int), 0);

	map < string, string >::iterator it;
	for (it = credentials.begin(); it != credentials.end(); it++) {
		int usernameLen = it->first.length();
		send(connFd, &usernameLen, sizeof(int), 0);
		send(connFd, it->first.c_str(), usernameLen, 0);
	}
}