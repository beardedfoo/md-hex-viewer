// A self-hosted hex dump utility for the Sega Mega Drive/Genesis
// Author: @beardedfoo
// Source: https://github.com/beardedfoo/md-hex-viewer
#include <genesis.h>

// Where on screen should the hex display appear?
#define HEX_X 5
#define HEX_Y 2

// Where on screen should the memory base address appear?
#define BASE_X 5
#define BASE_Y 1

// Where on screen should the row address offsets appear?
#define OFFSET_X 1
#define OFFSET_Y 2

// Where on screen should the ascii display appear?
#define ASCII_X 30
#define ASCII_Y 2

// Which bytes should be shown as ascii chars?
#define ASCII_MIN_VALUE 32
#define ASCII_MAX_VALUE 126

// For non-displayable ascii values, what char should be shown
// instead?
#define DEFAULT_ASCII_CHAR '.'

// How many bytes wide should the editor be?
#define EDITOR_COLS 8

// How many rows of bytes should be shown?
#define EDITOR_ROWS 24

// How many tiles of padding to display between bytes on screen
#define BYTE_PADDING 1

// On screen bytes take up 2 tiles
#define BYTE_WIDTH 2

// How much should the base addr change by on input?
#define BASE_STEP_SIZE 8
#define BASE_LARGE_STEP_SIZE 0x100

// An array for byte to hex string conversion. This approach
// optimizes for cpu cycles over memory usage, but realistically
// doesn't take up very much space in ROM.
char *itoa_map[0x100] = {
  "00",  "01",  "02",  "03",  "04",  "05",  "06",  "07",
  "08",  "09",  "0A",  "0B",  "0C",  "0D",  "0E",  "0F",
  "10",  "11",  "12",  "13",  "14",  "15",  "16",  "17",
  "18",  "19",  "1A",  "1B",  "1C",  "1D",  "1E",  "1F",
  "20",  "21",  "22",  "23",  "24",  "25",  "26",  "27",
  "28",  "29",  "2A",  "2B",  "2C",  "2D",  "2E",  "2F",
  "30",  "31",  "32",  "33",  "34",  "35",  "36",  "37",
  "38",  "39",  "3A",  "3B",  "3C",  "3D",  "3E",  "3F",
  "40",  "41",  "42",  "43",  "44",  "45",  "46",  "47",
  "48",  "49",  "4A",  "4B",  "4C",  "4D",  "4E",  "4F",
  "50",  "51",  "52",  "53",  "54",  "55",  "56",  "57",
  "58",  "59",  "5A",  "5B",  "5C",  "5D",  "5E",  "5F",
  "60",  "61",  "62",  "63",  "64",  "65",  "66",  "67",
  "68",  "69",  "6A",  "6B",  "6C",  "6D",  "6E",  "6F",
  "70",  "71",  "72",  "73",  "74",  "75",  "76",  "77",
  "78",  "79",  "7A",  "7B",  "7C",  "7D",  "7E",  "7F",
  "80",  "81",  "82",  "83",  "84",  "85",  "86",  "87",
  "88",  "89",  "8A",  "8B",  "8C",  "8D",  "8E",  "8F",
  "90",  "91",  "92",  "93",  "94",  "95",  "96",  "97",
  "98",  "99",  "9A",  "9B",  "9C",  "9D",  "9E",  "9F",
  "A0",  "A1",  "A2",  "A3",  "A4",  "A5",  "A6",  "A7",
  "A8",  "A9",  "AA",  "AB",  "AC",  "AD",  "AE",  "AF",
  "B0",  "B1",  "B2",  "B3",  "B4",  "B5",  "B6",  "B7",
  "B8",  "B9",  "BA",  "BB",  "BC",  "BD",  "BE",  "BF",
  "C0",  "C1",  "C2",  "C3",  "C4",  "C5",  "C6",  "C7",
  "C8",  "C9",  "CA",  "CB",  "CC",  "CD",  "CE",  "CF",
  "D0",  "D1",  "D2",  "D3",  "D4",  "D5",  "D6",  "D7",
  "D8",  "D9",  "DA",  "DB",  "DC",  "DD",  "DE",  "DF",
  "E0",  "E1",  "E2",  "E3",  "E4",  "E5",  "E6",  "E7",
  "E8",  "E9",  "EA",  "EB",  "EC",  "ED",  "EE",  "EF",
  "F0",  "F1",  "F2",  "F3",  "F4",  "F5",  "F6",  "F7",
  "F8",  "F9",  "FA",  "FB",  "FC",  "FD",  "FE",  "FF",
};

// Convert an unsigned 8-bit integer to string in hex representation
void u8toa(u8 b, char *str) {
  str[0] = itoa_map[b & 0xFF][0];
  str[1] = itoa_map[b & 0xFF][1];
  str[2] = NULL;
}

// Convert an unsigned 32-bit integer to string in hex representaton
void u32toa(u32 b, char *str) {
  // Convert each byte of the 32-bit int in order.
  // Isolate each byte by shifting bits and masking the value.
  u8toa((b >> 8*3) & 0xFF, str);
  u8toa((b >> 8*2) & 0xFF, str+2);
  u8toa((b >> 8*1) & 0xFF, str+4);
  u8toa((b >> 8*0) & 0xFF, str+6);
}

// Convert a byte to a string containing a single printable ascii
// char
void byte_to_charstr(u8 b, char *str) {
  // Only some byte ranges are printable in ASCII
  if (b >= ASCII_MIN_VALUE && b <= ASCII_MAX_VALUE) {
    // For the printable range use the raw byte value as a char
    str[0] = b;
  } else {
    // For non-printable ranges use some default character instead
    str[0] = DEFAULT_ASCII_CHAR;
  }

  // Strings in C are NULL terminated, so this means no more
  // characters follow for this string.
  str[1] = NULL;
}

int main()
{
  // Initialize the video chip
  VDP_init();

  // Initialize gamepad handling
  JOY_init();

  // Begin viewing the memory at this address
  u32 base = 0x00000000;

  // Create a pointer to the base memory address. By accessing
  // this "array" we are directly accessing that memory address
  // and the addresses that follow.
  u8 *data = (u8*)base;

  // Allocate a string buffer for string conversion operations
  char msg[16];

  // Loop the rest of the app
  while(1) {
    // Wait for the video chip to finish drawing a frame
    VDP_waitVSync();

    // Read the current state of the gamepad for player 1
    u8 joy_state = JOY_readJoypad(JOY_1);

    // Decrement the memory base with d-pad up
    // (lower addresses appear higher on the screen)
    if (joy_state & BUTTON_UP) {
      if (base >= BASE_STEP_SIZE) {
        base -= BASE_STEP_SIZE;
      }
    }

    // Decrement the memory base MORE with d-pad left
    if (joy_state & BUTTON_LEFT) {
      if (base >= BASE_LARGE_STEP_SIZE) {
        base -= BASE_LARGE_STEP_SIZE;
      }
    }

    // Increment the memory base with d-pad down
    // (higher addresses appear lower on the screen)
    if (joy_state & BUTTON_DOWN) {
      base += BASE_STEP_SIZE;
    }

    // Increment the memory base MORE with d-pad right
    if (joy_state & BUTTON_RIGHT) {
      base += BASE_LARGE_STEP_SIZE;
    }

    // Set the data pointer to the base memory address
    data = (u8*)base;

    // Print the current base on screen
    u32toa(base & 0xFFFFFF00, msg);
    VDP_drawText("base: 0x", BASE_X, BASE_Y);
    VDP_drawText(msg, BASE_X + 8, BASE_Y);

    // Draw hex display
    for (u8 row = 0; row < EDITOR_ROWS; row++) {
      // Draw the row offset (address relative to base for start of row)
      u8toa((row * EDITOR_COLS) + (base & 0xFF), msg);
      msg[2] = ':';
      msg[3] = NULL;
      VDP_drawText(msg, OFFSET_X, OFFSET_Y + row);

      for (u8 col = 0; col < EDITOR_COLS; col++) {
        // Calculate the memory offset for this byte relative to the base address
        u8 offset = (row * 8) + col;

        // Convert the byte at the designated address into a hex string
        u8toa(data[offset], msg);

        // Calculate the position on screen for this byte and draw
        // it within the hex display
        VDP_drawText(msg, HEX_X + col*(BYTE_WIDTH + BYTE_PADDING),
          HEX_Y + row);

        // Convert this byte as a raw ascii character to string
        byte_to_charstr(data[offset], msg);

        // Calculate the positon on screen for this byte and draw
        // it within the ascii display
        VDP_drawText(msg, ASCII_X + col, ASCII_Y + row);
      }
    }
  }
}
