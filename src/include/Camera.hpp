#pragma once

#include "opencv2/opencv.hpp"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <thread>

// #define CAMERA_WIDTH 3840
// #define CAMERA_HEIGHT 2160
/* 1280x720 */
#define CAMERA_WIDTH 1280
#define CAMERA_HEIGHT 720

class Camera {
  private:
    cv::VideoCapture capture_;

    std::queue<std::shared_ptr<cv::Mat>> frame_queue_;

    std::mutex frame_queue_mutex_;
    std::condition_variable frame_queue_cond_;

    std::atomic_bool is_running_;
    std::thread capture_thread_;
    std::function<void()> capture_thread_fn_;

  public:
    Camera();

    void start();
    void stop();

    std::shared_ptr<cv::Mat> get_frame();
};