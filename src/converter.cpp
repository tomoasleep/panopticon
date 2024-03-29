#include <iostream>
#include <vector>
#include <deque>
// #include "opencv/cv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#define THUMBNAIL_ROW_X_COUNT 8
#define THUMBNAIL_ROW_Y_COUNT 8
#define THUMBNAIL_TIME 10

using namespace cv;
using namespace std;

typedef std::deque<Mat> VideoBuffer;
typedef vector<VideoBuffer> VideoBuffersX;
typedef vector<VideoBuffersX> VideoBuffersXY;

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
                Size wide_size(full_size.width + cell_size.width * 2 + 10, full_size.height + 10);
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
                            if (isDebug) cout << "startp: " << startp.x << ' ' << startp.y << endl;
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
    int each_frame_count, frame_capture_ratio, division_number;
    int row_x_number, row_y_number;
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
        vxy.resize(row_y_number);
        for (int y = 0; y < row_y_number; y++) {
            vxy[y] = VideoBuffersX(); VideoBuffersX &vx = vxy[y];
            vx.resize(row_x_number);

            for (int x = 0; x < row_x_number; x++) {
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

        // decide division_number
        int div;
        for (div = 8; div > 3; div--) {
            if (width / div > 160 && height / div > 130) break;
        }
        division_number = div;
        if (isDebug) cout << "division_number: " << division_number << endl;

        cell_size = Size(width / division_number, height / division_number);
        if (isDebug) cout << "cell_size: " << cell_size.width << ' ' << cell_size.height << endl;

        // decide frame row counts
        int row;
        for (row = 8; row > 4; row--) {
            if (cell_size.width * row <= 1920) break;
        }
        row_y_number = row;

        for (int row = 8; row > 4; row--) {
            if (cell_size.height * row <= 1080) break;
        }
        row_x_number = row;
        if (isDebug) cout << "row_number(x, y): " << row_x_number <<  ' ' << row_y_number << endl;

        full_size = Size(cell_size.width * row_x_number, cell_size.height * row_y_number);
        if (isDebug) cout << "full_size: " << full_size.width << ' ' << full_size.height << endl;

        frame_count = source.get(CV_CAP_PROP_FRAME_COUNT);
        if (isDebug) cout << "frame_count: " << frame_count << endl;

        each_frame_count = (int)floor(frame_count / (row_x_number * row_y_number));
        if (isDebug) cout << "each_frame_count: " << each_frame_count << endl;


        calc_and_adjust_fps();
    }

    void calc_and_adjust_fps() {
        fps = frame_count / (row_x_number * row_y_number * THUMBNAIL_TIME);
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
    Converter conv("UrbanIsolation_SD.mp4", true);
    conv.write("urban.avi");
}
