// LESRO PROGRAM _ Open Split View Window
// Annotation: 


#include <windows.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <string>


void my_SendCommand_Function(UINT, UINT, UINT);
void my_SetLink_Function(const std::string&);

const std::string link_youtube = "https://www.youtube.com/feed/playlists";






int main()
{

	ShellExecute(NULL, L"open", L"chrome.exe", L" --new-window --profile-directory=\"Profile 1\"   \"https://translate.google.com/?hl=de&sl=de&tl=en&op=translate\" ", NULL, SW_SHOWMAXIMIZED);
	
	std::wstring wide_window_title;
	do
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		HWND hWnd = GetForegroundWindow();
		wchar_t title[1024];
		GetWindowTextW( hWnd , title , 1024 );
		wide_window_title = title;

	} while ( wide_window_title.find(L"Google Übersetzer") != std::wstring::npos);

	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	my_SendCommand_Function(VK_SHIFT, VK_MENU, 'N');
	my_SetLink_Function(link_youtube);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	my_SendCommand_Function(VK_CONTROL, 0, 'V');
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	my_SendCommand_Function(0, 0, VK_RETURN);
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	my_SendCommand_Function(0, 0, VK_F11);


	return 0;
}






void my_SendCommand_Function(UINT mod_01, UINT mod_02, UINT key_03)
{

	std::vector<INPUT> inputs(6);
	int i = 0;

	if (mod_01) { inputs[i].ki.wVk = mod_01; inputs[i].ki.dwFlags = 0; i++; }
	if (mod_02) { inputs[i].ki.wVk = mod_02; inputs[i].ki.dwFlags = 0; i++; }
	if (key_03) { inputs[i].ki.wVk = key_03; inputs[i].ki.dwFlags = 0; i++; }
	if (key_03) { inputs[i].ki.wVk = key_03; inputs[i].ki.dwFlags = KEYEVENTF_KEYUP; i++; }
	if (mod_02) { inputs[i].ki.wVk = mod_02; inputs[i].ki.dwFlags = KEYEVENTF_KEYUP; i++; }
	if (mod_01) { inputs[i].ki.wVk = mod_01; inputs[i].ki.dwFlags = KEYEVENTF_KEYUP; i++; }


	for (int counter = 0; counter < i; counter++)
	{
		inputs[counter].type = INPUT_KEYBOARD;
		inputs[counter].ki.wScan = 0;
		inputs[counter].ki.time = 0;
		inputs[counter].ki.dwExtraInfo = 0;
	}

	inputs.resize(i);
	SendInput(i, inputs.data(), sizeof(INPUT));

}



void my_SetLink_Function(const std::string& link)
{

	OpenClipboard(NULL);
	EmptyClipboard();
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, link.size() + 1);
	if (!hg) { CloseClipboard(); return; }
	memcpy(GlobalLock(hg), link.c_str(), link.size());
	GlobalUnlock(hg);
	SetClipboardData(CF_TEXT, hg);
	CloseClipboard();

}