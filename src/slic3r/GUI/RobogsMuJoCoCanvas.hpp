#pragma once

#include <wx/glcanvas.h>
#include <wx/timer.h>
#include <memory>
#include <string>
#include <vector>

#include <mujoco/mujoco.h>

namespace Slic3r {
    namespace GUI {

        class RobogsMuJoCoCanvas final : public wxGLCanvas
        {
        public:
            explicit RobogsMuJoCoCanvas(wxWindow* parent);
            ~RobogsMuJoCoCanvas() override;

            // 加载 MuJoCo 模型
            bool LoadModel(const std::string& path);

            // 是否已经有模型
            bool HasModel() const { return m_model != nullptr; }

            // 从 txt 载入 qpos 轨迹（txt 每行: [step, qpos0, qpos1, ...]）
            // fps 仅用于回放时的定时器间隔
            bool LoadQposTrajectoryTxt(const std::string& path, int fps);

            // 开始/停止正常物理仿真（mj_step）
            void Start();
            void Stop();

            // 开始/停止 qpos 轨迹回放
            // loop = true 表示播放完从头循环
            void StartReplay(bool loop, int fps);
            void StopReplay();

        private:
            // 事件
            void OnPaint(wxPaintEvent&);
            void OnResize(wxSizeEvent&);
            void OnTimer(wxTimerEvent&);
            void OnMouse(wxMouseEvent&);
            void OnLeave(wxMouseEvent&);
            void OnKeyDown(wxKeyEvent&);
            void OnChar(wxKeyEvent&);

            // 内部辅助
            void UpdateRects();  // 根据窗口大小更新 UI rect
            void SendUIEvent(int type, int button = mjBUTTON_NONE,
                double sx = 0.0, double sy = 0.0);

            void StepSimulation();  // 正常物理步进
            void StepReplay();      // 轨迹回放步进

        private:
            std::unique_ptr<wxGLContext> m_ctx;
            wxTimer m_timer;

            // MuJoCo 核心
            mjModel* m_model = nullptr;
            mjData* m_data = nullptr;

            // 视图相关
            mjvCamera  m_cam{};
            mjvOption  m_opt{};
            mjvScene   m_scn{};
            mjrContext m_con{};

            // 内置 UI
            mjUI      m_ui{};
            mjuiState m_uistate{};

            // 状态
            bool   m_ready = false;  // 是否已加载模型
            bool   m_ui_inited = false;
            double m_lastx = 0.0;
            double m_lasty = 0.0;

            // UI 绑定变量（静态，保证地址稳定）
            static int    s_paused;
            static double s_speed;

            // 轨迹回放数据
            std::vector<mjtNum> m_qpos_traj;  // size = frames * nq
            int   m_traj_frames = 0;         // 帧数
            int   m_traj_fps = 50;        // 回放 fps
            int   m_replay_index = 0;         // 当前回放帧
            bool  m_mode_replay = false;     // 是否处于回放模式
            bool  m_replay_loop = true;      // 播放完是否循环

            wxDECLARE_EVENT_TABLE();
        };

    } // namespace GUI
} // namespace Slic3r
