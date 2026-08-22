#include <span>

#include "onnxruntime_cxx_api.h"
#include "mapf_common/meta/mapfaster_encoder.h"

#include <opencv2/imgproc.hpp>

namespace mapf::meta {
    const cv::Vec3b FREE{255, 255, 255}; // RGB white
    const cv::Vec3b OBSTACLE{255, 0, 0}; // RGB red
    const cv::Vec3b START{0, 255, 0}; // RGB green
    const cv::Vec3b GOAL{0, 0, 255}; // RGB blue
    const cv::Vec3b PATH{0, 0, 0}; // RGB black

    void preprocess_rgb_one_into(
        const cv::Mat &rgb,
        std::span<float> out
    ) {
        if (out.size() != mapfaster_encoded_input_size()) {
            throw std::invalid_argument("invalid output span size");
        }

        if (rgb.dims != 2 || rgb.channels() != 3 || rgb.type() != CV_8UC3) {
            throw std::invalid_argument("expected HxWx3 uint8 RGB image");
        }

        const int h = rgb.rows;
        const int w = rgb.cols;

        const double scale = static_cast<double>(IMAGE_SIZE) / std::max(h, w);
        const int new_w = static_cast<int>(std::round(w * scale));
        const int new_h = static_cast<int>(std::round(h * scale));

        cv::Mat resized;
        cv::resize(rgb, resized, cv::Size(new_w, new_h), 0, 0, cv::INTER_NEAREST);

        cv::Mat padded(
            IMAGE_SIZE,
            IMAGE_SIZE,
            CV_8UC3,
            cv::Scalar(0, 0, 0)
        );

        const int top = (IMAGE_SIZE - new_h) / 2;
        const int left = (IMAGE_SIZE - new_w) / 2;

        resized.copyTo(padded(cv::Rect(left, top, new_w, new_h)));

        static constexpr float mean[3] = {0.485f, 0.456f, 0.406f};
        static constexpr float std[3] = {0.229f, 0.224f, 0.225f};

        for (int y = 0; y < IMAGE_SIZE; ++y) {
            const auto *row = padded.ptr<cv::Vec3b>(y);

            for (int x = 0; x < IMAGE_SIZE; ++x) {
                const int pixel_index = y * IMAGE_SIZE + x;

                for (int c = 0; c < 3; ++c) {
                    const float v = static_cast<float>(row[x][c]) / 255.0f;

                    out[c * IMAGE_SIZE * IMAGE_SIZE + pixel_index] = (v - mean[c]) / std[c];
                }
            }
        }
    }

    void mapfaster_encode_into(
        const Grid &grid,
        std::span<const Agent> agents,
        const std::unordered_map<Agent, std::vector<Pos> > &paths,
        std::span<float> out
    ) {
        if (out.size() != mapfaster_encoded_input_size()) {
            throw std::invalid_argument("invalid mapfaster output buffer size");
        }

        cv::Mat image(grid.height, grid.width, CV_8UC3);

        for (int row = 0; row < grid.height; ++row) {
            for (int col = 0; col < grid.width; ++col) {
                image.at<cv::Vec3b>(row, col) =
                        grid.is_blocked(row, col) ? OBSTACLE : FREE;
            }
        }

        for (const auto &agent: agents) {
            const auto &path = paths.at(agent);

            for (const auto &[row, col]: path) {
                image.at<cv::Vec3b>(row, col) = PATH;
            }
        }

        for (const auto &[start, goal]: agents) {
            image.at<cv::Vec3b>(start.row, start.col) = START;
            image.at<cv::Vec3b>(goal.row, goal.col) = GOAL;
        }

        preprocess_rgb_one_into(image, out);
    }
}
