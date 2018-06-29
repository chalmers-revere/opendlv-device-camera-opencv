# opendlv-device-camera-opencv
OpenDLV Microservice to interface with OpenCV-accessible camera devices

Adjusting the maximum size of pages in bytes that can be locked in RAM by adding the following lines to `/etc/security/limits.conf`:

```
*    hard   memlock           unlimited
*    soft   memlock           unlimited
```

To use this microservice, your need to allow access to your X11 server:
```
xhost +
```

Next, you can start the streaming (omit `--verbose` to not display the window showing the raw image; the parameter ulimit specifies that maximum bytes to be locked in RAM and __must__ match with the desired video resolution: Width * Height * BitsPerPixel/8) with a network camera with login in credentials (user, password and ip):
```
docker run \
           --rm \
           -ti \
           --init \
           -v /tmp/.X11-unix:/tmp/.X11-unix \
           --network host \
           -e DISPLAY=$DISPLAY \
           -v /dev/shm:/dev/shm \
           --ulimit memlock=2359296:2359296 \
           producer:latest \
              --cid=111 \
              --stream_address=http://user:password@ip/axis-cgi/mjpg/video.cgi\?channel=0\&.mjpg \
              --width=1280 \
              --height=720 \
              --bpp=24 \
              --name=opencv \
              --bgr2rgb \
              --verbose

```

The directory `example` contains a small program that demonstrates how to access the shared memory that is created from the previous microservice. You can build it as follows:

```
cd example
docker build -t consumer -f Dockerfile.amd64 .
```

Finally, you can run the example as follows (obviously, the parameters must match with the settings for `producer`; the parameter ulimit specifies that maximum bytes to be locked in RAM):

```
docker run \
           --rm \
           -ti \
           --init \
           -v /tmp/.X11-unix:/tmp/.X11-unix \
           -e DISPLAY=$DISPLAY \
           -v /dev/shm:/dev/shm \
           --ulimit memlock=2359296:2359296 \
           example \
               --cid=111 \
               --name=camera0 \
               --width=1280 \
               --height=720 \
               --bpp=24 \
               --verbose
```
string("http://revere:AUTOnomous@192.168.0.5/axis-cgi/mjpg/video.cgi?user=revere&password=AUTOnomous&channel=0&.mjpg"