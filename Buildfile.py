import urllib.request
import subprocess, os
class main(BuildBase):
    SRC_FILES=["main.cpp"]
    INCLUDE_PATHS=[get_dep_path("asio", "asio/include"),get_dep_path("boost ", ""), os.path.join("external", "libusb","include")]

    OUTPUT_TYPE=EXE

    OUTPUT_NAME="qemu-usb-hotplug"

    STATIC_LIBS=["external/libusb/libusb.a"]

class download_usb_ids(BuildBase):
    def build(cls):
        urllib.request.urlretrieve("http://www.linux-usb.org/usb.ids", "usb.ids")

class parse_usb_ids(BuildBase):
    def build(cls):
        exec(open("parse.py","r").read())
        

class clone_libusb(BuildBase):
    def build(cls):
        if not os.path.exists("external"):
            os.makedirs("external")

        subprocess.run(["git", "clone", "--depth=1", "https://github.com/libusb/libusb"], cwd="external")

class build_libusb(BuildBase):
    def build(cls):
        pass
