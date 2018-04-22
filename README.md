# opendlv-device-camera-opencv
OpenDLV Microservice to interface with OpenCV-accessible camera devices

```
docker run --rm -ti --init --device /dev/video0 -v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=$DISPLAY -v /dev/shm:/dev/shm chalmersrevere/opendlv-device-camera-opencv-multi:v0.0.1 --camera=/dev/video0 --cid=111 --verbose --name=camera0 --width=1024 --height=768 --bpp=24
```
