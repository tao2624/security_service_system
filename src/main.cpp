#include <exception>
#include <iostream>
#include <src/libs/freetype/lv_freetype.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "Camera.hpp"
#include "FFmpeg.hpp"
#include "Font.hpp"
#include "RknnPool.hpp"
#include "PageManager.hpp"
#include "lvgl/lvgl.h"
#include "lvgl/src/display/lv_display.h"

#include "UI.hpp"

uint16_t display_width;
uint16_t display_height;
bool is_fullscreen_mode;
bool is_maximized_mode;

static void setup_application_config(int argc, char ** argv);

static const char * get_env_or_default(const char * env_name, const char * default_value)
{
    const char * env_value = getenv(env_name);
    return env_value ? env_value : default_value;
}

#if LV_USE_EVDEV
static void setup_touch_input_device(lv_display_t * display)
{
    /* 初始化触摸输入设备 (触摸屏/鼠标)
     * 使用 'evtest' 命令查找正确的输入设备
     * 推荐使用 /dev/input/by-id/ 路径，或者使用 /dev/input/eventX
     */
    const char * touch_device_path = getenv("LV_LINUX_EVDEV_POINTER_DEVICE");
    if(touch_device_path == NULL) {
        // touch_device_path = get_env_or_default("LV_LINUX_EVDEV_POINTER_DEVICE", "/dev/input/event7");
        touch_device_path = "/dev/input/event1";
    }

    if(touch_device_path == NULL) {
        fprintf(stderr, "请设置 LV_LINUX_EVDEV_POINTER_DEVICE 环境变量\n");
        exit(1);
    }

    lv_indev_t * input_device = lv_evdev_create(LV_INDEV_TYPE_POINTER, touch_device_path);
    lv_indev_set_display(input_device, display);

    /* 设置鼠标光标图标 */
    LV_IMAGE_DECLARE(mouse_cursor_icon);
    lv_obj_t * cursor_icon = lv_image_create(lv_screen_active());
    lv_image_set_src(cursor_icon, &mouse_cursor_icon);
    lv_indev_set_cursor(input_device, cursor_icon);
}
#endif

#if LV_USE_LINUX_FBDEV
static void initialize_framebuffer_display(void)
{
    const char * fb_device = get_env_or_default("LV_LINUX_FBDEV_DEVICE", "/dev/fb0");
    lv_display_t * fb_display = lv_linux_fbdev_create();

#if LV_USE_EVDEV
    setup_touch_input_device(fb_display);
#endif

    lv_linux_fbdev_set_file(fb_display, fb_device);
}
#elif LV_USE_LINUX_DRM

static void initialize_drm_display(void)
{
    const char * drm_device = get_env_or_default("LV_LINUX_DRM_CARD", "/dev/dri/card0");
    
    // 激活 MIPI DSI 显示屏
    printf("正在启用 MIPI DSI 显示屏...\n");
    system("modetest -M rockchip -s 448:1024x600 > /dev/null 2>&1");
    system("modetest -M rockchip -w 448:2:0 > /dev/null 2>&1");  // 启用DPMS
    
    // 等待硬件初始化完成
    usleep(500000);  // 延时500毫秒
    
    lv_display_t * drm_display = lv_linux_drm_create();
    if(!drm_display) {
        fprintf(stderr, "DRM显示器创建失败\n");
        exit(1);
    }
#if LV_USE_EVDEV
    setup_touch_input_device(drm_display);
#endif
    // 配置 DSI 连接器 (ID: 448)
    printf("配置 DRM 设备: %s，连接器ID: 448 (DSI)\n", drm_device);
    lv_linux_drm_set_file(drm_display, drm_device, 448);
    printf("DRM 设备配置完成\n");
    
    // 显示屏幕信息
    printf("屏幕分辨率: %dx%d\n", lv_display_get_horizontal_resolution(drm_display), 
                              lv_display_get_vertical_resolution(drm_display));
    printf("=== DRM 显示器初始化完成 ===\n");
}

#endif

#if LV_USE_WAYLAND == 0
void run_main_event_loop(void)
{
    uint32_t sleep_time;

    /* 处理LVGL任务循环 */
    while(1) {
        sleep_time = lv_timer_handler(); /* 返回到下次定时器执行的时间 */
        usleep(sleep_time * 1000);
    }
}
#endif

/*
 * 解析命令行参数和环境变量
 * 配置应用程序参数
 */
static void setup_application_config(int argc, char ** argv)
{
    int option  = 0;
    bool has_error = false;

    /* 设置默认值 */
    is_fullscreen_mode = is_maximized_mode = false;
    display_width  = atoi(getenv("LV_SIM_WINDOW_WIDTH") ?: "800");
    display_height = atoi(getenv("LV_SIM_WINDOW_HEIGHT") ?: "480");

    /* 解析命令行选项 */
    while((option = getopt(argc, argv, "fmw:h:")) != -1) {
        switch(option) {
            case 'f':
                is_fullscreen_mode = true;
                if(LV_USE_WAYLAND == 0) {
                    fprintf(stderr, "SDL驱动不支持启动时的全屏模式\n");
                    exit(1);
                }
                break;
            case 'm':
                is_maximized_mode = true;
                if(LV_USE_WAYLAND == 0) {
                    fprintf(stderr, "SDL驱动不支持启动时的最大化模式\n");
                    exit(1);
                }
                break;
            case 'w': display_width = atoi(optarg); break;
            case 'h': display_height = atoi(optarg); break;
            case ':': fprintf(stderr, "选项 -%c 需要参数。\n", optopt); exit(1);
            case '?': fprintf(stderr, "未知选项 -%c。\n", optopt); exit(1);
        }
    }
}

int main(int argc, char ** argv)
{
    // 初始化应用程序配置
    setup_application_config(argc, argv);

    lv_init();

    // 根据编译配置初始化显示器
#if LV_USE_LINUX_FBDEV
    initialize_framebuffer_display();
#elif LV_USE_LINUX_DRM
    initialize_drm_display();
#endif

    // 创建硬件接口实例
    Camera camera_module;
    FFmpeg stream_encoder;
    FaceRknnPool face_ai_pool;
    SecurityRknnPool security_ai_pool;
    
    // 初始化图像处理器
    ImageProcess face_image_processor{CAMERA_WIDTH, CAMERA_HEIGHT, face_ai_pool.get_retinaface_model_size()};
    ImageProcess object_image_processor{CAMERA_WIDTH, CAMERA_HEIGHT, security_ai_pool.get_yolo_model_size()};

    // 获取页面管理器单例
    auto & ui_manager = PageManager::getInstance();

    // 初始化人脸识别模块
    ui_manager.init(camera_module, face_ai_pool, face_image_processor);
    // 初始化安防监控模块
    ui_manager.init(camera_module, security_ai_pool, object_image_processor, stream_encoder);

    ui_manager.switchToPage(PageManager::PageType::MAIN_PAGE);

    run_main_event_loop();

    return 0;
}
