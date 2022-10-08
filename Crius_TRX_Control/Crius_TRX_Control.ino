#include <FastLED.h>
#include <Control_Surface.h> // Include the Control Surface library


// Include the display interface you'd like to use
#include <Display/DisplayInterfaces/DisplayInterfaceSSD1306.hpp>


// ----------------------------- MIDI Interface ----------------------------- //
// ========================================================================== //
/*
   Instantiate a MIDI interface to use for the Control Surface.
*/

// Instantiate a MIDI over USB interface.
USBMIDI_Interface midi;


// ------------------------------ LEDs Setup -------------------------------- //
// ========================================================================== //
// Define the array of leds.

Array<CRGB, 8> leds {};       //How many LEDs used from the LED strip
constexpr uint8_t ledpin = 3; //The data pin with the strip connected.

// Create a functor that maps the velocity and the index of a note to a color.
struct RainbowColorMapper {
  CHSV operator()(uint8_t velocity, uint8_t index) const {
    return CHSV(255 * index / leds.length, 255, 255u * velocity / 127u);
  }
};

NoteRangeFastLED<leds.length, RainbowColorMapper> midiled {
  leds,
  MIDI_Notes::C(1),           //Which NoteOn MIDI Message is the one that triggers the first LED
};


// ----------------------------- Display setup ------------------------------ //
// ========================================================================== //
/*
   Instantiate and initialize the SSD1306 OLED display
*/

constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;

constexpr int8_t OLED_DC_1_2 = 10;    // Data/Command pin of the 1st display
constexpr int8_t OLED_DC_3_4 = 4;     // Data/Command pin of the 2nd display
constexpr int8_t OLED_DC_5_6 = 66;    // Data/Command pin of the 3rd display
constexpr int8_t OLED_DC_7_8 = 19;    // Data/Command pin of the 4th display

constexpr int8_t OLED_reset_1_2 = 9;   // Reset pin of the 1st display
constexpr int8_t OLED_reset_3_4 = 12;  // Reset pin of the 2nd display
constexpr int8_t OLED_reset_5_6 = A6;  // Reset pin of the 3rd display
constexpr int8_t OLED_reset_7_8 = A7;  // Reset pin of the 4th display

constexpr int8_t OLED_CS_1_2 = 8;    // Chip Select pin of the 1st display
constexpr int8_t OLED_CS_3_4 = 13;   // Chip Select pin of the 2nd display
constexpr int8_t OLED_CS_5_6 = 67;   // Chip Select pin of the 3rd display
constexpr int8_t OLED_CS_7_8 = 18;   // Chip Select pin of the 4th display

constexpr int8_t OLED_MOSI_1_2 = 11; // D1 pin of the 1st display
constexpr int8_t OLED_MOSI_3_4 = 5;  // D1 pin of the 2nd display
constexpr int8_t OLED_MOSI_5_6 = A5; // D1 pin of the 3rd display
constexpr int8_t OLED_MOSI_7_8 = A8; // D1 pin of the 4th display

constexpr int8_t OLED_CLK_1_2 = 7;   // D0 pin of the 1st display
constexpr int8_t OLED_CLK_3_4 = 6;   // D0 pin of the 2nd display
constexpr int8_t OLED_CLK_5_6 = A4;  // D0 pin of the 3rd display
constexpr int8_t OLED_CLK_7_8 = A9;  // D0 pin of the 4th display

constexpr uint32_t SPI_Frequency = SPI_MAX_SPEED;


// Instantiate the displays
Adafruit_SSD1306 ssd1306Display_1_2(SCREEN_WIDTH, SCREEN_HEIGHT,
                                    OLED_MOSI_1_2, OLED_CLK_1_2, OLED_DC_1_2, OLED_reset_1_2, OLED_CS_1_2);


Adafruit_SSD1306 ssd1306Display_3_4(SCREEN_WIDTH, SCREEN_HEIGHT,
                                    OLED_MOSI_3_4, OLED_CLK_3_4, OLED_DC_3_4, OLED_reset_3_4, OLED_CS_3_4);

Adafruit_SSD1306 ssd1306Display_5_6(SCREEN_WIDTH, SCREEN_HEIGHT,
                                    OLED_MOSI_5_6, OLED_CLK_5_6, OLED_DC_5_6, OLED_reset_5_6, OLED_CS_5_6);

Adafruit_SSD1306 ssd1306Display_7_8(SCREEN_WIDTH, SCREEN_HEIGHT,
                                    OLED_MOSI_7_8, OLED_CLK_7_8, OLED_DC_7_8, OLED_reset_7_8, OLED_CS_7_8);


// --------------------------- Display interface ---------------------------- //
// ========================================================================== //

#if defined(ADAFRUIT_SSD1306_HAS_SETBUFFER) && ADAFRUIT_SSD1306_HAS_SETBUFFER
// We'll use a static buffer to avoid dynamic memory usage, and to allow
// multiple displays to reuse one single buffer.
static uint8_t buffer[(SCREEN_WIDTH * SCREEN_HEIGHT + 7) / 8];
#endif


// Implement the display interface, specifically, the begin and drawBackground
// methods.
class MySSD1306_DisplayInterface : public SSD1306_DisplayInterface {
  public:
    MySSD1306_DisplayInterface(Adafruit_SSD1306 &display)
      : SSD1306_DisplayInterface(display) {}

    void begin() override {
#if defined(ADAFRUIT_SSD1306_HAS_SETBUFFER) && ADAFRUIT_SSD1306_HAS_SETBUFFER
      disp.setBuffer(buffer);
#endif
      // Initialize the Adafruit_SSD1306 display
      if (!disp.begin())
        FATAL_ERROR(F("SSD1306 initialization failed."), 0x1306);

      // If you override the begin method, remember to call the super class method
      SSD1306_DisplayInterface::begin();
    }

    void drawBackground() override {
      disp.drawLine(1, 8, 126, 8, WHITE);
    }

} display_1_2 = ssd1306Display_1_2, display_3_4 = ssd1306Display_3_4, display_5_6 = ssd1306Display_5_6, display_7_8 = ssd1306Display_7_8;


// ------------------------------- Bank setup ------------------------------- //
// ========================================================================== //
/*
   Create a bank and a bank selector to change its setting.
*/

Bank<1> bank(8); // Create a new bank with two tracks per bank

// Create a new bank selector to control the bank using a push button
IncrementSelector<1> bankselector {bank, 1};


// -------------------------- MIDI Input Elements --------------------------- //
// ========================================================================== //
/*
   Define all elements that listen for MIDI messages.
*/

// Main MCU LCD screen, used to get track names
MCU::LCD<> lcd {};

// Time display keeps track of the bar counter
MCU::TimeDisplay timedisplay {};

// Play / Record
NoteValue play {MCU::PLAY};
NoteValue record {MCU::RECORD};

// Mute
Bankable::NoteValue<1> mute[] {
  {bank, MCU::MUTE_1},
  {bank, MCU::MUTE_2},
  {bank, MCU::MUTE_3},
  {bank, MCU::MUTE_4},
  {bank, MCU::MUTE_5},
  {bank, MCU::MUTE_6},
  {bank, MCU::MUTE_7},
  {bank, MCU::MUTE_8},
};

// Solo
Bankable::NoteValue<1> solo[] {
  {bank, MCU::SOLO_1},
  {bank, MCU::SOLO_2},
  {bank, MCU::SOLO_3},
  {bank, MCU::SOLO_4},
  {bank, MCU::SOLO_5},
  {bank, MCU::SOLO_6},
  {bank, MCU::SOLO_7},
  {bank, MCU::SOLO_8},
};

NoteValue rudeSolo {MCU::RUDE_SOLO};

// Record arm / ready
Bankable::NoteValue<1> recrdy[] {
  {bank, MCU::REC_RDY_1},
  {bank, MCU::REC_RDY_2},
  {bank, MCU::REC_RDY_3},
  {bank, MCU::REC_RDY_4},
  {bank, MCU::REC_RDY_5},
  {bank, MCU::REC_RDY_6},
  {bank, MCU::REC_RDY_7},
  {bank, MCU::REC_RDY_8},
};


// VU meters
MCU::Bankable::VU<1> vu[] {
  {bank, 1, MCU::VUDecay::Hold},
  {bank, 2, MCU::VUDecay::Hold},
  {bank, 3, MCU::VUDecay::Hold},
  {bank, 4, MCU::VUDecay::Hold},
  {bank, 5, MCU::VUDecay::Hold},
  {bank, 6, MCU::VUDecay::Hold},
  {bank, 7, MCU::VUDecay::Hold},
  {bank, 8, MCU::VUDecay::Hold},
};

// VPot rings
MCU::Bankable::VPotRing<1> vpot[] {
  {bank, 1},
  {bank, 2},
  {bank, 3},
  {bank, 4},
  {bank, 5},
  {bank, 6},
  {bank, 7},
  {bank, 8},
};


// ---------------------------- Display Elements ---------------------------- //
// ========================================================================== //
/*
   Define all display elements that display the state of the input elements.
*/

// Track names
MCU::LCDDisplay lcddisps[] {
  // track (1), position (0, 40), font size (1)
  {display_1_2, lcd, bank, 7, {0, 40}, 1, WHITE},
  {display_1_2, lcd, bank, 8, {64, 40}, 1, WHITE},
  {display_3_4, lcd, bank, 5, {0, 40}, 1, WHITE},
  {display_3_4, lcd, bank, 6, {64, 40}, 1, WHITE},
  {display_5_6, lcd, bank, 3, {0, 40}, 1, WHITE},
  {display_5_6, lcd, bank, 4, {64, 40}, 1, WHITE},
  {display_5_6, lcd, bank, 3, {0, 40}, 1, WHITE},
  {display_5_6, lcd, bank, 4, {64, 40}, 1, WHITE},
  {display_7_8, lcd, bank, 1, {0, 40}, 1, WHITE},
  {display_7_8, lcd, bank, 2, {64, 40}, 1, WHITE},
};

// Time display
MCU::TimeDisplayDisplay timedisplaydisplay {
  // position (0, 0), font size (1)
  display_7_8, timedisplay, {0, 0}, 1, WHITE,
};

// Play / Record
BitmapDisplay<> playDisp {
  display_7_8, play, XBM::play_7, {16 + 64, 0}, WHITE,
};
BitmapDisplay<> recordDisp {
  display_7_8, record, XBM::record_7, {26 + 64, 0}, WHITE,
};

// Mute
BitmapDisplay<> muteDisp[] {
  {display_1_2, mute[6], XBM::mute_10B, {14, 50}, WHITE},
  {display_1_2, mute[7], XBM::mute_10B, {14 + 64, 50}, WHITE},
  {display_3_4, mute[4], XBM::mute_10B, {14, 50}, WHITE},
  {display_3_4, mute[5], XBM::mute_10B, {14 + 64, 50}, WHITE},
  {display_5_6, mute[2], XBM::mute_10B, {14, 50}, WHITE},
  {display_5_6, mute[3], XBM::mute_10B, {14 + 64, 50}, WHITE},
  {display_7_8, mute[0], XBM::mute_10B, {14, 50}, WHITE},
  {display_7_8, mute[1], XBM::mute_10B, {14 + 64, 50}, WHITE},
};

// Solo
BitmapDisplay<> soloDisp[] {
  {display_1_2, solo[6], XBM::solo_10B, {14, 50}, WHITE},
  {display_1_2, solo[7], XBM::solo_10B, {14 + 64, 50}, WHITE},
  {display_3_4, solo[4], XBM::solo_10B, {14, 50}, WHITE},
  {display_3_4, solo[5], XBM::solo_10B, {14 + 64, 50}, WHITE},
  {display_5_6, solo[2], XBM::solo_10B, {14, 50}, WHITE},
  {display_5_6, solo[3], XBM::solo_10B, {14 + 64, 50}, WHITE},
  {display_7_8, solo[0], XBM::solo_10B, {14, 50}, WHITE},
  {display_7_8, solo[1], XBM::solo_10B, {14 + 64, 50}, WHITE},
};

BitmapDisplay<> rudeSoloDisp {
  display_7_8, rudeSolo, XBM::solo_7, {36 + 64, 0}, WHITE};

// Record arm / ready
BitmapDisplay<> recrdyDisp[] {
  {display_1_2, recrdy[6], XBM::rec_rdy_10B, {14 + 14, 50}, WHITE},
  {display_1_2, recrdy[7], XBM::rec_rdy_10B, {14 + 14 + 64, 50}, WHITE},
  {display_3_4, recrdy[4], XBM::rec_rdy_10B, {14 + 14, 50}, WHITE},
  {display_3_4, recrdy[5], XBM::rec_rdy_10B, {14 + 14 + 64, 50}, WHITE},
  {display_5_6, recrdy[2], XBM::rec_rdy_10B, {14 + 14, 50}, WHITE},
  {display_5_6, recrdy[3], XBM::rec_rdy_10B, {14 + 14 + 64, 50}, WHITE},
  {display_7_8, recrdy[0], XBM::rec_rdy_10B, {14 + 14, 50}, WHITE},
  {display_7_8, recrdy[1], XBM::rec_rdy_10B, {14 + 14 + 64, 50}, WHITE},
};

// VU meters
MCU::VUDisplay<> vuDisp[] {
  // position (32+11, 60), width (16), bar height (3) px, bar spacing (1) px
  {display_1_2, vu[6], {32 + 11, 60}, 16, 3, 1, WHITE},
  {display_1_2, vu[7], {32 + 11 + 64, 60}, 16, 3, 1, WHITE},
  {display_3_4, vu[4], {32 + 11, 60}, 16, 3, 1, WHITE},
  {display_3_4, vu[5], {32 + 11 + 64, 60}, 16, 3, 1, WHITE},
  {display_5_6, vu[2], {32 + 11, 60}, 16, 3, 1, WHITE},
  {display_5_6, vu[3], {32 + 11 + 64, 60}, 16, 3, 1, WHITE},
  {display_7_8, vu[0], {32 + 11, 60}, 16, 3, 1, WHITE},
  {display_7_8, vu[1], {32 + 11 + 64, 60}, 16, 3, 1, WHITE},

};

// VPot rings
MCU::VPotDisplay<> vpotDisp[] {
  // position (0, 10), outer radius (14) px, inner radius (12) px
  {display_1_2, vpot[6], {0, 10}, 14, 12, WHITE},
  {display_1_2, vpot[7], {64, 10}, 14, 12, WHITE},
  {display_3_4, vpot[4], {0, 10}, 14, 12, WHITE},
  {display_3_4, vpot[5], {64, 10}, 14, 12, WHITE},
  {display_5_6, vpot[2], {0, 10}, 14, 12, WHITE},
  {display_5_6, vpot[3], {64, 10}, 14, 12, WHITE},
  {display_7_8, vpot[0], {0, 10}, 14, 12, WHITE},
  {display_7_8, vpot[1], {64, 10}, 14, 12, WHITE},
};

// Bank seting
BankDisplay bankDisp[] {
  // first track of the bank (1), position (0, 50), font size (2)
  {display_1_2, bank, 7, {0, 50}, 2, WHITE},
  {display_1_2, bank, 8, {64, 50}, 2, WHITE},
  {display_3_4, bank, 5, {0, 50}, 2, WHITE},
  {display_3_4, bank, 6, {64, 50}, 2, WHITE},
  {display_5_6, bank, 3, {0, 50}, 2, WHITE},
  {display_5_6, bank, 4, {64, 50}, 2, WHITE},
  {display_7_8, bank, 1, {0, 50}, 2, WHITE},
  {display_7_8, bank, 2, {64, 50}, 2, WHITE},
};

// ------------------------ Setup Buttons and Cursor------------------------- //
// ========================================================================== //

//Butons - 16 Analog IO Multiplexers HC4067 (x4)
CD74HC4067 mux1 {
  A0,              // Analog input pin
  {14, 15, 16, 17},// Address pins S0, S1, S2, S3
  A10              // Enable Pin
};

CD74HC4067 mux2 {
  A0,              // Analog input pin
  {14, 15, 16, 17},// Address pins S0, S1, S2, S3
  A1               // Enable Pin
};

CD74HC4067 mux3 {
  A0,              // Analog input pin
  {14, 15, 16, 17},// Address pins S0, S1, S2, S3
  A3               // Enable Pin
};

CD74HC4067 mux4 {
  A0,              // Analog input pin
  {14, 15, 16, 17},// Address pins S0, S1, S2, S3
  A2               // Enable Pin
};

//NoteButton buttons[] {
//  {mux1.pin(0), MCU::PLAY},       //Midi Note 94 = PLAY
//  {mux1.pin(1), MCU::STOP},       //Midi Note 93 = STOP
//  {mux1.pin(2), MCU::RECORD},     //Midi Note 95 = RECORD
//  {mux1.pin(3), 87},              //Midi Note 87 = Punch-In
//  {mux1.pin(4), MCU::CYCLE},      //Midi Note 86 = LOOP
//  {mux1.pin(5), 88},              //Midi Note 88 = Punch-Out
//  {mux1.pin(6), MCU::REWIND},     //Midi Note 91 = Rewind
//  {mux1.pin(7), MCU::FAST_FWD },    //Midi Note 92 = Fast Forward
//  {mux1.pin(8), MCU::SELECT_1},     //Midi Note 24 = Select Track 1
//  {mux1.pin(9), MCU::SELECT_2},     //Midi Note 25 = Select Track 2
//  {mux1.pin(10), MCU::SELECT_3},    //Midi Note 26 = Select Track 3
//  {mux1.pin(11), MCU::SELECT_4},    //Midi Note 27 = Select Track 4
//  {mux1.pin(12), MCU::SELECT_5},    //Midi Note 28 = Select Track 5
//  {mux1.pin(13), MCU::SELECT_6},    //Midi Note 29 = Select Track 6
//  {mux1.pin(14), MCU::SELECT_7},    //Midi Note 30 = Select Track 7
//  {mux1.pin(15), MCU::SELECT_8},    //Midi Note 31 = Select Track 8
//};

NoteButton buttonsRecRDY[] {
  {mux4.pin(0), MCU::REC_RDY_1, 127}, //If you change the last number "127" to "0"
  {mux4.pin(1), MCU::REC_RDY_2, 127}, //you send a Note Off Message
  {mux4.pin(2), MCU::REC_RDY_3, 127},
  {mux4.pin(3), MCU::REC_RDY_4, 127},
  {mux4.pin(4), MCU::REC_RDY_5, 127},
  {mux4.pin(5), MCU::REC_RDY_6, 127},
  {mux4.pin(6), MCU::REC_RDY_7, 127},
  {mux4.pin(7), MCU::REC_RDY_8, 127},
  {mux4.pin(8), MCU::SELECT_1, 127},
  {mux4.pin(9), MCU::SELECT_2, 127},
  {mux4.pin(10), MCU::SELECT_3, 127},
  {mux4.pin(11), MCU::SELECT_4, 127},
  {mux4.pin(12), MCU::SELECT_5, 127},
  {mux4.pin(13), MCU::SELECT_6, 127},
  {mux4.pin(14), MCU::SELECT_7, 127},
  {mux4.pin(15), MCU::SELECT_8, 127},
};

NoteButton buttons2[] {
  {mux2.pin(0), MCU::MUTE_1 , 127},
  {mux2.pin(1), MCU::MUTE_2 , 127},
  {mux2.pin(2), MCU::MUTE_3 , 127},
  {mux2.pin(3), MCU::MUTE_4 , 127},
  {mux2.pin(4), MCU::MUTE_5 , 127},
  {mux2.pin(5), MCU::MUTE_6 , 127},
  {mux2.pin(6), MCU::MUTE_7 , 127},
  {mux2.pin(7), MCU::MUTE_8 , 127},
  {mux2.pin(8), MCU::SOLO_1 , 127},
  {mux2.pin(9), MCU::SOLO_2 , 127},
  {mux2.pin(10), MCU::SOLO_3 , 127},
  {mux2.pin(11), MCU::SOLO_4 , 127},
  {mux2.pin(12), MCU::SOLO_5 , 127},
  {mux2.pin(13), MCU::SOLO_6 , 127},
  {mux2.pin(14), MCU::SOLO_7 , 127},
  {mux2.pin(15), MCU::SOLO_8 , 127},
};

NoteButton buttons3[] {
  {mux3.pin(0), MCU::UP},            //Midi Note 96 = Cursor Move Up
  {mux3.pin(1), MCU::DOWN},          //Midi Note 97 = Cursor Move Down
  {mux3.pin(2), MCU::LEFT},          //Midi Note 98 = Cursor Move Left
  {mux3.pin(3), MCU::RIGHT},         //Midi Note 99 = Cursor Move Right
  {mux3.pin(4), MCU::ZOOM},          //Midi Note 100 = Cursor Center Function
  {mux3.pin(5), 74},                 //Midi Note 74 = Session View/Arrangement View
  {mux3.pin(6), 77},                 //Midi Note 77 = Hide/Show Browser Side Bar
  {mux3.pin(7), 78},                 //Midi Note 78 = Show/Hide Info/Detail View Window
  {mux3.pin(8), 75},                 //Midi Note 75 = Clip View Selector Clip Preview/Device Preview
  {mux3.pin(9), 76},                 //Midi Note 76 = Undo
  {mux3.pin(10), MCU::SCRUB},        //MCU::SCRUB = Plays all the Clips of the Session and then the Next row of Clips
  {mux3.pin(11), MCU::FAST_FWD},     //Midi Note  83 = Draw Mode
  {mux3.pin(12), MCU::REWIND},       //Midi Note 82 = Αdd Locator
  {mux3.pin(13), MCU::RECORD},       //Midi Note 89 = Timeline Start
  {mux3.pin(14), MCU::STOP},         //Midi Note 90 = Timeline End
  {mux3.pin(15), MCU::PLAY},         //PLAY SCENE 101
};                                   // Midi Note 81 = FolowMCU::SCRUB = Plays all the Clips of the Session and then the Next row of Clips

//Rotary Encoders (x16)
//Track PAN Control Change Rotary Encoders (x8)

// Instantiate a CCAbsoluteEncoder object
CCRotaryEncoder enc1 {
  {52, 53},     // pins
  MCU::V_POT_1, // MIDI address (CC number + optional channel)
  5,            // optional multiplier if the control isn't fast enough
};

CCRotaryEncoder enc2 {
  {50, 51},     // pins
  MCU::V_POT_2, // MIDI address (CC number + optional channel)
  5,            // optional multiplier if the control isn't fast enough
};

CCRotaryEncoder enc3 {
  {48, 49},     // pins
  MCU::V_POT_3, // MIDI address (CC number + optional channel)
  5,            // optional multiplier if the control isn't fast enough
};

CCRotaryEncoder enc4 {
  {46, 47},     // pins
  MCU::V_POT_4, // MIDI address (CC number + optional channel)
  5,            // optional multiplier if the control isn't fast enough
};

CCRotaryEncoder enc5 {
  {44, 45},     // pins
  MCU::V_POT_5, // MIDI address (CC number + optional channel)
  5,            // optional multiplier if the control isn't fast enough
};

CCRotaryEncoder enc6 {
  {42, 43},     // pins
  MCU::V_POT_6, // MIDI address (CC number + optional channel)
  5,            // optional multiplier if the control isn't fast enough
};

CCRotaryEncoder enc7 {
  {40, 41},     // pins
  MCU::V_POT_7, // MIDI address (CC number + optional channel)
  5,            // optional multiplier if the control isn't fast enough
};

CCRotaryEncoder enc8 {
  {38, 39},      // pins
  MCU::V_POT_8 , // MIDI address (CC number + optional channel)
  5,             // optional multiplier if the control isn't fast enough
};

//Track Volume Pitch Bend Rotary Encoders (x8)
PBAbsoluteEncoder enc9 {
  {36, 37},       // pins
  CHANNEL_1,      // MIDI address (CC number + optional channel)
  508,            // optional multiplier if the control isn't fast enough
};

PBAbsoluteEncoder enc10 {
  {34, 35},       // pins
  CHANNEL_2,      // MIDI address (CC number + optional channel)
  508,            // optional multiplier if the control isn't fast enough
};

PBAbsoluteEncoder enc11 {
  {32, 33},       // pins
  CHANNEL_3,      // MIDI address (CC number + optional channel)
  508,            // optional multiplier if the control isn't fast enough
};

PBAbsoluteEncoder enc12 {
  {30, 31},       // pins
  CHANNEL_4,      // MIDI address (CC number + optional channel)
  508,            // optional multiplier if the control isn't fast enough
};

PBAbsoluteEncoder enc13 {
  {28, 29},       // pins
  CHANNEL_5,      // MIDI address (CC number + optional channel)
  508,            // optional multiplier if the control isn't fast enough
};

PBAbsoluteEncoder enc14 {
  {26, 27},       // pins
  CHANNEL_6,      // MIDI address (CC number + optional channel)
  508,            // optional multiplier if the control isn't fast enough
};

PBAbsoluteEncoder enc15 {
  {24, 25},       // pins
  CHANNEL_7,      // MIDI address (CC number + optional channel)
  508,            // optional multiplier if the control isn't fast enough
};

PBAbsoluteEncoder enc16 {
  {22, 23},       // pins
  CHANNEL_8,      // MIDI address (CC number + optional channel)
  508,            // optional multiplier if the control isn't fast enough
};

//Master Volume Pitch Bend Rotary Encoder
PBPotentiometer potentiometer {
  A11,            // Analog pin connected to potentiometer
  CHANNEL_9,      // Channel 9 for Master Volume channel
};


void setup() {
  Control_Surface.begin(); // Initialize Control Surface

  // See FastLED examples and documentation for more information.
  FastLED.addLeds<NEOPIXEL, ledpin>(leds.data, leds.length);
  FastLED.setCorrection(TypicalPixelString);
  midiled.setBrightness(128);

  // Select the correct relative MIDI CC mode.
  // Options:
  //   - TWOS_COMPLEMENT (default)
  //   - BINARY_OFFSET
  //   - SIGN_MAGNITUDE
  //   - NEXT_ADDRESS
  // Aliases:
  //   - REAPER_RELATIVE_1
  //   - REAPER_RELATIVE_2
  //   - REAPER_RELATIVE_3
  //   - TRACKTION_RELATIVE
  //   - MACKIE_CONTROL_RELATIVE
  //   - KORG_KONTROL_INC_DEC_1
  RelativeCCSender::setMode(relativeCCmode::MACKIE_CONTROL_RELATIVE);
}

void loop() {
  Control_Surface.loop();   // Update the Control Surface
  if (midiled.getDirty()) { // If the colors changed
    FastLED.show();         // Update the LEDs with the new colors
  }
}
