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
#include "opendlv-standard-message-set.hpp"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

//docker run --rm -ti --init --device /dev/video0 -v /tmp/.X11-unix:/tmp/.X11-ix -e DISPLAY=$DISPLAY -v /dev/shm:/dev/shm  d opendlv-device-camera-v4l --camera=0 --cid=111 --verbose

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{0};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("camera")) || (0 == commandlineArguments.count("cid")) ) {
        std::cerr << argv[0] << " interfaces with the given V4L camera id (i.e., 0 for /dev/video0) and publishes it to a running OpenDaVINCI session using the OpenDLV Standard Message Set." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --camera=<V4L id> --cid=<OpenDaVINCI session> [--id=<Identifier in case of multiple cameras>] [--verbose]" << std::endl;
        std::cerr << "Example: " << argv[0] << " --camera=0 --cid=111" << std::endl;
        retCode = 1;
    }
    else {
        const uint32_t V4L_CAMERA_ID{(commandlineArguments["camera"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["camera"])) : 0};
        const uint32_t ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};

        (void)ID;

        cv::VideoCapture videoStream(V4L_CAMERA_ID);
        if (videoStream.isOpened()) {
            // Interface to a running OpenDaVINCI session (ignoring any incoming Envelopes).
            cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

            cluon::SharedMemory sharedMemory{"/camera0", 4};
            if (sharedMemory.valid()) {
                while (od4.isRunning()) {
                    cv::Mat frame;
                    if (videoStream.read(frame)) {
                        if (VERBOSE) {
                            cv::imshow("camera", frame);
                        }

                        // TODO: Store image in shared memory; now: simple test data.
                        sharedMemory.lock();
                        uint32_t *data = reinterpret_cast<uint32_t *>(sharedMemory.data());
                        *data += 1;
                        sharedMemory.unlock();
                    }
                    else {
                        break;
                    }
                    cv::waitKey(10);
                }
            }
            else {
                std::cerr << argv[0] << ": Failed to create shared memory '/camera0'." << std::endl;
            }
        }
        else {
            std::cerr << argv[0] << ": Failed to open camera " << V4L_CAMERA_ID << std::endl;
        }
    }
    return retCode;
}

