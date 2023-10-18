
#if defined(_MSC_VER)		
	#pragma comment(lib, "user32.lib")
	#pragma comment(lib, "gdi32.lib")
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdio.h>

using int64 = signed long long int;
using uint = unsigned int;
using uint64 = unsigned long long int;
using isz = ptrdiff_t;
#define scast	static_cast



#include <format>

namespace sdf2
{
template <class T, ptrdiff_t N> inline constexpr ptrdiff_t strz_cap(const T (&)[N]) noexcept { return N-1; }

template<class T> inline isz strfz_len(const T* src);
template<> inline isz strfz_len(const char* src) { return strlen(src); }
template<> inline isz strfz_len(const wchar_t* src) { return wcslen(src); }

inline isz strf_wcs_from_mbs(wchar_t* dest, isz dest_capacity, const char* src, isz src_len) {
	isz copy_len = src_len;
    if (copy_len > dest_capacity) { copy_len = dest_capacity; }
	const int dest_size_with_null_char = dest_capacity+1;
	int result = MultiByteToWideChar(CP_UTF8, 0, src, copy_len, dest, dest_size_with_null_char);	
	dest[copy_len] = L'\0';
	return copy_len;
}
inline isz strf_mbs_from_wcs(char* dest, isz dest_capacity, const wchar_t* src, isz src_len) {
    isz copy_len = src_len;
    if (copy_len > dest_capacity) { copy_len = dest_capacity; }	
	const int dest_size_with_null_char = dest_capacity+1;
	int result = WideCharToMultiByte(CP_UTF8, 0, src, copy_len, dest, dest_size_with_null_char, nullptr, nullptr);	
	dest[copy_len] = '\0';
	return copy_len;
}

//-------------------
template <class... _Types>
_NODISCARD inline void print(const std::string_view _Fmt, const _Types&... _Args) {
	std::printf("%s", std::vformat(_Fmt, std::make_format_args(_Args...)).c_str());
}
template <class... _Types>
_NODISCARD inline void println(const std::string_view _Fmt, const _Types&... _Args) {
	std::printf("%s\n", std::vformat(_Fmt, std::make_format_args(_Args...)).c_str());
}
//-------------------

class std_console {
private:
	void* hconsole = nullptr;	
	const uint color_yellow = 6;

public:
	std_console() {}
	~std_console() { FreeConsole(); }

	bool create_window(const char* window_title, int xpos, int ypos, int width, int height) {
		if (!AllocConsole()) {
			MessageBoxW(nullptr, L"Couldn't create output console", L"Error", 0);
			return false; 
		}

		HWND console_window = GetConsoleWindow();
		MoveWindow(console_window, xpos, ypos, width, height, TRUE);

		wchar_t wconsole_title[256] = {};
		sdf2::strf_wcs_from_mbs(wconsole_title, sdf2::strz_cap(wconsole_title), window_title, sdf2::strfz_len(window_title));
		SetWindowTextW(console_window, wconsole_title);	

		// The freopen_s function closes the file currently associated with stream and reassigns stream to the file specified by path.
		freopen("conin$", "r", stdin);
		freopen("conout$", "w", stdout);
		freopen("conout$", "w", stderr);

		hconsole = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hconsole, color_yellow);
		return true;
	}

};

//#include <thread>
//#include <chrono>
void sys_sleep(uint dwMilliseconds) {
	Sleep(dwMilliseconds);
	//std::this_thread::sleep_for(std::chrono::milliseconds(dwMilliseconds));	
}

class game_timer_qpc {
public:

	float period() {
		int64 cnts_per_sec = 0;
		QueryPerformanceFrequency((LARGE_INTEGER*)&cnts_per_sec);
		// secs_per_cnt_qpc is 0.0000001f, 1e-07;
		float secs_per_cnt_qpc = 1.0f / scast<float>(cnts_per_sec);
		//println("{}", secs_per_cnt_qpc);
		return secs_per_cnt_qpc;
	}

	int64 get_time() {
		// Retrieves the current value of the performance counter, 
		// which is a high resolution (<1 micro second) time stamp.
		int64 prev_time_qpc = 0;
		QueryPerformanceCounter((LARGE_INTEGER*)&prev_time_qpc);
		return prev_time_qpc;
	}
};

}


using namespace sdf2;
//-------------------------

namespace wm_wnd2
{

class app_vars {
public:
	bool app_quit = false;
	bool app_active = true;
	bool app_minimized = false;
};
app_vars g_av;


bool winapp_init(WNDPROC m_wnd_proc, HINSTANCE m_hInstance, const wchar_t* m_app_class_name) {
	// register window class
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_OWNDC; //CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = m_wnd_proc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = m_hInstance;
	wcex.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
	wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	//wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.hbrBackground = CreateSolidBrush(RGB(25, 25, 112));
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = m_app_class_name;
	wcex.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&wcex)) {
		printf("Cannot Register WinApi Class.\n");
		return false;
	}
	return true;
}

bool winapp_create_window(HWND& m_hwnd, const char* wnd_title, int xpos, int ypos, int width, int height, bool fullscreen, bool borderless, 
	HINSTANCE m_hInstance, const wchar_t* m_app_class_name) {
	uint dwStyle = 0;
	uint dwExStyle = 0;
	dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
	dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

	int x, y, w, h;
	RECT window_rect = {0, 0, width, height};
	AdjustWindowRectEx(&window_rect, dwStyle, false, dwExStyle);
	x = xpos + window_rect.left;
	y = ypos + window_rect.top;
	w = window_rect.right - window_rect.left;
	h = window_rect.bottom - window_rect.top;	

	wchar_t wstr_wnd_title[256] = {};
	sdf2::strf_wcs_from_mbs(wstr_wnd_title, sdf2::strz_cap(wstr_wnd_title), wnd_title, sdf2::strfz_len(wnd_title));

	HWND handle_wnd = CreateWindowExW(dwExStyle,					
						m_app_class_name,			        
						wstr_wnd_title,	 		
						dwStyle,
						x, y, w, h,
						nullptr, nullptr, m_hInstance, nullptr);									

	if (!handle_wnd) {							
		printf("Cannot Create Window.\n");
		return false;
	}  
		
	//ShowWindow(handle_wnd, iCmdShow);
	ShowWindow(handle_wnd, SW_SHOW);   	
	UpdateWindow(handle_wnd); 
	SetForegroundWindow(handle_wnd);						
	SetFocus(handle_wnd);	

	m_hwnd = handle_wnd;
	return true;
}

void winapp_poll_events(MSG& m_msg) {
	if(PeekMessageW(&m_msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&m_msg);
		DispatchMessageW(&m_msg);
	}
}

bool winapp_is_quit_msg(const MSG& m_msg) {
	return (m_msg.message == WM_QUIT);
}

void winapp_deinit(HWND m_hwnd, HINSTANCE m_hInstance, const wchar_t* m_app_class_name) {
	DestroyWindow(m_hwnd);
	UnregisterClassW(m_app_class_name, m_hInstance);
}

}


using namespace wm_wnd2;


//------------------------------
//-wi

// NOTE:
// Search for "//-wi" for modifications specific to WickedEngine to base winapi template.
// Set WE_PATH below to Wicked engine path
// compile WickedEngine first in Release mode to get "WickedEngine_Windows.lib" in correct location

// DO NOT FORGET:
// Copy "shaders" folder from "WickedEngine/Template_Windows" to "test_we/x64/Release"

// CONFIGURE HERE FIRST
#define WE_PATH		"E:/nex/WickedEngine_test/"

#define WE_CONTENT_PATH  WE_PATH "Content/models/"
#define WE_SHADER_PATH  WE_PATH "WickedEngine/shaders/"

//E:/nex/WickedEngine_test/BUILD/x64/Release/WickedEngine_Windows.lib
#pragma comment(lib, WE_PATH "BUILD/x64/Release/WickedEngine_Windows.lib")

// Include engine headers:
#include "WickedEngine.h"
#include "wiScene_Components.h"

// Create the Wicked Engine application:
wi::Application application;
//------------------------------


//---------------------------------------------
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//======================================
// WinMain
//======================================

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
	BOOL dpi_success = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	std_console gconstd;
	gconstd.create_window("Std Console", 864, 0, 640, 640);
	printf("SysConsole Initialized.\n");
	println("Sample program");	  	

    //-------------------------
    int xpos = 10;
	int ypos = 50;
	int width = 800;
	int height = 450;
	const char* wnd_title = "Sample window";									

	const wchar_t* APP_CLASS_NAME = L"SAMPLE_APP_CLASS";
    HWND m_hwnd = nullptr;
    MSG m_msg = {};

    winapp_init(MainWndProc, hInstance, APP_CLASS_NAME);
	winapp_create_window(m_hwnd, wnd_title, xpos, ypos, width, height, false, false, hInstance, APP_CLASS_NAME);

	//------------------------------
	//-wi
	println("------------------------------");
	println("CHECK: Compile in Release mode.");
	println("CHECK: Set WE_PATH to WickedEngine path.");
	println("CHECK: If program window auto closes, compile again, this time compiled shaders will be found from previous run.");
	println("------------------------------");

	wchar_t w_lpCmdLine[256] = {};
	sdf2::strf_wcs_from_mbs(w_lpCmdLine, sdf2::strz_cap(w_lpCmdLine), szCmdLine, sdf2::strfz_len(szCmdLine));
	wi::arguments::Parse(w_lpCmdLine); // wants wchar_t from wWinMain, convert szCmdLine to wstring first.

	// Set shader source path.
	std::string CURR_SHADERPATH = WE_SHADER_PATH;
	println("Shader files source path: {}", CURR_SHADERPATH);
	wi::renderer::SetShaderSourcePath(CURR_SHADERPATH);
	// Set compiled shader binary path.
	wi::renderer::SetShaderPath(wi::helper::GetCurrentPath() + "/shaders/");

	// Initialize Wicked Engine after setting shader path.
	application.Initialize();
	// Assign window that you will render to:
	application.SetWindow(m_hwnd);
	// just show some basic info:
	application.infoDisplay.active = true;
	application.infoDisplay.watermark = true;
	application.infoDisplay.resolution = true;
	application.infoDisplay.fpsinfo = true;	
	
	//------------------------------

	//rs->init_r(hInstance, wnd1.hwnd, wnd1.width, wnd1.height, wnd1.fullscreen);
	//rs->setup();
	//---
	game_timer_qpc nw_timer;
	int64 prev_time_qpc = nw_timer.get_time();
    
	// program main loop
    while (!g_av.app_quit) {
		//nw_app.poll_events();
		winapp_poll_events(m_msg);		

		//if (nw_app.is_quit_msg()) { g_av.sys_quit(); }	
		if (winapp_is_quit_msg(m_msg)) { g_av.app_quit = true; }	

		if (g_av.app_active && !g_av.app_minimized) {
			int64 curr_time_qpc = nw_timer.get_time();
			float delta_time_qpc = (curr_time_qpc - prev_time_qpc)*nw_timer.period();					

			//process_input();
			//calculate_frame_stats();
			//rs->render(delta_time_qpc);
			//rs->swap_buffers();
			//------------------------------
			//-wi
			if (wi::input::Press(wi::input::KEYBOARD_BUTTON_SPACE)) { println("SPACEBAR Press"); } // You can check if a button is pressed or not (this only triggers once)
			if (wi::input::Down(wi::input::KEYBOARD_BUTTON_SPACE)) { println("SPACEBAR Down"); } // You can check if a button is pushed down or not (this triggers repeatedly)
			application.Run(); // run the update - render loop
			//------------------------------
    
			prev_time_qpc = curr_time_qpc;	

		} else {
    		sys_sleep(1);
			continue;
		} 
        
    } // while: not app_quit

	//rs->cleanup();
	//rs->deinit_r();

	winapp_deinit(m_hwnd, hInstance, APP_CLASS_NAME);
    
    return scast<int>(m_msg.wParam);
}


//======================================
// Window Procedure
//======================================

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {

    case WM_ACTIVATE:							
		g_av.app_active = scast<bool>(LOWORD(wParam) != WA_INACTIVE);
		g_av.app_minimized = scast<bool>(HIWORD(wParam));
		return 0;		

    case WM_CREATE:
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        PostQuitMessage(0);        
        return 0;

    case WM_QUIT:
        PostQuitMessage(0);        
        return 0;

    case WM_KEYDOWN:
		//gkeys[wParam] = true;
        switch (wParam) {
		case VK_ESCAPE:
            PostQuitMessage(0);
            return 0;		
        }
        return 0;								
	
	case WM_KEYUP:								
		//gkeys[wParam] = false;
		return 0;


	//-------------------------
	//-wi
    case WM_SIZE:

    case WM_DPICHANGED:
		if (application.is_window_active) {
			application.SetWindow(hWnd);
		}
        break;

	case WM_CHAR:
		switch (wParam) {
		case VK_BACK:
			wi::gui::TextInputField::DeleteFromInput();
			break;
		case VK_RETURN:
			break;
		default: {
			const wchar_t c = (const wchar_t)wParam;
			wi::gui::TextInputField::AddInput(c);
		}
		break;
		}
		break;

	case WM_INPUT:
		wi::input::rawinput::ParseMessage((void*)lParam);
		break;

	case WM_KILLFOCUS:
		application.is_window_active = false;
		break;
	case WM_SETFOCUS:
		application.is_window_active = true;
		break;

	//-------------------------


    default: break;
    } 
    
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
