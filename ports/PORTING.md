# Porting guide
To port this source to a new platform you need to do the following:

In i_system_e32.cc:
- Implement rendering of the framebuffer

In wadfilereader.cc
- Implement a wad file reader

Somewhere:
- Implement keyboard handling


An example for Qt running on posix (macOS or Linux) is found in the Qt/ directory. A headless version that renders the first 10 seconds of the demo can be found in headless/ . It outputs frames to the headless/screenbuffers directory. These can be converted to an animated gif using the sb2gif.py script - called like this:

python3 sb2gif.py headless

## Framebuffer rendering
The framebuffer is rendered in I_FinishUpdate_e32(). Palette is a 256-entry RGB table defined like this:

``` C
struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} palette[256];
```

srcBuffer is containing indices into the palette, and it is containing 160 lines of *240* pixels each (even though width is set to 120). That data needs to be rendered somehow.

## Reading the WAD data
You need to implement ```WR_Init()``` to initialize reading of WAD data, and WR_Read() with the following signature:

``` C
void WR_Read(uint8_t *dst, int offset, int len)
```

dst is a pointer to where data should be put. offset is the offset (in bytes) into the file where reading should start, and len is the number of bytes to read.

## Keyboard handling
To support keyboard input, you have to include d_event.h to get the definition of event_t. You set event.type to either ev_keyup or ev_keydown, and then event.data1 according to these definitions:

```C
#define KEYD_A          1   // Use / Sprint
#define KEYD_B          2   // Fire  
#define KEYD_L          3   // Strafe left (+A Weapon down)
#define KEYD_R          4   // Strafe right (+A Weapon up)
#define KEYD_UP         5   // Run forward
#define KEYD_DOWN       6   // Back up
#define KEYD_LEFT       7   // Turn left
#define KEYD_RIGHT      8   // Turn right
#define KEYD_START      9   // Menu
#define KEYD_SELECT     10  // Automap
```

## Extending main
main() can be found i i_main.cc, and can be extended as desired. 