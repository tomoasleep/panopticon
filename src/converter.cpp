#include <iostream>
#include <vector>
// #include "opencv/cv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#define THUMBNAIL_ROW_X_COUNT 10
#define THUMBNAIL_ROW_Y_COUNT 10
#define THUMBNAIL_TIME 10

using namespace cv;

int main(void) {
    double width, height, frame_count, fps;
    vector<Mat> cols;

    VideoCapture video("sample.mp4");
    if (!video.isOpened()) return -1;

    width = video.get(CV_CAP_PROP_FRAME_WIDTH);
    height = video.get(CV_CAP_PROP_FRAME_HEIGHT);
    frame_count = video.get(CV_CAP_PROP_FRAME_COUNT);
    fps = frame_count / (THUMBNAIL_ROW_X_COUNT * THUMBNAIL_ROW_Y_COUNT * THUMBNAIL_TIME);
    int each_frame_count =
        (int)floor(frame_count / (THUMBNAIL_ROW_X_COUNT * THUMBNAIL_ROW_Y_COUNT));
    double orig_fps = video.get(CV_CAP_PROP_FPS);
    Size size = Size((int)(width / THUMBNAIL_ROW_X_COUNT),
                     (int)(height / THUMBNAIL_ROW_Y_COUNT));

    // double old_pos = video.get(CV_CAP_PROP_POS_FRAMES);
    // // video >> frame;
    // video.retrieve(frame);
    // double new_pos = video.get(CV_CAP_PROP_POS_FRAMES);

    VideoWriter dstVideo("output.avi", CV_FOURCC_DEFAULT, fps, size);

    std::cout << frame_count << " " << fps << " " << orig_fps << " " << each_frame_count << std::endl;
    std::cout << video.get(CV_CAP_PROP_FOURCC) << std::endl;

    video.set(CV_CAP_PROP_POS_FRAMES, 100);
    Mat frame, dstFrame;
    for (int i = 0; i < each_frame_count; i++) {
        video >> frame;
        resize(frame, dstFrame, size, 0, 0, CV_INTER_LINEAR);
        dstVideo << dstFrame;
        std::cout << i << std::endl;
        // imshow("live", dstFrame);
        // waitKey(30);
    }

    video.release();
    dstVideo.release();

    // std::cout << old_pos << " " << new_pos << std::endl;
    // resize(frame, dst, Size(width / THUMBNAIL_ROW_X_COUNT, height / THUMBNAIL_ROW_Y_COUNT), 0, 0, CV_INTER_LINEAR);

    // imshow("frame", frame);
    // imshow("dst", dst);
    // waitKey(0);

    // while(true) {
    //     video >> frame;
    //     if (!frame.data) break;

    //     Mat col;
    //     frame.col(0).copyTo(col);
    //     cols.push_back(col);
    // }

    // std::cout << cols.size() << std::endl;
    return 0;
}
