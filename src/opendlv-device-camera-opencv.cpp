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

#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{0};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("stream_address")) || (0 == commandlineArguments.count("cid")) || (0 == commandlineArguments.count("width")) || (0 == commandlineArguments.count("height")) || (0 == commandlineArguments.count("bpp")) ) {
        std::cerr << argv[0] << " interfaces with the given V4L camera (e.g., /dev/video0) and publishes it to a running OpenDaVINCI session using the OpenDLV Standard Message Set." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --stream_address=<Stream address> --cid=<OpenDaVINCI session> --width=<width> --height=<height> --bpp=<bits per pixel> [--name=<unique name for the associated shared memory>] [--verbose]" << std::endl;
        std::cerr << "         --stream_address:    the stream address to load with OpenCV" << std::endl;
        std::cerr << "         --width:   desired width of a frame" << std::endl;
        std::cerr << "         --height:  desired height of a frame" << std::endl;
        std::cerr << "         --bpp:     desired bits per pixel of a frame (must be either 8 or 24)" << std::endl;
        std::cerr << "         --bgr2rgb: convert BGR to RGB" << std::endl;
        std::cerr << "         --name:    when omitted, '/cam0' is chosen" << std::endl;
        std::cerr << "         --verbose: when set, the raw image is displayed" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=111 --camera=/dev/video0 --name=cam0" << std::endl;
        retCode = 1;
    } else {
      uint32_t const WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
      uint32_t const HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
      uint32_t const BPP{static_cast<uint32_t>(std::stoi(commandlineArguments["bpp"]))};
      std::string VIDEO_STREAM_ADDRESS{commandlineArguments["stream_address"]};

      uint32_t const SIZE{WIDTH * HEIGHT * BPP/8};
      std::string const NAME{(commandlineArguments["name"].size() != 0) ? commandlineArguments["name"] : "/cam0"};
      bool const VERBOSE{commandlineArguments.count("verbose") != 0};
      bool const BGR2RGB{commandlineArguments.count("bgr2rgb") != 0};
      (void) BGR2RGB;

      cv::VideoCapture capture(VIDEO_STREAM_ADDRESS);
      if (capture.isOpened()) {
        capture.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
        capture.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
        capture.set(CV_CAP_PROP_FORMAT, (BPP == 24 ? CV_CAP_MODE_RGB : CV_CAP_MODE_GRAY));
      } else {
        std::cerr << "Could not open camera '" << VIDEO_STREAM_ADDRESS << "'" << std::endl;
      }

      cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

      std::unique_ptr<cluon::SharedMemory> sharedMemory(new cluon::SharedMemory{NAME, SIZE});
      if (sharedMemory && sharedMemory->valid()) {
        std::clog << argv[0] << ": Data from camera available in shared memory '" << sharedMemory->name() << "' (" << sharedMemory->size() << ")." << std::endl;

        while (od4.isRunning()) {
          cv::Mat frame;
          if (capture.read(frame)) {
            sharedMemory->lock();
            ::memcpy(sharedMemory->data(), reinterpret_cast<char*>(frame.data), frame.step * frame.rows);
            sharedMemory->unlock();
            sharedMemory->notifyAll();
            if (VERBOSE) {
              cv::imshow(sharedMemory->name(), frame);
            }
            cv::waitKey(10);
          }
        }
      } else {
        std::cerr << argv[0] << ": Failed to create shared memory '" << NAME << "'." << std::endl;
      }

      capture.release();
    }
    return retCode;
}

