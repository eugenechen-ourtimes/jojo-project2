#ifndef _COMMAND_HPP_
#define _COMMAND_HPP_

enum Command
{
	/* Home */
	signUp = 0x01,
	login = 0x02,
	quit = 0x03,

	/* Register */
	checkUsernameTaken = 0x04,
	createAccount = 0x05,
	cancelSignUp = 0x06,

	/* Online */
	sendMessage = 0x07,
	sendFile = 0x08,
	logout = 0x09,
	listUsers = 0x0a
};

#endif