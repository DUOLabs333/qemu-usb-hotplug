import urllib.request
import subprocess, os

class usb_ids(BuildBase):
    def build(cls):
        if not os.path.exists("usb.ids"):
            urllib.request.urlretrieve("http://www.linux-usb.org/usb.ids", "usb.ids")
        if not os.path.exists("usb.ids.json"):
            exec(open("parse.py","r").read())

class libusb(BuildBase):
    OUTPUT_NAME="external/libusb/libusb/.libs/libusb-1.0.a"

    def build(cls):
        if not os.path.exists("external/libusb"):
            os.makedirs("external/libusb", exist_ok=True)
            urllib.request.urlretrieve("https://github.com/libusb/libusb/releases/download/v1.0.27/libusb-1.0.27.tar.bz2", "libusb.tar.bz2")
            subprocess.run(["tar","-xvf", "libusb.tar.bz2", "--strip-components=1", "-C", "external/libusb"])
            os.remove("libusb.tar.bz2")

        os.chdir("external/libusb")
        if not os.path.exists(os.path.relpath(cls.OUTPUT_NAME, "external/libusb")):
            subprocess.run(["./configure", "--enable-static"])
            subprocess.run(["make", "-j8"])


class main(BuildBase):
    SRC_FILES=["main.cpp"]
    INCLUDE_PATHS=[get_dep_path("asio", "asio/include"),get_dep_path("boost"), os.path.join("external", "libusb")]

    OUTPUT_TYPE=EXE

    OUTPUT_NAME="qemu-usb-hotplug"

    STATIC_LIBS=[libusb]

    DEPENDENCIES=[usb_ids]

    FRAMEWORKS=["CoreFoundation","IOKit", "Security"]

    def __init__(self):

        #Libraries required to build libusb
        if PLATFORM=="linux":
                self.SHARED_LIBS.append("udev")

