#include "color.hpp"
#include "CreateAccountResult.hpp"
#include "LoginResult.hpp"
#include "SignUpHelper.hpp"
#include <assert.h>

class CommandHelper {
	public:
		static const string strEmpty;	/* [Empty] */
		static const string strHidden;  /* [Hidden] */

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

		CommandHelper(int connFd, State state) {
			username = string("anonymous");
			this->connFd = connFd;
			this->state = state;
			showHomePage();
			signUpHelper.setFd(connFd);
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
				fprintf(stderr,"input \033[33m\033[1m \\send [ID] [-m] 'message' \033[0mto send a message to a specific ID. Message transmitting is the default action of \033[33m\033[1m\\send\033[0m, so [-m] is selectively input\n\n");
	fprintf(stderr,"input \033[33m\033[1m \\send [ID] [-f 'filename'] \033[0mto send a file to a specific ID. Make sure you have -f before the filename\n\n");
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
			::send(connFd, &command, sizeof(int), 0);
			::recv(connFd, &permit, sizeof(bool), 0);
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
			fprintf(stderr,"Please key in your password or input \033[33m\033[1m\\return \033[0m to exit\n");
			char* password;
			password = getpass("Input your password: ");
			if(!strcmp(password,"\\return")){
				refresh() ;
				return  ;
			}
			
			Command command = ::login;

			::send(connFd, &command, sizeof(int), 0);
			int IDLen = strlen(ID);
			int passwordLen = strlen(password) ;
			::send(connFd, &IDLen, sizeof(int), 0);
			::send(connFd, ID, IDLen, 0);
			::send(connFd, &passwordLen, sizeof(int), 0);
			::send(connFd, password, passwordLen, 0);
			LoginResult result = Uninitialized;
			recv(connFd, &result, sizeof(int), 0);

			if (result == Login) {
				fprintf(stderr, GRN "=> login successful\n" RESET);
				username = string(ID) ;
				state = ::ONLINE ;
				refresh();
				return ;
			}

			if (result == UsernameDoesNotExist) {
				fprintf(stderr, YEL "=> username does not exist\n" RESET);
				refresh();
				return;
			}

			if (result == PasswordIncorrect) {
				fprintf(stderr, RED "=> password incorrect\n" RESET);
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

			fprintf(stderr, "server seems to disconnect\n");
		}

		void quit()
		{
			if (state != ::HOME) {
				promptStateIncorrect();
				return;
			}

			Command command = ::quit;
			bool ack = false;
			::send(connFd, &command, sizeof(int), 0);
			::recv(connFd, &ack, sizeof(bool), 0);
			if (ack) exit(0);
		}

		void setUsername(string arg)
		{
			if (state != ::REGISTER) {
				promptStateIncorrect();
				return;
			}

			if (arg.empty()) {
				fprintf(stderr, "missing input string\n");
				return;
			}
			signUpHelper.handleInputUsername(arg);
			signUpHelper.refresh();
		}

		void setPassword()
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
			state = ::HOME;
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
			if (result == Incomplete) {
				fprintf(stderr, "please complete the form first\n");
				return;
			}

			if (result == UsernameTaken) {
				fprintf(stderr, "sorry, this account was taken by another client process\n%s\n", del);
				signUpHelper.refresh();
				return;
			}

			if (result == OK) {
				fprintf(stderr, "create account success\n");
				promptReturningToHomePage();
				state = ::HOME;
				signUpHelper.reset();
				showHomePage();
				return;
			} 

			if (result == FullAccount) {
				fprintf(stderr, "rejected (full account)\n");
				promptReturningToHomePage();
				state = ::HOME;
				signUpHelper.reset();
				showHomePage();
				return;
			}

			fprintf(stderr, "server seems to disconnect\n");
		}

		void list()
		{
			Command command = ::listUsers ;
			::send(connFd, &command, sizeof(int), 0);
			int size;
			bool permit = false;
			::recv(connFd, &permit, sizeof(bool), 0);
			if (!permit) {
				fprintf(stderr, "list request rejected (state is not ONLINE)\n");
				return;
			}

			::recv(connFd, &size, sizeof(int), 0);
			fprintf(stderr, "\033[32m\033[1m\033[45menrolled users:\033[0m\n\n");
			while(size--){
				int usernameLen ;
				char username[32] ;
				::recv(connFd, &usernameLen, sizeof(int), 0);
				::recv(connFd, username, usernameLen, 0);
				username[usernameLen] = '\0' ;
				fprintf(stderr,"* %s\n",username);
			}
			return ;
		}

		void send(int ret, string command, string ID, string message)
		{
			fprintf(stderr, "handle send\n");
			if (ret == 3 || command == "-m"){
				// message 
				sendMessage(ID, message);
			}
			else if(command == "-f"){
				// file X
			}
		}
		
		void sendMessage(string target, string message){
			Command command = ::sendMessage ;
			::send(connFd, &command, sizeof(int), 0);

			int usernameLen = username.length() ;
                        ::send(connFd, &usernameLen, sizeof(int), 0);
                        ::send(connFd, username.c_str(), usernameLen, 0);
			
			int targetLen = target.length() ;
                        ::send(connFd, &targetLen, sizeof(int), 0);
                        ::send(connFd, target.c_str(), targetLen, 0);
			
			int messageLen = message.length() ;
                        ::send(connFd, &messageLen, sizeof(int), 0);
                        ::send(connFd, message.c_str(), messageLen, 0);
			
			return ;
		}

		void logout()
		{
			Command command = ::logout ;
            assert(::send(connFd, &command, sizeof(int), 0) == sizeof(int));
			int usernameLen = username.length();
			assert(::send(connFd, &usernameLen, sizeof(int), 0) == sizeof(int));
			assert(::send(connFd, username.c_str(), usernameLen, 0) == usernameLen);
			bool ack = true;
			::recv(connFd, &ack, sizeof(bool), 0);
			if (ack) {
				state = ::HOME;
				refresh();
			} else {
				fprintf(stderr, "username incorrect\n");
			}
		}


		
	private:
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