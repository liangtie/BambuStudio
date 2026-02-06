#include "RobogsMujoco.hpp"
#include "RobogsMuJoCoCanvas.hpp" 

#include <wx/uri.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>
#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/timer.h>
#include <wx/filedlg.h>

#pragma comment(lib, "winhttp.lib")
#include <winhttp.h>

#ifdef __WXMSW__
#include <windows.h>
#endif

#include <string>   // 为 std::string


namespace Slic3r {
    namespace GUI {

        namespace {

            // 在宿主面板下查找已经存在的 MuJoCo canvas
            static RobogsMuJoCoCanvas* find_canvas(wxPanel* host)
            {
                if (!host) return nullptr;
                for (wxWindow* child : host->GetChildren()) {
                    if (auto* c = dynamic_cast<RobogsMuJoCoCanvas*>(child))
                        return c;
                }
                return nullptr;
            }

            // 确保宿主里有且仅有一个 canvas，没有就创建
            static RobogsMuJoCoCanvas* ensure_canvas(wxPanel* host)
            {
                if (!host) return nullptr;
                if (auto* c = find_canvas(host)) return c;

                if (!host->GetSizer())
                    host->SetSizer(new wxBoxSizer(wxVERTICAL));

                auto* canvas = new RobogsMuJoCoCanvas(host);
                host->GetSizer()->Add(canvas, 1, wxEXPAND);
                host->Layout();
                if (host->GetParent()) host->GetParent()->Layout();
                return canvas;
            }

        } // anonymous namespace

        wxBEGIN_EVENT_TABLE(RobogsMujoco, wxPanel)
#ifdef __WXMSW__
            EVT_TIMER(wxID_ANY, RobogsMujoco::onPollSimWindow)
#endif
            wxEND_EVENT_TABLE()

            RobogsMujoco::RobogsMujoco(wxWindow* parent)
            : wxPanel(parent, wxID_ANY)
        {
            initUI();
        }

        void RobogsMujoco::initUI()
        {
            const int PAD = 8;
            auto* main_sizer = new wxBoxSizer(wxVERTICAL);

            // ---------------- 顶部工具栏 ----------------
            auto* top_sizer = new wxBoxSizer(wxHORIZONTAL);

            // 这里用英文避免中文乱码，你可以之后自行改回中文
            m_d_txt = new Button(this, wxString::FromUTF8(u8"下载TXT"));
            m_xml = new Button(this, wxString::FromUTF8(u8"导入XML"));
            m_i_txt = new Button(this, wxString::FromUTF8(u8"导入TXT"));
            m_replay = new Button(this, wxString::FromUTF8(u8"播放"));

            const wxSize btn_min(140, 40);
            m_d_txt->SetMinSize(btn_min);
            m_xml->SetMinSize(btn_min);
            m_i_txt->SetMinSize(btn_min);
            m_replay->SetMinSize(btn_min);

            top_sizer->Add(m_d_txt, 0, wxALL | wxALIGN_CENTER_VERTICAL, PAD);
            top_sizer->Add(m_xml, 0, wxTOP | wxBOTTOM | wxRIGHT | wxALIGN_CENTER_VERTICAL, PAD);
            top_sizer->Add(m_i_txt, 0, wxTOP | wxBOTTOM | wxRIGHT | wxALIGN_CENTER_VERTICAL, PAD);
            top_sizer->AddStretchSpacer(1);
            top_sizer->Add(m_replay, 0, wxALL | wxALIGN_CENTER_VERTICAL, PAD);

            main_sizer->Add(top_sizer, 0, wxEXPAND);
            main_sizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, PAD);

            // --------- 中间：文件管理区（仅记录路径，不操作 MuJoCo） ---------
            m_file_panel = new wxPanel(this, wxID_ANY);
            auto* file_sizer = new wxBoxSizer(wxVERTICAL);

            // XML 行
            {
                auto* row = new wxBoxSizer(wxHORIZONTAL);
                auto* label = new wxStaticText(m_file_panel, wxID_ANY, _L("XML:"));
                m_xml_label = new wxStaticText(m_file_panel, wxID_ANY, _L("None"));
                m_xml_clear = new Button(m_file_panel, _L("Clear"));

                row->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
                row->Add(m_xml_label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
                row->Add(m_xml_clear, 0, wxALIGN_CENTER_VERTICAL);

                file_sizer->Add(row, 0, wxEXPAND | wxALL, 2);
            }

            // TXT 行
            {
                auto* row = new wxBoxSizer(wxHORIZONTAL);
                auto* label = new wxStaticText(m_file_panel, wxID_ANY, _L("TXT:"));
                m_traj_label = new wxStaticText(m_file_panel, wxID_ANY, _L("None"));
                m_traj_clear = new Button(m_file_panel, _L("Clear"));

                row->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
                row->Add(m_traj_label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
                row->Add(m_traj_clear, 0, wxALIGN_CENTER_VERTICAL);

                file_sizer->Add(row, 0, wxEXPAND | wxALL, 2);
            }

            m_file_panel->SetSizer(file_sizer);
            main_sizer->Add(m_file_panel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, PAD);

            // --------- 下方：嵌入 MuJoCo Canvas 的宿主面板 ---------
            m_view_host = new wxPanel(this, wxID_ANY);
            m_view_host->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
            m_view_host->SetSizer(new wxBoxSizer(wxVERTICAL));
            main_sizer->Add(m_view_host, 1, wxEXPAND | wxALL, PAD);

            // 绑定按钮事件
            m_d_txt->Bind(wxEVT_BUTTON, &RobogsMujoco::onDownload, this);
            m_xml->Bind(wxEVT_BUTTON, &RobogsMujoco::onXml, this);
            m_i_txt->Bind(wxEVT_BUTTON, &RobogsMujoco::onImport, this);
            m_replay->Bind(wxEVT_BUTTON, &RobogsMujoco::onReplay, this);

            // 清空按钮只操作路径字符串，不操作 canvas，避免空指针
            m_xml_clear->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
                m_xml_path.clear();
                refreshFileLabels();
                });
            m_traj_clear->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
                m_traj_path.clear();
                refreshFileLabels();
                });

            m_view_host->Bind(wxEVT_SIZE, &RobogsMujoco::onHostResize, this);

#ifdef __WXMSW__
            // 兼容旧接口的空定时器（不再真正使用）
            m_poll_timer = new wxTimer(this);
            Bind(wxEVT_TIMER, &RobogsMujoco::onPollSimWindow, this, m_poll_timer->GetId());
#endif

            SetSizer(main_sizer);
            Layout();

            // 提前创建一个空 canvas，保证后续 replay 时一定存在
            ensure_canvas(m_view_host);

            refreshFileLabels();
        }

        void RobogsMujoco::refreshFileLabels()
        {
            if (!m_file_panel) return;

            wxString xmlShown = m_xml_path.empty()
                ? _L("None")
                : wxFileName(m_xml_path).GetFullName();
            wxString txtShown = m_traj_path.empty()
                ? _L("None")
                : wxFileName(m_traj_path).GetFullName();

            if (m_xml_label)  m_xml_label->SetLabel(xmlShown);
            if (m_traj_label) m_traj_label->SetLabel(txtShown);

            if (m_xml_clear)  m_xml_clear->Enable(!m_xml_path.empty());
            if (m_traj_clear) m_traj_clear->Enable(!m_traj_path.empty());

            m_file_panel->Layout();
        }

        // ===================== 按钮事件 =====================

        void RobogsMujoco::onDownload(wxCommandEvent& WXUNUSED(event))
        {
            // 这里留空：如果你之后要做“从服务器下载 TXT”，可以在此实现
            wxLogMessage("Download TXT is not implemented in this prototype.");
        }

        void RobogsMujoco::onXml(wxCommandEvent& WXUNUSED(event))
        {
            auto* canvas = ensure_canvas(m_view_host);
            if (!canvas) {
                wxLogError("Failed to initialize MuJoCo canvas.");
                return;
            }

            wxFileDialog dlg(
                this,
                _L("Select MuJoCo XML"),
                "",
                "",
                "MuJoCo XML (*.xml;*.mjcf)|*.xml;*.mjcf|MuJoCo Binary (*.mjb)|*.mjb|All files (*.*)|*.*",
                wxFD_OPEN | wxFD_FILE_MUST_EXIST
            );
            if (dlg.ShowModal() != wxID_OK)
                return;

            // 只记录路径 + 更新 UI，不再在这里操作 canvas
            m_xml_path = dlg.GetPath();
            refreshFileLabels();

            // 如果你仍然希望“导入 XML 后立刻显示模型”，可以解开下面注释：
            /*
            if (!canvas->LoadModel(m_xml_path.ToStdString()))
                wxLogError("Failed to load model: %s", m_xml_path);
            else
                canvas->Start();
            */
        }

        void RobogsMujoco::onImport(wxCommandEvent& WXUNUSED(event))
        {
            wxFileDialog dlg(
                this,
                _L("Select trajectory TXT"),
                "",
                "",
                "Text files (*.txt)|*.txt|All files (*.*)|*.*",
                wxFD_OPEN | wxFD_FILE_MUST_EXIST
            );
            if (dlg.ShowModal() != wxID_OK)
                return;

            m_traj_path = dlg.GetPath();
            refreshFileLabels();
        }

        void RobogsMujoco::onReplay(wxCommandEvent& WXUNUSED(event))
        {
            auto* canvas = ensure_canvas(m_view_host);
            if (!canvas) {
                wxLogError("Failed to initialize MuJoCo canvas.");
                return;
            }

            if (m_xml_path.empty()) {
                wxMessageBox(_L("Please load a MuJoCo XML file first."),
                    _L("Replay"), wxOK | wxICON_INFORMATION, this);
                return;
            }
            if (m_traj_path.empty()) {
                wxMessageBox(_L("Please load a trajectory TXT file first."),
                    _L("Replay"), wxOK | wxICON_INFORMATION, this);
                return;
            }

            // 1) 播放前先加载 XML，可视化模型
            if (!canvas->LoadModel(m_xml_path.ToStdString())) {
                wxLogError("Failed to load model: %s", m_xml_path);
                return;
            }

            // 2) 加载 qpos 轨迹（假定首列是 step，其余是 qpos）
            const int fps = 50;  // 你之前 replay.py 用的是 50
            if (!canvas->LoadQposTrajectoryTxt(m_traj_path.ToStdString(), fps)) {
                wxLogError("Failed to load trajectory txt: %s", m_traj_path);
                return;
            }

            // 3) 开始循环回放
            canvas->StartReplay(/*loop=*/true, /*fps_override=*/fps);
        }

        void RobogsMujoco::onHostResize(wxSizeEvent& event)
        {
            event.Skip();
            if (auto* canvas = find_canvas(m_view_host))
                canvas->Refresh(false);
        }

        // ================= Windows 下兼容旧接口的空实现 =================
#ifdef __WXMSW__
        void RobogsMujoco::onPollSimWindow(wxTimerEvent& WXUNUSED(event))
        {
            // 以前是轮询外部 simulate.exe，现在内嵌 MuJoCo 不需要了
        }

        void RobogsMujoco::embedSimWindow(HWND)
        {
            // 不再嵌入外部窗口，留空实现即可
        }

        HWND RobogsMujoco::findTopWindowByPid(DWORD)
        {
            // 不再搜索外部窗口，直接返回 nullptr
            return nullptr;
        }
#endif // __WXMSW__

    } // namespace GUI
} // namespace Slic3r


//#include "RobogsMujoco.hpp"
//#include "mujoco/RobogsMuJoCoCanvas.hpp" 
//
//#include <wx/uri.h>
//#include <wx/stdpaths.h>
//#include <wx/utils.h>
//#include <wx/wx.h>
//#include <wx/statline.h>
//#include <wx/timer.h>
//#include <wx/filedlg.h>
//
//#pragma comment(lib, "winhttp.lib")
//#include <winhttp.h>
//
//#ifdef __WXMSW__
//#include <windows.h>
//#endif
//
//#include <string>   // 为 std::string
//
//namespace Slic3r {
//    namespace GUI {
//
//        namespace {
//            // 在 host 的子控件中查找 RobogsMuJoCoCanvas
//            static RobogsMuJoCoCanvas* find_canvas(wxPanel* host)
//            {
//                if (!host) return nullptr;
//                for (wxWindow* child : host->GetChildren()) {
//                    if (auto* c = dynamic_cast<RobogsMuJoCoCanvas*>(child))
//                        return c;
//                }
//                return nullptr;
//            }
//
//            // 确保 host 内有一个 RobogsMuJoCoCanvas，没有就创建并放进 sizer
//            static RobogsMuJoCoCanvas* ensure_canvas(wxPanel* host)
//            {
//                if (!host) return nullptr;
//                if (auto* c = find_canvas(host)) return c;
//
//                if (!host->GetSizer())
//                    host->SetSizer(new wxBoxSizer(wxVERTICAL));
//
//                auto* canvas = new RobogsMuJoCoCanvas(host);
//                host->GetSizer()->Add(canvas, 1, wxEXPAND);
//                host->Layout();
//                if (host->GetParent())
//                    host->GetParent()->Layout();
//                return canvas;
//            }
//        }
//
//        // ========================================================
//
//        RobogsMujoco::RobogsMujoco(wxWindow* parent)
//            : wxPanel(parent, wxID_ANY)
//        {
//            initUI();
//        }
//
//        void RobogsMujoco::initUI()
//        {
//            const int PAD = 8;
//            auto* main_sizer = new wxBoxSizer(wxVERTICAL);
//
//            // ================= 顶部工具栏 =================
//            auto* top_sizer = new wxBoxSizer(wxHORIZONTAL);
//            m_d_txt = new Button(this, wxString::FromUTF8(u8"下载TXT"));
//            m_xml = new Button(this, wxString::FromUTF8(u8"导入XML"));
//            m_i_txt = new Button(this, wxString::FromUTF8(u8"导入TXT"));
//            m_replay = new Button(this, wxString::FromUTF8(u8"播放"));
//
//            const wxSize btn_min(140, 40);
//            m_d_txt->SetMinSize(btn_min);
//            m_xml->SetMinSize(btn_min);
//            m_i_txt->SetMinSize(btn_min);
//            m_replay->SetMinSize(btn_min);
//
//            top_sizer->Add(m_d_txt, 0, wxALL | wxALIGN_CENTER_VERTICAL, PAD);
//            top_sizer->Add(m_xml, 0, wxTOP | wxBOTTOM | wxRIGHT | wxALIGN_CENTER_VERTICAL, PAD);
//            top_sizer->Add(m_i_txt, 0, wxTOP | wxBOTTOM | wxRIGHT | wxALIGN_CENTER_VERTICAL, PAD);
//            top_sizer->AddStretchSpacer(1);
//            top_sizer->Add(m_replay, 0, wxALL | wxALIGN_CENTER_VERTICAL, PAD);
//
//            main_sizer->Add(top_sizer, 0, wxEXPAND);
//            main_sizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, PAD);
//
//            // ================= 中部 MuJoCo 视图区 =================
//            m_view_host = new wxPanel(this, wxID_ANY);
//            m_view_host->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
//            m_view_host->SetSizer(new wxBoxSizer(wxVERTICAL));
//            main_sizer->Add(m_view_host, 1, wxEXPAND | wxALL, PAD);
//
//            // ================= 底部文件管理列表 =================
//            main_sizer->Add(
//                new wxStaticText(this, wxID_ANY, wxString::FromUTF8(u8"已导入文件（双击可删除）：")),
//                0, wxLEFT | wxRIGHT | wxTOP, PAD
//            );
//
//            m_file_list = new wxListCtrl(
//                this,
//                wxID_ANY,
//                wxDefaultPosition,
//                wxSize(-1, 150),
//                wxLC_REPORT | wxLC_SINGLE_SEL
//            );
//            m_file_list->InsertColumn(0, wxString::FromUTF8(u8"类型"), wxLIST_FORMAT_LEFT, 100);
//            m_file_list->InsertColumn(1, wxString::FromUTF8(u8"路径"), wxLIST_FORMAT_LEFT, 400);
//
//            main_sizer->Add(m_file_list, 0, wxEXPAND | wxALL, PAD);
//
//            // ================= 事件绑定 =================
//            m_d_txt->Bind(wxEVT_BUTTON, &RobogsMujoco::onDownload, this);
//            m_xml->Bind(wxEVT_BUTTON, &RobogsMujoco::onXml, this);
//            m_i_txt->Bind(wxEVT_BUTTON, &RobogsMujoco::onImport, this);
//            m_replay->Bind(wxEVT_BUTTON, &RobogsMujoco::onReplay, this);
//
//            // 双击列表项 -> 删除该文件记录 + 重置画布
//            m_file_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &RobogsMujoco::onRemoveFile, this);
//
//            m_view_host->Bind(wxEVT_SIZE, &RobogsMujoco::onHostResize, this);
//
//#ifdef __WXMSW__
//            // 仅为了兼容旧声明，不启用
//            m_poll_timer = new wxTimer(this);
//            Bind(wxEVT_TIMER, &RobogsMujoco::onPollSimWindow, this, m_poll_timer->GetId());
//#endif
//
//            SetSizer(main_sizer);
//            Layout();
//
//            // 提前创建一个空 canvas，避免第一次点按钮时再构造
//            ensure_canvas(m_view_host);
//        }
//
//        // =============== 顶部按钮回调 ===============
//
//        void RobogsMujoco::onDownload(wxCommandEvent& /*event*/)
//        {
//            wxMessageBox(
//                wxString::FromUTF8(u8"“下载TXT” 功能暂未实现，可在此加入 HTTP 下载逻辑。"),
//                wxString::FromUTF8(u8"提示"),
//                wxOK | wxICON_INFORMATION,
//                this
//            );
//        }
//
//        void RobogsMujoco::onXml(wxCommandEvent& /*event*/)
//        {
//            auto* canvas = ensure_canvas(m_view_host);
//            if (!canvas) {
//                wxLogError("Failed to initialize MuJoCo canvas.");
//                return;
//            }
//
//            wxFileDialog dlg(
//                this,
//                wxString::FromUTF8(u8"选择 MuJoCo 模型文件"),
//                "",
//                "",
//                u8"MuJoCo XML (*.xml;*.mjcf)|*.xml;*.mjcf|MuJoCo Binary (*.mjb)|*.mjb|All files (*.*)|*.*",
//                wxFD_OPEN | wxFD_FILE_MUST_EXIST
//            );
//            if (dlg.ShowModal() != wxID_OK)
//                return;
//
//            const wxString model = dlg.GetPath();
//            m_xml_path = model.ToStdString();
//
//            if (!canvas->LoadModel(m_xml_path)) {
//                wxLogError("Failed to load model: %s", model);
//                return;
//            }
//
//            // 添加到文件列表
//            addFileToList(wxString::FromUTF8(u8"XML"), model);
//
//            // 开始 ~60Hz 仿真
//            canvas->Start();
//        }
//
//        void RobogsMujoco::onImport(wxCommandEvent& /*event*/)
//        {
//            auto* canvas = ensure_canvas(m_view_host);
//            if (!canvas || !canvas->HasModel()) {
//                wxLogWarning("%s",
//                    wxString::FromUTF8(u8"请先导入 XML 模型，再导入 TXT 轨迹。").ToUTF8().data());
//                return;
//            }
//
//            wxFileDialog dlg(
//                this,
//                wxString::FromUTF8(u8"选择 qpos 轨迹 TXT 文件"),
//                "",
//                "",
//                u8"TXT 文件 (*.txt)|*.txt|All files (*.*)|*.*",
//                wxFD_OPEN | wxFD_FILE_MUST_EXIST
//            );
//            if (dlg.ShowModal() != wxID_OK)
//                return;
//
//            const wxString traj = dlg.GetPath();
//            m_traj_path = traj.ToStdString();
//
//            const int fps = 50;
//            if (!canvas->LoadQposTrajectoryTxt(m_traj_path, fps)) {
//                wxLogError("导入轨迹失败: %s", traj);
//                return;
//            }
//
//            addFileToList(wxString::FromUTF8(u8"TXT"), traj);
//        }
//
//        void RobogsMujoco::onReplay(wxCommandEvent& /*event*/)
//        {
//            auto* canvas = find_canvas(m_view_host);
//            if (!canvas) {
//                wxLogWarning("%s",
//                    wxString::FromUTF8(u8"MuJoCo 画布尚未初始化。").ToUTF8().data());
//                return;
//            }
//            if (!canvas->HasModel()) {
//                wxLogWarning("%s",
//                    wxString::FromUTF8(u8"请先导入 XML 模型。").ToUTF8().data());
//                return;
//            }
//            if (m_traj_path.empty()) {
//                wxLogWarning("%s",
//                    wxString::FromUTF8(u8"请先导入 TXT 轨迹。").ToUTF8().data());
//                return;
//            }
//
//            const int fps = 50;
//            canvas->StartReplay(true, fps);
//        }
//
//        // =============== 文件列表管理 ===============
//
//        void RobogsMujoco::addFileToList(const wxString& type, const wxString& path)
//        {
//            long idx = m_file_list->InsertItem(m_file_list->GetItemCount(), type);
//            m_file_list->SetItem(idx, 1, path);
//        }
//
//        void RobogsMujoco::onRemoveFile(wxListEvent& event)
//        {
//            long idx = event.GetIndex();
//            if (idx < 0) return;
//
//            wxString type = m_file_list->GetItemText(idx);
//            wxString path = m_file_list->GetItemText(idx, 1);
//
//            if (wxMessageBox(
//                wxString::FromUTF8(u8"确认从列表删除该文件记录？\n（不会删除磁盘上的实际文件）"),
//                wxString::FromUTF8(u8"删除确认"),
//                wxYES_NO | wxICON_QUESTION,
//                this) != wxYES) {
//                return;
//            }
//
//            // 同步清理内部路径
//            if (type == wxString::FromUTF8(u8"XML")) {
//                if (m_xml_path == path.ToStdString())
//                    m_xml_path.clear();
//            }
//            else if (type == wxString::FromUTF8(u8"TXT")) {
//                if (m_traj_path == path.ToStdString())
//                    m_traj_path.clear();
//            }
//
//            // 删除列表项
//            m_file_list->DeleteItem(idx);
//
//            // ✨ 关键：重置下方 MuJoCo 画布为“初始状态”
//            if (auto* canvas = find_canvas(m_view_host)) {
//                canvas->Stop();      // 停止计时器
//                canvas->Destroy();   // 销毁旧 canvas（释放 m_model / m_data 等）
//            }
//            // 创建一个全新的空 canvas，避免空指针访问
//            ensure_canvas(m_view_host);
//            m_view_host->Layout();
//            Layout();
//
//            wxLogMessage("已从列表移除：%s", path);
//        }
//
//        // =============== 尺寸变化 ===============
//
//        void RobogsMujoco::onHostResize(wxSizeEvent& event)
//        {
//            event.Skip();
//            if (auto* canvas = find_canvas(m_view_host))
//                canvas->Refresh(false);
//        }
//
//#ifdef __WXMSW__
//        // 仅为兼容旧声明，保持空实现
//        void RobogsMujoco::onPollSimWindow(wxTimerEvent&) {}
//        void RobogsMujoco::embedSimWindow(HWND) {}
//        HWND RobogsMujoco::findTopWindowByPid(DWORD) { return nullptr; }
//#endif
//
//    } // namespace GUI
//} // namespace Slic3r