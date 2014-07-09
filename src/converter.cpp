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
                Mat combined_frame(full_size, CV_8UC3);

                for (auto vxitr = divi.begin(); vxitr != divi.end(); ++vxitr) {
                    int y = distance(divi.begin(), vxitr);
                    VideoBuffersX &vx = *vxitr;

                    for (auto vitr = vx.begin(); vitr != vx.end(); ++vitr) {
                        int x = distance(vx.begin(), vitr);
                        VideoBuffer &buffer = *vitr;

                        if (isDebug) cout << x << ' ' << y << ' ' << i << endl;

                        // create ROI
                        Point startp(cell_size.width * x, cell_size.height * y);
                        if (isDebug) cout << startp.x << ' ' << startp.y << endl;
                        if (isDebug) cout << cell_size.width << ' ' << cell_size.height << endl;
                        Rect roi_rect(startp, cell_size);
                        Mat roi(combined_frame, roi_rect);

                        // write frame
                        if (buffer.empty()) throw errorInfo(i, x, y);
                        buffer.front().copyTo(roi);
                        buffer.pop_front();
                    }
                }
                if (isDebug) cout << "write: " << i << endl;

                writer << combined_frame;
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
    int each_frame_count;
    bool isDebug;
    VideoCapture source;

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
                    resize(frame, shrunken_frame, cell_size, 0, 0, CV_INTER_LINEAR);
                    buffer.push_back(shrunken_frame);
                }
                if (isDebug) cout << "read: " << x << ' ' << y << endl;
            }
        }

        if (isDebug) cout << vxy.size() << " " << vxy[0].size() << " " << vxy[0][0].size() << endl;
        return vxy;
    }

    void calcSourceSize() {
        double width = source.get(CV_CAP_PROP_FRAME_WIDTH);
        double height = source.get(CV_CAP_PROP_FRAME_HEIGHT);

        frame_count = source.get(CV_CAP_PROP_FRAME_COUNT);
        each_frame_count = (int)floor(frame_count / (THUMBNAIL_ROW_X_COUNT * THUMBNAIL_ROW_Y_COUNT));
        fps = frame_count / (THUMBNAIL_ROW_X_COUNT * THUMBNAIL_ROW_Y_COUNT * THUMBNAIL_TIME);
        full_size = Size(width, height);
        cell_size = Size((int)(width / THUMBNAIL_ROW_X_COUNT), (int)(height / THUMBNAIL_ROW_Y_COUNT));
    }

};

int main(void) {
    Converter conv("sample.mp4", false);
    conv.write("conv.avi");
}
