#include "Font.hpp"
#include "Lvgl.hpp"
#include "UI.hpp"
#include "Util.hpp"
#include <iostream>
#include <mutex>
#include <src/core/lv_obj.h>
#include <src/layouts/flex/lv_flex.h>
#include <src/misc/lv_area.h>
#include <src/misc/lv_color.h>
#include <src/misc/lv_palette.h>
#include <thread>

extern "C" {
LV_IMAGE_DECLARE(background);
}

static LvObject * surveillance_screen;
static LvImageDsc * video_stream_desc_ = new LvImageDsc;
static LvLabel * detection_alert_label;
static LvImage * monitor_display_;
static LvTimer * refresh_timer;
static LvObject * recording_indicator;
static LvAnimation * recording_blink_anim;

static void opacity_animation_callback_(void * target, int32_t opacity_value)
{
    lv_obj_set_style_bg_opa((lv_obj_t *)target, opacity_value, 0);
}

SecurityCameraPage::SecurityCameraPage(Camera & camera, ImageProcess & image_process,
                                       SecurityRknnPool & security_rknn_pool, FFmpeg & ffmpeg)
    : camera_(camera), image_process_(image_process), security_rknn_pool_(security_rknn_pool), ffmpeg_(ffmpeg)
{
    surveillance_screen = new LvObject(nullptr);
    surveillance_screen->set_style_bg_image_src(&background, 0).set_style_text_font(Font16::get_font(), 0);

    monitor_display_ = new LvImage(surveillance_screen->raw());
    monitor_display_->align(LV_ALIGN_CENTER, 0, -50);

    detection_alert_label = new LvLabel(surveillance_screen->raw(), "检测到人体！", lv_palette_main(LV_PALETTE_RED));
    detection_alert_label->align(LV_ALIGN_CENTER, 0, 100).set_style_text_font(Font24::get_font(), 0).add_flag(LV_OBJ_FLAG_HIDDEN);

    recording_indicator = new LvObject(monitor_display_->raw());

    recording_indicator->add_flag(LV_OBJ_FLAG_HIDDEN)
        .align(LV_ALIGN_TOP_RIGHT, -10, 10)
        .set_size(100, 40)
        .set_flex_flow(LV_FLEX_FLOW_ROW)
        .set_flex_align(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER)
        .set_style_border_color(lv_color_hex(0xff0000), 0)
        .set_style_bg_opa(LV_OPA_30, 0)
        .remove_flag(LV_OBJ_FLAG_SCROLLABLE);

    LvObject recording_dot(recording_indicator->raw());

    recording_dot.set_size(20, 20)
        .set_style_radius(20, 0)
        .set_style_border_width(0, 0)
        .set_style_bg_color(lv_color_hex(0xff0000), 0)
        .remove_flag(LV_OBJ_FLAG_SCROLLABLE);

    LvLabel recording_text(recording_indicator->raw(), "REC", lv_color_black());

    recording_blink_anim = new LvAnimation();
    recording_blink_anim->set_var(recording_dot.raw())
        .set_exec_cb(opacity_animation_callback_)
        .set_duration(1000)
        .set_delay(0)
        .set_values(0, 255)
        .set_path_cb(lv_anim_path_ease_in_out)
        .set_repeat_count(LV_ANIM_REPEAT_INFINITE);

    // 创建界面控件
    create_navigation_button();
    create_recording_controls();
    create_auto_record_controls();
    create_alert_controls();
    initialize_video_timer();

    refresh_timer->pause();
}

void SecurityCameraPage::create_navigation_button()
{
    LvButton navigation_button(surveillance_screen->raw(), "返回");
    navigation_button.set_pos(10, 10)
        .set_style_radius(20, 0)                           // 圆角
        .set_style_bg_color(lv_color_hex(0x3498db), 0)     // 蓝色背景
        .set_style_text_color(lv_color_white(), 0)         // 白色文字
        .set_style_border_width(2, 0)                      // 使用边框代替阴影
        .set_style_border_color(lv_color_hex(0x2980b9), 0) // 深色边框
        .add_event_cb(
            [&](lv_event_t * event, void * user_data) { 
                PageManager::getInstance().switchToPage(PageManager::PageType::MAIN_PAGE); 
            },
            LV_EVENT_CLICKED, nullptr);
}

void SecurityCameraPage::create_recording_controls()
{
    LvObject manual_record_container(surveillance_screen->raw());
    manual_record_container.set_size(LV_SIZE_CONTENT, LV_SIZE_CONTENT)
        .align(LV_ALIGN_BOTTOM_MID, -250, 0)
        .set_style_bg_opa(LV_OPA_TRANSP, 0)
        .set_style_border_width(0, 0)
        .set_flex_flow(LV_FLEX_FLOW_COLUMN)
        .set_flex_align(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    LvSwitch manual_record_switch(manual_record_container.raw());
    manual_record_switch.set_size(80, 40)  
        .set_style_bg_color(lv_color_hex(0xbdc3c7), 0)    // 默认状态背景颜色
        .set_style_bg_color(lv_color_hex(0x2ecc71), LV_PART_INDICATOR)  // 指示器颜色
        .set_style_bg_color(lv_color_hex(0x27ae60), LV_PART_INDICATOR | LV_STATE_CHECKED)  // 选中状态颜色
        .set_style_border_width(5, LV_PART_INDICATOR)     // 指示器阴影
        .set_style_pad_all(2, 0)                          // 内边距
        .set_style_radius(20, 0)                          // 圆角
        .set_style_radius(20, LV_PART_INDICATOR);         // 指示器圆角
    
    manual_record_switch.add_event_cb(
        [&](lv_event_t * event, void * user_data) {
            auto switch_target = (lv_obj_t *)lv_event_get_target(event);
            if(lv_obj_has_state(switch_target, LV_STATE_CHECKED)) {
                this->start_manual_recording();
            } else {
                this->stop_manual_recording();
            }
        },
        LV_EVENT_VALUE_CHANGED, nullptr);
    
    LvLabel manual_record_label(manual_record_container.raw(), "录像", lv_color_black());
}

void SecurityCameraPage::create_auto_record_controls()
{
    LvObject auto_detection_container(surveillance_screen->raw());
    auto_detection_container.set_size(LV_SIZE_CONTENT, LV_SIZE_CONTENT)
        .align(LV_ALIGN_BOTTOM_MID, 0, 0)
        .set_style_bg_opa(LV_OPA_TRANSP, 0)
        .set_style_border_width(0, 0)
        .set_flex_flow(LV_FLEX_FLOW_COLUMN)
        .set_flex_align(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    LvSwitch auto_detection_switch(auto_detection_container.raw());
    auto_detection_switch.set_size(80, 40)  
        .set_style_bg_color(lv_color_hex(0xbdc3c7), 0)    // 默认状态背景颜色
        .set_style_bg_color(lv_color_hex(0x2ecc71), LV_PART_INDICATOR)  // 指示器颜色
        .set_style_bg_color(lv_color_hex(0x27ae60), LV_PART_INDICATOR | LV_STATE_CHECKED)  // 选中状态颜色
        .set_style_border_width(5, LV_PART_INDICATOR)     // 指示器阴影
        .set_style_pad_all(2, 0)                          // 内边距
        .set_style_radius(20, 0)                          // 圆角
        .set_style_radius(20, LV_PART_INDICATOR);         // 指示器圆角
    
    auto_detection_switch.add_event_cb(
        [&](lv_event_t * event, void * user_data) {
            auto switch_target = (lv_obj_t *)lv_event_get_target(event);
            if(lv_obj_has_state(switch_target, LV_STATE_CHECKED)) {
                auto_recording_enabled_ = true;
            } else {
                auto_recording_enabled_ = false;
            }
        },
        LV_EVENT_VALUE_CHANGED, nullptr);
    
    LvLabel auto_detection_label(auto_detection_container.raw(), "检测到人时自动录像", lv_color_black());
}

void SecurityCameraPage::create_alert_controls()
{
    LvObject notification_container(surveillance_screen->raw());
    notification_container.set_size(LV_SIZE_CONTENT, LV_SIZE_CONTENT)
        .align(LV_ALIGN_BOTTOM_MID, 250, 0)
        .set_style_bg_opa(LV_OPA_TRANSP, 0)
        .set_style_border_width(0, 0)
        .set_flex_flow(LV_FLEX_FLOW_COLUMN)
        .set_flex_align(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    LvSwitch notification_switch(notification_container.raw());
    notification_switch.set_size(80, 40)  
        .set_style_bg_color(lv_color_hex(0xbdc3c7), 0)    // 默认状态背景颜色
        .set_style_bg_color(lv_color_hex(0x2ecc71), LV_PART_INDICATOR)  // 指示器颜色
        .set_style_bg_color(lv_color_hex(0x27ae60), LV_PART_INDICATOR | LV_STATE_CHECKED)  // 选中状态颜色
        .set_style_border_width(5, LV_PART_INDICATOR)     // 指示器阴影
        .set_style_pad_all(2, 0)                          // 内边距
        .set_style_radius(20, 0)                          // 圆角
        .set_style_radius(20, LV_PART_INDICATOR);         // 指示器圆角
    
    notification_switch.add_event_cb(
        [&](lv_event_t * event, void * user_data) {
            auto switch_target = (lv_obj_t *)lv_event_get_target(event);
            if(lv_obj_has_state(switch_target, LV_STATE_CHECKED)) {
                alert_enabled_ = true;
            } else {
                alert_enabled_ = false;
            }
        },
        LV_EVENT_VALUE_CHANGED, nullptr);
    
    LvLabel notification_label(notification_container.raw(), "检测到有人时报警", lv_color_black());
}

void SecurityCameraPage::initialize_video_timer()
{
    refresh_timer = new LvTimer(
        [&](lv_timer_t * timer_handle, void * user_data) {
            std::unique_lock<std::mutex> sync_lock(video_sync_mutex_);
            sync_condition_.wait(sync_lock, [this]() { return !stream_processing_turn_ || !surveillance_active_; });

            if(!surveillance_active_) {
                return;
            }

            auto processed_result = security_rknn_pool_.get_image_result_from_queue();

            stream_processing_turn_ = true;
            sync_condition_.notify_one();

            if(processed_result) {

                cv::resize(*processed_result, *processed_result, cv::Size(800, 450));

                memset(video_stream_desc_, 0, sizeof(LvImageDsc));

                video_stream_desc_->raw()->data      = processed_result->data;
                video_stream_desc_->raw()->data_size = processed_result->total() * processed_result->elemSize();
                video_stream_desc_->raw()->header.w  = processed_result->cols;
                video_stream_desc_->raw()->header.h  = processed_result->rows;
                video_stream_desc_->raw()->header.cf = LV_COLOR_FORMAT_RGB888;

                monitor_display_->set_src(video_stream_desc_->raw());
            }
        },
        10, nullptr);
}

void SecurityCameraPage::show()
{
    lv_screen_load(surveillance_screen->raw());

    surveillance_active_ = true;

    refresh_timer->resume();

    ffmpeg_.start_process_frame();

    std::thread([this]() {
        try {
            camera_.start();
            while(surveillance_active_) {
                auto captured_frame = camera_.get_frame();
                security_rknn_pool_.add_inference_task(std::move(captured_frame), image_process_);
                {
                    std::unique_lock<std::mutex> sync_lock(video_sync_mutex_);
                    // 等待直到轮到流处理线程取帧
                    sync_condition_.wait(sync_lock, [this]() { return stream_processing_turn_; });
                    auto detection_result = security_rknn_pool_.get_image_result_from_queue(true);

                    stream_processing_turn_ = false;
                    sync_condition_.notify_one();

                    if(detection_result) {
                        ffmpeg_.push_frame(std::move(detection_result));

                        // 自动录像逻辑处理
                        handle_auto_recording_logic();

                        // 报警处理逻辑
                        handle_alert_logic();
                    }
                }
            }
            camera_.stop();
        } catch(std::exception & error) {
            std::cerr << "Error: " << error.what() << std::endl;
        }
    }).detach();
}

void SecurityCameraPage::handle_auto_recording_logic()
{
    // 自动录像逻辑:
    // - 开启且检测到人且未开始: 记录时间并开始录像
    // - 开启且检测到人且已开始: 更新时间戳
    // - 开启且未检测到人且已开始且超时: 停止录像
    if(auto_recording_enabled_) {
        if(security_rknn_pool_.is_person) {
            if(!auto_recording_active_) {
                auto_recording_active_   = true;
                recording_start_timestamp_ = std::time(nullptr);
                this->start_manual_recording();
                std::cout << "开始录像" << std::endl;
            } else {
                recording_start_timestamp_ = std::time(nullptr);
            }
        } else {
            if(auto_recording_active_ && std::time(nullptr) - recording_start_timestamp_ >
                                            SECURITY_CAMERA_PAGE_AUTO_RECORD_DELAY_TIME) {
                auto_recording_active_ = false;
                this->stop_manual_recording();
                std::cout << "停止录像1" << std::endl;
            }
        }
    } else {
        if(auto_recording_active_) {
            auto_recording_active_ = false;
            this->stop_manual_recording();
            std::cout << "停止录像2" << std::endl;
        }
    }
}

void SecurityCameraPage::handle_alert_logic()
{
    if(alert_enabled_ && security_rknn_pool_.is_person && !alert_processing_) {
        alert_processing_ = true;
        std::thread([this]() {
            lv_async_call([](void *) { detection_alert_label->remove_flag(LV_OBJ_FLAG_HIDDEN); }, nullptr);
            execute_command("sudo -u elf env XDG_RUNTIME_DIR=/run/user/$(id -u elf) "
                        "PULSE_SERVER=unix:/run/user/$(id -u elf)/pulse/native paplay /home/elf/Downloads/alert.wav");
            std::this_thread::sleep_for(std::chrono::seconds(2));
            alert_processing_ = false;
            lv_async_call([](void *) { detection_alert_label->add_flag(LV_OBJ_FLAG_HIDDEN); }, nullptr);
        }).detach();
    }
}

void SecurityCameraPage::hide()
{
    ffmpeg_.stop_process_frame();

    surveillance_active_ = false;

    refresh_timer->pause();
}

void SecurityCameraPage::start_manual_recording()
{
    ffmpeg_.start_record();

    lv_async_call(
        [](void *) {
            recording_indicator->remove_flag(LV_OBJ_FLAG_HIDDEN);
            recording_blink_anim->start();
        },
        nullptr);
}

void SecurityCameraPage::stop_manual_recording()
{
    ffmpeg_.stop_record();

    lv_async_call(
        [](void *) {
            recording_indicator->add_flag(LV_OBJ_FLAG_HIDDEN);
            recording_blink_anim->stop();
        },
        nullptr);
}
