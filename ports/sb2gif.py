# This script converts the .raw files in screenbuffers/ to an animated GIF.
import os
from PIL import Image
import glob
import sys

def read_header(file):
    """ 
    Reads the custom header from the .raw file. 
    struct image_header_s
    {
        uint32_t sequence_no;
        uint32_t timestamp_ms;
        uint16_t width;
        uint16_t height;
        struct {
            uint8_t r;
            uint8_t g;
            uint8_t b;
        } palette[256] __attribute__((packed));
    } header;
    """
    header_data = file.read(4 + 4 + 2 + 2 + (256 * 3))
    sequence_no = int.from_bytes(header_data[0:4], 'little')
    timestamp_ms = int.from_bytes(header_data[4:8], 'little')
    width = int.from_bytes(header_data[8:10], 'little')
    height = int.from_bytes(header_data[10:12], 'little')
    
    palette = []
    for i in range(256):
        r = header_data[12 + i*3]
        g = header_data[12 + i*3 + 1]
        b = header_data[12 + i*3 + 2]
        palette.append((r, g, b))
    
    return sequence_no, timestamp_ms, width, height, palette

def read_image_data(file, width, height):
    """ Reads the pixel data from the .raw file. """
    pixel_data = file.read(width * height)
    return pixel_data

def convert_raw_to_numpy_image(pixel_data, width, height, palette):
    """ Converts raw pixel data to a PIL Image using the provided palette. """
    img = Image.new('P', (width, height))
    img.putpalette([value for rgb in palette for value in rgb])
    img.putdata(pixel_data)
    return img

def read_images_from_raw_files(directory):
    """ Reads all .raw files in the specified directory and returns a list of PIL Images. """
    images = []
    timestamps = []
    last_seq_no = 0
    raw_files = sorted(glob.glob(os.path.join(directory, '*.raw')))
    
    for raw_file in raw_files:
        with open(raw_file, 'rb') as file:
            sequence_no, timestamp_ms, width, height, palette = read_header(file)
            if sequence_no != last_seq_no + 1:
                print(f"Warning: Missing frame(s) between sequence {last_seq_no} and {sequence_no}")    
            last_seq_no = sequence_no
            timestamps.append(timestamp_ms)
            pixel_data = read_image_data(file, width, height)
            img = convert_raw_to_numpy_image(pixel_data, width, height, palette)
            images.append(img)
    
    return (images,timestamps)

def save_images_as_gif(images, output_file, timestamps, scale=1):
    """ Saves a list of PIL Images as an animated GIF. """
    durations = []
    for i in range(1, len(timestamps)):
        durations.append(min(timestamps[i] - timestamps[i-1],65535))
    durations.append(durations[-1])  # Last frame duration same as second last
    # Scale images if needed
    if scale != 1:
        images = [img.resize((img.width * scale, img.height * scale), Image.NEAREST) for img in images]
    
    images[0].save(output_file, save_all=True, append_images=images[1:], duration=durations, loop=0)

if __name__ == "__main__":
    input_directory = os.path.join(sys.argv[1],'screenbuffers')
    output_gif = os.path.join(sys.argv[1],'output.gif')
    
    images,timestamps = read_images_from_raw_files(input_directory)
    # Compute average FPS
    fps = len(images) / ((timestamps[-1] - timestamps[0]) / 1000.0)
    print(f"Read {len(images)} frames with average FPS: {fps:.2f}")

    durations = []
    for i in range(1,len(timestamps)):
        durations.append(timestamps[i]-timestamps[i-1])
    durations.sort()
    perc90 = int(len(durations)*0.90+0.5)
    fps90=1000.0/durations[perc90]
    print(f"95% of the time FPS will be at least: {fps90}")
    
    save_images_as_gif(images, output_gif, timestamps,2)
    print(f"Saved animated GIF to {output_gif}")