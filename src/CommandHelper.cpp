#include "color.hpp"
#include "CreateAccountResult.hpp"
#include "LoginResult.hpp"
#include "SignUpHelper.hpp"
#include <assert.h>
#include <ctype.h>
#include <libgen.h>
#include <stdint.h>
#include <vector>
#include "common.hpp"
#include <dirent.h>
#include <sys/stat.h>
#define PasswordBuffer 1024


class CommandHelper {
	public:
		static const string strEmpty;	/* [Empty] */
		static const string strHidden;  /* [Hidden] */
		static const string savedPasswordFolder; /* ../data/client/pass/ */
		static const string downloadFolder;
		static const string downloadListFolder;

		static const string HELP;		/* help */
		static const string REFRESH;	/* refresh */
		static const string SIGN_UP;	/* sign-up */
		static const string LOGIN;		/* login */
		static const string QUIT;		/* quit */
		static const string USERNAME;	/* username */
		static const string PASSWORD;	/* password */
		static const string CONFIRM_PASSWORD;	/* confirm-password */
		static const string CANCEL;		/* cancel */
		static const string CREATE_ACCOUNT;		/* create-account */
		static const string LIST;		/* list */
		static const string SEND;		/* send */
		static const string LOGOUT;		/* logout */
		static const string HISTORY;
		static const string DOWNLOAD;
		static const string DOWNLOADLIST;

		CommandHelper(int connFd, State state)
		{
			this->connFd = connFd;
			setState(state);
			signUpHelper.setFd(connFd);
			memset(storedPassword, '\0', PasswordBuffer);
			refresh();
		}

		void help()
		{
			fprintf(stderr, "input \033[33m\033[1m\\refresh\033[0m to refresh your web page\n");
			showLocalCommands();
		}

		void showLocalCommands()
		{
			if (state == ::HOME) {
				fprintf(stderr, "\ninput \033[33m\033[1m\\sign-up\033[0m to enter the signup page\n");
				fprintf(stderr, "\ninput \033[33m\033[1m\\login\033[0m to log in\n");
				fprintf(stderr, "\ninput \033[33m\033[1m\\quit\033[0m to leave the application\n");
				return;
			}

			if (state == ::REGISTER) {
				fprintf(stderr, "\ninput \033[33m\033[1m\\username [username]\033[0m to register account with [username]\n");
				fprintf(stderr, "\ninput \033[33m\033[1m\\password\033[0m to set the password; to execute this instruction, the username must be set.\n");

				fprintf(stderr, "\ninput \033[33m\033[1m\\confirm-password\033[0m to confirm your password; Password should be set before.\n");
				fprintf(stderr, "\ninput \033[33m\033[1m\\cancel\033[0m to cancel sign up and return to previous page\n");
				fprintf(stderr, "\ninput \033[33m\033[1m\\create-account\033[0m: you can create the account successfully if username, password, and confirm-password have been correctly set\n");
				return;
			}

			if (state == ::ONLINE) {
				fprintf(stderr,"\ninput \033[33m\033[1m\\send [-m] [ID] \'message\' \033[0mto send a message to a specific ID; message transmitting is the default action of \033[33m\033[1m\\send\033[0m, so [-m] is optional.\n");
				fprintf(stderr,"\ninput \033[33m\033[1m\\send [-f] [ID] \'filename\' \033[0mto send a file to a specific ID; make sure you have specify \'-f\' for file sending.\n");
				fprintf(stderr,"\ninput \033[33m\033[1m\\list\033[0m to get a list of person who has signed up for Chatroom\n");
				fprintf(stderr,"\ninput \033[33m\033[1m\\history\033[0m to view previous message\n");	
				fprintf(stderr,"\ninput \033[33m\033[1m\\download-list\033[0m to view the files available for downloading\n");
				fprintf(stderr,"\ninput \033[33m\033[1m\\download [filename]\033[0m to download the corresponding file\n");
				fprintf(stderr,"\ninput \033[33m\033[1m\\logout\033[0m to log out and go back to initial login menu\n");
			}
		}

		void refresh()
		{
			if (state == ::HOME) {
				showHomePage();
				return;
			}
			if (state == ::REGISTER) {
				signUpHelper.refresh();
				return;
			}
			if (state == ::ONLINE) {
				showOnlinePage();
				return;
			}
		}

		void signUp()
		{
			if (state != ::HOME) {
				promptStateIncorrect();
				return;
			}

			Command command = ::signUp;
			bool permit;
			send(connFd, &command, sizeof(int), 0);
			recv(connFd, &permit, sizeof(bool), 0);
			if (permit) {
				state = ::REGISTER;
				fprintf(stderr, "entering sign-up page ...\n%s\n", del);
				signUpHelper.refresh();
			} else {
				fprintf(stderr, "rejected (reason: account full)\n");
			}
		}

		void login()
		{
			if (state != ::HOME) {
				promptStateIncorrect();
				return;
			}
			
			fprintf(stderr,"Please input your username or input \033[33m\033[1m\\return \033[0m to exit\n");
			fprintf(stderr,"Input your username: ");
			char line[64];
			char username[32];
			fgets(line, 64, stdin);
			sscanf(line, "%s", username);
			if(!strcmp(username,"\\return")){
				refresh();
				return;
			}

			char* password;

			string savedPasswordPath = savedPasswordFolder + string(username);
			FILE *fp = fopen(savedPasswordPath.c_str(), "r");
			bool passwordFileAccessible = (fp != NULL);
			if (passwordFileAccessible) {
				fprintf(stderr,"\033[33m\033[5mfast login\033[0m\n");
				if (fscanf(fp, "%s", storedPassword) == EOF) storedPassword[0] = '\0';
				password = storedPassword;
				fclose(fp);
			} else {
				if (errno != ENOENT) {
					perror(savedPasswordPath.c_str());
					exit(-1);
				}
				fprintf(stderr,"Please key in your password or input \033[33m\033[1m\\return \033[0m to exit\n");
				password = getpass("Input your password: ");
				if (!strcmp(password,"\\return")) {
					refresh();
					return;
				}
			}
			
			Command command = ::login;

			send(connFd, &command, sizeof(int), 0);
			int userLen = strlen(username);
			int passwordLen = strlen(password) ;
			send(connFd, &userLen, sizeof(int), 0);
			send(connFd, username, userLen, 0);
			send(connFd, &passwordLen, sizeof(int), 0);
			send(connFd, password, passwordLen, 0);
			LoginResult result = Uninitialized;
			recv(connFd, &result, sizeof(int), 0);

			if (result == Login) {
				fprintf(stderr, GRN "=> login successful\n" RESET);
				setUsername(string(username));
				setState(::ONLINE);
				if (!passwordFileAccessible) {
					savePassword(savedPasswordPath.c_str(), password);
				}
				refresh();
				return;
			}

			if (result == UsernameDoesNotExist) {
				fprintf(stderr, YEL "=> username does not exist\n" RESET);
				if (passwordFileAccessible) {
					unlink(savedPasswordPath.c_str());
				}
				refresh();
				return;
			}

			if (result == PasswordIncorrect) {
				fprintf(stderr, RED "=> password incorrect\n" RESET);
				if (passwordFileAccessible) {
					unlink(savedPasswordPath.c_str());
				}
				refresh();
				return;
			}

			if (result == AlreadyOnline) {
				fprintf(stderr, CYN "=> rejected (already online)\n" RESET);
				return;
			}

			if (result == LoginByAnotherProcess) {
				fprintf(stderr, CYN "=> rejected (login by another process)\n" RESET);
				return;
			}
			if (result == ChatroomFull) {
				fprintf(stderr, BLU "=> rejected (chatroom full)\n" RESET);
				return;
			}

			fprintf(stderr, "server disconnected\n");
		}

		void savePassword(string path, char *password)
		{
			FILE *fp = fopen(path.c_str(), "w");
			if (fp == NULL) {
				perror(path.c_str());
				exit(-1);
			}

			fprintf(stderr, "\033[31m\033[1mDo you want to save your password? (Y/N) : \033[0m");
			char line[64];
			char feedback[64];
			fgets(line, 64, stdin);
			sscanf(line, "%s", feedback);
			int L = strlen(feedback);
			for (int i = 0; i < L; i++)
				feedback[i] = tolower(feedback[i]);

			if (strcmp(feedback, "y") == 0 || strcmp(feedback, "yes") == 0) {
				fprintf(fp, "%s\n", password);
				fclose(fp);
			} else {
				fclose(fp);
				unlink(path.c_str());
			}
		}

		void quit()
		{
			if (state != ::HOME) {
				promptStateIncorrect();
				return;
			}

			Command command = ::quit;
			bool ack = false;
			send(connFd, &command, sizeof(int), 0);
			recv(connFd, &ack, sizeof(bool), 0);
			if (ack) exit(0);
		}

		void inputUsername(string arg)
		{
			if (state != ::REGISTER) {
				promptStateIncorrect();
				return;
			}

			signUpHelper.handleInputUsername(arg);
			signUpHelper.refresh();
		}

		void inputPassword()
		{
			if (state != ::REGISTER) {
				promptStateIncorrect();
				return;
			}

			if (!signUpHelper.usernameValid()) {
				fprintf(stderr, "Please complete username first\n");
				return;
			}

			char *password;
			password = getpass("Please key in your password: ");
			signUpHelper.handleInputPassword(string(password));
			signUpHelper.refresh();
		}

		void confirmPassword()
		{
			if (state != ::REGISTER) {
				promptStateIncorrect();
				return;
			}

			if (!signUpHelper.usernameValid()) {
				fprintf(stderr, "Please complete username first\n");
				return;
			}

			if (!signUpHelper.passwordValid()) {
				fprintf(stderr, "Please complete password first\n");
				return;
			}

			char *password;
			password = getpass("Please re-input your password: ");
			signUpHelper.handleConfirmPassword(string(password));
			signUpHelper.refresh();
		}

		void cancelSignUp()
		{
			if (state != ::REGISTER) {
				promptStateIncorrect();
				return;
			}
			setState(::HOME);
			signUpHelper.cancel();
			promptReturningToHomePage();
			showHomePage();
		}

		void createAccount()
		{
			if (state != ::REGISTER) {
				promptStateIncorrect();
				return;
			}

			CreateAccountResult result = signUpHelper.createAccount();

			if (result == UsernameInvalid) {
				fprintf(stderr, "Username Invalid\n%s\n", del);
				return;
			}

			if (result == PasswordInvalid) {
				fprintf(stderr, "Password Invalid\n%s\n", del);
				return;
			}

			if (result == UsernameTaken) {
				fprintf(stderr, "sorry, this account was taken by another client process\n%s\n", del);
				signUpHelper.refresh();
				return;
			}

			if (result == FullAccount) {
				fprintf(stderr, "rejected (full account)\n");
				promptReturningToHomePage();
				setState(::HOME);
				signUpHelper.reset();
				showHomePage();
				return;
			}

			if (result == Incomplete) {
				fprintf(stderr, "please complete the form first\n");
				return;
			}

			if (result == OK) {
				fprintf(stderr, "create account success\n");
				promptReturningToHomePage();
				setState(::HOME);
				signUpHelper.reset();
				showHomePage();
				return;
			} 

			fprintf(stderr, "server seems to disconnect\n");
		}

		void sendData(string option, string targetUserName, string content)
		{
			if (state != ::ONLINE) {
				promptStateIncorrect();
				return;
			}

			if (option == "-m") {
				sendMessage(targetUserName, content);
			} else {
				if (option == "-f") {
					sendFile(targetUserName, content);
				} else {
					fprintf(stderr, "Unknown option %s\n", option.c_str());
				}
			}
		}
		
		void sendMessage(string target, string message) {
			Command command = ::sendMessage;
			send(connFd, &command, sizeof(int), 0);

			int usernameLen = username.length();
			int targetLen = target.length();
			int messageLen = message.length();

			send(connFd, &usernameLen, sizeof(int), 0);
			send(connFd, username.c_str(), usernameLen, 0);

			bool a1 = false;
			recv(connFd, &a1, sizeof(bool), 0);

			if (!a1) {
				fprintf(stderr, "src username incorrect\n");
				return;
			}

			send(connFd, &targetLen, sizeof(int), 0);
			send(connFd, target.c_str(), targetLen, 0);

			bool a2 = false;
			recv(connFd, &a2, sizeof(bool), 0);

			if (!a2) {
				fprintf(stderr, "dst username does not exist\n");
				return;
			}

			send(connFd, &messageLen, sizeof(int), 0);
			send(connFd, message.c_str(), messageLen, 0);
		}

		void sendFile(string target, string arg)
		{
			int pathLen = arg.length();
			char *path = (char *)malloc(pathLen + 1);
			strcpy(path, arg.c_str());

			/* TODO check regular file */

			FILE *fp = fopen(path, "rb");
			if (fp == NULL) {
				perror(path);
				free(path);
				return;
			}

			fseek(fp, 0L, SEEK_END);
			int64_t sz = ftell(fp);

			if (sz > MaxFile) {
				fprintf(stderr, "%s: file size too large ... no more than %d MB.\n", path, Limit);
				fclose(fp);
				free(path);
				return;
			}

			rewind(fp);

			char *ts1 = strdup(path);
			char *ts2 = strdup(path);

			if (ts1 == NULL || ts2 == NULL) {
				fprintf(stderr, "strdup error\n");
				if (ts1 != NULL) free(ts1);
				if (ts2 != NULL) free(ts2);
				free(path);
				return;
			}

			char *dir = dirname(ts1);
			char *filename = basename(ts2);
			int filenameLen = strlen(filename);

			Command command = ::sendFile;
			send(connFd, &command, sizeof(int), 0);

			int usernameLen = username.length();
			int targetLen = target.length();

			send(connFd, &usernameLen, sizeof(int), 0);
			send(connFd, username.c_str(), usernameLen, 0);

			bool a1 = false;
			recv(connFd, &a1, sizeof(bool), 0);
			if (!a1) {
				fprintf(stderr, "username incorrect\n");
				fclose(fp); free(path); free(ts1); free(ts2);
				return;
			}
			
			send(connFd, &targetLen, sizeof(int), 0);
			send(connFd, target.c_str(), targetLen, 0);

			bool a2 = false;
			recv(connFd, &a2, sizeof(bool), 0);
			if (!a2) {
				fprintf(stderr, "dst username does not exist\n");
				fclose(fp); free(path); free(ts1); free(ts2);
				return;
			}

			send(connFd, &filenameLen, sizeof(int), 0);
			send(connFd, filename, filenameLen, 0);

			char buf[IOBufSize];
			int32_t size;

			send(connFd, &sz, 8, 0);
			bool permit = false;
			recv(connFd, &permit, sizeof(bool), 0);
			
			if (permit) {
				while (sz > 0) {
					size = (sz < IOBufSize) ? sz: IOBufSize;
					fread(buf, 1, size, fp);
					send(connFd, buf, size, 0);
					sz -= size;
				}
			}

			else {
				fprintf(stderr, "server says file size too large!\n");
			}

			fprintf(stderr, "file uploaded\n");

			fclose(fp); free(path); free(ts1); free(ts2);
		}

		void list()
		{
			Command command = ::listUsers;
			send(connFd, &command, sizeof(int), 0);
			int size;
			bool permit = false;
			recv(connFd, &permit, sizeof(bool), 0);
			if (!permit) {
				fprintf(stderr, "list request rejected (state is not ONLINE)\n");
				return;
			}

			recv(connFd, &size, sizeof(int), 0);
			fprintf(stderr, "\033[32m\033[1m\033[45menrolled users:\033[0m\n\n");
			while (size--) {
				int usernameLen;
				char username[32];
				recv(connFd, &usernameLen, sizeof(int), 0);
				recv(connFd, username, usernameLen, 0);
				username[usernameLen] = '\0';
				fprintf(stderr,"* %s\n", username);
			}
		}

		void history(const char *arg)
		{
			#define DEFAULT 20
			int bufSize = DEFAULT;
			if (arg != NULL) {
				bufSize = atoi(arg);
				if (bufSize <= 0) {
					fprintf(stderr, "argument should be positive\n");
					return;
				}
			}

			#undef DEFAULT

			Command command = ::history;
			assert(send(connFd, &command, sizeof(int), 0) == sizeof(int));
			int usernameLen = username.length();
			send(connFd, &usernameLen, sizeof(int), 0);
			send(connFd, username.c_str(), usernameLen, 0);

			bool a1 = false;
			recv(connFd, &a1, sizeof(bool), 0);
			if (!a1) {
				fprintf(stderr, "username incorrect\n");
				return;
			}

			bool permit = false;
			send(connFd, &bufSize, sizeof(int), 0);
			recv(connFd, &permit, sizeof(bool), 0);
			if (!permit) {
				fprintf(stderr, "server: argument should be positive\n");
				return;
			}

			int lineCount = -1;
			char line[1000];
			char srcUser[64];
			char dstUser[64];
			char content[256];
			char time_cstr[32];
			int type;

			recv(connFd, &lineCount, sizeof(int), 0);
			#define COLOR "\033[36m\033[1m"

			while (lineCount--) {
				int L = -1;
				recv(connFd, &L, sizeof(int), 0);
				recv(connFd, line, L, 0);

				line[L] = '\0';
				
				type = Zero;
				
				sscanf(line, "%s%s%s%d%s", srcUser, dstUser, content, &type, time_cstr);
				if (type != Message && type != File) {
					fprintf(stderr, "Unknown type %d\n", type);
					continue;
				}
				
				int isFile = (type == File) ? 1: 0;

				fprintf(stderr, COLOR "%s" RESET " %s " COLOR "%s" RESET " %s\t%s\n", 
					srcUser,
					arrow[isFile],
					dstUser,
					content,
					time_cstr
					);
			}

			#undef COLOR
		}

		void showDownloadList(const char *arg)
		{
			string downloadListPath = downloadListFolder + getUsername();
			FILE *fp = fopen(downloadListPath.c_str(), "r");
			if (fp == NULL) {
				if (errno != ENOENT) {
					perror(downloadListPath.c_str());
					exit(-1);
				}
				fprintf(stderr,"No available file for downloading.\n");
				return;
			}

			fprintf(stderr,"\ninput \033[33m\033[1m\\download [%s]\033[0m to require downloading a file.\n","%s");
			
			char name[64];
			char time_cstr[32];
			queue < pair < string, string > > q;
			#define DEFAULT 10
			int bufSize = DEFAULT;
			#undef DEFAULT
			if (arg != NULL) {
				bufSize = atoi(arg);
				if (bufSize <= 0) {
					fprintf(stderr, "argument should be positive\n");
					return;
				}
			}
			while (fscanf(fp, "%s%s", name, time_cstr) != EOF) {
				q.push( pair < string, string > (string(name), string(time_cstr)) );
				if (q.size() > bufSize) q.pop();
			}

			while (!q.empty()) {
				pair < string, string > p = q.front();
				q.pop();
				fprintf(stderr, "\033[33m\033[1m%s\033[0m\t%s\n",
					p.first.c_str(),
					p.second.c_str()
					);
			}

			fclose(fp);
		}

		void download(string arg1, string arg2) {
			if (state != ::ONLINE) {				
				promptStateIncorrect();
				return ;
			}

			downloadRequest(arg1, arg2);
			return;
		}

		void downloadRequest(string filename, string timeStr)
		{
			fprintf(stderr, "file requesting\n");
					
			Command command = ::download;
			assert(send(connFd, &command, sizeof(int), 0) == sizeof(int));
			int usernameLen = username.length(), filenameLen = filename.length();
			int timeStrLen = timeStr.length();
			send(connFd, &usernameLen, sizeof(int), 0);
			send(connFd, username.c_str(), usernameLen, 0);
			
			bool a1 = false;
			recv(connFd, &a1, sizeof(bool), 0);
			if (!a1) {
				fprintf(stderr, "server says username incorrect\n");
				return;
			}

			send(connFd, &filenameLen, sizeof(int), 0);
			send(connFd, filename.c_str(), filenameLen, 0);

			send(connFd, &timeStrLen, sizeof(int), 0);
			if (timeStrLen > 0)
				send(connFd, timeStr.c_str(), timeStrLen, 0);


			int permit;
			recv(connFd, &permit, sizeof(int), 0);
			
			if (!permit) {
				fprintf(stderr, "File not exist.\n");
				return;
			}
			
			string selfDownloadFolder = downloadFolder + username + "/";
			
			mkdirIfNotExist(selfDownloadFolder.c_str());

			string downloadFilePath = selfDownloadFolder + filename;
			FILE *fp = fopen(downloadFilePath.c_str(), "wb");
			if (fp == NULL) {
				perror(downloadFilePath.c_str());
				exit(-1);
			}

			int64_t sz = 0;
			recv(connFd, &sz, 8, 0);
			assert(sz <= MaxFile);

			char buf[IOBufSize];
			int ret;
			while (sz > 0) {
				int32_t size = (sz < IOBufSize) ? sz: IOBufSize;
				int ret = recv(connFd, buf, size, 0);
				if (ret == 0) {
					fprintf(stderr, "error\n");
					exit(-1);
				}
				fwrite(buf, 1, ret, fp);
				sz -= ret;
			}
			fclose(fp);
			fprintf(stderr, "%s downloaded\n", filename.c_str());
		}

		void logout()
		{
			Command command = ::logout;
            assert(send(connFd, &command, sizeof(int), 0) == sizeof(int));
			int usernameLen = username.length();
			assert(send(connFd, &usernameLen, sizeof(int), 0) == sizeof(int));
			if (usernameLen > 0)
				assert(send(connFd, username.c_str(), usernameLen, 0) == usernameLen);
			bool ack = true;
			recv(connFd, &ack, sizeof(bool), 0);
			if (ack) {
				setState(::HOME);
				setUsername("");
				refresh();
			} else {
				fprintf(stderr, "username incorrect\n");
			}
		}

		void setState(State state)
		{
			this->state = state;
		}

		State getState()
		{
			return state;
		}

		void setUsername(string username)
		{
			this->username = username;
		}

		string getUsername()
		{
			return username;
		}

		void promptStateIncorrect() {
			fprintf(stderr, "state incorrect\n");
		}
		
	private:
		char storedPassword[PasswordBuffer];
		int connFd;
		State state;
		string username;
		SignUpHelper signUpHelper;

		void promptReturningToHomePage() {
			fprintf(stderr, "returning to home page ...\n%s\n", del);
		}

		void showHomePage()
		{
			fprintf(stderr,"\t*===========================*\n");
			fprintf(stderr,"\t*=                         =*\n");
			fprintf(stderr,"\t*= \033[5mWelcome to Chatroom 1.0 \033[0m=*\n");	
			fprintf(stderr,"\t*=                         =*\n");
			fprintf(stderr,"\t*===========================*\n\n");
			fprintf(stderr,"\tStart with an instruction\n");
			fprintf(stderr,"\t\033[33m\033[1m\\sign-up    \\login    \\quit    \\help \n\033[0m");
		}

		void showOnlinePage()
		{
			printf("\n\tHello \033[32m\033[1m\033[5m%s\033[0m >< How are you ?\n", username.empty() ? "[anonymous]": username.c_str());
			printf("\t\033[33m\033[1m\\send     \\list     \\history    \\download-list\033[0m\n");		
			fprintf(stderr,"\t\033[33m\033[1m\\download    \\help    \\logout\033[0m\n");
		}
};

const string CommandHelper::strEmpty = "[Empty]";
const string CommandHelper::strHidden = "[Hidden]";
const string CommandHelper::savedPasswordFolder = "../data/client/pass/";
const string CommandHelper::downloadFolder = "../data/client/download/";
const string CommandHelper::downloadListFolder = "../data/client/downloadList/";

const string CommandHelper::HELP = "\\help";
const string CommandHelper::REFRESH = "\\refresh";
const string CommandHelper::SIGN_UP = "\\sign-up";
const string CommandHelper::LOGIN = "\\login";
const string CommandHelper::QUIT = "\\quit";
const string CommandHelper::USERNAME = "\\username";
const string CommandHelper::PASSWORD = "\\password";
const string CommandHelper::CONFIRM_PASSWORD = "\\confirm-password";
const string CommandHelper::CANCEL = "\\cancel";
const string CommandHelper::CREATE_ACCOUNT = "\\create-account";
const string CommandHelper::LIST = "\\list";
const string CommandHelper::SEND = "\\send";
const string CommandHelper::LOGOUT = "\\logout";
const string CommandHelper::HISTORY = "\\history";
const string CommandHelper::DOWNLOAD = "\\download";
const string CommandHelper::DOWNLOADLIST = "\\download-list";
