#ifndef slic3r_RobogsMujoco_hpp_
#define slic3r_RobogsMujoco_hpp_

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
#include <string>

namespace Slic3r {
    namespace GUI {
        class RobogsMujoco final : public wxPanel
        {
        public:
            explicit RobogsMujoco(wxWindow* parent);
            ~RobogsMujoco() override = default;

        private:
            void initUI();

            // 顶部按钮
            void onDownload(wxCommandEvent& event);  // 下载 TXT（占位）
            void onXml(wxCommandEvent& event);       // 导入 XML（只记录路径）
            void onImport(wxCommandEvent& event);    // 导入 TXT（只记录路径）
            void onReplay(wxCommandEvent& event);    // 播放：真正加载 XML + TXT 并 replay
            void onHostResize(wxSizeEvent& event);

#ifdef __WXMSW__
            // 为兼容老代码保留的空实现（不再使用外部 simulate.exe）
            void onPollSimWindow(wxTimerEvent& event);
            void embedSimWindow(HWND hwnd);
            HWND findTopWindowByPid(DWORD pid);
            wxTimer* m_poll_timer = nullptr;
#endif

            // 顶部工具栏按钮（使用 Button，不用 wxButton）
            Button* m_d_txt = nullptr;
            Button* m_xml = nullptr;
            Button* m_i_txt = nullptr;
            Button* m_replay = nullptr;

            // “文件管理”区域：显示当前已选择的 XML / TXT，并支持清空
            wxPanel* m_file_panel = nullptr;
            wxStaticText* m_xml_label = nullptr;
            wxStaticText* m_traj_label = nullptr;
            Button* m_xml_clear = nullptr;
            Button* m_traj_clear = nullptr;

            // 下方嵌入 MuJoCo 的宿主面板
            wxPanel* m_view_host = nullptr;

            // 记录文件完整路径（这里只是字符串，不碰 MuJoCo 指针）
            wxString m_xml_path;
            wxString m_traj_path;

            // 根据当前路径刷新 label、清空按钮状态
            void refreshFileLabels();

            wxDECLARE_EVENT_TABLE();
        };

    } // namespace GUI
} // namespace Slic3r

#endif // slic3r_RobogsMujoco_hpp_


//#ifndef slic3r_RobogsMujoco_hpp_
//#define slic3r_RobogsMujoco_hpp_
//
//#include <wx/panel.h>
//#include <wx/button.h>
//#include <wx/textctrl.h>
//#include <wx/sizer.h>
//#include <wx/filedlg.h>
//#include "GUI_App.hpp"
//#include <wx/mediactrl.h>
//#include <atomic>
//#include <wx/wx.h>
//#include <wx/zipstrm.h>
//#include <thread>
//#include <wx/timer.h>
//#include <string>
//
//namespace Slic3r {
//    namespace GUI {
//
//        class RobogsMujoco : public wxPanel
//        {
//        public:
//            RobogsMujoco(wxWindow* parent);
//
//            enum { ID_UPLOAD_TIMER = wxID_HIGHEST + 1001 };
//
//        private:
//            Button* m_d_txt;
//            Button* m_xml;
//            Button* m_i_txt;
//            Button* m_replay;
//            wxPanel* m_view_host;
//            std::string m_traj_path;
//            std::string m_xml_path;
//            wxListCtrl* m_file_list = nullptr;
//#ifdef __WXMSW__
//            wxTimer* m_poll_timer = nullptr;
//#endif
//            void initUI();
//            void onDownload(wxCommandEvent& event);
//            void onXml(wxCommandEvent& event);
//            void onImport(wxCommandEvent& event);
//            void onReplay(wxCommandEvent& event);
//            void onHostResize(wxSizeEvent& event);
//
//            void addFileToList(const wxString& type, const wxString& path);
//            void onRemoveFile(wxListEvent& event);
//
//#ifdef __WXMSW__
//
//            void onPollSimWindow(wxTimerEvent&);    // 定时器回调
//            void embedSimWindow(HWND simHwnd);      // 真正嵌入
//            HWND findTopWindowByPid(DWORD pid);
//
//            long     m_sim_pid{ 0 };              // wxExecute 返回的 PID
//            HWND     m_sim_hwnd{ nullptr };        // simulate 的主窗口句柄
//#endif
//
//
//        };
//    }
//} // namespace Slic3r::GUI
//
//#endif