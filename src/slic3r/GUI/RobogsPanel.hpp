#ifndef slic3r_RobogsPanel_hpp_
#define slic3r_RobogsPanel_hpp_

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

namespace Slic3r {
    namespace GUI {

        class RobogsPanel : public wxPanel
        {
        public:
            RobogsPanel(wxWindow* parent);

        enum { ID_UPLOAD_TIMER = wxID_HIGHEST + 1001 };

        private:
            // 按钮控件
            Button* m_btn_import_video;
            Button* m_btn_get_images;
            Button* m_btn_rebuild;
            Button* m_btn_train_gs;
            Button* m_btn_extract;
            Button* m_btn_download_gs;
            Button* m_btn_edit_gs;
            Button* m_btn_train_mesh;
            Button* m_btn_render_mesh;
            Button* m_btn_segment;
            Button* m_btn_generate;
            Button* m_btn_dye;
            Button* m_btn_cad_print;
            Button* m_btn_confirm_upload_videos;
            Button* m_btn_cancel_upload_videos;
            Button* m_btn_assign;
            Button* m_btn_download_mesh;
            Button* m_btn_finetune;
            std::atomic<bool> m_extract_cancelled{ false };
            std::string m_task_id;
            std::atomic<int> m_upload_pct{ -1 };
            wxTimer* m_upload_timer{ nullptr };

            // 输入框
            wxTextCtrl* m_text_assign;
            wxTextCtrl* m_text_finetune;

            // 视频预览窗口
            wxPanel* m_video_container;
            wxMediaCtrl* m_media_ctrl;

            void initUI();
            void onImportVideo(wxCommandEvent& event);
            void OnGetImages(wxCommandEvent& event);
            void onRebuild(wxCommandEvent& event);
            void onTrainGS(wxCommandEvent& event);
            void onExtract(wxCommandEvent& event);
            void onDownload_gs(wxCommandEvent& event);
            void onEditGS(wxCommandEvent& event);
            void onTrainMesh(wxCommandEvent& event);
            void onRenderMesh(wxCommandEvent& event);
            void onDownloadMesh(wxCommandEvent& event);
            void onSegment(wxCommandEvent& event);
            void onGenerate(wxCommandEvent& event);
            void onDye(wxCommandEvent& event);
            void onAssignId(wxCommandEvent& event);
            void onFinetune(wxCommandEvent& event);
            void onCadPrint(wxCommandEvent& event);
            void OnUploadTimer(wxTimerEvent& event);
            void CloseUploadProgress(bool forceTo100 = false);

            // === 上传相关状态 ===
            wxString                    m_selected_video_path;
            std::unique_ptr<std::thread> m_upload_thread;
            std::atomic<bool>     m_upload_cancel{ false };
            std::atomic<bool>     m_upload_running{ false };
            wxProgressDialog* m_upload_progress{ nullptr };

            // 事件
            void onConfirmUpload(wxCommandEvent& event);
            void onCancelUpload(wxCommandEvent& event);
        };

    }
} // namespace Slic3r::GUI

#endif