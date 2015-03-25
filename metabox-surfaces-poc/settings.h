#define BOX_RENDER_SIZE 600 // pixels
#define BOX_PHYSICAL_SIZE 7 // "meters"
#define BOX_SLOTS 7 // slots per box-side
#define BOX_METERS_PER_SLOT ((float)BOX_PHYSICAL_SIZE/(float)BOX_SLOTS)
#define BOX_PIXELS_PER_SLOT ((float)BOX_RENDER_SIZE/(float)BOX_SLOTS)
#define PIXELS_PER_METER ((float)BOX_RENDER_SIZE/(float)BOX_PHYSICAL_SIZE)
#define FRICTION .4f
#define GRAVITY 40//9.8

#define B2_CAT_MAIN 1
#define B2_CAT_BOX_HULL 1<<1