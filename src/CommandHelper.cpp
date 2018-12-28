#include "color.hpp"
#include "CreateAccountResult.hpp"
#include "LoginResult.hpp"
#include "SignUpHelper.hpp"
#include <assert.h>
#include <ctype.h>
#include <libgen.h>
#include <stdint.h>
#include "common.hpp"
#define PasswordBuffer 1024


class CommandHelper {
	public:
		static const string strEmpty;	/* [Empty] */
		static const string strHidden;  /* [Hidden] */
		static const string savedPasswordFolder; /* ../data/client/pass/ */

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

		CommandHelper(int connFd, State state) {
			this->connFd = connFd;
			setState(state);
			signUpHelper.setFd(connFd);
			memset(storedPassword, '\0', PasswordBuffer);

			showHomePage();
		}

		void help()
		{
			fprintf(stderr, "available commands:\n");
			fprintf(stderr, "** help [-a]\n"
							"      Dispay this help\n");
			fprintf(stderr, "** refresh\n"
							"      Refresh your web page\n");
			showLocalCommands();
		}

		void showLocalCommands()
		{
			if (state == ::HOME) {
				fprintf(stderr, "** \\sign-up\n");
				fprintf(stderr, "** \\login\n");
				fprintf(stderr, "** \\quit\n");

				return;
			}

			if (state == ::REGISTER) {
				fprintf(stderr, "** \\username [username]\n");
				fprintf(stderr, "** \\password\n");
				fprintf(stderr, "** \\confirm-password\n");
				fprintf(stderr, "** \\cancel\n"
								"      Cancel sign up\n");
				fprintf(stderr, "** \\create-account\n");
				return;
			}

			if (state == ::ONLINE) {
				fprintf(stderr,"input \033[33m\033[1m \\send [-m] [ID] 'message' \033[0mto send a message to a specific ID. Message transmitting is the default action of \033[33m\033[1m\\send\033[0m, so [-m] is optional\n\n");
	fprintf(stderr,"input \033[33m\033[1m \\send [-f] [ID] 'filename' \033[0mto send a file to a specific ID. Make sure you have -f before the filename\n\n");
	fprintf(stderr,"input \033[33m\033[1m \\list \033[0mto ask a list of ID which has enrolled on the Chatroom.\n\n");
	fprintf(stderr,"input \033[33m\033[1m \\logout \033[0mto logout and go back to initial login menu.\n\n");				

				/*
				fprintf(stderr, "** \\list [-a]\n"
								"      List online users\n"
								"      List all users if [-a] specified\n");
				fprintf(stderr, "** \\send -m [message] [username]\n"
								"      Send message\n");
				fprintf(stderr, "** \\send -f [path] [username]\n"
								"      Send file\n");
				*/			
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
			if (state == ::ONLINE){
				showOnlinePage();
				return ;
			}

			fprintf(stderr, "refresh chat room\n");
			/* TODO question: how do we refresh in chat room ? */

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
			
			fprintf(stderr,"Please input your ID or input \033[33m\033[1m\\return \033[0m to exit\n");
			fprintf(stderr,"Input your username: ");
			char line[64];
			char ID[32];
			fgets(line, 64, stdin);
			sscanf(line, "%s", ID);
			if(!strcmp(ID,"\\return")){
				refresh() ;
				return  ;
			}

			char* password;

			string savedPasswordPath = savedPasswordFolder + string(ID);
			FILE *fp = fopen(savedPasswordPath.c_str(), "r");
			bool passwordFileAccessible = (fp != NULL);
			if (passwordFileAccessible) {
				fprintf(stderr,"\033[33m\033[5mfast login\033[0m\n");
				if (fscanf(fp, "%s", storedPassword) == EOF) storedPassword[0] = '\0';
				password = storedPassword;
				fclose(fp);
			} else {
				/*perror(savedPasswordPath.c_str());*/
				fprintf(stderr,"Please key in your password or input \033[33m\033[1m\\return \033[0m to exit\n");
				password = getpass("Input your password: ");
				if (!strcmp(password,"\\return")){
					refresh();
					return;
				}
			}
			
			Command command = ::login;

			send(connFd, &command, sizeof(int), 0);
			int IDLen = strlen(ID);
			int passwordLen = strlen(password) ;
			send(connFd, &IDLen, sizeof(int), 0);
			send(connFd, ID, IDLen, 0);
			send(connFd, &passwordLen, sizeof(int), 0);
			send(connFd, password, passwordLen, 0);
			LoginResult result = Uninitialized;
			recv(connFd, &result, sizeof(int), 0);

			if (result == Login) {
				fprintf(stderr, GRN "=> login successful\n" RESET);
				setUsername(string(ID));
				setState(::ONLINE);
				if (!passwordFileAccessible) {
					savePassword(savedPasswordPath.c_str(), password);
				}
				refresh();
				return ;
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
				/* perror(path.c_str()); */
				return;
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

		void list()
		{
			Command command = ::listUsers ;
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
				int usernameLen ;
				char username[32] ;
				recv(connFd, &usernameLen, sizeof(int), 0);
				recv(connFd, username, usernameLen, 0);
				username[usernameLen] = '\0' ;
				fprintf(stderr,"* %s\n",username);
			}
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

			send(connFd, &targetLen, sizeof(int), 0);
			send(connFd, target.c_str(), targetLen, 0);

			send(connFd, &messageLen, sizeof(int), 0);
			send(connFd, message.c_str(), messageLen, 0);
		}

		void sendFile(string target, string arg)
		{
			int pathLen = arg.length();
			char *path = (char *)malloc(pathLen);
			strcpy(path, arg.c_str());

			FILE *fp = fopen(path, "rb");
			if (fp == NULL) {
				perror(path);
				free(path);
				return;
			}

			fseek(fp, 0L, SEEK_END);
			size_t sz = ftell(fp);

			if (sz > 20 * MB) {
				fprintf(stderr, "%s: file size too large ... no more than 20 MB.\n", path);
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
			
			send(connFd, &targetLen, sizeof(int), 0);
			send(connFd, target.c_str(), targetLen, 0);

			send(connFd, &filenameLen, sizeof(int), 0);
			send(connFd, filename, filenameLen, 0);

			char buf[IOBufSize];
			int32_t size;

			send(connFd, &sz, sizeof(size_t), 0);

			while (sz != 0) {
				size = (sz < IOBufSize) ? sz: IOBufSize;
				fread(buf, 1, size, fp);
				send(connFd, &size, 4, 0);
				send(connFd, buf, size, 0);
				sz -= size;
			}

			fclose(fp); free(path); free(ts1); free(ts2);
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

		void history()
		{
			Command command = ::history ;
			assert(send(connFd, &command, sizeof(int), 0) == sizeof(int));
			int usernameLen = username.length();
			send(connFd, &usernameLen, sizeof(int), 0);
			send(connFd, username.c_str(), usernameLen, 0);

			int lineCount = -1;
			char line[1000];
			char fromUserName[64];
			char targetUserName[64];
			char content[256];
			char time_cstr[32];
			int type;

			recv(connFd, &lineCount, sizeof(int), 0);
			const char *color = "\033[36m\033[1m";
			while (lineCount--) {
				int L = -1;
				recv(connFd, &L, sizeof(int), 0);
				recv(connFd, line, L, 0);
				line[L] = '\0';
				
				type = Zero;
				
				sscanf(line, "%s%s%s%d%s", fromUserName, targetUserName, content, &type, time_cstr);
				if (type != Message && type != File) {
					fprintf(stderr, "Unknown type %d\n", type);
					continue;
				}
				
				int isFile = (type == File) ? 1: 0;

				fprintf(stderr, "%s%s%s %s %s%s%s %s\t%s\n",
					color,
					fromUserName,
					reset,
					arrow[isFile],
					color,
					targetUserName,
					reset,
					content,
					time_cstr
				);
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
		
	private:
		char storedPassword[PasswordBuffer];
		int connFd;
		State state;
		string username;
		SignUpHelper signUpHelper;
		
		void promptReturningToHomePage() {
			fprintf(stderr, "returning to home page ...\n%s\n", del);
		}
		void promptStateIncorrect() {
			fprintf(stderr, "state incorrect\n");
		}

		void showHomePage()
		{
			//fprintf(stderr, "Welcome to Home Page\n");
			//fprintf(stderr, "Options:\n");
			//fprintf(stderr, "** Sign Up\n");
			//fprintf(stderr, "** Log In\n");
			//fprintf(stderr, "**  Quit \n");
			fprintf(stderr,"\t*===========================*\n");
			fprintf(stderr,"\t*=                         =*\n");
			fprintf(stderr,"\t*= \033[5mWelcome to Chatroom 1.0 \033[0m=*\n");	
			fprintf(stderr,"\t*=                         =*\n");
			fprintf(stderr,"\t*===========================*\n\n");
			fprintf(stderr,"\tStart with an instruction\n");
			fprintf(stderr,"\t\033[33m\033[1m\\login     \\sign-up     \\quit \033[0m\n");
		}

		void showOnlinePage()
		{
			printf("\n\tHello \033[32m\033[1m\033[5m%s\033[0m >< How are you ?\n", username.c_str());
			printf("\t\033[33m\033[1m\\send     \\list     \\logout    \\help \033[0m\n");		
		}
};

const string CommandHelper::strEmpty = "[Empty]";
const string CommandHelper::strHidden = "[Hidden]";
const string CommandHelper::savedPasswordFolder = "../data/client/pass/";

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
