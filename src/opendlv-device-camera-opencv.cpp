/*
 * Copyright (C) 2018  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cluon-complete.hpp"

#include <libyuv.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("camera")) ||
         (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height")) ||
         (0 == commandlineArguments.count("freq")) ) {
        std::cerr << argv[0] << " interfaces with the given OpenCV-encapsulated camera (e.g., a V4L identifier like 0 or a stream address) and provides the captured image in two shared memory areas: one in I420 format and one in ARGB format." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --camera=<V4L dev node> --width=<width> --height=<height> [--name.i420=<unique name for the shared memory in I420 format>] [--name.argb=<unique name for the shared memory in ARGB format>] [--yuyv422] [--verbose]" << std::endl;
        std::cerr << "         --camera:    Camera to be used (can be a V4L identifier, e.g. 0, or a stream address)" << std::endl;
        std::cerr << "         --name.i420: name of the shared memory for the I420 formatted image; when omitted, video0.i420 is chosen" << std::endl;
        std::cerr << "         --name.argb: name of the shared memory for the I420 formatted image; when omitted, video0.argb is chosen" << std::endl;
        std::cerr << "         --width:     desired width of a frame" << std::endl;
        std::cerr << "         --height:    desired height of a frame" << std::endl;
        std::cerr << "         --freq:      desired frame rate" << std::endl;
        std::cerr << "         --yuyv422:   optional: input frame is of type YUYV422 (ie., instruct OpenCV to not convert it to RGB)" << std::endl;
        std::cerr << "         --verbose:   display captured image" << std::endl;
        std::cerr << "Example: " << argv[0] << " --camera=0 --width=640 --height=480 --freq=20 --verbose" << std::endl;
    } else {
        const std::string CAMERA{commandlineArguments["camera"]};
        const std::string NAME_I420{(commandlineArguments["name.i420"].size() != 0) ? commandlineArguments["name.i420"] : "video0.i420"};
        const std::string NAME_ARGB{(commandlineArguments["name.argb"].size() != 0) ? commandlineArguments["name.argb"] : "video0.argb"};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const float FREQ{static_cast<float>(std::stof(commandlineArguments["freq"]))};
        if ( !(FREQ > 0) ) {
            std::cerr << "[opendlv-device-camera-opencv]: freq must be larger than 0; found " << FREQ << "." << std::endl;
            return retCode;
        }

        const bool VERBOSE{commandlineArguments.count("verbose") != 0};
        const bool IS_YUYV422{commandlineArguments.count("yuyv422") != 0};

        cv::VideoCapture capture(CAMERA);
        if (capture.isOpened()) {
            capture.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
            capture.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
            capture.set(CV_CAP_PROP_FPS, static_cast<uint32_t>(FREQ));

            // Avoid using OpenCV for tranforming incoming frame to RGB as this is expensive.
            if (IS_YUYV422) {
                capture.set(CV_CAP_PROP_CONVERT_RGB, false);
            }
        }
        else {
            std::cerr << argv[0] << "Could not open camera '" << CAMERA << "'" << std::endl;
            return retCode;
        }

        std::unique_ptr<cluon::SharedMemory> sharedMemoryI420(new cluon::SharedMemory{NAME_I420, WIDTH * HEIGHT * 3/2});
        if (!sharedMemoryI420 || !sharedMemoryI420->valid()) {
            std::cerr << "[opendlv-device-camera-opencv]: Failed to create shared memory '" << NAME_I420 << "'." << std::endl;
            return retCode;
        }

        std::unique_ptr<cluon::SharedMemory> sharedMemoryARGB(new cluon::SharedMemory{NAME_ARGB, WIDTH * HEIGHT * 4});
        if (!sharedMemoryARGB || !sharedMemoryARGB->valid()) {
            std::cerr << "[opendlv-device-camera-opencv]: Failed to create shared memory '" << NAME_ARGB << "'." << std::endl;
            return retCode;
        }

        if ( (sharedMemoryI420 && sharedMemoryI420->valid()) &&
             (sharedMemoryARGB && sharedMemoryARGB->valid()) ) {
            std::clog << "[opendlv-device-camera-opencv]: Data from camera '" << CAMERA<< "' available in I420 format in shared memory '" << sharedMemoryI420->name() << "' (" << sharedMemoryI420->size() << ") and in ARGB format in shared memory '" << sharedMemoryARGB->name() << "' (" << sharedMemoryARGB->size() << ")." << std::endl;

            cv::Mat ARGB(HEIGHT, WIDTH, CV_8UC4, sharedMemoryARGB->data());

            while (!cluon::TerminateHandler::instance().isTerminated.load()) {
                cv::Mat frame;
                if (capture.read(frame)) {
                    cluon::data::TimeStamp ts{cluon::time::now()};
                    sharedMemoryI420->lock();
                    sharedMemoryI420->setTimeStamp(ts);
                    {
                        if (IS_YUYV422) {
                            libyuv::YUY2ToI420(reinterpret_cast<uint8_t*>(frame.data), WIDTH * 2 /* 2*WIDTH for YUYV 422*/,
                                               reinterpret_cast<uint8_t*>(sharedMemoryI420->data()), WIDTH,
                                               reinterpret_cast<uint8_t*>(sharedMemoryI420->data()+(WIDTH * HEIGHT)), WIDTH/2,
                                               reinterpret_cast<uint8_t*>(sharedMemoryI420->data()+(WIDTH * HEIGHT + ((WIDTH * HEIGHT) >> 2))), WIDTH/2,
                                               WIDTH, HEIGHT);
                        }
                        else {
                            libyuv::RGB24ToI420(reinterpret_cast<uint8_t*>(frame.data), WIDTH * 3 /* 3*WIDTH for RGB24*/,
                                                reinterpret_cast<uint8_t*>(sharedMemoryI420->data()), WIDTH,
                                                reinterpret_cast<uint8_t*>(sharedMemoryI420->data()+(WIDTH * HEIGHT)), WIDTH/2,
                                                reinterpret_cast<uint8_t*>(sharedMemoryI420->data()+(WIDTH * HEIGHT + ((WIDTH * HEIGHT) >> 2))), WIDTH/2,
                                                WIDTH, HEIGHT);
                        }
                    }
                    sharedMemoryI420->unlock();

                    sharedMemoryARGB->lock();
                    sharedMemoryARGB->setTimeStamp(ts);
                    {
                        libyuv::I420ToARGB(reinterpret_cast<uint8_t*>(sharedMemoryI420->data()), WIDTH,
                                           reinterpret_cast<uint8_t*>(sharedMemoryI420->data()+(WIDTH * HEIGHT)), WIDTH/2,
                                           reinterpret_cast<uint8_t*>(sharedMemoryI420->data()+(WIDTH * HEIGHT + ((WIDTH * HEIGHT) >> 2))), WIDTH/2,
                                           reinterpret_cast<uint8_t*>(sharedMemoryARGB->data()), WIDTH * 4, WIDTH, HEIGHT);

                        if (VERBOSE) {
                            cv::imshow(sharedMemoryARGB->name(), ARGB);
                            cv::waitKey(10); // Necessary to actually display the image.
                        }
                    }
                    sharedMemoryARGB->unlock();

                    sharedMemoryI420->notifyAll();
                    sharedMemoryARGB->notifyAll();
                }
            }
        }

        capture.release();
        retCode = 0;
    }
    return retCode;
}

