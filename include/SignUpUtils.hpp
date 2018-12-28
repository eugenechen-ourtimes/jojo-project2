#ifndef _SIGNUPUTILS_HPP_
#define _SIGNUPUTILS_HPP_

#include <string>
using namespace std;
class SignUpUtils {
	private:
		string username;
		string password;

	public:

		static const int passwordMinLength = 5;
		static const int usernameMaxLength = 10;

		void setUsername(string username)
		{
			this->username = username;
		}

		void setPassword(string password)
		{
			this->password = password;
		}

		string getUsername()
		{
			return username;
		}

		string getPassword()
		{
			return password;
		}

		bool passwordTooShort() {
			return password.length() < passwordMinLength;
		}

		bool usernameTooLong() {
			return username.length() > usernameMaxLength;
		}

		bool usernameValid()
		{
			return !usernameTooLong() && !username.empty();
		}

		bool passwordValid()
		{
			return !passwordTooShort();
		}
};

#endif