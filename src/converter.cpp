#include <iostream>
#include <vector>
#include <deque>
#include <array>
// #include "opencv/cv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#define THUMBNAIL_ROW_X_COUNT 4
#define THUMBNAIL_ROW_Y_COUNT 4
#define THUMBNAIL_TIME 10

using namespace cv;
using namespace std;

typedef std::deque<Mat> VideoBuffer;
typedef array<VideoBuffer, THUMBNAIL_ROW_X_COUNT> VideoBuffersX;
typedef array<VideoBuffersX, THUMBNAIL_ROW_Y_COUNT> VideoBuffersXY;

struct errorInfo {
    errorInfo(int ti, int tx, int ty){
        i = ti;
        x = tx;
        y = ty;
    }

    int i;
    int x;
    int y;
};

class Converter {
    public:

    Converter(string name, bool debug = false) {
        source = VideoCapture(name);
        isDebug = debug;
        calcSourceSize();
    }

    VideoWriter write(string name) {
        VideoWriter writer(name, CV_FOURCC_DEFAULT, fps, full_size);
        VideoBuffersXY divi = divideSource();

        try {
            for (int i = 0; i < each_frame_count; i++) {
                // create frame to edit
                Size wide_size(full_size.width + cell_size.width * 2, full_size.height);
                Mat combined_frame(wide_size, CV_8UC3);

                for (auto vxitr = divi.begin(); vxitr != divi.end(); ++vxitr) {
                    int y = distance(divi.begin(), vxitr);
                    VideoBuffersX &vx = *vxitr;

                    for (auto vitr = vx.begin(); vitr != vx.end(); ++vitr) {
                        int x = distance(vx.begin(), vitr);
                        VideoBuffer &buffer = *vitr;

                        if (isDebug) cout << "write: " << x << ' ' << y << ' ' << i << endl;

                        if (buffer.empty()) throw errorInfo(i, x, y);
                        Mat &cell_frame = buffer.front();

                        // prepare start point
                        Point startp(
                                cell_size.width * (x + 1 + i / (double)each_frame_count),
                                cell_size.height * y);
                        if (isDebug) cout << "startp: " << startp.x << ' ' << startp.y << endl;

                        // write frame
                        writeToRect(startp, combined_frame, cell_frame);

                        if (vitr == vx.end() - 1) {
                            Point startp(
                                    cell_size.width * (i / (double)each_frame_count),
                                    cell_size.height * y);
                            writeToRect(startp, combined_frame, cell_frame);
                        }

                        buffer.pop_front();
                    }
                }
                if (isDebug) cout << "write: " << i << endl;

                Rect writer_rect(Point(cell_size.width, 0), full_size);
                Mat writer_frame(combined_frame, writer_rect);
                writer << writer_frame;
            }
        } catch (const struct errorInfo &ei) {
            cerr << ei.x << ' ' << ei.y << ' ' << ei.i << endl;
            exit(1);
        }
        return writer;
    }

    private:

    Size full_size, cell_size;
    double frame_count, fps;
    int each_frame_count, frame_capture_ratio;
    bool isDebug;
    VideoCapture source;

    void writeToRect(Point &startp, Mat &target_frame, Mat &cell_frame) {
        Rect roi_rect(startp, cell_size);
        Mat roi(target_frame, roi_rect);
        cell_frame.copyTo(roi);
    }

    VideoBuffersXY divideSource() {
        VideoBuffersXY vxy;

        // Set source's pos to head
        source.set(CV_CAP_PROP_POS_FRAMES, 0);
        for (int y = 0; y < THUMBNAIL_ROW_Y_COUNT; y++) {
            vxy[y] = VideoBuffersX(); VideoBuffersX &vx = vxy[y];

            for (int x = 0; x < THUMBNAIL_ROW_X_COUNT; x++) {
                vx[x] = VideoBuffer(); VideoBuffer &buffer = vx[x];

                for (int i = 0; i < each_frame_count; i++) {
                    Mat frame, shrunken_frame;
                    source >> frame;
                    for (int j = 0; i < frame_capture_ratio - 1; j++) source.grab();

                    resize(frame, shrunken_frame, cell_size, 0, 0, CV_INTER_LINEAR);
                    buffer.push_back(shrunken_frame);
                    if (isDebug) cout << "read: " << x << ' ' << y << ' ' << i << endl;
                }
            }
        }

        if (isDebug) cout << vxy.size() << " " << vxy[0].size() << " " << vxy[0][0].size() << endl;
        return vxy;
    }

    void calcSourceSize() {
        double width = source.get(CV_CAP_PROP_FRAME_WIDTH);
        double height = source.get(CV_CAP_PROP_FRAME_HEIGHT);

        frame_count = source.get(CV_CAP_PROP_FRAME_COUNT);
        if (isDebug) cout << "frame_count: " << frame_count << endl;

        each_frame_count = (int)floor(frame_count / (THUMBNAIL_ROW_X_COUNT * THUMBNAIL_ROW_Y_COUNT));
        if (isDebug) cout << "each_frame_count: " << each_frame_count << endl;

        full_size = Size(width, height);
        if (isDebug) cout << "full_size: " << full_size.width << ' ' << full_size.height << endl;
        cell_size = Size((int)(width / THUMBNAIL_ROW_X_COUNT), (int)(height / THUMBNAIL_ROW_Y_COUNT));
        if (isDebug) cout << "cell_size: " << cell_size.width << ' ' << cell_size.height << endl;

        calc_and_adjust_fps();
    }

    void calc_and_adjust_fps() {
        fps = frame_count / (THUMBNAIL_ROW_X_COUNT * THUMBNAIL_ROW_Y_COUNT * THUMBNAIL_TIME);
        frame_capture_ratio = 1;
        if (isDebug) cout << "original_fps: " << fps << endl;
        while (fps > 60.0) {
            frame_capture_ratio *= 2;
            fps /= 2;
        }
        if (isDebug) cout << "fps: " << fps << endl;
        if (isDebug) cout << "capture_ratio: " << frame_capture_ratio << endl;
    }

};

int main(void) {
    Converter conv("sample.mp4", true);
    conv.write("conv.avi");
}
