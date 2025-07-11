#include "PageManager.hpp"
#include "Sensor.hpp"
#include "UI.hpp"
#include "Camera.hpp"
#include "Font.hpp"
#include "RknnPool.hpp"
#include "Lvgl.hpp"
#include <cstdint>
#include <exception>
#include <iostream>
#include <src/core/lv_obj.h>
#include <src/display/lv_display.h>
#include <src/misc/lv_area.h>
#include <src/misc/lv_async.h>
#include <src/misc/lv_event.h>
#include <thread>

extern "C" {
LV_IMAGE_DECLARE(background);
}

static LvImageDsc * video_frame_desc_ = new LvImageDsc;
static LvImage * camera_display_;
static LvLabel * registered_faces_label_;

AccessControlPage::AccessControlPage(Camera & camera, FaceRknnPool & face_rknn_pool, ImageProcess & image_process)
    : camera_(camera), face_rknn_pool_(face_rknn_pool), image_process_(image_process)
{
    primary_screen  = new LvObject(nullptr);
    standby_screen = new LvObject(nullptr);

    primary_screen->set_style_bg_image_src(&background, 0).set_style_text_font(Font16::get_font(), 0);
    standby_screen->set_style_bg_color(lv_color_black(), 0);

    camera_display_ = new LvImage(primary_screen->raw());

    camera_display_->align(LV_ALIGN_CENTER, 0, -50);

    // 创建界面控件
    setup_navigation_button();
    setup_recognition_toggle();
    setup_face_counter_display();
    setup_face_registration_button();
    initialize_display_timer();

    display_timer->pause();

    switch_to_standby_fn = [this]() { lv_screen_load(standby_screen->raw()); };
}

void AccessControlPage::setup_navigation_button()
{
    // 现代风格返回按钮
    LvButton navigation_button(primary_screen->raw(), "返回");
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

void AccessControlPage::setup_recognition_toggle()
{
    LvObject toggle_container(primary_screen->raw());
    toggle_container.set_size(LV_SIZE_CONTENT, LV_SIZE_CONTENT)
        .align(LV_ALIGN_BOTTOM_MID, 200, 0)
        .set_style_bg_opa(LV_OPA_TRANSP, 0)
        .set_style_border_width(0, 0)
        .set_flex_flow(LV_FLEX_FLOW_COLUMN)
        .set_flex_align(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    LvSwitch recognition_switch(toggle_container.raw());
    recognition_switch.set_size(80, 40)  
        .set_style_bg_color(lv_color_hex(0xbdc3c7), 0)    // 默认状态背景颜色
        .set_style_bg_color(lv_color_hex(0x2ecc71), LV_PART_INDICATOR)  // 指示器颜色
        .set_style_bg_color(lv_color_hex(0x27ae60), LV_PART_INDICATOR | LV_STATE_CHECKED)  // 选中状态颜色
        .set_style_border_width(5, LV_PART_INDICATOR)     // 指示器阴影
        .set_style_pad_all(2, 0)                          // 内边距
        .set_style_radius(20, 0)                          // 圆角
        .set_style_radius(20, LV_PART_INDICATOR);         // 指示器圆角

    LvLabel toggle_label(toggle_container.raw(), "人脸识别", lv_color_white());
    recognition_switch.add_event_cb(     // 人脸识别开关事件
        [&](lv_event_t * event, void * user_data) {
            // 判断状态
            auto target = (lv_obj_t *)lv_event_get_target(event);
            if(lv_obj_has_state(target, LV_STATE_CHECKED)) {
                face_rknn_pool_.change_face_recognition_status(true);
            } else {
                face_rknn_pool_.change_face_recognition_status(false);
            }
        },
        LV_EVENT_VALUE_CHANGED, nullptr);
}

void AccessControlPage::setup_face_counter_display()
{
    LvObject counter_container(primary_screen->raw());
    counter_container.set_size(LV_SIZE_CONTENT, LV_SIZE_CONTENT)
        .align(LV_ALIGN_BOTTOM_MID, -30, -33)
        .set_style_bg_opa(LV_OPA_TRANSP, 0)
        .set_style_border_width(0, 0)
        .set_flex_flow(LV_FLEX_FLOW_ROW)
        .set_style_text_font(Font24::get_font(), 0);

    LvLabel counter_prefix(counter_container.raw(), "已录入人脸数量:", lv_color_black());
    registered_faces_label_ = new LvLabel(counter_container.raw(), "0", lv_color_black());
}

void AccessControlPage::setup_face_registration_button()
{
    LvButton registration_button(primary_screen->raw(), "录入人脸");    // 录入人脸 按钮
    registration_button.align(LV_ALIGN_BOTTOM_MID, -200, -45)
        .set_style_radius(8, 0)                           // 圆角
        .set_style_bg_color(lv_color_hex(0x9b59b6), 0)    // 紫色背景
        .set_style_bg_grad_color(lv_color_hex(0x8e44ad), 0) // 渐变色
        .set_style_bg_grad_dir(LV_GRAD_DIR_VER, 0)        // 垂直渐变
        .set_style_text_color(lv_color_white(), 0)        // 白色文字
        .set_style_border_width(1, 0)                     // 边框宽度
        .set_style_border_color(lv_color_hex(0x7d3c98), 0) // 边框颜色
        .set_style_pad_hor(15, 0)                         // 水平内边距
        .set_style_pad_ver(10, 0);
        
    registration_button.add_event_cb(
        [&](lv_event_t * event, void * user_data) {
            std::thread(
                [&](LvLabel * label) {
                    std::unique_lock<std::mutex> lock(capture_frame_mutex_);

                    auto current_frame = camera_.get_frame();

                    lock.unlock();

                    face_rknn_pool_.add_inference_task(std::move(current_frame), image_process_, true);
                },
                registered_faces_label_)
                .detach();

            LvAsync::call([&]() {
                registered_faces_label_->set_text(
                    std::to_string(face_rknn_pool_.get_facenet_feature_vector_size() + 1).c_str());
            });
        },
        LV_EVENT_CLICKED, nullptr);
}

void AccessControlPage::initialize_display_timer()
{
    display_timer = new LvTimer(
        [&](lv_timer_t * timer_handle, void * user_data) {

            std::shared_ptr<cv::Mat> processed_frame = face_rknn_pool_.get_image_result_from_queue();

            if(processed_frame) {

                cv::resize(*processed_frame, *processed_frame, cv::Size(800, 450));

                memset(video_frame_desc_, 0, sizeof(LvImageDsc));

                video_frame_desc_->raw()->data      = processed_frame->data;
                video_frame_desc_->raw()->data_size = processed_frame->total() * processed_frame->elemSize();

                video_frame_desc_->raw()->header.w  = processed_frame->cols;
                video_frame_desc_->raw()->header.h  = processed_frame->rows;
                video_frame_desc_->raw()->header.cf = LV_COLOR_FORMAT_RGB888;

                camera_display_->set_src(video_frame_desc_->raw());
            }

        },
        10, nullptr);
}

void AccessControlPage::show()
{
    activate_normal_display();

    SR501::listen_state([&](bool person_detected) {
        try {
            std::cout << "person_detected: " << person_detected << std::endl;

            // 检测到人员时的处理逻辑:
            // - 有人且屏幕待机: 记录时间戳并激活正常显示
            // - 有人且屏幕正常: 更新时间戳
            // - 无人且屏幕正常: 检查超时后切换到待机
            // - 无人且屏幕待机: 保持现状
            if(person_detected) {
                previous_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::system_clock::now().time_since_epoch())
                                    .count();
                if(is_standby_mode) {
                    is_standby_mode = false;
                    activate_normal_display();
                }
            } else {
                uint64_t current_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                 std::chrono::system_clock::now().time_since_epoch())
                                                 .count();
                if(!is_standby_mode && current_timestamp - previous_timestamp > ACCESS_CONTROL_PAGE_DELAY_TIME) {
                    is_standby_mode = true;
                    activate_standby_display();
                }
            }
        } catch(std::exception & error) {
            std::cout << "AccessControlPage---show: " << error.what() << std::endl;
        }
    });
}

void AccessControlPage::hide()
{
    camera_display_->add_flag(LV_OBJ_FLAG_HIDDEN);
    display_timer->pause();
    processing_active = false;
    SR501::stop_listen_state();
}

void AccessControlPage::activate_normal_display()
{
    lv_screen_load(primary_screen->raw());

    processing_active = true;

    display_timer->resume();

    camera_display_->remove_flag(LV_OBJ_FLAG_HIDDEN);

    std::thread([&]() {
        try {
            camera_.start();

            while(processing_active) {
                std::unique_lock<std::mutex> frame_lock(capture_frame_mutex_);

                auto captured_frame = camera_.get_frame();

                frame_lock.unlock();

                face_rknn_pool_.add_inference_task(std::move(captured_frame), image_process_);
            }

            face_rknn_pool_.clean_image_results();
            camera_.stop();

        } catch(std::exception & error) {
            std::cout << "AccessControlPage---activate_normal_display: " << error.what() << std::endl;
        }
    }).detach();
}

void AccessControlPage::activate_standby_display()
{

    display_timer->pause();
    processing_active = false;

    lv_async_call(
        [](void * callback_data) {
            auto callback_function = static_cast<std::function<void()> *>(callback_data);
            (*callback_function)();
        },
        &switch_to_standby_fn);
}
