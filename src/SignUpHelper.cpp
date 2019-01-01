#include "SignUpHelper.hpp"
#include "Command.hpp"
#include "color.hpp"
#include "CommandHelper.hpp"
#include <sys/socket.h>
static const string strEmpty = "[Empty]";
static const string strHidden = "[Hidden]";

bool SignUpHelper::passwordMatched() {
	return getPassword() == confirmPassword;
}

bool SignUpHelper::formComplete()
{
	return usernameValid() && passwordValid() && passwordMatched();
}

SignUpHelper::SignUpHelper() {
	connFd = 0;
}

void SignUpHelper::setFd(int connFd) {
	this->connFd = connFd;
}

void SignUpHelper::setConfirmPassword(string confirmPassword)
{
	this->confirmPassword = confirmPassword;
}

bool SignUpHelper::usernameValid()
{
	return (SignUpUtils::usernameValid() && !usernameTaken);
}

void SignUpHelper::reset()
{
	usernameTaken = false;
	setUsername("");
	setPassword("");
	setConfirmPassword("");
}

void SignUpHelper::handleInputUsername(string username)
{
	setUsername(username);
	usernameTaken = false;
	if (SignUpUtils::usernameValid()) {
		/* check if this username is taken */
		Command command = ::checkUsernameTaken;
		int usernameLen = getUsername().length();
		::send(connFd, &command, sizeof(int), 0);
		::send(connFd, &usernameLen, sizeof(int), 0);
		::send(connFd, getUsername().c_str(), usernameLen, 0);
		::recv(connFd, &usernameTaken, sizeof(bool), 0);
	} 
}

void SignUpHelper::handleInputPassword(string password)
{
	setPassword(password);
	setConfirmPassword("");
}

void SignUpHelper::handleConfirmPassword(string confirmPassword)
{
	setConfirmPassword(confirmPassword);
}

void SignUpHelper::cancel()
{
	Command command = ::cancelSignUp;
	bool ack = false;
	::send(connFd, &command, sizeof(int), 0);
	::recv(connFd, &ack, sizeof(bool), 0);
	if (!ack) {
		fprintf(stderr, "cancel error\n");
		exit(-1);
	}
}

CreateAccountResult SignUpHelper::createAccount()
{
	if (!formComplete()) {
		return Incomplete;
	}

	Command command = ::createAccount;
	int usernameLen = getUsername().length();
	int passwordLen = getPassword().length();
	::send(connFd, &command, sizeof(int), 0);

	::send(connFd, &usernameLen, sizeof(int), 0);
	::send(connFd, getUsername().c_str(), usernameLen, 0);
	
	::send(connFd, &passwordLen, sizeof(int), 0);
	::send(connFd, getPassword().c_str(), passwordLen, 0);
	CreateAccountResult result = Undefined;
	::recv(connFd, &result, sizeof(int), 0);
	
	if (result == UsernameTaken) {
		usernameTaken = true;
	}
	return result;
}

void SignUpHelper::refresh()
{
	string username = getUsername();
	string password = getPassword();

	if (!username.empty()) 
		fprintf(stderr, usernameValid() ? GRN: RED);
	fprintf(stderr, "    Username:     ");
	if (!username.empty())
		fprintf(stderr, RESET);

	fprintf(stderr, "%15s #Make sure Username is no more than %2d characters\n",
		username.empty() ? strEmpty.c_str(): username.c_str(),
		usernameMaxLength
		);


	if (!password.empty())
		fprintf(stderr, passwordValid() ? GRN: RED);
	fprintf(stderr, "    Password:     ");
	if (!password.empty())
		fprintf(stderr, RESET);

	fprintf(stderr, "%15s #Make sure Password is no less than %2d characters\n",
		password.empty() ? strEmpty.c_str(): strHidden.c_str(),
		passwordMinLength
		);


	if (!confirmPassword.empty())
		fprintf(stderr, passwordMatched() ? GRN: RED);
	fprintf(stderr, "    Confirm Password: ");
	if (!confirmPassword.empty())
		fprintf(stderr, RESET);

	fprintf(stderr, "%15s\n",
		confirmPassword.empty() ? strEmpty.c_str(): strHidden.c_str());

	if (usernameTooLong()) {
		fprintf(stderr, YEL "=> Username Too Long\n" RESET);
		return;
	}

	if (usernameTaken) {
		fprintf(stderr, YEL "=> Username Already Taken\n" RESET);
		return;
	}

	if (!usernameAlphaNumeric()) {
		fprintf(stderr, YEL "=> Username May Only Contain Letters(a-z) and Numbers(0-9)\n" RESET);
		return;
	}

	if (!password.empty() && passwordTooShort()) {
		fprintf(stderr, YEL "=> Password Too Short\n" RESET);
		return;
	}

	if (!password.empty() && !passwordAlphaNumeric()) {
		fprintf(stderr, YEL "=> Password May Only Contain Letters(a-z) and Numbers(0-9)\n" RESET);
		return;
	}

	if (!confirmPassword.empty() && !passwordMatched()) {
		fprintf(stderr, YEL "=> Password Mismatched\n" RESET);
		return;
	}

	if (passwordValid() && confirmPassword.empty()) {
		fprintf(stderr,"Please input \033[33m\033[1m \\confirm-password\033[0m before creating account.\n");
		return;
	}

	if (formComplete()) {
		#define COLOR "\033[36m\033[1m"
		fprintf(stderr, COLOR "=> Ready to create account \n" RESET);
		#undef COLOR
	}

	fprintf(stderr,"\n" "    " BOLD "One account. Meet all your friends at Chatroom\n" RESET);
	fprintf(stderr,     "    " BYEL "\\username [%s]   %s   %s   %s   %s   %s\n" RESET,
		"%s",
		CommandHelper::PASSWORD.c_str(),
		CommandHelper::CONFIRM_PASSWORD.c_str(),
		CommandHelper::CANCEL.c_str(),
		CommandHelper::CREATE_ACCOUNT.c_str(),
		CommandHelper::HELP.c_str()
		);
}
