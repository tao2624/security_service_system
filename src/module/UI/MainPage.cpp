#include "Font.hpp"
#include "Lvgl.hpp"
#include "PageManager.hpp"
#include "UI.hpp"
#include <src/display/lv_display.h>
#include <src/layouts/flex/lv_flex.h>
#include <src/misc/lv_color.h>

extern "C" {
LV_IMAGE_DECLARE(background);
LV_IMAGE_DECLARE(face_icon);
LV_IMAGE_DECLARE(security_camera_icon);
}

MainPage::MainPage()
{
    // 创建主屏幕对象
    main_screen = new LvObject(nullptr);
    
    // 设置主屏幕样式和布局
    main_screen->set_style_bg_image_src(&background, 0)
        .set_style_text_font(Font24::get_font(), 0)
        .set_flex_flow(LV_FLEX_FLOW_ROW)
        .set_flex_align(LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 构建人脸识别功能按钮
    create_face_recognition_button();
    
    // 构建安防监控功能按钮
    create_security_monitor_button();
}

void MainPage::create_face_recognition_button()
{
    LvObject face_access_container{main_screen->raw()};
    face_access_container.set_size(LV_SIZE_CONTENT, LV_SIZE_CONTENT)
        .set_style_bg_opa(LV_OPA_TRANSP, 0)
        .set_style_border_width(0, 0)
        .set_flex_flow(LV_FLEX_FLOW_COLUMN)
        .set_flex_align(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER)
        .add_event_cb(
            [&](lv_event_t * event, void * user_data) {
                PageManager::getInstance().switchToPage(PageManager::PageType::ACCESS_CONTROL_PAGE);
            },
            LV_EVENT_CLICKED, nullptr);

    LvImage face_recognition_icon{face_access_container.raw()};
    face_recognition_icon.set_src(&face_icon);

    LvLabel face_access_text{face_access_container.raw(), "智能人脸通行", lv_color_white()};
}

void MainPage::create_security_monitor_button()
{
    LvObject monitor_system_container{main_screen->raw()};
    monitor_system_container.set_size(LV_SIZE_CONTENT, LV_SIZE_CONTENT)
        .set_style_bg_opa(LV_OPA_TRANSP, 0)
        .set_style_border_width(0, 0)
        .set_flex_flow(LV_FLEX_FLOW_COLUMN)
        .set_flex_align(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER)
        .add_event_cb(
            [&](lv_event_t * event, void * user_data) {
                PageManager::getInstance().switchToPage(PageManager::PageType::SECURITY_CAMERA_PAGE);
            },
            LV_EVENT_CLICKED, nullptr);

    LvImage monitor_system_icon{monitor_system_container.raw()};
    monitor_system_icon.set_src(&security_camera_icon);

    LvLabel monitor_system_text{monitor_system_container.raw(), "AI安防监控站", lv_color_white()};
}

void MainPage::show()
{
    lv_screen_load(main_screen->raw());
}

void MainPage::hide()
{
    // 页面隐藏时的清理工作
}