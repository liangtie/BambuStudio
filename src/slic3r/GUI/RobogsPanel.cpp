#include "RobogsPanel.hpp"
#include <wx/uri.h>
#include <windows.h>
#include <winhttp.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>
#pragma comment(lib, "winhttp.lib")


namespace Slic3r {
    namespace GUI {

        RobogsPanel::RobogsPanel(wxWindow* parent)
            : wxPanel(parent, wxID_ANY)
        {
            initUI();           
        }

        void RobogsPanel::initUI()
        {
            // 创建主布局
            auto main_sizer = new wxBoxSizer(wxVERTICAL);

            // 顶部按钮行
            auto top_sizer = new wxBoxSizer(wxHORIZONTAL);
            m_btn_import_video = new Button(this, _L("导入视频"));
            m_btn_get_images = new Button(this, _L("提取图片"));
            m_btn_rebuild = new Button(this, _L("稀疏重建"));
            m_btn_train_gs = new Button(this, _L("训练GS"));
            m_btn_extract = new Button(this, _L("提取GS"));
            m_btn_download_gs = new Button(this, _L("下载GS"));
            m_btn_edit_gs = new Button(this, _L("编辑GS"));
            m_btn_train_mesh = new Button(this, _L("训练Mesh"));
            m_btn_render_mesh = new Button(this, _L("渲染Mesh"));
            m_btn_download_mesh = new Button(this, _L("下载Mesh"));
            m_btn_segment = new Button(this, _L("分割Mesh"));

            top_sizer->Add(m_btn_import_video, 0, wxALL, 5);
            top_sizer->Add(m_btn_get_images, 0, wxALL, 5);
            top_sizer->Add(m_btn_rebuild, 0, wxALL, 5);
            top_sizer->Add(m_btn_train_gs, 0, wxALL, 5);
            top_sizer->Add(m_btn_extract, 0, wxALL, 5);
            top_sizer->Add(m_btn_download_gs, 0, wxALL, 5);
            top_sizer->Add(m_btn_edit_gs, 0, wxALL, 5);
            top_sizer->Add(m_btn_train_mesh, 0, wxALL, 5);
            top_sizer->Add(m_btn_render_mesh, 0, wxALL, 5);
            top_sizer->Add(m_btn_download_mesh, 0, wxALL, 5);
            top_sizer->Add(m_btn_segment, 0, wxALL, 5);

            // 绑定导入视频按钮事件
            m_btn_import_video->Bind(wxEVT_BUTTON, &RobogsPanel::onImportVideo, this);
            m_btn_get_images->Bind(wxEVT_BUTTON, &RobogsPanel::OnGetImages, this);
            m_btn_rebuild->Bind(wxEVT_BUTTON, &RobogsPanel::onRebuild, this);
            m_btn_train_gs->Bind(wxEVT_BUTTON, &RobogsPanel::onTrainGS, this);
            m_btn_extract->Bind(wxEVT_BUTTON, &RobogsPanel::onExtract, this);
            m_btn_download_gs->Bind(wxEVT_BUTTON, &RobogsPanel::onDownload_gs, this);
            m_btn_edit_gs->Bind(wxEVT_BUTTON, &RobogsPanel::onEditGS, this);
            m_btn_train_mesh->Bind(wxEVT_BUTTON, &RobogsPanel::onTrainMesh, this);
            m_btn_render_mesh->Bind(wxEVT_BUTTON, &RobogsPanel::onRenderMesh, this);
            m_btn_download_mesh->Bind(wxEVT_BUTTON, &RobogsPanel::onDownloadMesh, this);
            m_btn_segment->Bind(wxEVT_BUTTON, &RobogsPanel::onSegment, this);

            // 中间区域
            auto middle_sizer = new wxBoxSizer(wxHORIZONTAL);

            // 视频预览区
            m_video_container = new wxPanel(this, wxID_ANY);
            m_video_container->SetBackgroundColour(*wxLIGHT_GREY);
            

            // 创建媒体控件
            m_media_ctrl = new wxMediaCtrl(m_video_container, wxID_ANY);

            // 绑定停止事件，实现循环播放
            m_media_ctrl->Bind(wxEVT_MEDIA_STOP, [this](wxMediaEvent&) {     
                m_media_ctrl->Play();
             });

            // 绑定加载完成事件，自动播放
            m_media_ctrl->Bind(wxEVT_MEDIA_LOADED, [this](wxMediaEvent&) {

                m_media_ctrl->Play();
             });

            // 视频容器使用嵌套sizer以便自适应
            auto video_sizer = new wxBoxSizer(wxVERTICAL);
            video_sizer->Add(m_media_ctrl, 1, wxEXPAND | wxALL, 0);
            m_video_container->SetSizer(video_sizer);
            
            middle_sizer->Add(m_video_container, 1, wxEXPAND | wxALL, 5);

            // 右侧控制区
            auto right_sizer = new wxBoxSizer(wxVERTICAL);

            // 功能按钮
            m_btn_generate = new Button(this, _L("生成URDF/MJCF"));
            m_btn_dye = new Button(this, _L("模拟和渲染"));
            m_btn_cad_print = new Button(this, _L("cad to print"));

            m_btn_generate->Bind(wxEVT_BUTTON, &RobogsPanel::onGenerate, this);
            m_btn_dye->Bind(wxEVT_BUTTON, &RobogsPanel::onDye, this);
            m_btn_cad_print->Bind(wxEVT_BUTTON, &RobogsPanel::onCadPrint, this);

            // 上传视频和取消
            auto upload_btn_sizer = new wxBoxSizer(wxHORIZONTAL);
            m_btn_confirm_upload_videos = new Button(this, _L("上传视频"));
            m_btn_cancel_upload_videos = new Button(this, _L("取消上传"));
            upload_btn_sizer->Add(m_btn_confirm_upload_videos, 1, wxALL, 5);
            upload_btn_sizer->Add(m_btn_cancel_upload_videos, 1, wxALL, 5);

            // 绑定视频按钮
            m_btn_confirm_upload_videos->Bind(wxEVT_BUTTON, &RobogsPanel::onConfirmUpload, this);
            m_btn_cancel_upload_videos->Bind(wxEVT_BUTTON, &RobogsPanel::onCancelUpload, this);

            // Assin按钮和输入框
            auto assign_btns_sizer = new wxBoxSizer(wxHORIZONTAL);
            m_text_assign = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                wxDefaultPosition, wxSize(200, -1));
            m_btn_assign = new Button(this, _L("Assign id"));

            assign_btns_sizer->Add(m_text_assign, 1, wxALL, 5);
            assign_btns_sizer->Add(m_btn_assign, 0, wxALL, 5);
            
            // 微调按钮和输入框
            auto finetune_btns_sizer = new wxBoxSizer(wxHORIZONTAL);
            m_text_finetune = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                wxDefaultPosition, wxSize(200, -1));
            m_btn_finetune = new Button(this, _L("微调参数"));
            
            finetune_btns_sizer->Add(m_text_finetune, 1, wxALL, 5);
            finetune_btns_sizer->Add(m_btn_finetune, 0, wxALL, 5);

            m_btn_assign->Bind(wxEVT_BUTTON, &RobogsPanel::onAssignId, this);
            m_btn_finetune->Bind(wxEVT_BUTTON, &RobogsPanel::onFinetune, this);

            // 右侧其他按钮
            right_sizer->Add(upload_btn_sizer, 0, wxEXPAND | wxALL, 5);
            right_sizer->Add(assign_btns_sizer, 0, wxEXPAND | wxALL, 5);
            right_sizer->Add(finetune_btns_sizer, 0, wxEXPAND | wxALL, 5);

            right_sizer->Add(m_btn_generate, 0, wxEXPAND | wxALL, 5);
            right_sizer->Add(m_btn_dye, 0, wxEXPAND | wxALL, 5);
            right_sizer->Add(m_btn_cad_print, 0, wxEXPAND | wxALL, 5);
            //right_sizer->Add(btn_sizer, 0, wxALIGN_RIGHT | wxALL, 5);

            middle_sizer->Add(right_sizer, 0, wxEXPAND | wxALL, 5);
            main_sizer->Add(top_sizer, 0, wxEXPAND | wxALL, 5);
            main_sizer->Add(middle_sizer, 1, wxEXPAND | wxALL, 5);

            SetSizer(main_sizer);
            Layout();

            // 设置输入框的背景颜色为绿色
            wxColour light_green(144, 238, 144);
            m_text_assign->SetBackgroundColour(light_green);
            m_text_finetune->SetBackgroundColour(light_green);
        }

/* 上传视频 */
        
        // 上传视频的小工具
        static std::string PercentEncodeUtf8(const wxString& s) {
            wxCharBuffer utf8 = s.utf8_str();   // UTF-8 bytes
            const unsigned char* p = (const unsigned char*)utf8.data();
            size_t n = utf8.length();
            std::string out; out.reserve(n * 3);
            auto is_unreserved = [](unsigned char c)->bool {
                return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~';
                };
            static const char* hex = "0123456789ABCDEF";
            for (size_t i = 0; i < n; ++i) {
                unsigned char c = p[i];
                if (is_unreserved(c)) out.push_back((char)c);
                else { out.push_back('%'); out.push_back(hex[c >> 4]); out.push_back(hex[c & 0xF]); }
            }
            return out;
        }

        void RobogsPanel::onImportVideo(wxCommandEvent& event)
        {
            wxFileDialog openFileDialog(
                this,
                _L("选择视频文件"),
                "",
                "",
                _L("视频文件 (*.mp4;*.avi;*.mkv)|*.mp4;*.avi;*.mkv"),
                wxFD_OPEN | wxFD_FILE_MUST_EXIST
            );

            if (openFileDialog.ShowModal() == wxID_CANCEL)
                return;

            // 先停止当前播放
            if (m_media_ctrl->GetState() == wxMEDIASTATE_PLAYING) {
                m_media_ctrl->Stop();
            }

            wxString filePath = openFileDialog.GetPath();

            // 加载并播放
            if (!m_media_ctrl->Load(filePath))
            {
                wxMessageBox(_L("视频加载失败，请检查格式或路径。"));
                return;
            }
            m_selected_video_path = filePath;
        }

        bool UploadVideoWinHTTP(const std::wstring& base_url_no_trailing_slash,
            const std::wstring& server_filename,   // 仅文件名
            const std::wstring& content_type,      // 例如 L"video/mp4"
            const std::wstring& local_path,        // 本地文件绝对路径
            std::atomic<bool>& cancel,
            std::function<void(int)> on_progress,
            std::wstring* out_err_msg)
        {
            if (out_err_msg) out_err_msg->clear();

            // 1) 解析 base_url（不带文件名）
            URL_COMPONENTS uc{}; uc.dwStructSize = sizeof(uc);
            wchar_t host[256] = { 0 }, path[2048] = { 0 };
            uc.lpszHostName = host; uc.dwHostNameLength = _countof(host);
            uc.lpszUrlPath = path; uc.dwUrlPathLength = _countof(path);
            if (!WinHttpCrackUrl(base_url_no_trailing_slash.c_str(), 0, 0, &uc)) {
                if (out_err_msg) *out_err_msg = L"WinHttpCrackUrl failed";
                return false;
            }
            const bool is_https = (uc.nScheme == INTERNET_SCHEME_HTTPS);
            const INTERNET_PORT port = uc.nPort ? uc.nPort
                : (is_https ? INTERNET_DEFAULT_HTTPS_PORT
                    : INTERNET_DEFAULT_HTTP_PORT);

            // 2) 组装 object：基准路径 + 编码后的文件名 + ?overwrite=1

            // 基准路径（不含文件名）
            std::wstring basePathW(uc.lpszUrlPath, uc.dwUrlPathLength);
            std::string  basePathA(basePathW.begin(), basePathW.end());
            if (!basePathA.empty() && basePathA.back() != '/') basePathA.push_back('/');

            // 只对“文件名”做 UTF-8 百分号编码
            std::string encName = PercentEncodeUtf8(wxString(server_filename.c_str()));

            // 组装 object：/api/v1/upload/video/<encName>?overwrite=1
            std::string objectA = basePathA + encName + "?overwrite=1";
            std::wstring objectW(objectA.begin(), objectA.end());

            // 3) 打开本地文件
            HANDLE hf = ::CreateFileW(local_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hf == INVALID_HANDLE_VALUE) {
                if (out_err_msg) *out_err_msg = L"Open file failed";
                return false;
            }
            LARGE_INTEGER liSize{}; ::GetFileSizeEx(hf, &liSize);
            const ULONGLONG total = (ULONGLONG)liSize.QuadPart;

            // 4) WinHTTP 连接 & 打开请求
            HINTERNET hSession = WinHttpOpen(L"RobogsUploader/1.0",
                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!hSession) { if (out_err_msg) *out_err_msg = L"WinHttpOpen failed"; CloseHandle(hf); return false; }

            HINTERNET hConnect = WinHttpConnect(hSession, host, port, 0);
            if (!hConnect) { if (out_err_msg) *out_err_msg = L"WinHttpConnect failed"; WinHttpCloseHandle(hSession); CloseHandle(hf); return false; }

        //    // 用 objectW 作为 WinHttpOpenRequest 的第三参（包含 path+query）
            HINTERNET hRequest = WinHttpOpenRequest(
                hConnect, L"PUT", objectW.c_str(), nullptr,
                WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                is_https ? WINHTTP_FLAG_SECURE : 0);

            if (!hRequest) {
                if (out_err_msg) *out_err_msg = L"WinHttpOpenRequest failed";
                WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); CloseHandle(hf); return false;
            }

            // 5) 关闭 Expect: 100-continue，避免握手卡顿/失败
            std::wstring headers = L"Content-Type: " + content_type + L"\r\n";

            #if defined(WINHTTP_OPTION_DISABLE_FEATURE) && defined(WINHTTP_DISABLE_SENDEXPECTHEADER)
                        // 新 SDK：用选项关闭
                        {
                            DWORD disable = WINHTTP_DISABLE_SENDEXPECTHEADER;
                            WinHttpSetOption(hRequest, WINHTTP_OPTION_DISABLE_FEATURE, &disable, sizeof(disable));
                        }
            #else
                        // 旧 SDK：用空的 Expect 头覆盖（等效）
                        WinHttpAddRequestHeaders(
                            hRequest,
                            L"Expect: \r\n",                // 空 Expect
                            (DWORD)-1,
                            WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
            #endif

        //    // 6) 发送请求头 + 指定请求体长度（或分块）
            DWORD total_param = 0;
            if (total <= 0xFFFFFFFFULL) {
                total_param = (DWORD)total;                       // 走 Content-Length
            }
            else {
                total_param = WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH; // 走 chunked
                headers += L"Transfer-Encoding: chunked\r\n";
            }

            if (!WinHttpSendRequest(hRequest,
                headers.c_str(), (DWORD)headers.size(),
                WINHTTP_NO_REQUEST_DATA, 0,
                total_param, 0)) {
                if (out_err_msg) *out_err_msg = L"WinHttpSendRequest failed, err=" + std::to_wstring(GetLastError());
                // ... 清理并返回
            }

        //    // 7) 循环写入（可取消 & 进度）
            const DWORD BUF = 1 << 16;
            std::unique_ptr<char[]> buf(new char[BUF]);
            ULONGLONG sent = 0;
            DWORD readBytes = 0, written = 0;

            while (true) {
                if (cancel.load()) {
                    WinHttpCloseHandle(hRequest);
                    WinHttpCloseHandle(hConnect);
                    WinHttpCloseHandle(hSession);
                    CloseHandle(hf);
                    if (out_err_msg) *out_err_msg = L"Aborted by user";
                    return false;
                }

                if (!::ReadFile(hf, buf.get(), BUF, &readBytes, nullptr)) {
                    if (out_err_msg) *out_err_msg = L"ReadFile failed, err=" + std::to_wstring(GetLastError());
                    WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); CloseHandle(hf);
                    return false;
                }
                if (readBytes == 0) break; // EOF

                if (!WinHttpWriteData(hRequest, buf.get(), readBytes, &written) || written != readBytes) {
                    if (out_err_msg) *out_err_msg = L"WinHttpWriteData failed, err=" + std::to_wstring(GetLastError());
                    WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); CloseHandle(hf);
                    return false;
                }

                sent += written;
                if (total > 0 && on_progress) {
                    int pct = (int)((sent * 100) / total);
                    on_progress(std::min(100, std::max(0, pct)));
                }
            }

            // 8) 完成请求并读取响应
            if (!WinHttpReceiveResponse(hRequest, nullptr)) {
                if (out_err_msg) *out_err_msg = L"WinHttpReceiveResponse failed, err=" + std::to_wstring(GetLastError());
                WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); CloseHandle(hf);
                return false;
            }

            DWORD status = 0, slen = sizeof(status);
            if (!WinHttpQueryHeaders(hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &status, &slen, WINHTTP_NO_HEADER_INDEX))
            {
                if (out_err_msg) *out_err_msg = L"WinHttpQueryHeaders failed, err=" + std::to_wstring(GetLastError());
                WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); CloseHandle(hf);
                return false;
            }

            WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); CloseHandle(hf);

            if (status != 201) {
                if (out_err_msg) *out_err_msg = L"HTTP status " + std::to_wstring(status);
                return false;
            }
            return true;
        }
        
       


        void RobogsPanel::CloseUploadProgress(bool forceTo100)
        {
            if (m_upload_timer && m_upload_timer->IsRunning())
                m_upload_timer->Stop();                 // 先停表

            if (m_upload_progress) {
                wxProgressDialog* dlg = m_upload_progress;
                m_upload_progress = nullptr;            // 先置空，防下一拍再刷
                if (forceTo100) dlg->Update(100);
                dlg->Destroy();                         // 同线程销毁
            }
        }

        void RobogsPanel::onConfirmUpload(wxCommandEvent&) {
            if (m_selected_video_path.empty()) {
                wxMessageBox(_L("请先在左侧选择“导入视频”并预览一个本地视频。"));
                return;
            }
            if (m_upload_running.load()) {
                wxMessageBox(_L("已有上传在进行中。"));
                return;
            }

            const std::wstring base_url = L"https://api.dextwin.com/api/v1/upload/video"; // 不含文件名
            const std::wstring fname = wxFileName(m_selected_video_path).GetFullName().wc_str();
            const std::wstring ctype = L"video/mp4"; // 或根据扩展名判断

            m_upload_cancel = false;
            m_upload_running = true;
            m_upload_pct.store(0);

            m_btn_confirm_upload_videos->Disable();
            m_btn_cancel_upload_videos->Enable();

            m_upload_progress = new wxProgressDialog(
                _L("上传中"), _L("正在上传视频…"), 100, this,
                wxPD_CAN_ABORT | wxPD_ELAPSED_TIME | wxPD_REMAINING_TIME | wxPD_AUTO_HIDE);
            // 双保险：销毁时把指针清空
            m_upload_progress->Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent& e) {
                m_upload_progress = nullptr; e.Skip();
                });
            m_upload_progress->Show();

            // 定时器：每 100ms 刷一次 UI
            if (!m_upload_timer) {
                m_upload_timer = new wxTimer(this, ID_UPLOAD_TIMER);
                Bind(wxEVT_TIMER, &RobogsPanel::OnUploadTimer, this, ID_UPLOAD_TIMER);
            }
            m_upload_timer->Start(100);

            m_upload_thread.reset(new std::thread([this, base_url, fname, ctype, local = m_selected_video_path] {
                std::wstring errmsg;
                // 上传线程仅写原子百分比，不直接碰 UI
                bool ok = UploadVideoWinHTTP(
                    base_url, fname, ctype, local.wc_str(),
                    m_upload_cancel,
                    /* on_progress = */ [this](int pct) {
                        // 只在数值变化时写，避免震荡
                        int prev = m_upload_pct.load(std::memory_order_relaxed);
                        if (pct != prev) m_upload_pct.store(pct, std::memory_order_relaxed);
                    },
                    &errmsg);

                // 收尾在主线程做
                wxGetApp().CallAfter([this, ok, errmsg] {
                    CloseUploadProgress(/*forceTo100=*/ok);  // 先把对话框彻底关掉

                    // 让事件循环喘口气，再弹窗（防与进度框的模态/销毁流程打架）
                    auto top = wxTheApp->GetTopWindow();
                    wxGetApp().CallAfter([this, ok, errmsg, top] {
                        if (ok) {
                            wxMessageBox(_L("上传完成"), _L("完成"), wxOK | wxICON_INFORMATION, top);
                        }
                        else if (m_upload_cancel.load()) {
                            wxMessageBox(_L("已取消上传"), _L("已取消"), wxOK | wxICON_INFORMATION, top);
                        }
                        else {
                            wxMessageBox(_L("上传失败：") + wxString(errmsg), _L("错误"),
                                wxOK | wxICON_ERROR, top);
                        }
                        m_btn_confirm_upload_videos->Enable();
                        m_btn_cancel_upload_videos->Disable();
                        m_upload_running = false;
                        });
                    });

                }));
            m_upload_thread->detach();

        }

        // 定时器回调：读取原子、刷新进度、处理用户“取消”
        void RobogsPanel::OnUploadTimer(wxTimerEvent&) {
            // 拿一个本地快照，避免多次读取成员
            wxProgressDialog* dlg = m_upload_progress;

            // 对话框不存在或正在销毁 -> 停掉定时器并返回
            #if wxCHECK_VERSION(3,1,0)
                        if (!dlg || dlg->IsBeingDeleted())
            #else
                        if (!dlg)
            #endif
            {   // 看到空指针就立刻停表
                if (m_upload_timer && m_upload_timer->IsRunning()) m_upload_timer->Stop();
                return;
            }

            int pct = std::clamp(m_upload_pct.load(std::memory_order_relaxed), 0, 100);

            // Update 返回 false 表示用户点击了“取消”
            if (!dlg->Update(pct)) {
                m_upload_cancel = true;
                m_btn_cancel_upload_videos->Disable();
                // 不要在这里 Destroy，对话框关闭交给收尾逻辑或上传线程完成处统一做
            }
        }
        void RobogsPanel::onCancelUpload(wxCommandEvent&) {
            if (m_upload_running.load()) {
                m_upload_cancel = true;               // 下一次写入前会检查并中断
                m_btn_cancel_upload_videos->Disable();
            }
        }


/* 提取图片 */

        void RobogsPanel::OnGetImages(wxCommandEvent& event) {
            auto http = Http::post("https://api.dextwin.com/api/v1/video2image");
            std::string query_body = R"( {} )";
            http.header("Content-Type", "application/json").set_post_body(query_body);
            http.timeout_connect(20)
                .timeout_max(100)
                .on_complete([this](std::string body, unsigned status) {
                BOOST_LOG_TRIVIAL(info) << "提取图片" << body;
                try {
                    nlohmann::json parsed_obj = nlohmann::json::parse(body);
                    std::string task_id = parsed_obj["task_id"];

                    // 重置取消标志
                    m_extract_cancelled.store(false);

                    // 创建并显示进度对话框
                    auto progress_dialog = new wxProgressDialog(
                        _L("执行中"),
                        _L("正在提取图片，请等待..."),
                        100, this,
                        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT
                    );
                    progress_dialog->Show();

                    // 用于防止销毁后的排队事件访问已销毁对象
                    auto done_flag = std::make_shared<std::atomic_bool>(false);

                    // 计时器：让 'this' 拥有，不要让对话框拥有；使用稳定的 ID 便于过滤事件
                    int cancel_timer_id =
#if wxCHECK_VERSION(3,1,0)
                        wxWindow::NewControlId();
#else
                        wxNewId();
#endif
                    auto cancel_timer = new wxTimer(this, cancel_timer_id);

                    // 在 'this' 上绑定事件，避免由已销毁对话框接收事件
                    this->Bind(
                        wxEVT_TIMER,
                        [this, progress_dialog, cancel_timer, cancel_timer_id, done_flag](wxTimerEvent& e) {
                            if (e.GetId() != cancel_timer_id) return;

                            // 已经完成/取消则忽略任何排队的 tick
                            if (done_flag->load(std::memory_order_acquire)) return;

                            // 轮询取消按钮（UI 线程）
                            if (!progress_dialog->Update(-1)) {
                                // 用户点击了取消
                                m_extract_cancelled.store(true);

                                // 确保只执行一次关闭逻辑
                                if (!done_flag->exchange(true, std::memory_order_acq_rel)) {
                                    if (cancel_timer->IsRunning())
                                        cancel_timer->Stop();

                                    // 在 UI 线程删除计时器并销毁对话框
                                    wxGetApp().CallAfter([progress_dialog, cancel_timer]() {
                                        delete cancel_timer; // Stop 后删除，避免后续 tick
                                        if (progress_dialog) {
                                            progress_dialog->Destroy();
                                            wxMessageBox(_L("用户已取消任务"), _L("已取消"));
                                        }
                                        });
                                }
                            }
                        },
                        cancel_timer_id
                    );
                    cancel_timer->Start(200);

                    // 启动后台线程轮询
                    std::thread polling_thread([this, task_id, progress_dialog, cancel_timer, cancel_timer_id, done_flag]() {
                        bool is_completed = false;
                        while (!is_completed && !m_extract_cancelled.load()) {
                            // 拼接 URL
                            std::string url = "https://api.dextwin.com/api/v1/video2image/status/" + task_id;

                            // 请求体为空
                            std::string query_body = R"( {} )";

                            // 发送请求
                            auto http = Http::post(url);
                            http.header("Content-Type", "application/json").set_post_body(query_body);
                            http.timeout_connect(20)
                                .timeout_max(100)
                                .on_complete([this, progress_dialog, cancel_timer, done_flag, &is_completed](std::string body, unsigned status) {
                                try {
                                    nlohmann::json parsed_obj = nlohmann::json::parse(body);
                                    std::string progress = parsed_obj.value("progress", "");

                                    // 如果任务完成并且没有取消
                                    if (progress == "success" && !m_extract_cancelled.load()) {
                                        is_completed = true;

                                        // 切回 UI 线程做收尾
                                        wxGetApp().CallAfter([progress_dialog, cancel_timer, done_flag]() {
                                            // 确保只执行一次
                                            if (done_flag->exchange(true, std::memory_order_acq_rel))
                                                return;

                                            if (cancel_timer && cancel_timer->IsRunning())
                                                cancel_timer->Stop();

                                            delete cancel_timer;

                                            if (progress_dialog) {
                                                progress_dialog->Destroy();
                                                wxMessageBox(_L("提取图片完成"), _L("完成"));
                                            }
                                            });
                                    }
                                }
                                catch (...) {
                                    // 忽略解析错误，继续轮询
                                }
                                    })
                                .on_error([](std::string body, std::string error, unsigned status) {
                                // 可选：记录轮询错误并继续
                                BOOST_LOG_TRIVIAL(warning) << "Polling error: " << error << " body: " << body;
                                    })
                                .perform();

                            // 每轮间隔 5 秒
                            if (!is_completed && !m_extract_cancelled.load())
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                        }
                        });
                    polling_thread.detach();  // 异步执行轮询
                }
                catch (...) {
                    wxMessageDialog* dlg = new wxMessageDialog(this,
                        _L("执行命令失败"),
                        _L("错误"),
                        wxOK | wxCENTRE);
                    dlg->ShowModal();
                    dlg->Destroy();
                }
                    })
                .on_error([this](std::string body, std::string error, unsigned status) {
                BOOST_LOG_TRIVIAL(info) << (boost::format("error: %1%, message: %2%") % error % body).str();
                wxGetApp().CallAfter([this]() {
                    wxMessageDialog* dlg = new wxMessageDialog(this,
                        _L("网络请求失败"),
                        _L("错误"),
                        wxOK | wxCENTRE);
                    dlg->ShowModal();
                    dlg->Destroy();
                    });
                    })
                .perform();
        }



/* 稀疏重建 */
        void RobogsPanel::onRebuild(wxCommandEvent& event) {
            auto http = Http::post("https://api.dextwin.com/api/v1/image2sfm");
            std::string query_body = R"( {} )";
            http.header("Content-Type", "application/json").set_post_body(query_body);
            http.timeout_connect(20)
                .timeout_max(100)
                .on_complete([this](std::string body, unsigned status) {
                BOOST_LOG_TRIVIAL(info) << "稀疏重建" << body;
                try {
                    nlohmann::json parsed_obj = nlohmann::json::parse(body);
                    std::string task_id = parsed_obj["task_id"];


                    // 重置取消标志
                    m_extract_cancelled.store(false);

                    // 创建并显示进度对话框
                    auto progress_dialog = new wxProgressDialog(
                        _L("执行中"),
                        _L("正在稀疏重建，请等待..."),
                        100, this,
                        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT
                    );
                    progress_dialog->Show();

                    // 用于防止销毁后的排队事件访问已销毁对象
                    auto done_flag = std::make_shared<std::atomic_bool>(false);

                    // 计时器：让 'this' 拥有，不要让对话框拥有；使用稳定的 ID 便于过滤事件
                    int cancel_timer_id =
#if wxCHECK_VERSION(3,1,0)
                        wxWindow::NewControlId();
#else
                        wxNewId();
#endif
                    auto cancel_timer = new wxTimer(this, cancel_timer_id);

                    // 在 'this' 上绑定事件，避免由已销毁对话框接收事件
                    this->Bind(
                        wxEVT_TIMER,
                        [this, progress_dialog, cancel_timer, cancel_timer_id, done_flag](wxTimerEvent& e) {
                            if (e.GetId() != cancel_timer_id) return;

                            // 已经完成/取消则忽略任何排队的 tick
                            if (done_flag->load(std::memory_order_acquire)) return;

                            // 轮询取消按钮（UI 线程）
                            if (!progress_dialog->Update(-1)) {
                                // 用户点击了取消
                                m_extract_cancelled.store(true);

                                // 确保只执行一次关闭逻辑
                                if (!done_flag->exchange(true, std::memory_order_acq_rel)) {
                                    if (cancel_timer->IsRunning())
                                        cancel_timer->Stop();

                                    // 在 UI 线程删除计时器并销毁对话框
                                    wxGetApp().CallAfter([progress_dialog, cancel_timer]() {
                                        delete cancel_timer; // Stop 后删除，避免后续 tick
                                        if (progress_dialog) {
                                            progress_dialog->Destroy();
                                            wxMessageBox(_L("用户已取消任务"), _L("已取消"));
                                        }
                                        });
                                }
                            }
                        },
                        cancel_timer_id
                    );
                    cancel_timer->Start(200);

                    // 启动后台线程轮询
                    std::thread polling_thread([this, task_id, progress_dialog, cancel_timer, cancel_timer_id, done_flag]() {
                        bool is_completed = false;
                        while (!is_completed && !m_extract_cancelled.load()) {
                            // 拼接 URL
                            std::string url = "https://api.dextwin.com/api/v1/image2sfm/status/" + task_id;

                            // 请求体为空
                            std::string query_body = R"( {} )";

                            // 发送请求
                            auto http = Http::post(url);
                            http.header("Content-Type", "application/json").set_post_body(query_body);
                            http.timeout_connect(20)
                                .timeout_max(100)
                                .on_complete([this, progress_dialog, cancel_timer, done_flag, &is_completed](std::string body, unsigned status) {
                                try {
                                    nlohmann::json parsed_obj = nlohmann::json::parse(body);
                                    std::string progress = parsed_obj.value("progress", "");

                                    // 如果任务完成并且没有取消
                                    if (progress == "success" && !m_extract_cancelled.load()) {
                                        is_completed = true;

                                        // 切回 UI 线程做收尾
                                        wxGetApp().CallAfter([progress_dialog, cancel_timer, done_flag]() {
                                            // 确保只执行一次
                                            if (done_flag->exchange(true, std::memory_order_acq_rel))
                                                return;

                                            if (cancel_timer && cancel_timer->IsRunning())
                                                cancel_timer->Stop();

                                            delete cancel_timer;

                                            if (progress_dialog) {
                                                progress_dialog->Destroy();
                                                wxMessageBox(_L("稀疏重建完成"), _L("完成"));
                                            }
                                            });
                                    }
                                }
                                catch (...) {
                                    // 忽略解析错误，继续轮询
                                }
                                    })
                                .on_error([](std::string body, std::string error, unsigned status) {
                                // 可选：记录轮询错误并继续
                                BOOST_LOG_TRIVIAL(warning) << "Polling error: " << error << " body: " << body;
                                    })
                                .perform();

                            // 每轮间隔 5 秒
                            if (!is_completed && !m_extract_cancelled.load())
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                        }
                        });
                    polling_thread.detach();  // 异步执行轮询
                }
                catch (...) {
                    wxMessageDialog* dlg = new wxMessageDialog(this,
                        _L("执行命令失败"),
                        _L("错误"),
                        wxOK | wxCENTRE);
                    dlg->ShowModal();
                    dlg->Destroy();
                }
                    })
                .on_error([this](std::string body, std::string error, unsigned status) {
                BOOST_LOG_TRIVIAL(info) << (boost::format("error: %1%, message: %2%") % error % body).str();
                wxGetApp().CallAfter([this]() {
                    wxMessageDialog* dlg = new wxMessageDialog(this,
                        _L("网络请求失败"),
                        _L("错误"),
                        wxOK | wxCENTRE);
                    dlg->ShowModal();
                    dlg->Destroy();
                    });
                    })
                .perform();
        }
        

/* 训练高斯 */
        void RobogsPanel::onTrainGS(wxCommandEvent& event) {
            auto http = Http::post("https://api.dextwin.com/api/v1/gsplat_trainer");
            std::string query_body = R"( {} )";
            http.header("Content-Type", "application/json").set_post_body(query_body);
            http.timeout_connect(20)
                .timeout_max(100)
                .on_complete([this](std::string body, unsigned status) {
                BOOST_LOG_TRIVIAL(info) << "训练GS" << body;
                try {
                    nlohmann::json parsed_obj = nlohmann::json::parse(body);
                    std::string task_id = parsed_obj["task_id"];

                    // 重置取消标志
                    m_extract_cancelled.store(false);

                    // 创建并显示进度对话框
                    auto progress_dialog = new wxProgressDialog(
                        _L("执行中"),
                        _L("正在训练GS，请等待..."),
                        100, this,
                        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT
                    );
                    progress_dialog->Show();

                    // 用于防止销毁后的排队事件访问已销毁对象
                    auto done_flag = std::make_shared<std::atomic_bool>(false);

                    // 计时器：让 'this' 拥有，不要让对话框拥有；使用稳定的 ID 便于过滤事件
                    int cancel_timer_id =
#if wxCHECK_VERSION(3,1,0)
                        wxWindow::NewControlId();
#else
                        wxNewId();
#endif
                    auto cancel_timer = new wxTimer(this, cancel_timer_id);

                    // 在 'this' 上绑定事件，避免由已销毁对话框接收事件
                    this->Bind(
                        wxEVT_TIMER,
                        [this, progress_dialog, cancel_timer, cancel_timer_id, done_flag](wxTimerEvent& e) {
                            if (e.GetId() != cancel_timer_id) return;

                            // 已经完成/取消则忽略任何排队的 tick
                            if (done_flag->load(std::memory_order_acquire)) return;

                            // 轮询取消按钮（UI 线程）
                            if (!progress_dialog->Update(-1)) {
                                // 用户点击了取消
                                m_extract_cancelled.store(true);

                                // 确保只执行一次关闭逻辑
                                if (!done_flag->exchange(true, std::memory_order_acq_rel)) {
                                    if (cancel_timer->IsRunning())
                                        cancel_timer->Stop();

                                    // 在 UI 线程删除计时器并销毁对话框
                                    wxGetApp().CallAfter([progress_dialog, cancel_timer]() {
                                        delete cancel_timer; // Stop 后删除，避免后续 tick
                                        if (progress_dialog) {
                                            progress_dialog->Destroy();
                                            wxMessageBox(_L("用户已取消任务"), _L("已取消"));
                                        }
                                        });
                                }
                            }
                        },
                        cancel_timer_id
                    );
                    cancel_timer->Start(200);

                    // 启动后台线程轮询
                    std::thread polling_thread([this, task_id, progress_dialog, cancel_timer, cancel_timer_id, done_flag]() {
                        bool is_completed = false;
                        while (!is_completed && !m_extract_cancelled.load()) {
                            // 拼接 URL
                            std::string url = "https://api.dextwin.com/api/v1/gsplat_trainer/status/" + task_id;

                            // 请求体为空
                            std::string query_body = R"( {} )";

                            // 发送请求
                            auto http = Http::post(url);
                            http.header("Content-Type", "application/json").set_post_body(query_body);
                            http.timeout_connect(20)
                                .timeout_max(100)
                                .on_complete([this, progress_dialog, cancel_timer, done_flag, &is_completed](std::string body, unsigned status) {
                                try {
                                    nlohmann::json parsed_obj = nlohmann::json::parse(body);
                                    std::string progress = parsed_obj.value("progress", "");

                                    // 如果任务完成并且没有取消
                                    if (progress == "success" && !m_extract_cancelled.load()) {
                                        is_completed = true;

                                        // 切回 UI 线程做收尾
                                        wxGetApp().CallAfter([progress_dialog, cancel_timer, done_flag]() {
                                            // 确保只执行一次
                                            if (done_flag->exchange(true, std::memory_order_acq_rel))
                                                return;

                                            if (cancel_timer && cancel_timer->IsRunning())
                                                cancel_timer->Stop();

                                            delete cancel_timer;

                                            if (progress_dialog) {
                                                progress_dialog->Destroy();
                                                wxMessageBox(_L("训练GS完成"), _L("完成"));
                                            }
                                            });
                                    }
                                }
                                catch (...) {
                                    // 忽略解析错误，继续轮询
                                }
                                    })
                                .on_error([](std::string body, std::string error, unsigned status) {
                                // 可选：记录轮询错误并继续
                                BOOST_LOG_TRIVIAL(warning) << "Polling error: " << error << " body: " << body;
                                    })
                                .perform();

                            // 每轮间隔 5 秒
                            if (!is_completed && !m_extract_cancelled.load())
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                        }
                        });
                    polling_thread.detach();  // 异步执行轮询
                }
                catch (...) {
                    wxMessageDialog* dlg = new wxMessageDialog(this,
                        _L("执行命令失败"),
                        _L("错误"),
                        wxOK | wxCENTRE);
                    dlg->ShowModal();
                    dlg->Destroy();
                }
                    })
                .on_error([this](std::string body, std::string error, unsigned status) {
                BOOST_LOG_TRIVIAL(info) << (boost::format("error: %1%, message: %2%") % error % body).str();
                wxGetApp().CallAfter([this]() {
                    wxMessageDialog* dlg = new wxMessageDialog(this,
                        _L("网络请求失败"),
                        _L("错误"),
                        wxOK | wxCENTRE);
                    dlg->ShowModal();
                    dlg->Destroy();
                    });
                    })
                .perform();
        }


/* 提取GS */
        void RobogsPanel::onExtract(wxCommandEvent& event) {
            auto http = Http::post("https://api.dextwin.com/api/v1/extract_ply");
            std::string query_body = R"( {} )";
            http.header("Content-Type", "application/json").set_post_body(query_body);
            http.timeout_connect(20)
                .timeout_max(100)
                .on_complete([this](std::string body, unsigned status) {
                BOOST_LOG_TRIVIAL(info) << "提取GS" << body;
                try {
                    nlohmann::json parsed_obj = nlohmann::json::parse(body);
                    std::string task_id = parsed_obj["task_id"];

                    m_task_id = task_id;

                    // 重置取消标志
                    m_extract_cancelled.store(false);

                    // 创建并显示进度对话框
                    auto progress_dialog = new wxProgressDialog(
                        _L("执行中"),
                        _L("正在提取GS，请等待..."),
                        100, this,
                        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT
                    );
                    progress_dialog->Show();

                    // 用于防止销毁后的排队事件访问已销毁对象
                    auto done_flag = std::make_shared<std::atomic_bool>(false);

                    // 计时器：让 'this' 拥有，不要让对话框拥有；使用稳定的 ID 便于过滤事件
                    int cancel_timer_id =
#if wxCHECK_VERSION(3,1,0)
                        wxWindow::NewControlId();
#else
                        wxNewId();
#endif
                    auto cancel_timer = new wxTimer(this, cancel_timer_id);

                    // 在 'this' 上绑定事件，避免由已销毁对话框接收事件
                    this->Bind(
                        wxEVT_TIMER,
                        [this, progress_dialog, cancel_timer, cancel_timer_id, done_flag](wxTimerEvent& e) {
                            if (e.GetId() != cancel_timer_id) return;

                            // 已经完成/取消则忽略任何排队的 tick
                            if (done_flag->load(std::memory_order_acquire)) return;

                            // 轮询取消按钮（UI 线程）
                            if (!progress_dialog->Update(-1)) {
                                // 用户点击了取消
                                m_extract_cancelled.store(true);

                                // 确保只执行一次关闭逻辑
                                if (!done_flag->exchange(true, std::memory_order_acq_rel)) {
                                    if (cancel_timer->IsRunning())
                                        cancel_timer->Stop();

                                    // 在 UI 线程删除计时器并销毁对话框
                                    wxGetApp().CallAfter([progress_dialog, cancel_timer]() {
                                        delete cancel_timer; // Stop 后删除，避免后续 tick
                                        if (progress_dialog) {
                                            progress_dialog->Destroy();
                                            wxMessageBox(_L("用户已取消任务"), _L("已取消"));
                                        }
                                        });
                                }
                            }
                        },
                        cancel_timer_id
                    );
                    cancel_timer->Start(200);

                    // 启动后台线程轮询
                    std::thread polling_thread([this, task_id, progress_dialog, cancel_timer, cancel_timer_id, done_flag]() {
                        bool is_completed = false;
                        while (!is_completed && !m_extract_cancelled.load()) {
                            // 拼接 URL
                            std::string url = "https://api.dextwin.com/api/v1/extract_ply/status/" + task_id;

                            // 请求体为空
                            std::string query_body = R"( {} )";

                            // 发送请求
                            auto http = Http::post(url);
                            http.header("Content-Type", "application/json").set_post_body(query_body);
                            http.timeout_connect(20)
                                .timeout_max(100)
                                .on_complete([this, progress_dialog, cancel_timer, done_flag, &is_completed](std::string body, unsigned status) {
                                try {
                                    nlohmann::json parsed_obj = nlohmann::json::parse(body);
                                    std::string progress = parsed_obj.value("progress", "");

                                    // 如果任务完成并且没有取消
                                    if (progress == "success" && !m_extract_cancelled.load()) {
                                        is_completed = true;

                                        // 切回 UI 线程做收尾
                                        wxGetApp().CallAfter([progress_dialog, cancel_timer, done_flag]() {
                                            // 确保只执行一次
                                            if (done_flag->exchange(true, std::memory_order_acq_rel))
                                                return;

                                            if (cancel_timer && cancel_timer->IsRunning())
                                                cancel_timer->Stop();

                                            delete cancel_timer;

                                            if (progress_dialog) {
                                                progress_dialog->Destroy();
                                                wxMessageBox(_L("提取GS完成"), _L("完成"));
                                            }
                                            });
                                    }
                                }
                                catch (...) {
                                    // 忽略解析错误，继续轮询
                                }
                                    })
                                .on_error([](std::string body, std::string error, unsigned status) {
                                // 可选：记录轮询错误并继续
                                BOOST_LOG_TRIVIAL(warning) << "Polling error: " << error << " body: " << body;
                                    })
                                .perform();

                            // 每轮间隔 5 秒
                            if (!is_completed && !m_extract_cancelled.load())
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                        }
                        });
                    polling_thread.detach();  // 异步执行轮询
                }
                catch (...) {
                    wxMessageDialog* dlg = new wxMessageDialog(this,
                        _L("执行命令失败"),
                        _L("错误"),
                        wxOK | wxCENTRE);
                    dlg->ShowModal();
                    dlg->Destroy();
                }
                    })
                .on_error([this](std::string body, std::string error, unsigned status) {
                BOOST_LOG_TRIVIAL(info) << (boost::format("error: %1%, message: %2%") % error % body).str();
                wxGetApp().CallAfter([this]() {
                    wxMessageDialog* dlg = new wxMessageDialog(this,
                        _L("网络请求失败"),
                        _L("错误"),
                        wxOK | wxCENTRE);
                    dlg->ShowModal();
                    dlg->Destroy();
                    });
                    })
                .perform();
        }


/* 下载GS */
        void RobogsPanel::onDownload_gs(wxCommandEvent& event) {
            // 先检查是否有可用的 task_id
            if (m_task_id.empty()) {
                wxMessageBox(_L("暂无可下载的任务，请先完成提取步骤。"),
                    _L("提示"), wxOK | wxICON_INFORMATION, this);
                return;
            }

            // 生成下载链接
            const std::string url =
                "https://api.dextwin.com/api/v1/extract_ply/download/" + m_task_id;

            // 转为 wxString 并使用默认浏览器打开
            const wxString wxurl = wxString::FromUTF8(url.c_str());

            // 第二个参数可选：wxBROWSER_NEW_WINDOW 尝试新窗口/标签打开（平台相关）
            if (!wxLaunchDefaultBrowser(wxurl, wxBROWSER_NEW_WINDOW)) {
                // 打不开浏览器时给出提示，并把链接展示出来便于手动复制
                wxMessageBox(_L("无法打开默认浏览器，请复制链接手动打开：\n") + wxurl,
                    _L("打开失败"), wxOK | wxICON_ERROR, this);
            }
        }

/* 编辑GS */
        void RobogsPanel::onEditGS(wxCommandEvent& event) {
            // 定义要打开的网址
            wxString url = "https://superspl.at/editor";

            // 使用默认浏览器打开网址
            if (!wxLaunchDefaultBrowser(url)) {
                // 如果打开失败，显示错误信息
                wxMessageBox("Failed to open browser. Please check your system's default browser settings.", "Error", wxOK | wxICON_ERROR);
            }
        }

/* 训练Mesh */
        void RobogsPanel::onTrainMesh(wxCommandEvent& event) {
            auto http = Http::post("https://api.dextwin.com/api/v1/train_mesh");
            std::string query_body = R"( {} )";
            http.header("Content-Type", "application/json").set_post_body(query_body);
            http.timeout_connect(20)
                .timeout_max(100)
                .on_complete([this](std::string body, unsigned status) {
                BOOST_LOG_TRIVIAL(info) << "训练Mesh" << body;
                try {
                    nlohmann::json parsed_obj = nlohmann::json::parse(body);
                    std::string task_id = parsed_obj["task_id"];


                    // 重置取消标志
                    m_extract_cancelled.store(false);

                    // 创建并显示进度对话框
                    auto progress_dialog = new wxProgressDialog(
                        _L("执行中"),
                        _L("正在训练Mesh，请等待..."),
                        100, this,
                        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT
                    );
                    progress_dialog->Show();

                    // 用于防止销毁后的排队事件访问已销毁对象
                    auto done_flag = std::make_shared<std::atomic_bool>(false);

                    // 计时器：让 'this' 拥有，不要让对话框拥有；使用稳定的 ID 便于过滤事件
                    int cancel_timer_id =
#if wxCHECK_VERSION(3,1,0)
                        wxWindow::NewControlId();
#else
                        wxNewId();
#endif
                    auto cancel_timer = new wxTimer(this, cancel_timer_id);

                    // 在 'this' 上绑定事件，避免由已销毁对话框接收事件
                    this->Bind(
                        wxEVT_TIMER,
                        [this, progress_dialog, cancel_timer, cancel_timer_id, done_flag](wxTimerEvent& e) {
                            if (e.GetId() != cancel_timer_id) return;

                            // 已经完成/取消则忽略任何排队的 tick
                            if (done_flag->load(std::memory_order_acquire)) return;

                            // 轮询取消按钮（UI 线程）
                            if (!progress_dialog->Update(-1)) {
                                // 用户点击了取消
                                m_extract_cancelled.store(true);

                                // 确保只执行一次关闭逻辑
                                if (!done_flag->exchange(true, std::memory_order_acq_rel)) {
                                    if (cancel_timer->IsRunning())
                                        cancel_timer->Stop();

                                    // 在 UI 线程删除计时器并销毁对话框
                                    wxGetApp().CallAfter([progress_dialog, cancel_timer]() {
                                        delete cancel_timer; // Stop 后删除，避免后续 tick
                                        if (progress_dialog) {
                                            progress_dialog->Destroy();
                                            wxMessageBox(_L("用户已取消任务"), _L("已取消"));
                                        }
                                        });
                                }
                            }
                        },
                        cancel_timer_id
                    );
                    cancel_timer->Start(200);

                    // 启动后台线程轮询
                    std::thread polling_thread([this, task_id, progress_dialog, cancel_timer, cancel_timer_id, done_flag]() {
                        bool is_completed = false;
                        while (!is_completed && !m_extract_cancelled.load()) {
                            // 拼接 URL
                            std::string url = "https://api.dextwin.com/api/v1/train_mesh/status/" + task_id;

                            // 请求体为空
                            std::string query_body = R"( {} )";

                            // 发送请求
                            auto http = Http::post(url);
                            http.header("Content-Type", "application/json").set_post_body(query_body);
                            http.timeout_connect(20)
                                .timeout_max(100)
                                .on_complete([this, progress_dialog, cancel_timer, done_flag, &is_completed](std::string body, unsigned status) {
                                try {
                                    nlohmann::json parsed_obj = nlohmann::json::parse(body);
                                    std::string progress = parsed_obj.value("progress", "");

                                    // 如果任务完成并且没有取消
                                    if (progress == "success" && !m_extract_cancelled.load()) {
                                        is_completed = true;

                                        // 切回 UI 线程做收尾
                                        wxGetApp().CallAfter([progress_dialog, cancel_timer, done_flag]() {
                                            // 确保只执行一次
                                            if (done_flag->exchange(true, std::memory_order_acq_rel))
                                                return;

                                            if (cancel_timer && cancel_timer->IsRunning())
                                                cancel_timer->Stop();

                                            delete cancel_timer;

                                            if (progress_dialog) {
                                                progress_dialog->Destroy();
                                                wxMessageBox(_L("训练Mesh完成"), _L("完成"));
                                            }
                                            });
                                    }
                                }
                                catch (...) {
                                    // 忽略解析错误，继续轮询
                                }
                                    })
                                .on_error([](std::string body, std::string error, unsigned status) {
                                // 可选：记录轮询错误并继续
                                BOOST_LOG_TRIVIAL(warning) << "Polling error: " << error << " body: " << body;
                                    })
                                .perform();

                            // 每轮间隔 5 秒
                            if (!is_completed && !m_extract_cancelled.load())
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                        }
                        });
                    polling_thread.detach();  // 异步执行轮询
                }
                catch (...) {
                    wxMessageDialog* dlg = new wxMessageDialog(this,
                        _L("执行命令失败"),
                        _L("错误"),
                        wxOK | wxCENTRE);
                    dlg->ShowModal();
                    dlg->Destroy();
                }
                    })
                .on_error([this](std::string body, std::string error, unsigned status) {
                BOOST_LOG_TRIVIAL(info) << (boost::format("error: %1%, message: %2%") % error % body).str();
                wxGetApp().CallAfter([this]() {
                    wxMessageDialog* dlg = new wxMessageDialog(this,
                        _L("网络请求失败"),
                        _L("错误"),
                        wxOK | wxCENTRE);
                    dlg->ShowModal();
                    dlg->Destroy();
                    });
                    })
                .perform();
        }


/* 渲染Mesh */
        void RobogsPanel::onRenderMesh(wxCommandEvent& event) {
            auto http = Http::post("https://api.dextwin.com/api/v1/render_mesh");
            std::string query_body = R"( {} )";
            http.header("Content-Type", "application/json").set_post_body(query_body);
            http.timeout_connect(20)
                .timeout_max(100)
                .on_complete([this](std::string body, unsigned status) {
                BOOST_LOG_TRIVIAL(info) << "渲染Mesh" << body;
                try {
                    nlohmann::json parsed_obj = nlohmann::json::parse(body);
                    std::string task_id = parsed_obj["task_id"];

                    m_task_id = task_id;

                    // 重置取消标志
                    m_extract_cancelled.store(false);

                    // 创建并显示进度对话框
                    auto progress_dialog = new wxProgressDialog(
                        _L("执行中"),
                        _L("正在渲染Mesh，请等待..."),
                        100, this,
                        wxPD_APP_MODAL | wxPD_AUTO_HIDE | wxPD_CAN_ABORT
                    );
                    progress_dialog->Show();

                    // 用于防止销毁后的排队事件访问已销毁对象
                    auto done_flag = std::make_shared<std::atomic_bool>(false);

                    // 计时器：让 'this' 拥有，不要让对话框拥有；使用稳定的 ID 便于过滤事件
                    int cancel_timer_id =
#if wxCHECK_VERSION(3,1,0)
                        wxWindow::NewControlId();
#else
                        wxNewId();
#endif
                    auto cancel_timer = new wxTimer(this, cancel_timer_id);

                    // 在 'this' 上绑定事件，避免由已销毁对话框接收事件
                    this->Bind(
                        wxEVT_TIMER,
                        [this, progress_dialog, cancel_timer, cancel_timer_id, done_flag](wxTimerEvent& e) {
                            if (e.GetId() != cancel_timer_id) return;

                            // 已经完成/取消则忽略任何排队的 tick
                            if (done_flag->load(std::memory_order_acquire)) return;

                            // 轮询取消按钮（UI 线程）
                            if (!progress_dialog->Update(-1)) {
                                // 用户点击了取消
                                m_extract_cancelled.store(true);

                                // 确保只执行一次关闭逻辑
                                if (!done_flag->exchange(true, std::memory_order_acq_rel)) {
                                    if (cancel_timer->IsRunning())
                                        cancel_timer->Stop();

                                    // 在 UI 线程删除计时器并销毁对话框
                                    wxGetApp().CallAfter([progress_dialog, cancel_timer]() {
                                        delete cancel_timer; // Stop 后删除，避免后续 tick
                                        if (progress_dialog) {
                                            progress_dialog->Destroy();
                                            wxMessageBox(_L("用户已取消任务"), _L("已取消"));
                                        }
                                        });
                                }
                            }
                        },
                        cancel_timer_id
                    );
                    cancel_timer->Start(200);

                    // 启动后台线程轮询
                    std::thread polling_thread([this, task_id, progress_dialog, cancel_timer, cancel_timer_id, done_flag]() {
                        bool is_completed = false;
                        while (!is_completed && !m_extract_cancelled.load()) {
                            // 拼接 URL
                            std::string url = "https://api.dextwin.com/api/v1/render_mesh/status/" + task_id;

                            // 请求体为空
                            std::string query_body = R"( {} )";

                            // 发送请求
                            auto http = Http::post(url);
                            http.header("Content-Type", "application/json").set_post_body(query_body);
                            http.timeout_connect(20)
                                .timeout_max(100)
                                .on_complete([this, progress_dialog, cancel_timer, done_flag, &is_completed](std::string body, unsigned status) {
                                try {
                                    nlohmann::json parsed_obj = nlohmann::json::parse(body);
                                    std::string progress = parsed_obj.value("progress", "");

                                    // 如果任务完成并且没有取消
                                    if (progress == "success" && !m_extract_cancelled.load()) {
                                        is_completed = true;

                                        // 切回 UI 线程做收尾
                                        wxGetApp().CallAfter([progress_dialog, cancel_timer, done_flag]() {
                                            // 确保只执行一次
                                            if (done_flag->exchange(true, std::memory_order_acq_rel))
                                                return;

                                            if (cancel_timer && cancel_timer->IsRunning())
                                                cancel_timer->Stop();

                                            delete cancel_timer;

                                            if (progress_dialog) {
                                                progress_dialog->Destroy();
                                                wxMessageBox(_L("渲染Mesh完成"), _L("完成"));
                                            }
                                            });
                                    }
                                }
                                catch (...) {
                                    // 忽略解析错误，继续轮询
                                }
                                    })
                                .on_error([](std::string body, std::string error, unsigned status) {
                                // 可选：记录轮询错误并继续
                                BOOST_LOG_TRIVIAL(warning) << "Polling error: " << error << " body: " << body;
                                    })
                                .perform();

                            // 每轮间隔 5 秒
                            if (!is_completed && !m_extract_cancelled.load())
                                std::this_thread::sleep_for(std::chrono::seconds(5));
                        }
                        });
                    polling_thread.detach();  // 异步执行轮询
                }
                catch (...) {
                    wxMessageDialog* dlg = new wxMessageDialog(this,
                        _L("执行命令失败"),
                        _L("错误"),
                        wxOK | wxCENTRE);
                    dlg->ShowModal();
                    dlg->Destroy();
                }
                    })
                .on_error([this](std::string body, std::string error, unsigned status) {
                BOOST_LOG_TRIVIAL(info) << (boost::format("error: %1%, message: %2%") % error % body).str();
                wxGetApp().CallAfter([this]() {
                    wxMessageDialog* dlg = new wxMessageDialog(this,
                        _L("网络请求失败"),
                        _L("错误"),
                        wxOK | wxCENTRE);
                    dlg->ShowModal();
                    dlg->Destroy();
                    });
                    })
                .perform();
        }


/* 下载Mesh */
        void RobogsPanel::onDownloadMesh(wxCommandEvent& event) {
            // 先检查是否有可用的 task_id
            if (m_task_id.empty()) {
                wxMessageBox(_L("暂无可下载的任务，请先完成渲染Mesh步骤。"),
                    _L("提示"), wxOK | wxICON_INFORMATION, this);
                return;
            }

            // 生成下载链接
            const std::string url =
                "https://api.dextwin.com/api/v1/render_mesh/download/" + m_task_id;

            // 转为 wxString 并使用默认浏览器打开
            const wxString wxurl = wxString::FromUTF8(url.c_str());

            // 第二个参数可选：wxBROWSER_NEW_WINDOW 尝试新窗口/标签打开（平台相关）
            if (!wxLaunchDefaultBrowser(wxurl, wxBROWSER_NEW_WINDOW)) {
                // 打不开浏览器时给出提示，并把链接展示出来便于手动复制
                wxMessageBox(_L("无法打开默认浏览器，请复制链接手动打开：\n") + wxurl,
                    _L("打开失败"), wxOK | wxICON_ERROR, this);
            }
        }


/* Mesh 分割 */
        void RobogsPanel::onSegment(wxCommandEvent& event) {
            // 定义要打开的网址
            wxString url = "https://3d.pic.net/#model=assets/models/cow.ply";

            // 使用默认浏览器打开网址
            if (!wxLaunchDefaultBrowser(url)) {
                // 如果打开失败，显示错误信息
                wxMessageBox("Failed to open browser. Please check your system's default browser settings.", "Error", wxOK | wxICON_ERROR);
            }
        }

/* Assign ID */
        void RobogsPanel::onAssignId(wxCommandEvent& event) {
            // 创建一个提示框
            auto member_dialog = new wxMessageDialog(
                this,
                _L("此功能仅限会员使用"),
                _L("会员提示"),
                wxOK | wxCENTRE
            );

            // 显示弹窗
            member_dialog->ShowModal();
            member_dialog->Destroy();
        }

/* 微调参数 */
        void RobogsPanel::onFinetune(wxCommandEvent& event) {
            // 创建一个提示框
            auto member_dialog = new wxMessageDialog(
                this,
                _L("此功能仅限会员使用"),
                _L("会员提示"),
                wxOK | wxCENTRE
            );

            // 显示弹窗
            member_dialog->ShowModal();
            member_dialog->Destroy();
        }

/* 生成URDF/MJCF */
        void RobogsPanel::onGenerate(wxCommandEvent& event) {
            // 创建一个提示框
            auto member_dialog = new wxMessageDialog(
                this,
                _L("此功能仅限会员使用"), 
                _L("会员提示"),  
                wxOK | wxCENTRE 
            );

            // 显示弹窗
            member_dialog->ShowModal();
            member_dialog->Destroy();
        }

/* 模拟渲染 */
        void RobogsPanel::onDye(wxCommandEvent& event) {
            // 创建一个提示框
            auto member_dialog = new wxMessageDialog(
                this,
                _L("此功能仅限会员使用"),
                _L("会员提示"),
                wxOK | wxCENTRE
            );

            // 显示弹窗
            member_dialog->ShowModal();
            member_dialog->Destroy();
        }

/* 打印 */
        void RobogsPanel::onCadPrint(wxCommandEvent& event) {
            // 创建一个提示框
            auto member_dialog = new wxMessageDialog(
                this,
                _L("此功能仅限会员使用"),
                _L("会员提示"),
                wxOK | wxCENTRE
            );

            // 显示弹窗
            member_dialog->ShowModal();
            member_dialog->Destroy();
        }
    }
} // namespace Slic3r::GUI