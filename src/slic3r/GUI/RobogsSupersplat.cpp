#include "RobogsSupersplat.hpp"

#include <wx/uri.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>
#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/statline.h>
#include <wx/filedlg.h>
#include <string>
#include <wx/weakref.h>
#include <wx/app.h>
#include <wx/event.h>
#include <wx/artprov.h>

#ifdef __WXMSW__
#include <Windows.h>
#include <wrl.h>      
#include "WebView2.h"   
#include "WebView2EnvironmentOptions.h"
using Microsoft::WRL::ComPtr;
#endif

#ifndef SUPERSPLAT_DEV_URL_STR
#define SUPERSPLAT_DEV_URL_STR "https://superspl.at/editor"
#endif

//namespace Slic3r {
//    namespace GUI {
//
//#ifdef __WXMSW__
//        // 把 WebView2 指针藏到 .cpp（头文件不露出 WebView2 头）
//        struct RobogsSupersplat::WebView2Ptrs {
//            ComPtr<ICoreWebView2Environment> env;
//            ComPtr<ICoreWebView2Controller>  controller;
//            ComPtr<ICoreWebView2>            webview;
//        };
//#endif
//
//        // ---------------- UI ----------------
//
//        RobogsSupersplat::RobogsSupersplat(wxWindow* parent)
//            : wxPanel(parent, wxID_ANY)
//        {
//            initUI();
//        }
//
//        void RobogsSupersplat::initUI()
//        {
//            const int PAD = 8;
//            auto* main_sizer = new wxBoxSizer(wxVERTICAL);
//
//            // 顶部工具栏：Open
//            auto* top = new wxBoxSizer(wxHORIZONTAL);
//            m_open = new Button(this, _L("Open SuperSplat"));
//#if !__has_include("slic3r/GUI/Widgets/Button.hpp")
//            m_open->SetMinSize(wxSize(160, 38));
//#endif
//            top->Add(m_open, 0, wxALL | wxALIGN_CENTER_VERTICAL, PAD);
//            top->AddStretchSpacer();
//            main_sizer->Add(top, 0, wxEXPAND);
//            main_sizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, PAD);
//
//            // 下半：宿主面板
//            m_view_host = new wxPanel(this, wxID_ANY);
//            m_view_host->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
//            m_view_host->SetSizer(new wxBoxSizer(wxVERTICAL));
//            main_sizer->Add(m_view_host, 1, wxEXPAND | wxALL, PAD);
//
//#ifdef __WXMSW__
//            m_hwndHost = (void*)m_view_host->GetHandle(); // HWND
//            m_wv = new WebView2Ptrs();                    // 延迟创建 env/controller/webview
//#endif
//
//            // 事件
//            m_open->Bind(wxEVT_BUTTON, &RobogsSupersplat::onOpen, this);
//            m_view_host->Bind(wxEVT_SIZE, &RobogsSupersplat::onHostResize, this);
//
//            SetSizer(main_sizer);
//            Layout();
//        }
//
//        void RobogsSupersplat::onOpen(wxCommandEvent&)
//        {
//#ifdef __WXMSW__
//            ensureWebView2();
//            if (m_edgeReady && m_wv && m_wv->webview) {
//                m_wv->webview->Navigate(L"" SUPERSPLAT_DEV_URL_STR);
//            }
//            else {
//                wxLogMessage("Initializing WebView2 runtime...");
//            }
//#else
//            wxLogWarning("WebView2 embedding is only implemented on Windows.");
//#endif
//        }
//
//        void RobogsSupersplat::onHostResize(wxSizeEvent& e)
//        {
//            e.Skip();
//#ifdef __WXMSW__
//            resizeWebView2();
//#endif
//        }
//
//#ifdef __WXMSW__
//
//        // 创建 WebView2（异步）
//        void RobogsSupersplat::ensureWebView2()
//        {
//            if (m_edgeReady || !m_hwndHost) return;
//
//            // 初始化 COM（STA）。若模式已设则忽略返回值。
//            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
//
//            // userDataFolder：放到用户本地数据目录，避免 Program Files 权限问题
//            const wxString ud = wxStandardPaths::Get().GetUserLocalDataDir() + "\\WebView2";
//            const std::wstring userDataFolder = ud.ToStdWstring();
//
//            // 运行时保险丝（可选）：若同进程存在 CEF，直接告警返回，避免已知冲突
//            if (GetModuleHandleW(L"libcef.dll") || GetModuleHandleW(L"chrome_elf.dll")) {
//                wxLogError("Detected CEF modules in process. Do NOT mix CEF and WebView2 in the same process.");
//                return;
//            }
//
//            // 创建环境（使用系统 Edge WebView2 Runtime）
//            HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
//                /*browserExecutableFolder*/ nullptr,
//                /*userDataFolder*/          userDataFolder.c_str(),
//                /*options*/                 nullptr,
//                Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
//                    [this](HRESULT res, ICoreWebView2Environment* env) -> HRESULT
//                    {
//                        if (FAILED(res) || !env) {
//                            wxLogError("WebView2 environment creation failed: 0x%08X", (unsigned)res);
//                            return S_OK;
//                        }
//                        m_wv->env = env;
//
//                        HWND hwnd = static_cast<HWND>(m_hwndHost);
//                        return env->CreateCoreWebView2Controller(
//                            hwnd,
//                            Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
//                                [this, hwnd](HRESULT res2, ICoreWebView2Controller* controller) -> HRESULT
//                                {
//                                    if (FAILED(res2) || !controller) {
//                                        wxLogError("WebView2 controller creation failed: 0x%08X", (unsigned)res2);
//                                        return S_OK;
//                                    }
//                                    m_wv->controller = controller;
//
//                                    // 获取 CoreWebView2 对象
//                                    controller->get_CoreWebView2(m_wv->webview.GetAddressOf());
//
//                                    // 尺寸与位置
//                                    resizeWebView2();
//
//                                    // 基础事件：禁止外部弹窗（target=_blank）
//                                    if (m_wv->webview) {
//                                        m_wv->webview->add_NewWindowRequested(
//                                            Microsoft::WRL::Callback<ICoreWebView2NewWindowRequestedEventHandler>(
//                                                [this](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT
//                                                {
//                                                    args->put_Handled(TRUE);
//                                                    LPWSTR uri = nullptr;
//                                                    if (SUCCEEDED(args->get_Uri(&uri)) && uri && m_wv && m_wv->webview) {
//                                                        m_wv->webview->Navigate(uri);
//                                                    }
//                                                    if (uri) CoTaskMemFree(uri);
//                                                    return S_OK;
//                                                }
//                                            ).Get(), nullptr);
//
//                                        // 可选：只允许 localhost（示例）
//                                        m_wv->webview->add_NavigationStarting(
//                                            Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
//                                                [](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT
//                                                {
//                                                    LPWSTR uri = nullptr;
//                                                    if (SUCCEEDED(args->get_Uri(&uri)) && uri) {
//                                                        std::wstring u(uri);
//                                                        CoTaskMemFree(uri);
//                                                        const bool ok = u.rfind(L"http://localhost:", 0) == 0
//                                                            || u.rfind(L"http://127.0.0w.1:", 0) == 0;
//                                                        if (!ok) args->put_Cancel(TRUE);
//                                                    }
//                                                    return S_OK;
//                                                }
//                                            ).Get(), nullptr);
//                                    }
//
//                                    m_edgeReady = true;
//                                    return S_OK;
//                                }
//                            ).Get());
//                    }
//                ).Get());
//
//            if (FAILED(hr)) {
//                wxLogError("CreateCoreWebView2EnvironmentWithOptions failed: 0x%08X", (unsigned)hr);
//            }
//        }
//
//        void RobogsSupersplat::resizeWebView2()
//        {
//            if (!m_wv || !m_wv->controller) return;
//            HWND hwnd = static_cast<HWND>(m_hwndHost);
//            RECT rc{}; ::GetClientRect(hwnd, &rc);
//            RECT bounds{ 0, 0, rc.right - rc.left, rc.bottom - rc.top };
//            m_wv->controller->put_Bounds(bounds);
//        }
//
//#endif // __WXMSW__
//
//    } // namespace GUI
//} // namespace Slic3r


#ifndef DEFAULT_HOMEPAGE_URL_STR
#define DEFAULT_HOMEPAGE_URL_STR "https://www.bing.com/"
#endif
#ifndef DEFAULT_SEARCH_PREFIX_STR
#define DEFAULT_SEARCH_PREFIX_STR "https://www.bing.com/search?q="
#endif


namespace Slic3r {
    namespace GUI {

#ifdef __WXMSW__
        // ---------------- WebView2 Host：只持有 COM 指针与 HWND，不碰 wx ----------------
        struct RobogsSupersplat::Wv2Host {
            ComPtr<ICoreWebView2Environment> env;
            ComPtr<ICoreWebView2Controller>  controller;
            ComPtr<ICoreWebView2>            webview;
            HWND hwnd{ nullptr };

            std::atomic<bool> creating{ false };
            std::atomic<bool> ready{ false };
            std::wstring      pending_url;   // 创建完成后要导航的 URL
            EventRegistrationToken ev_newWindow{};
            std::wstring          script_id;

            // 在 Wv2Host::close() 里做清理（放在 Reset 前后都行）
            void close() {
                if (webview) {
                    if (ev_newWindow.value) {
                        webview->remove_NewWindowRequested(ev_newWindow);
                        ev_newWindow.value = 0;
                    }
                    if (!script_id.empty()) {
                        webview->RemoveScriptToExecuteOnDocumentCreated(script_id.c_str());
                        script_id.clear();
                    }
                }
                if (controller) { controller->Close(); controller.Reset(); }
                webview.Reset();
                env.Reset();
                ready = false;
                creating = false;
                pending_url.clear();
            }
        };
#endif

        // ---------------- UI ----------------

        RobogsSupersplat::RobogsSupersplat(wxWindow* parent)
            : wxPanel(parent, wxID_ANY)
        {
            initUI();
        }

        RobogsSupersplat::~RobogsSupersplat()
        {
            m_disposed = true;

#ifdef __WXMSW__
            // 先解绑/停止，再关闭 WebView2
            if (m_view_host) {
                m_view_host->Unbind(wxEVT_SIZE, &RobogsSupersplat::onHostResize, this);
                m_view_host->Unbind(wxEVT_DESTROY, &RobogsSupersplat::onHostDestroy, this);
            }
            if (m_wv2) {
                m_wv2->close();
                m_wv2.reset();
            }
#endif
        }

        void RobogsSupersplat::initUI()
        {
            const int PAD = 6;
            auto* main = new wxBoxSizer(wxVERTICAL);

            // 顶部工具条
            auto* bar = new wxBoxSizer(wxHORIZONTAL);
            m_back = new wxButton(this, wxID_ANY);
            m_forward = new wxButton(this, wxID_ANY);
            m_reload = new wxButton(this, wxID_ANY);
            m_home = new wxButton(this, wxID_ANY);
            m_addr = new wxTextCtrl(this, wxID_ANY, DEFAULT_HOMEPAGE_URL_STR,
                wxDefaultPosition, wxSize(420, -1),
                wxTE_PROCESS_ENTER);
            auto* go = new wxButton(this, wxID_ANY);
            m_open = new wxButton(this, wxID_ANY, "SuperSplat");

            auto set_icon = [](wxButton* btn,
                const wxArtID& primary,
                const wxArtID& fallback,
                const wxString& tooltip,
                const wxSize& size = wxSize(16, 16))
                {
                    if (!btn) return;
                    wxBitmap bmp = wxArtProvider::GetBitmap(primary, wxART_BUTTON, size);
                    if (!bmp.IsOk() && !fallback.empty())
                        bmp = wxArtProvider::GetBitmap(fallback, wxART_BUTTON, size);

#if wxCHECK_VERSION(3,1,0)
                    if (bmp.IsOk()) {
                        btn->SetBitmap(bmp);
                        btn->SetBitmapPosition(wxLEFT);
                        btn->SetBitmapMargins(4, 0);
                    }
#else
                    if (bmp.IsOk()) btn->SetBitmapLabel(bmp); // 兼容老 wx
#endif
                    if (!tooltip.empty()) btn->SetToolTip(tooltip);
                };

            // 这里直接替换你原先的 5 行 set_icon 调用 —— 不再使用 wxART_REFRESH / wxART_HOME
            set_icon(m_back, wxART_GO_BACK, wxART_UNDO, _("返回"));
            set_icon(m_forward, wxART_GO_FORWARD, wxART_REDO, _("前进"));
            set_icon(m_reload, wxART_REDO, wxART_REDO, _("刷新"));  // ← 原 wxART_REFRESH 改为 wxART_REDO
            set_icon(m_home, wxART_GO_UP, wxART_GO_UP, _("主页"));    // ← 原 wxART_HOME 改为 wxART_GO_UP
            set_icon(go, wxART_FIND, wxART_FIND, _("搜索"));

			// 设置按钮最小尺寸
            const wxSize btn(42, 30);
            m_back->SetMinSize(btn); m_forward->SetMinSize(btn); m_reload->SetMinSize(btn);
            m_home->SetMinSize(wxSize(72, 30));
            go->SetMinSize(wxSize(60, 30));
            m_open->SetMinSize(wxSize(150, 30));

            bar->Add(m_back, 0, wxALL | wxALIGN_CENTER_VERTICAL, PAD);
            bar->Add(m_forward, 0, wxTOP | wxBOTTOM | wxRIGHT | wxALIGN_CENTER_VERTICAL, PAD);
            bar->Add(m_reload, 0, wxTOP | wxBOTTOM | wxRIGHT | wxALIGN_CENTER_VERTICAL, PAD);
            bar->Add(m_home, 0, wxTOP | wxBOTTOM | wxRIGHT | wxALIGN_CENTER_VERTICAL, PAD);
            bar->Add(m_addr, 1, wxTOP | wxBOTTOM | wxRIGHT | wxALIGN_CENTER_VERTICAL, PAD);
            bar->Add(go, 0, wxTOP | wxBOTTOM | wxRIGHT | wxALIGN_CENTER_VERTICAL, PAD);
            bar->AddStretchSpacer();
            bar->Add(m_open, 0, wxALL | wxALIGN_CENTER_VERTICAL, PAD);

            main->Add(bar, 0, wxEXPAND);
            main->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, PAD);

            // 浏览区宿主
            m_view_host = new wxPanel(this, wxID_ANY);
            m_view_host->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
            auto* hostSizer = new wxBoxSizer(wxVERTICAL);
            m_view_host->SetSizer(hostSizer);
            main->Add(m_view_host, 1, wxEXPAND | wxALL, PAD);

#ifdef __WXMSW__
            m_hwndHost = (void*)m_view_host->GetHandle();
            m_wv2 = std::make_shared<Wv2Host>();
#endif

            // 绑定：全部用成员函数
            this->Bind(wxEVT_BUTTON, &RobogsSupersplat::onOpen, this, m_open->GetId());
            this->Bind(wxEVT_BUTTON, &RobogsSupersplat::onBack, this, m_back->GetId());
            this->Bind(wxEVT_BUTTON, &RobogsSupersplat::onForward, this, m_forward->GetId());
            this->Bind(wxEVT_BUTTON, &RobogsSupersplat::onReload, this, m_reload->GetId());
            this->Bind(wxEVT_BUTTON, &RobogsSupersplat::onHome, this, m_home->GetId());
            this->Bind(wxEVT_BUTTON, &RobogsSupersplat::onGo, this, go->GetId());
            this->Bind(wxEVT_TEXT_ENTER, &RobogsSupersplat::onAddrEnter, this, m_addr->GetId());

            m_view_host->Bind(wxEVT_SIZE, &RobogsSupersplat::onHostResize, this);
            m_view_host->Bind(wxEVT_DESTROY, &RobogsSupersplat::onHostDestroy, this);
            this->Bind(wxEVT_DESTROY, &RobogsSupersplat::onDestroy, this);

            SetSizer(main);
            Layout();

#ifdef __WXMSW__
            // 启动就创建并打开主页
            ensureWebView2AndMaybeNavigate(L"" DEFAULT_HOMEPAGE_URL_STR);
#endif
        }

        // ---------------- 事件处理 ----------------

        void RobogsSupersplat::onOpen(wxCommandEvent&)
        {
#ifdef __WXMSW__
            if (m_disposed) return;
            ensureWebView2AndMaybeNavigate(L"" SUPERSPLAT_DEV_URL_STR);
            if (m_addr) m_addr->ChangeValue(SUPERSPLAT_DEV_URL_STR);
#endif
        }
        void RobogsSupersplat::onGo(wxCommandEvent&) { if (!m_disposed) navigateText(m_addr ? m_addr->GetValue() : wxString()); }
        void RobogsSupersplat::onAddrEnter(wxCommandEvent&) { if (!m_disposed) navigateText(m_addr ? m_addr->GetValue() : wxString()); }

        void RobogsSupersplat::onBack(wxCommandEvent&)
        {
#ifdef __WXMSW__
            if (m_disposed || !m_wv2 || !m_wv2->ready || !m_wv2->webview) return;
            m_wv2->webview->GoBack();
#endif
        }
        void RobogsSupersplat::onForward(wxCommandEvent&)
        {
#ifdef __WXMSW__
            if (m_disposed || !m_wv2 || !m_wv2->ready || !m_wv2->webview) return;
            m_wv2->webview->GoForward();
#endif
        }
        void RobogsSupersplat::onReload(wxCommandEvent&)
        {
#ifdef __WXMSW__
            if (m_disposed || !m_wv2 || !m_wv2->ready || !m_wv2->webview) return;
            m_wv2->webview->Reload();
#endif
        }
        void RobogsSupersplat::onHome(wxCommandEvent&)
        {
#ifdef __WXMSW__
            if (m_disposed) return;
            ensureWebView2AndMaybeNavigate(L"" DEFAULT_HOMEPAGE_URL_STR);
            if (m_addr) m_addr->ChangeValue(DEFAULT_HOMEPAGE_URL_STR);
#endif
        }

        void RobogsSupersplat::onHostResize(wxSizeEvent& e)
        {
            e.Skip(); // 交给 sizer
#ifdef __WXMSW__
            if (m_disposed) return;
            resizeWebView2();
#endif
        }

        void RobogsSupersplat::onDestroy(wxWindowDestroyEvent& e)
        {
            m_disposed = true;
            e.Skip();
        }

        void RobogsSupersplat::onHostDestroy(wxWindowDestroyEvent& e)
        {
            m_disposed = true;
#ifdef __WXMSW__
            if (m_wv2) m_wv2->close();
#endif
            e.Skip();
        }

        // 统一的“地址栏文本 -> URL”解析与导航
        void RobogsSupersplat::navigateText(const wxString& text)
        {
#ifdef __WXMSW__
            if (m_disposed) return;

            wxString t = text;
            t.Trim(true).Trim(false);
            if (t.empty()) return;

            auto looks_like_url = [](const wxString& s)->bool {
                return s.StartsWith("http://") || s.StartsWith("https://") || s.StartsWith("file://")
                    || s.StartsWith("edge://") || s.StartsWith("about:");
                };

            wxString url;
            if (looks_like_url(t)) url = t;
            else if (t.StartsWith("www.")) url = "https://" + t;
            else url = DEFAULT_SEARCH_PREFIX_STR + t;

            if (m_wv2 && m_wv2->ready && m_wv2->webview) {
                m_wv2->webview->Navigate(url.ToStdWstring().c_str());
            }
            else {
                ensureWebView2AndMaybeNavigate(url.ToStdWstring());
            }
            if (m_addr) m_addr->ChangeValue(url);
#endif
        }

#ifdef __WXMSW__

        void RobogsSupersplat::ensureWebView2AndMaybeNavigate(const std::wstring& initial_url)
        {
#ifdef __WXMSW__
            if (m_disposed) return;
            if (!m_wv2) m_wv2 = std::make_shared<Wv2Host>();

            // 已就绪：直接导航
            if (m_wv2->ready && m_wv2->webview) {
                if (!initial_url.empty())
                    m_wv2->webview->Navigate(initial_url.c_str());
                return;
            }

            // 并发创建闸门
            bool expected = false;
            if (!m_wv2->creating.compare_exchange_strong(expected, true)) {
                if (!initial_url.empty()) m_wv2->pending_url = initial_url;
                return;
            }
            if (!initial_url.empty()) m_wv2->pending_url = initial_url;

            // 与 CEF 同进程会崩：硬拦
            if (GetModuleHandleW(L"libcef.dll")) {
                m_wv2->creating = false;
                wxLogError("检测到 CEF 已加载，请勿与 WebView2 同进程同时使用。");
                return;
            }

            HWND hwnd = static_cast<HWND>(m_hwndHost);
            if (!::IsWindow(hwnd)) { m_wv2->creating = false; return; }
            m_wv2->hwnd = hwnd;

            // COM 初始化（幂等）
            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

            // 运行开关（按需）
            const bool kDisableGpu = false;
            const bool kNoProxy = false;
            const bool kDevTools = false;

            // 用户数据目录
            const wxString ud = wxStandardPaths::Get().GetUserLocalDataDir() + "\\WebView2";
            const std::wstring userDataFolder = ud.ToStdWstring();

            // 浏览器附加参数
            std::wstring extraArgs;
            if (kDisableGpu) extraArgs += L" --disable-gpu --disable-gpu-compositing";
            if (kNoProxy)    extraArgs += L" --no-proxy-server";

            // 仅捕获 Host 的弱引用
            std::weak_ptr<Wv2Host> weak = m_wv2;

            // 版本日志（不用 SUCCEEDED）
            {
                LPWSTR ver = nullptr;
                HRESULT hr_ver = GetAvailableCoreWebView2BrowserVersionString(nullptr, &ver);
                if (hr_ver >= 0 && ver) {
                    wxLogMessage("WebView2 Runtime version: %ls", ver);
                    CoTaskMemFree(ver);
                }
                else {
                    wxLogWarning("未检测到 WebView2 Runtime 或无法读取版本。");
                }
            }

            // ====== 新 SDK（含 WebView2EnvironmentOptions.h） ======
            HRESULT hr = E_FAIL;
#if __has_include("WebView2EnvironmentOptions.h")
#include "WebView2EnvironmentOptions.h"
            {
                Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions> options
                    = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
                if (!extraArgs.empty())
                    options->put_AdditionalBrowserArguments(extraArgs.c_str());
                options->put_Language(L"zh-CN");

                hr = CreateCoreWebView2EnvironmentWithOptions(
                    nullptr, userDataFolder.c_str(), options.Get(),
                    Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                        [weak, kDevTools](HRESULT res, ICoreWebView2Environment* env) -> HRESULT
                        {
                            auto host = weak.lock();
                            if (!host) return S_OK;

                            if (res < 0 || !env) {
                                host->creating = false;
                                wxLogError("WebView2 环境创建失败: 0x%08X", (unsigned)res);
                                return S_OK;
                            }
                            host->env = env;

                            return env->CreateCoreWebView2Controller(
                                host->hwnd,
                                Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                                    [weak, kDevTools](HRESULT res2, ICoreWebView2Controller* controller) -> HRESULT
                                    {
                                        auto host = weak.lock();
                                        if (!host) { if (controller) controller->Close(); return S_OK; }

                                        if (res2 < 0 || !controller) {
                                            host->creating = false;
                                            wxLogError("WebView2 控制器创建失败: 0x%08X", (unsigned)res2);
                                            return S_OK;
                                        }

                                        // 保存到 ComPtr
                                        host->controller = controller;

                                        host->controller->get_CoreWebView2(host->webview.GetAddressOf());
                                        if (!host->webview) {
                                            host->creating = false;
                                            host->controller->Close();
                                            host->controller.Reset();
                                            wxLogError("WebView2 对象为空。");
                                            return S_OK;
                                        }

                                        // 可见 + 尺寸（DIP）+ 缩放
                                        host->controller->put_IsVisible(TRUE);
                                        if (::IsWindow(host->hwnd)) {
                                            RECT prc{}; ::GetClientRect(host->hwnd, &prc);
                                            const UINT dpi = GetDpiForWindow(host->hwnd);
                                            const double sc = dpi / 96.0;
                                            RECT dip{ 0, 0,
                                                (LONG)std::lround((prc.right - prc.left) / sc),
                                                (LONG)std::lround((prc.bottom - prc.top) / sc) };
                                            host->controller->put_Bounds(dip);

                                            Microsoft::WRL::ComPtr<ICoreWebView2Controller3> c3;
                                            if (host->controller.As(&c3) >= 0 && c3) c3->put_RasterizationScale(sc);
                                            Microsoft::WRL::ComPtr<ICoreWebView2Controller4> c4;
                                            if (host->controller.As(&c4) >= 0 && c4)
                                                c4->put_BoundsMode(COREWEBVIEW2_BOUNDS_MODE_USE_RASTERIZATION_SCALE);
                                            Microsoft::WRL::ComPtr<ICoreWebView2Controller2> c2;
                                            if (host->controller.As(&c2) >= 0 && c2) {
                                                COREWEBVIEW2_COLOR bg{ 255,255,255,255 };
                                                c2->put_DefaultBackgroundColor(bg);
                                            }
                                        }

                                        // === A) 把“新窗口”强制在当前 WebView 打开（必须保存 token）===
                                        EventRegistrationToken t{};
                                        HRESULT hr_add = host->webview->add_NewWindowRequested(
                                            Microsoft::WRL::Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                                                [weak](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT
                                                {
                                                    auto h = weak.lock();
                                                    if (!h) return S_OK;
                                                    args->put_Handled(TRUE); // 拦截新窗口

                                                    LPWSTR uri = nullptr;
                                                    HRESULT hr_uri = args->get_Uri(&uri);
                                                    if (hr_uri >= 0 && uri && uri[0] != L'\0') {
                                                        if (h->webview) h->webview->Navigate(uri);
                                                    }
                                                    else {
                                                        if (h->webview) h->webview->Navigate(L"about:blank");
                                                    }
                                                    if (uri) CoTaskMemFree(uri);
                                                    return S_OK;
                                                }
                                            ).Get(),
                                            &t // ★★ 不能传 nullptr
                                        );
                                        if (hr_add >= 0) host->ev_newWindow = t;

                                        // === B) 兜底：注入脚本劫持 window.open / target="_blank" ===
                                        static const wchar_t* kOpenHijackScript = LR"JS(
                                  (function(){
                                    try {
                                      const _open = window.open;
                                      Object.defineProperty(window,'open',{
                                        configurable:true,writable:true,
                                        value:function(url){
                                          try{ if(url && typeof url==='string'){ window.location.href = url; return null; } }
                                          catch(e){}
                                          return _open.apply(window, arguments);
                                        }
                                      });
                                      document.addEventListener('click', function(evt){
                                        var a = evt.target && evt.target.closest ? evt.target.closest('a[target="_blank"]') : null;
                                        if(!a || !a.href) return;
                                        evt.preventDefault();
                                        try{ window.location.href = a.href; }catch(e){}
                                      }, true);
                                      document.addEventListener('click', function(evt){
                                        var a = evt.target && evt.target.closest ? evt.target.closest('a') : null;
                                        if(!a || !a.href) return;
                                        if(evt.button===1 || evt.ctrlKey || evt.metaKey){
                                          evt.preventDefault();
                                          try{ window.location.href = a.href; }catch(e){}
                                        }
                                      }, true);
                                    } catch(e){}
                                  })();
                                )JS";
                                        Microsoft::WRL::ComPtr<ICoreWebView2> wv_local = host->webview;
                                        HRESULT hr_script =
                                            wv_local->AddScriptToExecuteOnDocumentCreated(
                                                kOpenHijackScript,
                                                Microsoft::WRL::Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                                                    [weak](HRESULT code, LPCWSTR id)->HRESULT {
                                                        auto h = weak.lock();
                                                        if (!h) return S_OK;
                                                        if (code >= 0 && id) h->script_id = id;
                                                        return S_OK;
                                                    }
                                                ).Get()
                                            );
                                        (void)hr_script;

                                        host->ready = true;
                                        host->creating = false;

                                        if (kDevTools) host->webview->OpenDevToolsWindow();

                                        if (!host->pending_url.empty()) {
                                            host->webview->Navigate(host->pending_url.c_str());
                                            host->pending_url.clear();
                                        }
                                        return S_OK;
                                    }
                                ).Get());
                        }
                    ).Get());
            }
#else
            // ====== 老 SDK（没有 WebView2EnvironmentOptions.h） ======
            {
                if (!extraArgs.empty())
                    SetEnvironmentVariableW(L"WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", extraArgs.c_str());

                hr = CreateCoreWebView2EnvironmentWithOptions(
                    nullptr, userDataFolder.c_str(), nullptr,
                    Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                        [weak, kDevTools](HRESULT res, ICoreWebView2Environment* env) -> HRESULT
                        {
                            auto host = weak.lock();
                            if (!host) return S_OK;

                            if (res < 0 || !env) {
                                host->creating = false;
                                wxLogError("WebView2 环境创建失败: 0x%08X", (unsigned)res);
                                return S_OK;
                            }
                            host->env = env;

                            return env->CreateCoreWebView2Controller(
                                host->hwnd,
                                Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                                    [weak, kDevTools](HRESULT res2, ICoreWebView2Controller* controller) -> HRESULT
                                    {
                                        auto host = weak.lock();
                                        if (!host) { if (controller) controller->Close(); return S_OK; }

                                        if (res2 < 0 || !controller) {
                                            host->creating = false;
                                            wxLogError("WebView2 控制器创建失败: 0x%08X", (unsigned)res2);
                                            return S_OK;
                                        }

                                        host->controller = controller;
                                        host->controller->get_CoreWebView2(host->webview.GetAddressOf());
                                        if (!host->webview) {
                                            host->creating = false;
                                            host->controller->Close();
                                            host->controller.Reset();
                                            wxLogError("WebView2 对象为空。");
                                            return S_OK;
                                        }

                                        // 可见 + 基本尺寸（老 SDK 无 c3/c4 就只设 Bounds）
                                        host->controller->put_IsVisible(TRUE);
                                        if (::IsWindow(host->hwnd)) {
                                            RECT prc{}; ::GetClientRect(host->hwnd, &prc);
                                            const UINT dpi = GetDpiForWindow(host->hwnd);
                                            const double sc = dpi / 96.0;
                                            RECT dip{ 0, 0,
                                                (LONG)std::lround((prc.right - prc.left) / sc),
                                                (LONG)std::lround((prc.bottom - prc.top) / sc) };
                                            host->controller->put_Bounds(dip);
                                        }

                                        // 同样注册事件（token 不能为 nullptr）
                                        EventRegistrationToken t{};
                                        HRESULT hr_add = host->webview->add_NewWindowRequested(
                                            Microsoft::WRL::Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                                                [weak](ICoreWebView2*, ICoreWebView2NewWindowRequestedEventArgs* args) -> HRESULT
                                                {
                                                    auto h = weak.lock();
                                                    if (!h) return S_OK;
                                                    args->put_Handled(TRUE);
                                                    LPWSTR uri = nullptr;
                                                    HRESULT hr_uri = args->get_Uri(&uri);
                                                    if (hr_uri >= 0 && uri && uri[0] != L'\0') {
                                                        if (h->webview) h->webview->Navigate(uri);
                                                    }
                                                    else {
                                                        if (h->webview) h->webview->Navigate(L"about:blank");
                                                    }
                                                    if (uri) CoTaskMemFree(uri);
                                                    return S_OK;
                                                }
                                            ).Get(),
                                            &t
                                        );
                                        if (hr_add >= 0) host->ev_newWindow = t;

                                        // 注入脚本兜底
                                        static const wchar_t* kOpenHijackScript = LR"( (function(){ try{
                                    const _open = window.open;
                                    Object.defineProperty(window,'open',{configurable:true,writable:true,
                                      value:function(url){ try{ if(url&&typeof url==='string'){ window.location.href=url; return null; } }catch(e){} return _open.apply(window,arguments);}
                                    });
                                    document.addEventListener('click',function(evt){
                                      var a = evt.target && evt.target.closest ? evt.target.closest('a[target="_blank"]') : null;
                                      if(!a||!a.href) return; evt.preventDefault(); try{ window.location.href=a.href; }catch(e){}
                                    }, true);
                                  }catch(e){} })(); )";
                                        Microsoft::WRL::ComPtr<ICoreWebView2> wv_local = host->webview;
                                        HRESULT hr_script =
                                            wv_local->AddScriptToExecuteOnDocumentCreated(
                                                kOpenHijackScript,
                                                Microsoft::WRL::Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                                                    [weak](HRESULT code, LPCWSTR id)->HRESULT {
                                                        auto h = weak.lock();
                                                        if (!h) return S_OK;
                                                        if (code >= 0 && id) h->script_id = id;
                                                        return S_OK;
                                                    }
                                                ).Get()
                                            );
                                        (void)hr_script;

                                        host->ready = true;
                                        host->creating = false;

                                        if (kDevTools) host->webview->OpenDevToolsWindow();

                                        if (!host->pending_url.empty()) {
                                            host->webview->Navigate(host->pending_url.c_str());
                                            host->pending_url.clear();
                                        }
                                        return S_OK;
                                    }
                                ).Get());
                        }
                    ).Get());
            }
#endif

            if (hr < 0) {
                m_wv2->creating = false;
                wxLogError("CreateCoreWebView2EnvironmentWithOptions 失败: 0x%08X", (unsigned)hr);
            }
#else
            (void)initial_url;
#endif
        }


        void RobogsSupersplat::resizeWebView2()
        {
#ifdef __WXMSW__
            if (m_disposed || !m_wv2 || !m_wv2->controller) return;
            HWND hwnd = static_cast<HWND>(m_hwndHost);
            if (!::IsWindow(hwnd)) return;

            RECT prc{}; ::GetClientRect(hwnd, &prc);
            const UINT dpi = GetDpiForWindow(hwnd);
            const double scale = dpi / 96.0;

            RECT dip{ 0, 0,
                (LONG)std::lround((prc.right - prc.left) / scale),
                (LONG)std::lround((prc.bottom - prc.top) / scale) };

            m_wv2->controller->put_Bounds(dip);

            Microsoft::WRL::ComPtr<ICoreWebView2Controller3> c3;
            if (SUCCEEDED(m_wv2->controller.As(&c3))) {
                c3->put_RasterizationScale(scale);
            }
            Microsoft::WRL::ComPtr<ICoreWebView2Controller4> c4;
            if (SUCCEEDED(m_wv2->controller.As(&c4))) {
                c4->put_BoundsMode(COREWEBVIEW2_BOUNDS_MODE_USE_RASTERIZATION_SCALE);
            }
#endif
        }

#endif // __WXMSW__

    } // namespace GUI
} // namespace Slic3r