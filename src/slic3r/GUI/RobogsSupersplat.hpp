#pragma once
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/filedlg.h>
#include "GUI_App.hpp"
#include <wx/mediactrl.h>
#include <atomic>
#include <wx/wx.h>
#include <wx/zipstrm.h>
#include <thread>
#include <wx/timer.h>
#include <memory>


//namespace Slic3r {
//    namespace GUI {
//
//        class RobogsSupersplat : public wxPanel {
//        public:
//            explicit RobogsSupersplat(wxWindow* parent);
//            ~RobogsSupersplat() override = default;
//
//            void initUI();
//
//        private:
//            void onOpen(wxCommandEvent& e);
//            void onHostResize(wxSizeEvent& e);
//
//            #ifdef __WXMSW__
//            void ensureWebView2();     // 创建 WebView2（异步）
//            void resizeWebView2();     // 调整 WebView2 尺寸
//            #endif
//
//        private:
//            Button* m_open{ nullptr };
//            wxPanel* m_view_host{ nullptr };
//
//            #ifdef __WXMSW__
//            void* m_hwndHost{ nullptr };   // HWND（void* 避免额外依赖）
//            bool       m_edgeReady{ false };
//            // WebView2 COM 智能指针使用 WRL
//            struct WebView2Ptrs;
//            WebView2Ptrs* m_wv{ nullptr };      // 前向声明 + PIMPL，避免在头文件引入 WebView2 头
//            #endif
//        };
//
//    } // namespace GUI
//}

class wxCommandEvent;
class wxSizeEvent;
class wxWindowDestroyEvent;

namespace Slic3r {
    namespace GUI {

        class RobogsSupersplat : public wxPanel {
        public:
            explicit RobogsSupersplat(wxWindow* parent);
            ~RobogsSupersplat() override;

            void initUI();

        private:
            // 顶部交互
            void onOpen(wxCommandEvent& e);        // 打开 SuperSplat (dev)
            void onGo(wxCommandEvent& e);          // 跳转到地址栏
            void onAddrEnter(wxCommandEvent& e);   // 地址栏回车
            void onBack(wxCommandEvent& e);
            void onForward(wxCommandEvent& e);
            void onReload(wxCommandEvent& e);
            void onHome(wxCommandEvent& e);

            // 布局/销毁
            void onHostResize(wxSizeEvent& e);
            void onDestroy(wxWindowDestroyEvent& e);
            void onHostDestroy(wxWindowDestroyEvent& e);

            // 工具
            void navigateText(const wxString& text);

#ifdef __WXMSW__
            void ensureWebView2AndMaybeNavigate(const std::wstring& initial_url);
            void resizeWebView2();
#endif

        private:
            // 顶部 UI
            wxButton* m_open{ nullptr };
            wxButton* m_back{ nullptr };
            wxButton* m_forward{ nullptr };
            wxButton* m_reload{ nullptr };
            wxButton* m_home{ nullptr };
            wxTextCtrl* m_addr{ nullptr };

            // 浏览区
            wxPanel* m_view_host{ nullptr };

            // 生命周期标记
            bool        m_disposed{ false };

#ifdef __WXMSW__
            void* m_hwndHost{ nullptr };   // HWND（void* 避免在头引 Windows 头）

            // 只在 .cpp 定义的 WebView2 宿主
            struct Wv2Host;
            std::shared_ptr<Wv2Host> m_wv2;
#endif
        };

    } // namespace GUI
} // namespace Slic3r