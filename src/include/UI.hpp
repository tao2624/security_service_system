#pragma once

#include "Camera.hpp"
#include "FFmpeg.hpp"
#include "Lvgl.hpp"
#include "RknnPool.hpp"
#include "PageManager.hpp"

#include <atomic>
#include <cstdint>

#define ACCESS_CONTROL_PAGE_DELAY_TIME 7000
#define SECURITY_CAMERA_PAGE_AUTO_RECORD_DELAY_TIME 2

class MainPage : public BasePage {
  private:
    LvObject * main_screen;
    
    void create_face_recognition_button();
    void create_security_monitor_button();

  public:
    MainPage();
    void show() override;
    void hide() override;
};

// 访问控制页面
class AccessControlPage : public BasePage {
  private:
    Camera & camera_;
    FaceRknnPool & face_rknn_pool_;
    ImageProcess & image_process_;
    LvTimer * display_timer;
    std::atomic_bool processing_active = false;
    std::mutex capture_frame_mutex_;

    LvObject * primary_screen;
    LvObject * standby_screen;

    std::atomic_bool is_standby_mode = false;

    // 记录前一次时间戳
    uint64_t previous_timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();

    std::function<void()> switch_to_standby_fn;

    // 私有方法：创建UI组件
    void setup_navigation_button();
    void setup_recognition_toggle();
    void setup_face_counter_display();
    void setup_face_registration_button();
    void initialize_display_timer();

  public:
    AccessControlPage(Camera & camera, FaceRknnPool & face_rknn_pool, ImageProcess & image_process);

    void show() override;
    void hide() override;

    void activate_standby_display();
    void activate_normal_display();
};

// 安防监控页面
class SecurityCameraPage : public BasePage {
  private:
    Camera & camera_;
    ImageProcess & image_process_;
    SecurityRknnPool & security_rknn_pool_;
    FFmpeg & ffmpeg_;

    std::atomic_bool surveillance_active_     = false;
    std::atomic_bool manual_recording_active_ = false;
    std::atomic_bool auto_recording_enabled_ = false;
    std::atomic_bool auto_recording_active_ = false;
    std::atomic_bool alert_enabled_ = false;
    std::atomic_bool alert_processing_ = false;
    uint64_t recording_start_timestamp_ = 0;

    std::mutex video_sync_mutex_;
    std::condition_variable sync_condition_;
    bool stream_processing_turn_ = false; // true表示轮到流处理线程取帧

    // 私有方法：创建UI组件和处理逻辑
    void create_navigation_button();
    void create_recording_controls();
    void create_auto_record_controls();
    void create_alert_controls();
    void initialize_video_timer();
    void handle_auto_recording_logic();
    void handle_alert_logic();
    void start_manual_recording();
    void stop_manual_recording();

  public:
    SecurityCameraPage(Camera & camera, ImageProcess & image_process, SecurityRknnPool & security_rknn_pool,
                       FFmpeg & ffmpeg);
    void show() override;
    void hide() override;
};

// 加载文件列表页面
class FileListPage : public BasePage {
  private:

  public:
    FileListPage();
    void show() override;
    void hide() override;
};
