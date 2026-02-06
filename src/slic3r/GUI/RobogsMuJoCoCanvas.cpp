#include "RobogsMuJoCoCanvas.hpp"

#include <wx/wx.h>
#include <algorithm>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#else
#include <OpenGL/gl.h>
#endif

using namespace Slic3r::GUI;

// OpenGL attributes
static int sGLAttrs[] = {
    WX_GL_RGBA, WX_GL_DOUBLEBUFFER,
    WX_GL_DEPTH_SIZE, 24,
    WX_GL_SAMPLE_BUFFERS, 1, WX_GL_SAMPLES, 4,
    0
};

// static UI storage
int    RobogsMuJoCoCanvas::s_paused = 0;
double RobogsMuJoCoCanvas::s_speed = 1.0;

#ifdef _WIN32
// 输出 MuJoCo 错误到调试窗口
static void MJErrorToDebugger(const char* msg)
{
    ::OutputDebugStringA("MuJoCo error: ");
    ::OutputDebugStringA(msg);
    ::OutputDebugStringA("\n");
}
static void MJWarnToDebugger(const char* msg)
{
    ::OutputDebugStringA("MuJoCo warn: ");
    ::OutputDebugStringA(msg);
    ::OutputDebugStringA("\n");
}
#endif

wxBEGIN_EVENT_TABLE(RobogsMuJoCoCanvas, wxGLCanvas)
EVT_PAINT(RobogsMuJoCoCanvas::OnPaint)
EVT_SIZE(RobogsMuJoCoCanvas::OnResize)
EVT_TIMER(wxID_ANY, RobogsMuJoCoCanvas::OnTimer)
EVT_LEFT_DOWN(RobogsMuJoCoCanvas::OnMouse)
EVT_LEFT_UP(RobogsMuJoCoCanvas::OnMouse)
EVT_RIGHT_DOWN(RobogsMuJoCoCanvas::OnMouse)
EVT_RIGHT_UP(RobogsMuJoCoCanvas::OnMouse)
EVT_MIDDLE_DOWN(RobogsMuJoCoCanvas::OnMouse)
EVT_MIDDLE_UP(RobogsMuJoCoCanvas::OnMouse)
EVT_MOTION(RobogsMuJoCoCanvas::OnMouse)
EVT_MOUSEWHEEL(RobogsMuJoCoCanvas::OnMouse)
EVT_LEAVE_WINDOW(RobogsMuJoCoCanvas::OnLeave)
EVT_KEY_DOWN(RobogsMuJoCoCanvas::OnKeyDown)
EVT_CHAR(RobogsMuJoCoCanvas::OnChar)
wxEND_EVENT_TABLE()

RobogsMuJoCoCanvas::RobogsMuJoCoCanvas(wxWindow* parent)
    : wxGLCanvas(parent, wxID_ANY, sGLAttrs)
    , m_ctx(new wxGLContext(this))
    , m_timer(this)
{
    mjv_defaultCamera(&m_cam);
    mjv_defaultOption(&m_opt);

    // 清零 UI 结构体，避免未初始化内存
    std::memset(&m_ui, 0, sizeof(m_ui));
    std::memset(&m_uistate, 0, sizeof(m_uistate));
    std::memset(&m_scn, 0, sizeof(m_scn));
    std::memset(&m_con, 0, sizeof(m_con));

#ifdef _WIN32
    mju_user_error = MJErrorToDebugger;
    mju_user_warning = MJWarnToDebugger;
#endif
}

RobogsMuJoCoCanvas::~RobogsMuJoCoCanvas()
{
    Stop();
    StopReplay();

    if (m_ui_inited) {
        mjr_freeContext(&m_con);
        mjv_freeScene(&m_scn);
    }
    if (m_data) {
        mj_deleteData(m_data);
        m_data = nullptr;
    }
    if (m_model) {
        mj_deleteModel(m_model);
        m_model = nullptr;
    }
}

bool RobogsMuJoCoCanvas::LoadModel(const std::string& path)
{
    char err[1024] = { 0 };
    mjModel* model = mj_loadXML(path.c_str(), nullptr, err, sizeof(err));
    if (!model) {
        wxLogError("MuJoCo: failed to load model: %s", err);
        return false;
    }

    // 替换旧模型
    if (m_data) {
        mj_deleteData(m_data);
        m_data = nullptr;
    }
    if (m_model) {
        mj_deleteModel(m_model);
        m_model = nullptr;
    }

    m_model = model;
    m_data = mj_makeData(m_model);

    SetCurrent(*m_ctx);

    if (!m_ui_inited) {
        // 创建 scene & context
        mjv_makeScene(m_model, &m_scn, 10000);
        mjr_makeContext(m_model, &m_con, mjFONTSCALE_150);

        // UI 主题
        m_ui.spacing = mjui_themeSpacing(1);
        m_ui.color = mjui_themeColor(1);
        m_ui.rectid = 1; // rect[1] 作为 UI 区域

        // 右侧 UI 定义
        mjuiDef defs[] = {
            { mjITEM_SECTION,   "Simulation",  1, nullptr,  "" },
            { mjITEM_CHECKINT,  "Pause",       2, &s_paused, "" },
            { mjITEM_BUTTON,    "Reset",       2, nullptr,   "" },
            { mjITEM_SLIDERNUM, "Speed",       2, &s_speed,  "0.1 4 0.1" },
            { mjITEM_END,       "",            0, nullptr,   "" }
        };
        mjui_add(&m_ui, defs);

        // 两个 rect：0=3D, 1=UI
        m_uistate.nrect = 2;
        UpdateRects();
        mjui_resize(&m_ui, &m_con);
        mjui_update(-1, -1, &m_ui, &m_uistate, &m_con);

        m_ui_inited = true;
    }
    else {
        // 替换模型时，同步 UI 布局
        UpdateRects();
        mjui_resize(&m_ui, &m_con);
        mjui_update(-1, -1, &m_ui, &m_uistate, &m_con);
    }

    mj_resetData(m_model, m_data);
    m_ready = true;
    Refresh(false);
    return true;
}

bool RobogsMuJoCoCanvas::LoadQposTrajectoryTxt(const std::string& path, int fps)
{
    if (!m_model) {
        wxLogWarning("No MuJoCo model loaded; cannot load trajectory.");
        return false;
    }

    std::ifstream fin(path);
    if (!fin.is_open()) {
        wxLogError("Failed to open trajectory txt: %s", path);
        return false;
    }

    const int nq = m_model->nq;
    if (nq <= 0) {
        wxLogError("Model has nq = %d; invalid for qpos trajectory.", nq);
        return false;
    }

    std::vector<mjtNum> buffer;
    buffer.reserve(1024 * nq);

    std::string line;
    int line_idx = 0;
    while (std::getline(fin, line)) {
        ++line_idx;
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        std::stringstream ss(line);
        std::vector<double> vals;
        double v;
        while (ss >> v) vals.push_back(v);
        if (vals.size() < static_cast<size_t>(nq + 1)) {
            wxLogWarning("Line %d: expect >= %d values, got %zu. Skipped.",
                line_idx, nq + 1, vals.size());
            continue;
        }
        // 跳过第一个 step 索引，从第 2 个值开始是 qpos
        for (int i = 0; i < nq; ++i) {
            buffer.push_back(static_cast<mjtNum>(vals[i + 1]));
        }
    }

    fin.close();

    const int frames = (nq > 0) ? static_cast<int>(buffer.size() / nq) : 0;
    if (frames <= 0) {
        wxLogWarning("Trajectory file has no valid frames: %s", path);
        m_qpos_traj.clear();
        m_traj_frames = 0;
        return false;
    }

    m_qpos_traj.swap(buffer);
    m_traj_frames = frames;
    m_traj_fps = (fps > 0) ? fps : 50;
    m_replay_index = 0;
    m_mode_replay = false;

    wxLogMessage("Loaded qpos trajectory: %d frames (nq=%d) from %s",
        m_traj_frames, nq, path.c_str());
    return true;
}

void RobogsMuJoCoCanvas::Start()
{
    if (!m_ready) return;
    m_mode_replay = false;  // 正常仿真
    m_timer.Start(16);      // ~60Hz
}

void RobogsMuJoCoCanvas::Stop()
{
    m_timer.Stop();
}

void RobogsMuJoCoCanvas::StartReplay(bool loop, int fps)
{
    if (!m_ready || m_traj_frames <= 0) {
        wxLogWarning("Cannot start replay: no model or trajectory.");
        return;
    }

    m_replay_loop = loop;
    if (fps > 0) m_traj_fps = fps;
    m_replay_index = 0;
    m_mode_replay = true;

    int interval_ms = static_cast<int>(1000.0 / std::max(1, m_traj_fps));
    m_timer.Start(interval_ms);

    wxLogMessage("Start replay: frames=%d, fps=%d, loop=%d",
        m_traj_frames, m_traj_fps, m_replay_loop ? 1 : 0);
}

void RobogsMuJoCoCanvas::StopReplay()
{
    m_mode_replay = false;
    m_replay_index = 0;
}

void RobogsMuJoCoCanvas::OnTimer(wxTimerEvent&)
{
    if (!m_ready || !m_model || !m_data) return;

    if (m_mode_replay) {
        StepReplay();
    }
    else {
        StepSimulation();
    }
    Refresh(false);
}

void RobogsMuJoCoCanvas::StepSimulation()
{
    if (s_paused) return;

    int sub = std::max(1, (int)std::lround(s_speed));
    for (int i = 0; i < sub; ++i) {
        mj_step(m_model, m_data);
    }
}

void RobogsMuJoCoCanvas::StepReplay()
{
    if (m_traj_frames <= 0) return;

    int nq = m_model->nq;
    if (nq <= 0) return;

    if (m_replay_index >= m_traj_frames) {
        if (m_replay_loop) {
            m_replay_index = 0;
        }
        else {
            StopReplay();
            return;
        }
    }

    // 将当前帧 qpos 写入 m_data
    const mjtNum* src = &m_qpos_traj[static_cast<size_t>(m_replay_index) * nq];
    std::copy(src, src + nq, m_data->qpos);

    // 用 forward 更新 kinematics
    mj_forward(m_model, m_data);

    ++m_replay_index;
}

void RobogsMuJoCoCanvas::OnResize(wxSizeEvent& event)
{
    if (!m_ready) {
        event.Skip();
        return;
    }

    SetCurrent(*m_ctx);
    UpdateRects();
    if (m_ui_inited) {
        mjui_resize(&m_ui, &m_con);
        mjui_update(-1, -1, &m_ui, &m_uistate, &m_con);
    }

    event.Skip();
}

void RobogsMuJoCoCanvas::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    if (!m_ready) return;

    SetCurrent(*m_ctx);

    mjr_setBuffer(mjFB_WINDOW, &m_con);

    const wxSize sz = GetClientSize();
    const int UIW = 260;

    // 更新场景
    mjv_updateScene(m_model, m_data, &m_opt, nullptr, &m_cam, mjCAT_ALL, &m_scn);

    // 左侧 3D 视图
    mjrRect viewport;
    viewport.left = 0;
    viewport.bottom = 0;
    viewport.width = std::max(0, sz.x - UIW);
    viewport.height = sz.y;
    mjr_render(viewport, &m_scn, &m_con);

    // 渲染右侧 UI
    if (m_ui_inited) {
        mjui_render(&m_ui, &m_uistate, &m_con);
    }

    glFlush();
    SwapBuffers();
}

void RobogsMuJoCoCanvas::UpdateRects()
{
    const wxSize sz = GetClientSize();
    const int UIW = 260;

    // rect[0] = 3D 区域
    m_uistate.rect[0].left = 0;
    m_uistate.rect[0].bottom = 0;
    m_uistate.rect[0].width = std::max(0, sz.x - UIW);
    m_uistate.rect[0].height = sz.y;

    // rect[1] = UI 区域
    m_uistate.rect[1].left = std::max(0, sz.x - UIW);
    m_uistate.rect[1].bottom = 0;
    m_uistate.rect[1].width = std::min(UIW, sz.x);
    m_uistate.rect[1].height = sz.y;

    m_uistate.nrect = 2;
}

void RobogsMuJoCoCanvas::SendUIEvent(int type, int button, double sx, double sy)
{
    m_uistate.type = type;
    m_uistate.button = button;
    m_uistate.key = 0;

    m_uistate.control = wxGetKeyState(WXK_CONTROL);
    m_uistate.shift = wxGetKeyState(WXK_SHIFT);
    m_uistate.alt = wxGetKeyState(WXK_ALT);

    m_uistate.sx = sx;
    m_uistate.sy = sy;

    mjuiItem* it = mjui_event(&m_ui, &m_uistate, &m_con);

    if (it && it->type == mjITEM_BUTTON) {
        if (std::string(it->name) == "Reset") {
            if (m_model && m_data) {
                mj_resetData(m_model, m_data);
                m_replay_index = 0;
            }
        }
    }

    mjui_update(-1, -1, &m_ui, &m_uistate, &m_con);
}

void RobogsMuJoCoCanvas::OnMouse(wxMouseEvent& e)
{
    if (!m_ready) return;
    SetCurrent(*m_ctx);

    const wxPoint p = e.GetPosition();
    const wxSize sz = GetClientSize();

    const double x = static_cast<double>(p.x);
    const double y = static_cast<double>(sz.y - p.y); // MuJoCo 原点左下

    auto inRect = [](const mjrRect& r, double X, double Y) {
        return (X >= r.left && X < r.left + r.width &&
            Y >= r.bottom && Y < r.bottom + r.height);
        };

    // 判断鼠标落在哪个 rect（0=3D,1=UI）
    if (inRect(m_uistate.rect[1], x, y))
        m_uistate.mouserect = 1;
    else
        m_uistate.mouserect = 0;

    m_uistate.x = x;
    m_uistate.y = y;
    m_uistate.dx = x - m_lastx;
    m_uistate.dy = y - m_lasty;
    m_lastx = x;
    m_lasty = y;

    m_uistate.left = e.LeftIsDown();
    m_uistate.right = e.RightIsDown();
    m_uistate.middle = e.MiddleIsDown();

    if (e.GetWheelRotation() != 0) {
        const double step = (double)e.GetWheelRotation() / e.GetWheelDelta();
        SendUIEvent(mjEVENT_SCROLL, mjBUTTON_NONE, 0.0, step);

        // 在 3D 区域滚轮缩放相机
        if (m_uistate.mouserect == 0) {
            mjv_moveCamera(m_model, mjMOUSE_ZOOM, 0,
                -step * 0.05, &m_scn, &m_cam);
        }
        Refresh(false);
        return;
    }

    if (e.LeftDown() || e.RightDown() || e.MiddleDown()) {
        int btn = mjBUTTON_NONE;
        if (e.LeftDown())      btn = mjBUTTON_LEFT;
        else if (e.RightDown()) btn = mjBUTTON_RIGHT;
        else if (e.MiddleDown()) btn = mjBUTTON_MIDDLE;
        SendUIEvent(mjEVENT_PRESS, btn);
    }
    else if (e.Dragging()) {
        SendUIEvent(mjEVENT_MOVE);
    }
    else if (e.LeftUp() || e.RightUp() || e.MiddleUp()) {
        SendUIEvent(mjEVENT_RELEASE);
    }

    // 只在 3D 区域做相机交互
    if (m_uistate.mouserect == 0 && e.Dragging()) {
        if (m_uistate.left && !m_uistate.shift) {
            mjv_moveCamera(m_model, mjMOUSE_ROTATE_V,
                m_uistate.dx, -m_uistate.dy, &m_scn, &m_cam);
        }
        else if (m_uistate.left && m_uistate.shift) {
            mjv_moveCamera(m_model, mjMOUSE_MOVE_H,
                m_uistate.dx, -m_uistate.dy, &m_scn, &m_cam);
        }
        else if (m_uistate.right) {
            mjv_moveCamera(m_model, mjMOUSE_ZOOM,
                0, -m_uistate.dy, &m_scn, &m_cam);
        }
    }

    Refresh(false);
}

void RobogsMuJoCoCanvas::OnLeave(wxMouseEvent&)
{
    m_uistate.dx = 0;
    m_uistate.dy = 0;
}

void RobogsMuJoCoCanvas::OnKeyDown(wxKeyEvent& e)
{
    if (!m_ready) return;
    SetCurrent(*m_ctx);

    m_uistate.type = mjEVENT_KEY;
    m_uistate.key = e.GetKeyCode();
    mjui_event(&m_ui, &m_uistate, &m_con);
}

void RobogsMuJoCoCanvas::OnChar(wxKeyEvent& e)
{
    if (!m_ready) return;
    SetCurrent(*m_ctx);

    m_uistate.type = mjEVENT_KEY;
    m_uistate.key = e.GetUnicodeKey();
    mjui_event(&m_ui, &m_uistate, &m_con);
}
