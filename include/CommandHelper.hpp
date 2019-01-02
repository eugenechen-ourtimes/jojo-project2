#ifndef _COMMANDHELPER_HPP_
#define _COMMANDHELPER_HPP_

#include <string>
#include "State.hpp"
#include "SignUpHelper.hpp"
using namespace std;
class CommandHelper {
	public:
		static const char *version;
		static const char *arrow[2];
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

		CommandHelper(int connFd, State state);
		void help();
		void refresh();
		void signUp();
		void login();
		void quit();
		void inputUsername(string arg);
		void inputPassword();
		void confirmPassword();
		void cancelSignUp();
		void createAccount();
		void sendData(string option, string targetUserName, string content);
		void list();
		void history(const char *arg);
		void showDownloadList(const char *arg);
		void downloadRequest(string filename, string timeStr);
		void logout();
		void setState(State state);
		State getState();
		void setUsername(string username);
		string getUsername();
		void promptStateIncorrect();

	private:
		static const int PasswordBuffer = 1024;
		char storedPassword[PasswordBuffer];
		int connFd;
		State state;
		string username;
		SignUpHelper signUpHelper;

		void promptReturningToHomePage();
		void showHomePage();
		void showOnlinePage();

		void showLocalCommands();
		void savePassword(string path, const char *password);
		void sendMessage(string target, string message);
		void sendFile(string target, string arg);
};
#endif