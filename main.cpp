#include <Adafruit_Protomatter.h>
#include <Bounce2.h>
#include <FastLED.h>
#include <Wire.h>

#define HEIGHT 64
#define WIDTH 64
#define BACK_BUTTON 2
#define NEXT_BUTTON 3
#define MAX_FPS 120

static uint16_t x_pos;
static uint16_t y_pos;
static uint16_t z_pos;

int8_t selected_palette = 0;
uint16_t speed = 25;  // Try 1 for a very slow moving effect, or 60 for something that ends up looking like water.
uint16_t scale = 15;  // The higher the value of scale, the more "zoomed out" the noise iwll be.  A value of 1 will be so zoomed in, you'll mostly see solid colors.
uint16_t noise[HEIGHT][WIDTH];

CRGBPalette16 currentPalette(RainbowStripeColors_p);
uint8_t addrPins[] = {17, 18, 19, 20, 21};
uint8_t rgbPins[] = {7, 8, 9, 10, 11, 12};
Adafruit_Protomatter matrix(WIDTH, 4, 1, rgbPins, sizeof(addrPins), addrPins, 14, 15, 16, true);

Bounce next = Bounce();
Bounce back = Bounce();

uint32_t prevTime = 0;  // Used for frames-per-second throttle
uint8_t colorLoop = 0;

void err(int x) {
  uint8_t i;
  pinMode(LED_BUILTIN, OUTPUT);        // Using onboard LED
  for (i = 1;; i++) {                  // Loop forever...
    digitalWrite(LED_BUILTIN, i & 1);  // LED on/off blink to alert user
    delay(x);
  }
}

inline uint16_t color_merge(CRGB input) {
  return matrix.color565(input.red, input.green, input.blue);
}

void DrawOneFrame() {
  uint8_t ms = millis();
  int8_t yHueDelta8 = ((int32_t)cos16(ms * (12 / 1)) * (200 / HEIGHT)) / 32768;
  int8_t xHueDelta8 = ((int32_t)cos16(ms * (26 / 1)) * (200 / WIDTH)) / 32768;
  uint8_t lineStartHue = ms / 65536;
  for (uint8_t y = 0; y < HEIGHT; y++) {
    lineStartHue += yHueDelta8;
    uint8_t pixelHue = lineStartHue;
    for (uint8_t x = 0; x < WIDTH; x++) {
      pixelHue += xHueDelta8;
      matrix.drawPixel(x, y, matrix.color565(pixelHue, 255, 255));
    }
  }
}

void mapNoiseToLEDsUsingPalette() {
  static uint8_t ihue = 0;
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      uint8_t index = noise[x][y];
      uint8_t bri = noise[y][x];
      if (colorLoop) {  // if this palette is a 'loop', add a slowly-changing base value
        index += ihue;
      }
      matrix.drawPixel(x, y, color_merge(ColorFromPalette(currentPalette, index, bri)));
    }
  }
  ihue += 1;
}

void create_noise() {
  uint8_t dataSmoothing = 0;
  if (speed < 50) {
    dataSmoothing = 200 - (speed * 4);
  }
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      uint16_t data = inoise8(x_pos + scale * x, y_pos + scale * y, z_pos);
      if (dataSmoothing) {
        data = scale8(noise[x][y], dataSmoothing) + scale8(data, 256 - dataSmoothing);
      }
      noise[x][y] = data;
    }
  }
  z_pos += speed;
  x_pos += speed / 8;
  y_pos -= speed / 16;
}

void SetupRandomPalette() {
  // This function generates a random palette that's a gradient
  // between four different colors.  The first is a dim hue, the second is
  // a bright hue, the third is a bright pastel, and the last is
  // another bright hue.  This gives some visual bright/dark variation
  // which is more interesting than just a gradient of different hues.
  currentPalette = CRGBPalette16(
      CHSV(random8(), 255, 32),
      CHSV(random8(), 255, 255),
      CHSV(random8(), 128, 255),
      CHSV(random8(), 255, 255));
}

void SetupBlackAndWhiteStripedPalette() {
  // This function sets up a palette of black and white stripes,
  // using code.  Since the palette is effectively an array of
  // sixteen CRGB colors, the various fill_* functions can be used
  // to set them up.
  // 'black out' all 16 palette entries...
  fill_solid(currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;
}

void SetupPurpleAndGreenPalette() {
  // This function sets up a palette of purple and green stripes.
  CRGB purple = CHSV(HUE_PURPLE, 255, 255);
  CRGB green = CHSV(HUE_GREEN, 255, 255);
  CRGB black = CRGB::Black;

  currentPalette = CRGBPalette16(
      green, green, black, black,
      purple, purple, black, black,
      green, green, black, black,
      purple, purple, black, black);
}

void ChangePalette(char num) {
  switch (num) {
    case 0:
      currentPalette = RainbowColors_p;
      speed = 20;
      scale = 30;
      colorLoop = 1;
      break;
    case 1:
      SetupPurpleAndGreenPalette();
      speed = 10;
      scale = 50;
      colorLoop = 1;
      break;
    case 2:
      SetupBlackAndWhiteStripedPalette();
      speed = 20;
      scale = 30;
      colorLoop = 1;
      break;
    case 3:
      currentPalette = ForestColors_p;
      speed = 8;
      scale = 120;
      colorLoop = 0;
      break;
    case 4:
      currentPalette = CloudColors_p;
      speed = 4;
      scale = 30;
      colorLoop = 0;
      break;
    case 5:
      currentPalette = LavaColors_p;
      speed = 8;
      scale = 50;
      colorLoop = 0;
      break;
    case 6:
      currentPalette = OceanColors_p;
      speed = 20;
      scale = 90;
      colorLoop = 0;
      break;
    case 7:
      currentPalette = PartyColors_p;
      speed = 20;
      scale = 30;
      colorLoop = 1;
      break;
    case 8:
      SetupRandomPalette();
      speed = 20;
      scale = 20;
      colorLoop = 1;
      break;
    case 9:
      SetupRandomPalette();
      speed = 50;
      scale = 50;
      colorLoop = 1;
      break;
    case 10:
      SetupRandomPalette();
      speed = 90;
      scale = 90;
      colorLoop = 1;
      break;
    case 11:
      currentPalette = RainbowStripeColors_p;
      speed = 30;
      scale = 20;
      colorLoop = 1;
      break;
    default:
      break;
  }
}

void setup(void) {
  next.attach(NEXT_BUTTON, INPUT_PULLUP);
  next.interval(20);
  back.attach(BACK_BUTTON, INPUT_PULLUP);
  back.interval(20);
  matrix.begin();
}

void loop() {
  next.update();
  back.update();

  uint32_t t;
  while (((t = micros()) - prevTime) < (1000000L / MAX_FPS))  // Limit the animation frame rate to MAX_FPS.
    ;
  prevTime = t;

  if (next.fell()) {
    selected_palette += 1;
    if (selected_palette >= 12) {
      selected_palette = 0;
    }
    ChangePalette(selected_palette);
  }

  if (back.fell()) {
    selected_palette -= 1;
    if (selected_palette < 0) {
      selected_palette = 11;
    }
    ChangePalette(selected_palette);
  }

  matrix.fillScreen(0x0);
  create_noise();
  mapNoiseToLEDsUsingPalette();
  matrix.show();
}
