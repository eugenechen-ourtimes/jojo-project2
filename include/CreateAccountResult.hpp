#ifndef _CREATEACCOUNTRESULT_HPP_
#define _CREATEACCOUNTRESULT_HPP_

enum CreateAccountResult
{
	OK = 0x00,
	FullAccount = 0x01,
	UsernameTaken = 0x02,
	Undefined = 0x03,
	Incomplete = 0x04
};

#endif