#define NOMINMAX

#include <windows.h>
#include <iostream>
#include <string>
#include <limits>
#include <chrono>
#include <thread>


struct WhichGame
{
	int indice = 0;
	std::string game_name;
	std::wstring windowTitle;
	std::wstring link_Game;
};




void my_SendF11Command()
{

	INPUT inputs[2] = {};

	for (int i = 0; i < 2; i++)
	{
		inputs[i].type = INPUT_KEYBOARD;
		inputs[i].ki.wVk = VK_F11;
		inputs[i].ki.time = 0;
		inputs[i].ki.dwExtraInfo = 0;
	}

	inputs[0].ki.dwFlags = 0;
	inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(2, inputs, sizeof(INPUT));

}



int main()
{

	int games_counter = 1;
	int opt = 0;
	std::vector<WhichGame> context;

	context.push_back({ ++games_counter , "Chess"         , L"Chess.com"          , L"https://www.chess.com/de" });
	context.push_back({ ++games_counter , "Typing"        , L"Typing Test"        , L"https://www.typing.com/student/tests" });
	context.push_back({ ++games_counter , "Apple Game"    , L"Fruit Box"          , L"https://en.gamesaien.com/game/fruit_box/" });
	context.push_back({ ++games_counter , "Sudoku"        , L"Extrem Sudoku"      , L"https://sudoku.com/de/extreme/" });
	context.push_back({ ++games_counter , "Caca Palavras" , L"Caça Palavras"      , L"https://rachacuca.com.br/palavras/caca-palavras/" });
	context.push_back({ ++games_counter , "Jogo da Forca" , L"Jogo da Forca"      , L"https://rachacuca.com.br/palavras/jogo-da-forca/" });
	context.push_back({ ++games_counter , "Quebra Cabeca" , L"Quebra Cabeça"      , L"https://rachacuca.com.br/raciocinio/quebra-cabeca/" });
	context.push_back({ ++games_counter , "Mathe Lernen"  , L"mathe-lernen.net"   , L"https://mathe-lernen.net/" });


	do
	{

		std::cout << "\n | Choose a activity to realize:\n" << std::endl;

		for (int i = 0; i < context.size(); i++)
		{
			std::cout << "   [" << i + 1 << "] " << context[i].game_name << std::endl;
		}

		std::cout << "\n Chosen Option: ";

		if (!(std::cin >> opt))
		{
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			system("cls");
			std::cout << "\nInvalid input. Please enter a number between 1 and 8." << std::endl;
			continue;
		}

		system("cls");
		std::cout << "\nInvalid input. Please enter a number between 1 and 8." << std::endl;

	} while (opt < 1 || 8 < opt);



	std::wstring link = L" --incognito --new-window --directory-profile=\"Profile 1\" \"" + context[opt - 1].link_Game + L"\"";
	ShellExecute(NULL, L"open", L"chrome.exe", link.c_str(), NULL, SW_SHOWNORMAL);


	std::wstring Wnd_Title;
	auto startTime = std::chrono::steady_clock::now();
	bool found = false;

	do
	{

		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		HWND foreground_hWnd = GetForegroundWindow();
		wchar_t title[1024];
		GetWindowText(foreground_hWnd, title, 1024);
		Wnd_Title = title;

		if (Wnd_Title.find(context[opt - 1].windowTitle) != std::wstring::npos)
		{
			found = true;
			break;
		}

		auto currentTime = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);

		if (elapsed.count() >= 10)
		{
			break;
		}

	} while (true);

	if (found)
	{
		my_SendF11Command();
	}

	return 0;
}