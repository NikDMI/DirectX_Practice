#pragma once
#include <wrl.h>
#include <string>

using Microsoft::WRL::ComPtr;

class Exception :public std::exception {
public:
	Exception() :Exception("Exception") {}
	Exception(std::string info):std::exception(info.c_str()) {
		MessageBox(NULL, L"", L"Exception", MB_OK | MB_ICONERROR);//show text of exception
	}
};

inline void ShowDebugMessage(const wchar_t* message) {
	MessageBox(NULL, message, L"Debug", MB_OK | MB_ICONERROR);
}
