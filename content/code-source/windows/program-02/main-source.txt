
// LESRO PROGRAM : S.Y.P. - Save Youtube Playlist
// Annotation: Bem útil, só que não, deve haver outra forma mais eficiente, eu acho
// Date: d27/m03/y2026


#define NOMINMAX

#include <windows.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <limits>


struct SavePlaylist
{
	HWND init_Wnd = NULL;
	int skip_Videos = 0;
	int save_Videos = 0;
	bool is_Course;
};



void my_SendCommand ( UINT which_key )
{

	INPUT inputs[2] = {};

	for ( int i = 0 ; i < 2 ; i++ )
	{
		inputs[i].type = INPUT_KEYBOARD;
		inputs[i].ki.wScan = 0;
		inputs[i].ki.dwExtraInfo = 0;
		inputs[i].ki.time = 0;
	}

	inputs[0].ki.wVk = which_key; inputs[0].ki.dwFlags = 0;
	inputs[1].ki.wVk = which_key; inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput( 2 , inputs , sizeof(INPUT) );

}


void my_SavePlaylist (SavePlaylist context)
{

	context.init_Wnd = GetForegroundWindow();

	// wait until my fingers release the shortcut keys.
	std::this_thread::sleep_for(std::chrono::seconds(2));

	for ( int video = 0 ; video < context.save_Videos ; video++ )
	{

			std::cout << "\n | Saving: #" << (video + 1) << " Video!" << std::endl;

			// Save Video
			if ( video >= context.skip_Videos )
			{

				std::cout << "\n   ";

				for ( int step = 0 ; step < 3 ; step++ )
				{

					int repeat = (3 - step);
					if (context.is_Course && step == 0)
					{
						repeat = 2;
					}

					for ( int tab = 0 ; tab < repeat ; tab++ )
					{
						// Simulate Tab
						while ( context.init_Wnd != (GetForegroundWindow()) )
						{ std::this_thread::sleep_for(std::chrono::milliseconds(200)); }
						my_SendCommand(VK_TAB);
						std::this_thread::sleep_for(std::chrono::milliseconds(300));						

					}

					std::cout << (3 - step) << "xTab ";
					
					// Simulate Enter
					while ( context.init_Wnd != (GetForegroundWindow()) )
					{ std::this_thread::sleep_for(std::chrono::milliseconds(200)); }
					my_SendCommand(VK_RETURN);
					std::this_thread::sleep_for(std::chrono::milliseconds(1800));

					std::cout << "1xEnter ";

				}

				std::cout << "\n" << std::endl;

			}
			// Skip Video
			else
			{

				std::cout << "\n   ";

				for ( int i = 0 ; i < 3 ; i++ )
				{
					// Simulate Tab
					while ( context.init_Wnd != (GetForegroundWindow()) )
					{ std::this_thread::sleep_for(std::chrono::milliseconds(200)); }
					my_SendCommand(VK_TAB);
					std::this_thread::sleep_for(std::chrono::milliseconds(180));

				}

				std::cout << "Video Skipped!";
				std::cout << "\n" << std::endl;
			}

	}

	std::cout << "\n | ### Playlist Saved Succesfuly!\n" << std::endl;

}







template< typename T >
void my_GetValue (T& data_member , const char* show_text)
{

	while (true)
	{
		std::cout << show_text;

		if (!(std::cin >> data_member))
		{
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			system("cls");
			continue;
		}
		else { break; }

	}

}





int main ()
{

	SavePlaylist context;

	std::cout << "\n See the number of the last video, not the number of videos of the playlist!\n" << std::endl;
	bool was_F6_Pressed = false;
	bool was_F7_Pressed = false;
	bool was_F8_Pressed = false;
	bool was_F9_Pressed = false;

	while (true)
	{

		bool is_Control_Shift_Pressed = ( GetAsyncKeyState(VK_CONTROL) & 0x8000 ) && ( GetAsyncKeyState(VK_SHIFT) & 0x8000 );



		if ( ( GetAsyncKeyState(VK_F6) & 0x8000 ) && is_Control_Shift_Pressed && !was_F6_Pressed)
		{
			was_F6_Pressed = true;
			my_GetValue<int>( context.skip_Videos , "\n | Number of Videos to Skip:" );
			std::cout << std::endl;
		}
		else if ( !( GetAsyncKeyState(VK_F6) & 0x8000 ) && was_F6_Pressed )
		{ was_F6_Pressed = false; }



		if ( ( GetAsyncKeyState(VK_F7) & 0x8000 ) && is_Control_Shift_Pressed && !was_F7_Pressed)
		{
			was_F7_Pressed = true;
			my_GetValue<int>( context.save_Videos , "\n | Number of Videos to Save: " );
			std::cout << std::endl;
		}
		else if ( !( GetAsyncKeyState(VK_F7) & 0x8000 ) && was_F7_Pressed )
		{ was_F7_Pressed = false; }



		if ( ( GetAsyncKeyState(VK_F8) & 0x8000 ) && is_Control_Shift_Pressed && !was_F8_Pressed)
		{
			was_F8_Pressed = true;
			my_GetValue<bool>( context.is_Course , "\n | The Playlist is a Course: " );
			std::cout << std::endl;
		}
		else if ( !( GetAsyncKeyState(VK_F8) & 0x8000 ) && was_F8_Pressed )
		{ was_F8_Pressed = false; }



		if ((GetAsyncKeyState(VK_F9) & 0x8000) && is_Control_Shift_Pressed && !was_F9_Pressed)
		{
			was_F9_Pressed = true;
			my_SavePlaylist(context);
		}
		else if (!(GetAsyncKeyState(VK_F9) & 0x8000) && was_F9_Pressed)
		{
			was_F9_Pressed = false;
		}





		std::this_thread::sleep_for(std::chrono::milliseconds(35));

	}






	return 0;
}